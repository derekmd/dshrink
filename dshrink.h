struct header {
    char h_type[4];
    long int h_entries;
    long int h_begin;
} wad;

struct directory {
    long int d_begin;
    long int d_size;
    char d_name[8];
};

struct linedef {
    short int l_from;
    short int l_to;
    short int l_flags;
    short int l_type;
    short int l_tag;
    short int l_side1;
    short int l_side2;
};

struct sidedef {
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

void *memory(int size);
void get_wad(char *filename);
void xfer(char *str);
void get_linedefs();
void get_sidedefs();
void copy_sidedef(int source, int dest);
int get_dirent(char *name);
void examine_sidedefs();
int is_switch(int side);
int side_identical(int m, int t);
int change_linedefs(int old, int replacement);
void delete_sidedefs();
