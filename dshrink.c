#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

struct header
	{
	char h_type[4];
	long int h_entries;
	long int h_begin;
	} wad;

struct directory
	{
	long int d_begin;
	long int d_size;
	char d_name[8];
	};

struct linedef
	{
	short int l_from;
	short int l_to;
	short int l_flags;
	short int l_type;
	short int l_tag;
	short int l_side1;
	short int l_side2;
	};

struct sidedef
	{
	short int s_x_off;
	short int s_y_off;
	char s_upper[8];
	char s_lower[8];
	char s_normal[8];
	short int s_sector;
	};

void get_wad(char *);
int  get_dirent(char *);
void get_linedefs(void);
void get_sidedefs(void);

#define TRUE       1
#define FALSE      0
#define XFERLEN 10000

char xferbuf[XFERLEN];
long delta;
long deleted_sidedefs = 0;
long dir_entries;
long dir_start;

FILE *infile,*outfile;

struct directory *direc = 0;
char dir_name[] = "        ";

struct linedef *linedefs;
long num_lines = 0;

struct sidedef *sidedefs;
long num_sides = 0;
long sidedef_size;
char filename[32];

void *memory();
void get_wad();
void xfer();
void get_linedefs();
void get_sidedefs();
void copy_sidedef();
int get_dirent();
void examine_sidedefs();
int is_switch();
int side_identical();
int change_linedefs();
void delete_sidedefs();

int main(argc,argv)

long argc;
char **argv;

	{
	int n;

	if (argc < 2)
		{
		printf("usage: dshrink <filename>\n");
		exit(0);
		}
	strcpy(filename,*(++argv));             /* get name of wad file */
	get_wad(filename);
	get_linedefs();         /* read the linedefs */
	get_sidedefs();         /* read the sidedefs */
	examine_sidedefs();     /* look for duplicate sidedefs */
	delete_sidedefs();      /* and delete them   */

/* Since we've reduced the number of sidedefs we're carrying around,
   items lying beyond the sidedefs will have new starting points in the
   wad file.  "delta" is how far up everything moved.
 */

	delta = deleted_sidedefs*sidedef_size;
	direc[get_dirent("SIDEDEFS")].d_size -= delta;
	dir_start -= delta;     /* adjust the directory start */
	if((outfile = fopen("tmp.wad","wb")) == 0)
		{
		printf("***** Error: Couldn't open output WAD file tmp.wad\n");
		exit(0);
		}
	fwrite(&wad,4,1,outfile);
	fwrite(&dir_entries,4,1,outfile);
	fwrite(&dir_start,4,1,outfile);
	xfer("THINGS");
	fwrite(linedefs,(sizeof(struct linedef)*num_lines),1,outfile);
	fwrite(sidedefs,sidedef_size*num_sides,1,outfile);

/* transfer remaining data as is, from its old home to its new home */

	xfer("VERTEXES");
	xfer("SEGS");
	xfer("SSECTORS");
	xfer("NODES");
	xfer("SECTORS");
	xfer("REJECT");
	xfer("BLOCKMAP");

/* adjust the starting offsets in the directory */

	direc[get_dirent("VERTEXES")].d_begin -= delta;
	direc[get_dirent("SEGS")].d_begin -= delta;
	direc[get_dirent("SSECTORS")].d_begin -= delta;
	direc[get_dirent("NODES")].d_begin -= delta;
	direc[get_dirent("SECTORS")].d_begin -= delta;
	direc[get_dirent("REJECT")].d_begin -= delta;
	direc[get_dirent("BLOCKMAP")].d_begin -= delta;

/* write the directory and quit */

	fwrite(direc,(sizeof(struct directory)*dir_entries),1,outfile);
	fclose(outfile);
	printf("***** Created reduced WAD file tmp.wad\n");
	return 0;
	}


void *memory(size)

int size;

	{
	void *ret = malloc(size);

	if (!ret)
		{
		printf("***** Error: Can't malloc %d bytes\n", size);
		exit(0);
		}
	return ret;
	}


/* open the wad file and read the directory */

void get_wad(filename)

char *filename;

	{
	struct directory *dir;
	long n;
	int dirsize,wadhdrsize;

	wadhdrsize = sizeof(struct header);
	dirsize = sizeof(struct directory);
	if ((infile = fopen(filename,"rb")) == 0)
		{
		printf("***** Error: Can't open %s\n",filename);
		exit(0);
		}
	fread(&wad,wadhdrsize,1,infile);
	dir_entries = wad.h_entries;
	dir_start = wad.h_begin;
	printf("***** Successfully opened %s\n",filename);
	direc = dir = memory(dirsize*dir_entries);
	fseek(infile,dir_start,0);
	for (n = 0; n < dir_entries; n++)
		fread(dir++,dirsize,1,infile);
	}

