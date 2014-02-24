/**
	losless haar compression on 8 bits
*/
#include <memory.h>
#include <algorithm>
#include <vector>
#include "stdint.h"
#include "tga.h"
#include "lighthaar.h"
#include "utils.h"
#include "codec_cdf53.h"
#include "codec_haar.h"


void encodeFile(FILE* fout, FILE* fin)
{
	TGA_HEADER inHeader;
	readTGA(inHeader, fin);

	TGA_HEADER outHeader;
	memset(&outHeader, 0, sizeof(outHeader));
	outHeader.imagetype = IMAGE_TYPE_RGB;
	outHeader.bits = 24;
	outHeader.width = inHeader.width;
	outHeader.height = inHeader.height;
	outHeader.descriptor = inHeader.descriptor;
	writeTGA(outHeader, fout);

	int iBytesPerPixel = (inHeader.bits/8);
	unsigned char* inBuffer = new unsigned char[inHeader.width*inHeader.height*iBytesPerPixel];

	for(int j=0; j<inHeader.height; j++)
	{
		fread(&inBuffer[j*inHeader.width*iBytesPerPixel], iBytesPerPixel, inHeader.width, fin);
	}

	int width = inHeader.width;
	uint8_t* filter_R = new uint8_t[inHeader.height*inHeader.width];
	uint8_t* filter_G = new uint8_t[inHeader.height*inHeader.width];
	uint8_t* filter_B = new uint8_t[inHeader.height*inHeader.width];
	//memset(buffer2, 0, inHeader.width*inHeader.height*3);
	for(int j=0; j<inHeader.height; j++)
	{
		for(int i=0; i<inHeader.width; i++)
		{
			filter_R[j*width+i] = inBuffer[(j*width+i)*iBytesPerPixel+2];
			filter_G[j*width+i] = inBuffer[(j*width+i)*iBytesPerPixel+1];
			filter_B[j*width+i] = inBuffer[(j*width+i)*iBytesPerPixel+0];
		}
	}

	int iQuality = 4;
	shift_u8_to_s8(filter_R, inHeader.width, inHeader.height);
	shift_u8_to_s8(filter_G, inHeader.width, inHeader.height);
	shift_u8_to_s8(filter_B, inHeader.width, inHeader.height);
	//EncodeCDF53((int8_t*)filter_R, (int8_t*)filter_R, inHeader.width, inHeader.height, iQuality);
	//EncodeCDF53((int8_t*)filter_G, (int8_t*)filter_G, inHeader.width, inHeader.height, iQuality);
	//EncodeCDF53((int8_t*)filter_B, (int8_t*)filter_B, inHeader.width, inHeader.height, iQuality);
	EncodeHaar((int8_t*)filter_R, (int8_t*)filter_R, inHeader.width, inHeader.height, iQuality);
	EncodeHaar((int8_t*)filter_G, (int8_t*)filter_G, inHeader.width, inHeader.height, iQuality);
	EncodeHaar((int8_t*)filter_B, (int8_t*)filter_B, inHeader.width, inHeader.height, iQuality);
	
	//int64_t iEnergyR = EnergyHaarCoeff((int8_t*)filter_R, inHeader.width, inHeader.height);
	//int64_t iEnergyG = EnergyHaarCoeff((int8_t*)filter_G, inHeader.width, inHeader.height);
	//int64_t iEnergyB = EnergyHaarCoeff((int8_t*)filter_B, inHeader.width, inHeader.height);

#if 0
	// output coeffs without decoding
	for(int j=0; j<inHeader.height; j++)
	{
		for(int i=0; i<inHeader.width; i++)
		{
			filter_R[j*width+i] = (filter_R[j*width+i]) & 255;
			filter_G[j*width+i] = (filter_G[j*width+i]) & 255;
			filter_B[j*width+i] = (filter_B[j*width+i]) & 255;
		}
	}
#else
	//DecodeCDF53((int8_t*)filter_R, (int8_t*)filter_R, inHeader.width, inHeader.height, iQuality);
	//DecodeCDF53((int8_t*)filter_G, (int8_t*)filter_G, inHeader.width, inHeader.height, iQuality);
	//DecodeCDF53((int8_t*)filter_B, (int8_t*)filter_B, inHeader.width, inHeader.height, iQuality);
	DecodeHaar((int8_t*)filter_R, (int8_t*)filter_R, inHeader.width, inHeader.height, iQuality);
	DecodeHaar((int8_t*)filter_G, (int8_t*)filter_G, inHeader.width, inHeader.height, iQuality);
	DecodeHaar((int8_t*)filter_B, (int8_t*)filter_B, inHeader.width, inHeader.height, iQuality);

	//DecodeCDF53(filter_R, filter_R, inHeader.width, inHeader.height);
	//DecodeCDF53(filter_G, filter_G, inHeader.width, inHeader.height);
	//DecodeCDF53(filter_B, filter_B, inHeader.width, inHeader.height);
#endif

	shift_s8_to_u8(filter_R, inHeader.width, inHeader.height);
	shift_s8_to_u8(filter_G, inHeader.width, inHeader.height);
	shift_s8_to_u8(filter_B, inHeader.width, inHeader.height);

	unsigned char* outBuffer = new unsigned char[inHeader.width*inHeader.height*3];

	for(int j=0; j<inHeader.height; j++)
	{
		for(int i=0; i<inHeader.width; i++)
		{
			/*			
			outBuffer[(j*size+i)*3+2] = filter_R[j*size+i]/2+127;
			outBuffer[(j*size+i)*3+1] = filter_G[j*size+i]/2+127;
			outBuffer[(j*size+i)*3+0] = filter_B[j*size+i]/2+127;
			*/

			//outBuffer[(j*width+i)*3+2] = UClamp(filter_R[j*width+i]*3/2);
			//outBuffer[(j*width+i)*3+1] = UClamp(filter_G[j*width+i]*3/2);
			//outBuffer[(j*width+i)*3+0] = UClamp(filter_B[j*width+i]*3/2);

			outBuffer[(j*width+i)*3+2] = filter_R[j*width+i];
			outBuffer[(j*width+i)*3+1] = filter_G[j*width+i];
			outBuffer[(j*width+i)*3+0] = filter_B[j*width+i];
		}
	}

	for(int j=0; j<inHeader.height; j++)
	{
		fwrite(&outBuffer[j*inHeader.width*3], 3, inHeader.width, fout);
	}

	double psnr = PSNR(inBuffer, outBuffer, inHeader.width*inHeader.height*3);
	printf ("PSNR: %5.2f, ", psnr);

	double shift = SHIFT(inBuffer, outBuffer, inHeader.width*inHeader.height*3);
	printf ("SHIFT: %5.2f, ", shift);

	delete inBuffer;
	delete outBuffer;
	delete filter_R;
	delete filter_G;
	delete filter_B;
}



