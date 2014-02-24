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
	unsigned char* decompress_yuy2_buffer;
	bool swapfields;
	bool decompressing;

	// methods
	CodecInst()
		: yuy2_buffer(0)
		, median_buffer(0)
		, rgb_buffer(0)
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