/* transfer items from the input WAD file to tmp.wad */

void xfer(string)

char *string;

	{
	int n;
	long length;

	n = get_dirent(string);
	length = direc[n].d_size;
	fseek(infile,direc[n].d_begin,0);
	do        
		{
		n = (length > XFERLEN ? XFERLEN : length);
		fread(xferbuf,n,1,infile);
		fwrite(xferbuf,n,1,outfile);
		length -= XFERLEN;
		}
	while (length > 0);
	}

/* get linedefs from the WAD file */

void get_linedefs()

	{
	int n;
	long length;

	n = get_dirent("LINEDEFS");
	length = direc[n].d_size;
	if (length == 0)
		{
		printf("***** Error: No linedefs!\n");
		exit(0);
		}
	num_lines = length/(sizeof(struct linedef));
	linedefs = memory(length);
	fseek(infile,direc[n].d_begin,0);
	fread(linedefs,length,1,infile);
	}

/* get sidedefs from the WAD file */

void get_sidedefs()

	{
	int n;
	long length;

	sidedef_size = sizeof(struct sidedef);
	n = get_dirent("SIDEDEFS");
	length = direc[n].d_size;
	if (length == 0)
		{
		printf("***** Error: No sidedefs!\n");
		exit(0);
		}
	num_sides = length/sidedef_size;
	printf("***** Started with %d sidedefs\n",num_sides);
	sidedefs = memory(length);
	fseek(infile,direc[n].d_begin,0);
	fread(sidedefs,length,1,infile);
	}

void copy_sidedef(source,dest)

int source,dest;

	{
	int i;
	unsigned char *ptr1, *ptr2;

	ptr1 = &sidedefs[source];
	ptr2 = &sidedefs[dest];
	for (i = 0 ; i < sidedef_size ; i++)
		*ptr2++ = *ptr1++;
	}

/*

print_linedefs()

	{
	int i;
	struct linedef *line = linedefs;

	for (i = 0 ; i < num_lines ; i++)
		{
		printf("line %d:\n",i);
		if (line->l_side1 == -1)
			printf("none\n");
		else
			print_side(line->l_side1); 
		if (line->l_side2 == -1)
			printf("none\n");
		else
			print_side(line->l_side2);
		line++;
		}
	}
	
print_sidedefs()

	{
	int i;

	for (i = 0 ; i < num_sides ; i++)
		print_side(i);
	}

print_side(i)

int i;

	{
	struct sidedef *p = &sidedefs[i];

	printf("side %d:\n",i);
	printf("%d, %d, ",p->s_x_off,p->s_y_off);
	if (p->s_upper[0] == '-')
		printf("-       , ");
	else
		printf("%c%c%c%c%c%c%c%c, ",p->s_upper[0],p->s_upper[1],p->s_upper[2],p->s_upper[3],p->s_upper[4],p->s_upper[5],p->s_upper[6],p->s_upper[7]);
	if (p->s_lower[0] == '-')
		printf("-       , ");
	else
		printf("%c%c%c%c%c%c%c%c, ",p->s_lower[0],p->s_lower[1],p->s_lower[2],p->s_lower[3],p->s_lower[4],p->s_lower[5],p->s_lower[6],p->s_lower[7]);
	if (p->s_normal[0] == '-')
		printf("-       , ");
	else
		printf("%c%c%c%c%c%c%c%c, ",p->s_normal[0],p->s_normal[1],p->s_normal[2],p->s_normal[3],p->s_normal[4],p->s_normal[5],p->s_normal[6],p->s_normal[7]);
	printf("%d\n",p->s_sector);
	}

 */

/* get information on some item in the WAD file */

int get_dirent(name)

char *name;

	{
	struct directory *dir;
	int n;

	dir = direc;
	for (n = 0 ; n < dir_entries ; n++)
		{
		strncpy(dir_name,dir->d_name,8);
		if (strcmp(dir_name,name) == 0)
			return n;
		dir++;
		}
	printf("***** Error: Couldn't find %s\n",name);
	exit(0);
	}

/* Examine the sidedefs, looking for duplicates that can be removed.
   If the candidate is a switch, skip it, otherwise you might be
   dynamically changing textures at run time you didn't mean to. */

