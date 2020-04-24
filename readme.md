## Dshrink

Dshrink is a command line application to compress sidedefs in a Doom PWAD. It can greatly reduce the size of WADS, typically removing over 50% of the sidedefs.

It was originally written by Rand Phares in 1994.

### Usage

```
dshrink.exe in.wad
```

This will generate an output file tmp.wad. **DO NOT RENAME** this to in.wad. Modern level editors such as Doom Builder can _uncompress_ sidedefs however most older editors do not support this feature. This will leave a level that cannot be edited until the sidedefs are uncompressed.

This tool is meant to be run immediately before a map is released.

### WAD Limitations

* The input WAD must contain a single map with no additional lumps. i.e., no `TEXTURE2`, `MAPINFO`, `D_RUNNIN`, or `F_START`/`F_END`.
* The map must have less than 32,768 lindefs, which was the original limit of doom.exe. Most source ports support at least 65,535 linedefs however dshrink.exe cannot compress sidedefs of such a large map.
