#ifndef __PMP_FILE_H__
#define __PMP_FILE_H__


typedef unsigned short u16;
typedef unsigned int u32;

struct Tile {
	u16 texture1; // index into texture_textures[]
	u16 texture2; // index, or 0xFFFF for 'none'
	u32 priority; // ???
};

struct pmp_header
{
	char marker[4];
    u32 version;
    u32 data_size; 
};

#endif