void examine_sidedefs()

    {
    int master = 0;
    int target = 0;
    int deleted = 0;
    int change_linedefs();

    while (master < num_sides)
	{
	if (sidedefs[master].s_sector >= 0) /* skip if already obsolete */
	    if (!is_switch(master))       /* skip if texture is a switch */
		{
		target = master + 1;
		while (target < num_sides)
		    {
		    if (sidedefs[target].s_sector >= 0) /* skip if already obsolete */
			if (side_identical(master,target) > 0)
			    if (change_linedefs(target,master))
				{
				sidedefs[target].s_sector = -1;  /* obsolete */
				deleted++;
				}
		    target++;
		    }
		}
	master++;
	}
    }

/* If any of the 3 textures for this sidedef is a switch, the sidedef is not 
   a candidate for compression.  If this was allowed, then when the switch is
   activated, all duplicated sidedefs will change texture from SW1 to
   SW2.  (Actually, this might be a nice effect if you used the level 
   editor to manually make them all use the same sidedef...)
 */

int is_switch(side)

int side;

	{
	struct sidedef *ptr = &sidedefs[side];        
	
	if (ptr->s_upper[0] == 'S' && ptr->s_upper[1] == 'W')
		return(1);
	if (ptr->s_lower[0] == 'S' && ptr->s_lower[1] == 'W')
		return(1);
	if (ptr->s_normal[0] == 'S' && ptr->s_normal[1] == 'W')
		return(1);
	return(0);
	}

/* Are these two sidedefs identical? For some reason that escapes me now,
   we have to do this field-by-field rather than a quick 30-byte compare. */

int side_identical(m,t)

int m,t;

	{
	int i;
	struct sidedef *ptr1,*ptr2;

	ptr1 = &sidedefs[m];
	ptr2 = &sidedefs[t];
	if (ptr1->s_sector != ptr2->s_sector)
		return(0);
	if (ptr1->s_normal[0] != ptr2->s_normal[0])
		return(0);
	if (ptr1->s_upper[0] != ptr2->s_upper[0])
		return(0);
	if (ptr1->s_lower[0] != ptr2->s_lower[0])
		return(0);
	if (ptr1->s_normal[0] != '-')
		if (strncmp(&ptr1->s_normal[1],&ptr2->s_normal[1],7) != 0)
			return(0);
	if (ptr1->s_upper[0] != '-')
		if (strncmp(&ptr1->s_upper[1],&ptr2->s_upper[1],7) != 0)
			return(0);
	if (ptr1->s_lower[0] != '-')
		if (strncmp(&ptr1->s_lower[1],&ptr2->s_lower[1],7) != 0)
			return(0);
	if (ptr1->s_x_off != ptr2->s_x_off)
		return(0);
	if (ptr1->s_y_off != ptr2->s_y_off)
		return(0);
	return(1);
	}

/* Change the linedefs from the old sidedef to the new sidedef.
   The only time you don't want to do this is if the two sidedefs for a
   linedef end up being the same sidedef.  DOOM doesn't like this and will
   hang.  I know; I saw it.
 */
 
int change_linedefs(old,new)

int new,old;

	{
	int value = 1;
	int i;
	struct linedef *ptr = linedefs;

	for (i = 0 ; i < num_lines ; i++)
		{
		if (ptr->l_side1 == old)
			if (ptr->l_side2 == new)
				value = 0;
			else
				ptr->l_side1 = new;
		if (ptr->l_side2 == old)
			if (ptr->l_side1 == new)
				value = 0;
			else
				ptr->l_side2 = new;
		ptr++;
		}
	return(value);
	}

/* Delete the obsolete sidedefs that are no longer used by linedefs */

void delete_sidedefs()

	{
	int target = 0;
	int source = num_sides-1;

	while (source > target)
		{
		if (sidedefs[target].s_sector == -1)  /* is sidedef obsolete ? */
			{
			while (sidedefs[source].s_sector == -1)
				{
				num_sides--;    /* one less sidedef */
				deleted_sidedefs++;
				source--;       /* move to previous sidedef */
				}
			if (source <= target)
				continue;
			copy_sidedef(source,target);
			change_linedefs(source,target);
			sidedefs[source].s_sector = -1; /* now obsolete */
			num_sides--;    /* one less sidedef */
			deleted_sidedefs++;
			source--;       /* move to previous sidedef */
			}
		target++;
		}
	while (sidedefs[source].s_sector == -1)
		{
		num_sides--;    /* one less sidedef */
		deleted_sidedefs++;
		source--;       /* move to previous sidedef */
		}
	printf("***** Deleted %d duplicate sidedefs\n",deleted_sidedefs);
	printf("***** Leaving %d unique sidedefs\n",num_sides);
	}

