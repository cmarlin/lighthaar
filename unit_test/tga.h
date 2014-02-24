#ifndef TGA_HEADER_H
#define TGA_HEADER_H

#include <stdio.h>


#define IMAGE_TYPE_RGB (0x02)
#define DESCRIPTOR_H_FLIP (0x32)

#pragma pack(1)   // n=1

struct TGA_HEADER
{
    unsigned char  identsize;          // size of ID field that follows 18 byte header (0 usually)
    unsigned char  colourmaptype;      // type of colour map 0=none, 1=has palette
    unsigned char  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    short colourmapstart;     // first colour map entry in palette
    short colourmaplength;    // number of colours in palette
    unsigned char  colourmapbits;      // number of bits per palette entry 15,16,24,32

    short xstart;             // image x origin
    short ystart;             // image y origin
    short width;              // image width in pixels
    short height;             // image height in pixels
    unsigned char  bits;               // image bits per pixel 8,16,24,32
    unsigned char  descriptor;         // image descriptor bits (vh flip bits)
    
    // pixel data follows header
    
};

// datas in 24 bits are formated a 'BGR'



inline void readTGA(TGA_HEADER& header, FILE *fin)
{
	fread(&header.identsize, sizeof(header.identsize), 1, fin);
    fread(&header.colourmaptype, sizeof(header.colourmaptype), 1, fin);
    fread(&header.imagetype, sizeof(header.imagetype), 1, fin);

    fread(&header.colourmapstart, sizeof(header.colourmapstart), 1, fin);
    fread(&header.colourmaplength, sizeof(header.colourmaplength), 1, fin);
    fread(&header.colourmapbits, sizeof(header.colourmapbits), 1, fin);

    fread(&header.xstart, sizeof(header.xstart), 1, fin);
    fread(&header.ystart, sizeof(header.ystart), 1, fin);
    fread(&header.width, sizeof(header.width), 1, fin);
    fread(&header.height, sizeof(header.height), 1, fin);
    fread(&header.bits, sizeof(header.bits), 1, fin);
    fread(&header.descriptor, sizeof(header.descriptor), 1, fin);

	unsigned char dummy[255];
	fread(dummy, header.identsize, 1, fin);
}

inline void writeTGA(TGA_HEADER& header, FILE *fin)
{
	fwrite(&header.identsize, sizeof(header.identsize), 1, fin);
    fwrite(&header.colourmaptype, sizeof(header.colourmaptype), 1, fin);
    fwrite(&header.imagetype, sizeof(header.imagetype), 1, fin);

    fwrite(&header.colourmapstart, sizeof(header.colourmapstart), 1, fin);
    fwrite(&header.colourmaplength, sizeof(header.colourmaplength), 1, fin);
    fwrite(&header.colourmapbits, sizeof(header.colourmapbits), 1, fin);

    fwrite(&header.xstart, sizeof(header.xstart), 1, fin);
    fwrite(&header.ystart, sizeof(header.ystart), 1, fin);
    fwrite(&header.width, sizeof(header.width), 1, fin);
    fwrite(&header.height, sizeof(header.height), 1, fin);
    fwrite(&header.bits, sizeof(header.bits), 1, fin);
    fwrite(&header.descriptor, sizeof(header.descriptor), 1, fin);
}

#pragma pack()   // default

#endif //TGA_HEADER