void decodeFile(FILE* fout, FILE* fin)
{
	TGA_HEADER inHeader;
	readTGA(inHeader, fin);


	TGA_HEADER outHeader;
	memset(&outHeader, 0, sizeof(outHeader));
	outHeader.imagetype = IMAGE_TYPE_RGB;
	outHeader.bits = 24;
	outHeader.width = inHeader.width;
	outHeader.height = inHeader.height;
	outHeader.descriptor = inHeader.descriptor;
	writeTGA(outHeader, fout);

	unsigned char* inBuffer = new unsigned char[inHeader.width*inHeader.height*3];

	for(int j=0; j<inHeader.height; j++)
	{
		fread(&inBuffer[j*inHeader.width*3], 3, inHeader.width, fin);
	}

	int size = inHeader.width;
	uint8_t* filter_R = new uint8_t[size*size];
	uint8_t* filter_G = new uint8_t[size*size];
	uint8_t* filter_B = new uint8_t[size*size];
	//memset(buffer2, 0, inHeader.width*inHeader.height*3);
	for(int i=0; i<inHeader.width*inHeader.height; i++)
	{
		filter_R[i] = (inBuffer[i*3+2]-127)*2;
		filter_G[i] = (inBuffer[i*3+1]-127)*2;
		filter_B[i] = (inBuffer[i*3+0]-127)*2;
	}

	unsigned char* outBuffer = new unsigned char[size*size*3];

	// convert to bytes
	for (int i = 0; i < size*size; i++)
	{
		outBuffer[i*3+2] = (unsigned char) filter_R[i];
		outBuffer[i*3+1] = (unsigned char) filter_G[i];
		outBuffer[i*3+0] = (unsigned char) filter_B[i];
	}

	for(int j=0; j<inHeader.height; j++)
	{
		fwrite(&outBuffer[j*inHeader.width*3], 3, inHeader.width, fout);
	}

	delete inBuffer;
	delete outBuffer;
	delete filter_R;
	delete filter_G;
	delete filter_B;
}

