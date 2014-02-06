//
//


#include <windows.h>
#include <vfw.h>
#include "lighthaar.h"
#pragma hdrstop


static const DWORD FOURCC_LGTH = mmioFOURCC('L','G','T','H');   // our compressed format


extern HMODULE hModuleLightHaar;

using namespace LightHaar;

struct CodecInst
{

	unsigned char* yuy2_buffer;
	unsigned char* median_buffer;
	unsigned char* rgb_buffer;
	//uint8_t* y_buffer;
	//uint8_t* u_buffer;
	//uint8_t* v_buffer;

	LightHaar::Codec m_oCodec;
	/*
	static const int m_iFrameBufferCount = 3;
	FrameBuffer m_frameBuffers[m_iFrameBufferCount];
	int m_curFrameBuffer;

	int m_iFrameCounter;
	*/
	unsigned char* decompress_yuy2_buffer;
	bool swapfields;
	bool decompressing;

	// methods
	CodecInst()
		: yuy2_buffer(0)
		, median_buffer(0)
		, rgb_buffer(0)
		//, m_curFrameBuffer(0)
		//, m_iFrameCounter(0)
		, decompress_yuy2_buffer(0)
		, decompressing(false) {}

	BOOL QueryAbout();
	DWORD About(HWND hwnd);

	BOOL QueryConfigure();
	DWORD Configure(HWND hwnd);

	DWORD GetState(LPVOID pv, DWORD dwSize);
	DWORD SetState(LPVOID pv, DWORD dwSize);

	DWORD GetInfo(ICINFO* icinfo, DWORD dwSize);

	DWORD CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD Compress(ICCOMPRESS* icinfo, DWORD dwSize);
	DWORD CompressEnd();

	DWORD DecompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD Decompress(ICDECOMPRESS* icinfo, DWORD dwSize);
	DWORD DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressEnd();

	void AllocBuffers(int _width, int _height);
	void ReleaseBuffers();

	/*
	DWORD DrawQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DrawBegin(ICDRAWBEGIN* icinfo, DWORD dwSize);
	DWORD Draw(ICDRAW* icinfo, DWORD dwSize);
	DWORD DrawEnd();
	DWORD DrawWindow(PRECT prc);
	*/
};

CodecInst* Open(ICOPEN* icinfo);
DWORD Close(CodecInst* pinst);


extern "C" {
	// fixed pregenerated tables (in tables.cpp)
	extern const unsigned char left_yuv[], grad_yuv[], med_yuv[];
	extern const unsigned char left_rgb[], left_decorrelate_rgb[], grad_decorrelate_rgb[];
	extern const unsigned char classic_shift_luma[], classic_shift_chroma[];
	extern const unsigned char classic_add_luma[256], classic_add_chroma[256];

	// tables generated at runtime for compression/decompression
	extern unsigned char encode1_shift[256], encode2_shift[256], encode3_shift[256];
	extern unsigned encode1_add_shifted[256], encode2_add_shifted[256], encode3_add_shifted[256];
	struct DecodeTable {
		unsigned char* table_pointers[32];
		unsigned char table_data[129*25];
	};
	extern DecodeTable decode1, decode2, decode3;
	extern unsigned char decode1_shift[256], decode2_shift[256], decode3_shift[256];
};
