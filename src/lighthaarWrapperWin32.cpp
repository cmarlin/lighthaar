//
// LightHaar codec By Cyril MARLIN & Mathieu LEGRIS
// MIT licence
//
// Based on MSYUV sample code, which is:
// Copyright (c) 1993 Microsoft Corporation.
// All Rights Reserved.
//
//


#include "lighthaarWrapperWin32.h"
#include "resource.h"

#include <crtdbg.h>
#include <algorithm>
#include <vector>
#include <limits.h>

#include "arith.h"
#include "codec_rl0.h"
#include "codec_yuv.h"
#include "codec_cdf53.h"
#include "utils.h"

TCHAR szDescription[] = TEXT("LightHaar v0.1");
TCHAR szName[]        = TEXT("LightHaar");

//#define VERSION         0x00020001      // 2.1
#define VERSION         0x00000001      // 0.1


/********************************************************************
********************************************************************/

void Msg(const char fmt[], ...)
{
	static int debug = GetPrivateProfileInt("debug", "log", 0, "lighthaar.ini");
	if (!debug) return;

	DWORD written;
	char buf[2000];
	va_list val;

	va_start(val, fmt);
	wvsprintf(buf, fmt, val);

	const COORD _80x50 = {80,50};
	static BOOL startup = (AllocConsole(), SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), _80x50));
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, lstrlen(buf), &written, 0);
}


int AppFlags()
{
	static int flags = -1;
	if (flags < 0)
	{
		flags = 0;
		TCHAR apppath[MAX_PATH];
		if (GetModuleFileName(NULL, apppath, MAX_PATH))
		{
			TCHAR* appname = strrchr(apppath, '\\');
			appname = appname ? appname+1 : apppath;
			Msg("App name is %s; ", appname);
			if (!lstrcmpi(appname, TEXT("premiere.exe")))
				flags = 1;
			if (!lstrcmpi(appname, TEXT("veditor.exe")))
				flags = 1;
			if (!lstrcmpi(appname, TEXT("avi2mpg2_vfw.exe")))
				flags = 1;
			if (!lstrcmpi(appname, TEXT("bink.exe")))
				flags = 1;
			if (!lstrcmpi(appname, TEXT("afterfx.exe")))
				flags = 2;
			Msg("flags=%d\n", flags);
		}
	}
	return flags;
}

bool SuggestRGB()
{
	return !!GetPrivateProfileInt("debug", "rgboutput", AppFlags()&1, "lighthaar.ini");
}

bool AllowRGBA()
{
	return !!GetPrivateProfileInt("general", "enable_rgba", AppFlags()&2, "lighthaar.ini");
}


/********************************************************************
********************************************************************/

CodecInst* Open(ICOPEN* icinfo)
{
	if (icinfo && icinfo->fccType != ICTYPE_VIDEO)
		return NULL;

	CodecInst* pinst = new CodecInst();

	if (icinfo) icinfo->dwError = pinst ? ICERR_OK : ICERR_MEMORY;

	return pinst;
}

DWORD Close(CodecInst* pinst)
{
	//    delete pinst;       // this caused problems when deleting at app close time
	return 1;
}

/********************************************************************
********************************************************************/
/*
enum
{
	methodLeft=0, methodGrad=1, methodMedian=2,
	methodConvertToYUY2=-1, methodOld=-2,
	flagDecorrelate=64
};
*/
//int ConvertOldMethod(int bitcount)
//{
//	switch (bitcount&7) {
//	case 1: return methodLeft;
//	case 2: return methodLeft|flagDecorrelate;
//	case 3: return (bitcount>=24) ? methodGrad+flagDecorrelate : methodGrad;
//	case 4: return methodMedian;
//	default: return methodOld;
//	}
//}
#include <assert.h>

static inline int GetMethod(LPBITMAPINFOHEADER lpbi)
{
	if (lpbi->biCompression == FOURCC_LGTH)
	{
		//if (lpbi->biBitCount & 7)
		//	return ConvertOldMethod(lpbi->biBitCount);
		//else if (lpbi->biSize > sizeof(BITMAPINFOHEADER))
		return *((unsigned char*)lpbi + sizeof(BITMAPINFOHEADER));
	}
	assert(false);
	return 0; //methodOld;
}

static inline int GetBitCount(LPBITMAPINFOHEADER lpbi)
{
	if (lpbi->biCompression == FOURCC_LGTH && lpbi->biSize > sizeof(BITMAPINFOHEADER)+1)
	{
		int bpp_override = *((char*)lpbi + sizeof(BITMAPINFOHEADER) + 1);
		if (bpp_override)
			return bpp_override;
	}
	return lpbi->biBitCount;
}

//struct MethodName { int method; const char* name; };
/*MethodName yuv_method_names[] =
{
	{ methodLeft, "Predict left (fastest)" },
	{ methodGrad, "Predict gradient" },
	{ methodMedian, "Predict median (best)" }
};
MethodName rgb_method_names[] =
{
	{ methodLeft, "Predict left/no decorr. (fastest)" },
	{ methodLeft+flagDecorrelate, "Predict left" },
	{ methodGrad+flagDecorrelate, "Predict gradient (best)" },
	{ methodConvertToYUY2, "<-- Convert to YUY2" }
};

bool IsLegalMethod(int method, bool rgb)
{
	if (rgb) {
		return (method == methodOld || method == methodLeft
			|| method == methodLeft+flagDecorrelate || method == methodGrad+flagDecorrelate);
	}
	else {
		return (method == methodOld || method == methodLeft
			|| method == methodGrad || method == methodMedian);
	}
}
*/
/********************************************************************
********************************************************************/

BOOL CodecInst::QueryAbout() { return TRUE; }

static BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND) {
		switch (LOWORD(wParam)) {
case IDOK:
	EndDialog(hwndDlg, 0);
	break;
case IDC_HOMEPAGE:
	ShellExecute(NULL, NULL, "http://www.math.berkeley.edu/~benrg/huffyuv.html", NULL, NULL, SW_SHOW);
	break;
case IDC_EMAIL:
	ShellExecute(NULL, NULL, "mailto:benrg@math.berkeley.edu", NULL, NULL, SW_SHOW);
	break;
		}
	}
	return FALSE;
}

DWORD CodecInst::About(HWND hwnd)
{
	DialogBox(hModuleLightHaar, MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDialogProc);
	return ICERR_OK;
}

static BOOL CALLBACK ConfigureDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		HWND hctlIFreq = GetDlgItem(hwndDlg, IDC_I_FREQ);
		int iIFreq = GetPrivateProfileInt("general", "i_freq", 10, "lighthaar.ini");
		char tmp_string[32];
		_itoa_s(iIFreq, tmp_string, sizeof(tmp_string), 10);
		SendMessage(hctlIFreq, WM_SETTEXT, 0, (LPARAM)tmp_string);
		/*for (int i = 0; i < sizeof(yuv_method_names)/sizeof(yuv_method_names[0]); ++i)
		{
			SendMessage(hctlYUY2Method, CB_ADDSTRING, 0, (LPARAM)yuv_method_names[i].name);
			if (yuv_method_names[i].method == yuy2method)
				SendMessage(hctlYUY2Method, CB_SETCURSEL, i, 0);
		}*/
		/*
		HWND hctlRGBMethod = GetDlgItem(hwndDlg, IDC_RGBMETHOD);
		int rgbmethod = GetPrivateProfileInt("general", "rgbmethod", methodGrad+flagDecorrelate, "lighthaar.ini");
		for (int j = 0; j < sizeof(rgb_method_names)/sizeof(rgb_method_names[0]); ++j)
		{
			SendMessage(hctlRGBMethod, CB_ADDSTRING, 0, (LPARAM)rgb_method_names[j].name);
			if (rgb_method_names[j].method == rgbmethod)
				SendMessage(hctlRGBMethod, CB_SETCURSEL, j, 0);
		}*/
		
		HWND hctlQuality = GetDlgItem(hwndDlg, IDC_QUALITY);
		int iQuality = GetPrivateProfileInt("general", "quality", 10, "lighthaar.ini");
		_itoa_s(iQuality, tmp_string, sizeof(tmp_string), 10);
		SendMessage(hctlQuality, WM_SETTEXT, 0, (LPARAM)tmp_string);
		/*for (int j = 0; j < sizeof(rgb_method_names)/sizeof(rgb_method_names[0]); ++j)
		{
			SendMessage(hctlRGBMethod, CB_ADDSTRING, 0, (LPARAM)rgb_method_names[j].name);
			if (rgb_method_names[j].method == rgbmethod)
				SendMessage(hctlRGBMethod, CB_SETCURSEL, j, 0);
		}*/

		CheckDlgButton(hwndDlg, IDC_RGBOUTPUT,
			GetPrivateProfileInt("debug", "rgboutput", false, "lighthaar.ini") ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_RGBA,
			GetPrivateProfileInt("general", "enable_rgba", false, "lighthaar.ini") ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SWAPFIELDS,
			GetPrivateProfileInt("debug", "decomp_swap_fields", false, "lighthaar.ini") ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_LOG,
			GetPrivateProfileInt("debug", "log", false, "lighthaar.ini") ? BST_CHECKED : BST_UNCHECKED);
	}

	else if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				char tmp_string[32];
				SendMessage(GetDlgItem(hwndDlg, IDC_I_FREQ), WM_GETTEXT, (LPARAM)sizeof(tmp_string), (WPARAM)tmp_string);
				WritePrivateProfileString("general", "i_freq", tmp_string, "lighthaar.ini");
				SendMessage(GetDlgItem(hwndDlg, IDC_QUALITY), WM_GETTEXT, (LPARAM)sizeof(tmp_string), (WPARAM)tmp_string);
				WritePrivateProfileString("general", "quality", tmp_string, "lighthaar.ini");
			}
			WritePrivateProfileString("debug", "rgboutput",
				(IsDlgButtonChecked(hwndDlg, IDC_RGBOUTPUT) == BST_CHECKED) ? "1" : NULL, "lighthaar.ini");
			WritePrivateProfileString("general", "enable_rgba",
				(IsDlgButtonChecked(hwndDlg, IDC_RGBA) == BST_CHECKED) ? "1" : NULL, "lighthaar.ini");
			WritePrivateProfileString("debug", "decomp_swap_fields",
				(IsDlgButtonChecked(hwndDlg, IDC_SWAPFIELDS) == BST_CHECKED) ? "1" : "0", "lighthaar.ini");
			WritePrivateProfileString("debug", "log",
				(IsDlgButtonChecked(hwndDlg, IDC_LOG) == BST_CHECKED) ? "1" : "0", "lighthaar.ini");

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;

		default:
			return AboutDialogProc(hwndDlg, uMsg, wParam, lParam);    // handle email and home-page buttons
		}
	}
	return FALSE;
}

BOOL CodecInst::QueryConfigure() { return TRUE; }

DWORD CodecInst::Configure(HWND hwnd)
{
	DialogBox(hModuleLightHaar, MAKEINTRESOURCE(IDD_CONFIGURE), hwnd, ConfigureDialogProc);
	return ICERR_OK;
}


/********************************************************************
********************************************************************/


// we have no state information which needs to be stored

DWORD CodecInst::GetState(LPVOID pv, DWORD dwSize) { return 0; }

DWORD CodecInst::SetState(LPVOID pv, DWORD dwSize) { return 0; }


DWORD CodecInst::GetInfo(ICINFO* icinfo, DWORD dwSize)
{
	if (icinfo == NULL)
		return sizeof(ICINFO);

	if (dwSize < sizeof(ICINFO))
		return 0;

	icinfo->dwSize            = sizeof(ICINFO);
	icinfo->fccType           = ICTYPE_VIDEO;
	icinfo->fccHandler        = FOURCC_LGTH;
	icinfo->dwFlags           = VIDCF_FASTTEMPORALC|VIDCF_FASTTEMPORALD;

	icinfo->dwVersion         = VERSION;
	icinfo->dwVersionICM      = ICVERSION;
	MultiByteToWideChar(CP_ACP, 0, szDescription, -1, icinfo->szDescription, sizeof(icinfo->szDescription)/sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, szName, -1, icinfo->szName, sizeof(icinfo->szName)/sizeof(WCHAR));

	return sizeof(ICINFO);
}


/********************************************************************
********************************************************************/


struct PrintBitmapType
{
	char s[32];
	PrintBitmapType(LPBITMAPINFOHEADER lpbi)
	{
		if (!lpbi)
		{
			strcpy_s(s, 32, "(null)");
		}
		else
		{
			*(DWORD*)s = lpbi->biCompression;
			s[4] = 0;
			if (!isalnum(s[0]) || !isalnum(s[1]) || !isalnum(s[2]) || !isalnum(s[3]))
				wsprintfA(s, "%x", lpbi->biCompression);
			wsprintfA(strchr(s, 0), ", %d bits", GetBitCount(lpbi));
			//if (lpbi->biCompression == FOURCC_LGTH && !(lpbi->biBitCount&7) && lpbi->biSize > sizeof(BITMAPINFOHEADER))
			//	wsprintfA(strchr(s, 0), ", method %d", *((unsigned char*)lpbi + sizeof(BITMAPINFOHEADER)));
			if (lpbi->biCompression == FOURCC_LGTH && lpbi->biSize > sizeof(BITMAPINFOHEADER))
			{
				wsprintfA(strchr(s, 0), ", method %d", *((unsigned char*)lpbi + sizeof(BITMAPINFOHEADER)));
			}
		}
	}
};


/********************************************************************
********************************************************************/


// 0=unknown, -1=compressed YUY2, -2=compressed RGB, -3=compressed RGBA, 1=YUY2, 2=UYVY, 3=RGB 24-bit, 4=RGB 32-bit
static int GetBitmapType(LPBITMAPINFOHEADER lpbi)
{
	if (!lpbi)
		return 0;
	const int fourcc = lpbi->biCompression;
	if (fourcc == FOURCC_LGTH)
		return 1;
	const int bitcount = GetBitCount(lpbi);
	if (fourcc == 0 || fourcc == ' BID')
		return (bitcount == 24) ? 3 : (bitcount == 32) ? 4 : 0;
	//if (fourcc == FOURCC_HYU)
	//	return ((bitcount&~7) == 16) ? -1 : ((bitcount&~7) == 24) ? -2 : ((bitcount&~7) == 32) ? -3 : 0;
	return 0;
}


static bool CanCompress(LPBITMAPINFOHEADER lpbiIn)
{
	// todo: check size %16
	int intype = GetBitmapType(lpbiIn);
	//return (intype == 1 || intype == 2 || intype == 3 || (intype == 4 && AllowRGBA()));
	return intype == 3;
}


static bool CanCompress(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	if (!lpbiOut) return CanCompress(lpbiIn);

	int intype = GetBitmapType(lpbiIn);
	int outtype = GetBitmapType(lpbiOut);

	if (outtype < 0)
	{
		//if (!IsLegalMethod(GetMethod(lpbiOut), outtype != -1))
		return false;
	}

	switch (intype)
	{
	case 3:
		return (outtype == 1);
		/*
		case 1: case 2:
		return (outtype == -1);
		case 3:
		return (outtype == 1 || outtype == -1 || outtype == -2);
		case 4:
		return (outtype == -3 && AllowRGBA());
		*/
	default:
		return false;
	}
}

/********************************************************************
********************************************************************/

DWORD CodecInst::CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	Msg("CompressQuery: input = %s; output = %s\n", &PrintBitmapType(lpbiIn), &PrintBitmapType(lpbiOut));
	return CanCompress(lpbiIn, lpbiOut) ? ICERR_OK : ICERR_BADFORMAT;
}


DWORD CodecInst::CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	Msg("CompressGetFormat: input = %s; output = %s\n", &PrintBitmapType(lpbiIn), &PrintBitmapType(lpbiOut));
	if (!CanCompress(lpbiIn))
		return ICERR_BADFORMAT;

	int intype = GetBitmapType(lpbiIn);

	m_oCodec.m_iIFrameLoop = (GetPrivateProfileInt("general", "i_freq", m_oCodec.m_iIFrameLoop, "lighthaar.ini"));
	m_oCodec.m_iHaarQuality = (GetPrivateProfileInt("general", "quality", m_oCodec.m_iHaarQuality, "lighthaar.ini"));
	//bool compress_as_yuy2 = (intype<3);
	//int method;
	//if (!compress_as_yuy2)
	//{
	//	method = GetPrivateProfileInt("general", "rgbmethod", methodGrad+flagDecorrelate, "lighthaar.ini");
	//	if (method==methodConvertToYUY2)
	//		compress_as_yuy2 = true;
	//}
	//if (compress_as_yuy2)
	//	method = GetPrivateProfileInt("general", "yuy2method", methodMedian, "lighthaar.ini");

	if (!lpbiOut)
		return sizeof(BITMAPINFOHEADER)+sizeof(FrameInfo); //+hufftable_length;

	//int real_bit_count = (intype==4) ? 32 : (compress_as_yuy2) ? 16 : 24;
	int real_bit_count = (intype==4) ? 32 : 24;

	*lpbiOut = *lpbiIn;
	lpbiOut->biSize = sizeof(BITMAPINFOHEADER)+4+sizeof(FrameInfo); // +hufftable_length;
	lpbiOut->biCompression = FOURCC_LGTH;
	lpbiOut->biClrImportant = lpbiOut->biClrUsed = 0;
	lpbiOut->biBitCount = max(24, real_bit_count);
	unsigned char* extra_data = (unsigned char*)lpbiOut + sizeof(BITMAPINFOHEADER);
	extra_data[0] = 0; //method;
	extra_data[1] = real_bit_count;
	extra_data[2] = 0;
	extra_data[3] = 0;
	//lstrcpy((char*)extra_data+4, (const char*)hufftable);
	// write a FrameInfo ?
	return ICERR_OK;
}


DWORD CodecInst::CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	Msg("CompressBegin: input = %s; output = %s\n", &PrintBitmapType(lpbiIn), &PrintBitmapType(lpbiOut));

	if (!CanCompress(lpbiIn, lpbiOut))
		return ICERR_BADFORMAT;

	CompressEnd();  // free resources if necessary

	int intype = GetBitmapType(lpbiIn);
	int outtype = GetBitmapType(lpbiOut);
	int method = GetMethod(lpbiOut);

	if (outtype < 0)
	{
		//InitializeEncodeTables(GetEmbeddedHuffmanTable(lpbiOut));
		//encode_table_owner = this;
	}

	// allocate buffer if compressing RGB->YUY2->HFYU
	/*
	if (outtype == -1 && (method == methodGrad || intype == 3))
	yuy2_buffer = new unsigned char[lpbiIn->biWidth * lpbiIn->biHeight * 2 + 4];
	if (method == methodMedian)
	median_buffer = new unsigned char[lpbiIn->biWidth * lpbiIn->biHeight * 2 + 8];
	if (outtype == -2 && method == methodGrad+flagDecorrelate)
	rgb_buffer = new unsigned char[lpbiIn->biWidth * lpbiIn->biHeight * 3 + 4];
	if (outtype == -3 && method == methodGrad+flagDecorrelate)
	rgb_buffer = new unsigned char[lpbiIn->biWidth * lpbiIn->biHeight * 4 + 4];
	*/
	if (outtype == 1)
	{
		AllocBuffers(lpbiIn->biWidth, lpbiIn->biHeight);
	}
	//InitClip();
	m_oCodec.m_iIFrameCounter = 0;

	return ICERR_OK;
}

DWORD CodecInst::CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
	// Assume 24 bpp worst-case for YUY2 input, 40 bpp for RGB.
	// The actual worst case is 43/51 bpp currently, but this is exceedingly improbable
	// (probably impossible with real captured video)
	//return lpbiIn->biWidth * lpbiIn->biHeight * ((GetBitmapType(lpbiIn) <= 2) ? 3 : 5) + sizeof(FrameInfo);
	int iRL0Size = 0;
	toRL0(NULL, &iRL0Size, NULL, lpbiIn->biWidth * lpbiIn->biHeight * 3);
	return iRL0Size + sizeof(FrameInfo); 
}


template<typename T> static inline T* Align8(T* ptr) { return (T*)((unsigned(ptr)+7)&~7); }

DWORD CodecInst::Compress(ICCOMPRESS* icinfo, DWORD dwSize)
{
	if (icinfo->lpckid)
		*icinfo->lpckid = FOURCC_LGTH;
	//*icinfo->lpdwFlags = AVIIF_KEYFRAME;

	int intype = GetBitmapType(icinfo->lpbiInput);
	int outtype = GetBitmapType(icinfo->lpbiOutput);

	//if (outtype < 0)
	if (outtype == 1)
	{    // compressing (as opposed to converting RGB->YUY2)
		const int method = GetMethod(icinfo->lpbiOutput);

		//if (encode_table_owner != this)
		//{
		//	InitializeEncodeTables(GetEmbeddedHuffmanTable(icinfo->lpbiOutput));
		//	encode_table_owner = this;
		//}

		const uint8_t* const in = (uint8_t*)icinfo->lpInput;
		uint8_t* const out = (uint8_t*)icinfo->lpOutput;

		uint8_t* out_end = out;
		/*
		if (outtype == -1)
		{    // compressing to HFYU16 (4:2:2 YUV)
		int stride = icinfo->lpbiInput->biWidth * 2;
		int size = stride * icinfo->lpbiInput->biHeight;
		if (icinfo->lpbiInput->biHeight > 288) stride *= 2;    // if image is interlaced, double stride so fields are treated separately
		const unsigned char* yuy2in = in;

		if (intype == 3)
		{    // RGB24->YUY2->HFYU16
		ConvertRGB24toYUY2(in, yuy2_buffer, icinfo->lpbiInput->biWidth, icinfo->lpbiInput->biHeight);
		yuy2in = yuy2_buffer;
		size = icinfo->lpbiInput->biWidth * icinfo->lpbiInput->biHeight * 2;
		}
		if (method == methodGrad)
		{
		// Attempt to match yuy2_buffer alignment to input alignment.
		// Why, oh why, can't standard libraries just quadword-align
		// big memory blocks and be done with it?
		unsigned char* const aligned_yuy2_buffer = yuy2_buffer + 4*((int(yuy2in)&7) != (int(yuy2_buffer)&7));
		//        mmx_RowDiff(yuy2in, aligned_yuy2_buffer, yuy2in+size, stride);
		yuy2in = aligned_yuy2_buffer;
		}
		if (intype==2)
		{    // UYVY->HFYU16
		if (method == methodMedian)
		{
		unsigned char* aligned_median_buffer = Align8(median_buffer);
		//          mmx_MedianPredictUYVY(yuy2in, aligned_median_buffer, yuy2in+size, stride);
		//          out_end = asm_CompressUYVY(aligned_median_buffer, out, aligned_median_buffer+size);
		}
		else
		{
		//          out_end = asm_CompressUYVYDelta(yuy2in,out,yuy2in+size);
		}
		} else {    // YUY2->HFYU16
		if (method == methodMedian)
		{
		unsigned char* aligned_median_buffer = Align8(median_buffer);
		//          mmx_MedianPredictYUY2(yuy2in, aligned_median_buffer, yuy2in+size, stride);
		//          out_end = asm_CompressYUY2(aligned_median_buffer, out, aligned_median_buffer+size);
		}
		else
		{
		//          out_end = asm_CompressYUY2Delta(yuy2in,out,yuy2in+size);
		}
		}
		}
		else
		{// compressing to HFYU24 (RGB) or HFYU32 (RGBA)
		int stride = (icinfo->lpbiInput->biWidth * GetBitCount(icinfo->lpbiInput)) >> 3;
		int size = stride * icinfo->lpbiInput->biHeight;
		if (icinfo->lpbiInput->biHeight > 288) stride *= 2;    // if image is interlaced, double stride so fields are treated separately
		const unsigned char* rgbin = in;

		if ((method&~flagDecorrelate) == methodGrad) {
		unsigned char* const aligned_rgb_buffer = rgb_buffer + 4*((int(in)&7) != (int(rgb_buffer)&7));
		//        mmx_RowDiff(in, aligned_rgb_buffer, in+size, stride);
		rgbin = aligned_rgb_buffer;
		}
		if (outtype == -2) {    // RGB24->HFYU24
		if (method == methodLeft || method == methodOld)
		{
		//          out_end = asm_CompressRGBDelta(rgbin,out,rgbin+size);
		}
		else
		{
		//          out_end = asm_CompressRGBDeltaDecorrelate(rgbin,out,rgbin+size);
		}
		}
		else if (outtype == -3) {     // RGBA->HFYU32
		if (method == methodLeft || method == methodOld)
		{
		//          out_end = asm_CompressRGBADelta(rgbin,out,rgbin+size);
		}
		else
		{
		//          out_end = asm_CompressRGBADeltaDecorrelate(rgbin,out,rgbin+size);
		}
		}
		else
		return ICERR_BADFORMAT;
		}
		*/
		if (intype == 3) // 24 bits RGB
		{
			FrameInfo oFrameInfo;

			// update our internal encoding buffers (previous/current/diff frames)
			int iPreviousBuffer = m_oCodec.m_curFrameBuffer;
			m_oCodec.m_curFrameBuffer = (m_oCodec.m_curFrameBuffer+1) % m_oCodec.m_iFrameBufferCount;
			int iEncodeBuffer = -1;

			int iWidth = icinfo->lpbiInput->biWidth;
			int iHeight = icinfo->lpbiInput->biHeight;
			oFrameInfo.m_iQuality = m_oCodec.m_iHaarQuality; // we can adjust quality to mach bitrate
			//oFrameInfo.m_iQuality = 1; // for test

			// YUV space is half size the RGB one, so convert datas!
			ConvertRGB24toYCrCb(
				m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_y,
				m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_u,
				m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_v,
				in, 
				iWidth, 
				iHeight);

			if(m_oCodec.m_iIFrameCounter == 0)
			{
				oFrameInfo.m_iFrameType = I_Frame;
				iEncodeBuffer = m_oCodec.m_curFrameBuffer;
				*icinfo->lpdwFlags = AVIIF_KEYFRAME; // tell AVI this frame is a key/intra one
				// convert color buffers to haar coefficients
				//EncodeHaar(m_oCodec.m_frameBuffers[iEncodeBuffer].m_y,
				//	m_oCodec.m_frameBuffers[iEncodeBuffer].m_y, iWidth, iHeight, iHaarQuality);
				//EncodeHaar(m_oCodec.m_frameBuffers[iEncodeBuffer].m_u,
				//	m_oCodec.m_frameBuffers[iEncodeBuffer].m_u, iWidth>>1, iHeight>>1, iHaarQuality);
				//EncodeHaar(m_oCodec.m_frameBuffers[iEncodeBuffer].m_v,
				//	m_oCodec.m_frameBuffers[iEncodeBuffer].m_v, iWidth>>1, iHeight>>1, iHaarQuality);
			}
			else
			{
				oFrameInfo.m_iFrameType = P_Frame;
				// need another buffer for diff encoding
				iEncodeBuffer = (m_oCodec.m_curFrameBuffer+1) % m_oCodec.m_iFrameBufferCount;
/*
				DecodeHaar(m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_y,
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_y, iWidth, iHeight);
				DecodeHaar(m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_u,
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_u, iWidth>>1, iHeight>>1);
				DecodeHaar(m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_v,
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_v, iWidth>>1, iHeight>>1);
*/

				sub_buffer(
					m_oCodec.m_frameBuffers[iEncodeBuffer].m_y, // out(diff)
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_y, // current
					m_oCodec.m_frameBuffers[iPreviousBuffer].m_y, // previous
					iWidth,
					iHeight);

				sub_buffer(
					m_oCodec.m_frameBuffers[iEncodeBuffer].m_u,
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_u,
					m_oCodec.m_frameBuffers[iPreviousBuffer].m_u,
					iWidth>>1,
					iHeight>>1);

				sub_buffer(
					m_oCodec.m_frameBuffers[iEncodeBuffer].m_v,
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_v,
					m_oCodec.m_frameBuffers[iPreviousBuffer].m_v,
					iWidth>>1,
					iHeight>>1);
			}

			memcpy(out_end, &oFrameInfo, sizeof(FrameInfo));
			out_end += sizeof(FrameInfo);

			// convert color buffers to haar coefficients
			EncodeCDF53(m_oCodec.m_frameBuffers[iEncodeBuffer].m_y,
				m_oCodec.m_frameBuffers[iEncodeBuffer].m_y, iWidth, iHeight, oFrameInfo.m_iQuality);
			EncodeCDF53(m_oCodec.m_frameBuffers[iEncodeBuffer].m_u,
				m_oCodec.m_frameBuffers[iEncodeBuffer].m_u, iWidth>>1, iHeight>>1, oFrameInfo.m_iQuality);
			EncodeCDF53(m_oCodec.m_frameBuffers[iEncodeBuffer].m_v,
				m_oCodec.m_frameBuffers[iEncodeBuffer].m_v, iWidth>>1, iHeight>>1, oFrameInfo.m_iQuality);

			int iSize = iWidth*iHeight;
			int iSizeUV = iSize>>2;

			// RL0 encoding
			int iRL0Size = 0;
			toRL0(NULL, &iRL0Size, NULL, iSize); // get max size possible
			uint8_t* pRL0Buffer = new uint8_t[iRL0Size+(2*(iRL0Size>>2))+3*sizeof(int32_t)]; // write RL0Size for AE
			uint8_t* pRL0Position = pRL0Buffer;
			int iRL0SizeY = 0;
			int iRL0SizeU = 0;
			int iRL0SizeV = 0;
			toRL0(pRL0Position, &iRL0SizeY, (uint8_t*)m_oCodec.m_frameBuffers[iEncodeBuffer].m_y, iSize); pRL0Position+=iRL0SizeY;
			toRL0(pRL0Position, &iRL0SizeU, (uint8_t*)m_oCodec.m_frameBuffers[iEncodeBuffer].m_u, iSizeUV); pRL0Position+=iRL0SizeU;
			toRL0(pRL0Position, &iRL0SizeV, (uint8_t*)m_oCodec.m_frameBuffers[iEncodeBuffer].m_v, iSizeUV); pRL0Position+=iRL0SizeV;

			// AE(Arithmetic Encoding)
			iRL0Size = iRL0SizeY + iRL0SizeU + iRL0SizeV + 3*sizeof(int32_t);
			BinStream out;
			out.Open(out_end); // TODO: AE buffer may be bigger than preious(RL0) one
			out.Write(&iRL0SizeY, sizeof(int32_t)); // write expanded(RL0) size
			out.Write(&iRL0SizeU, sizeof(int32_t));
			out.Write(&iRL0SizeV, sizeof(int32_t));
			ArithEncoder* ae=new ArithEncoder(iRL0Size);
			ae->initialize_model();
			ae->initialize_arithmetic_encoder();
			ae->initialize_output_bitstream();
			ae->write(&out, pRL0Buffer, iRL0SizeY+iRL0SizeU+iRL0SizeV);
			ae->flush_arithmetic_encoder( &out );
			ae->flush_output_bitstream( &out );
			intptr_t iAESize = out.Seek(0, SEEK_CUR);
			delete ae;
			delete pRL0Buffer;

			// write append size
			out_end += iAESize;

			// needed for next frame encoding, in cas of delta encoding
			// decompress current frame with compression artefacts/errors
			DecodeCDF53(m_oCodec.m_frameBuffers[iEncodeBuffer].m_y,
				m_oCodec.m_frameBuffers[iEncodeBuffer].m_y, iWidth, iHeight, oFrameInfo.m_iQuality);
			DecodeCDF53(m_oCodec.m_frameBuffers[iEncodeBuffer].m_u,
				m_oCodec.m_frameBuffers[iEncodeBuffer].m_u, iWidth>>1, iHeight>>1, oFrameInfo.m_iQuality);
			DecodeCDF53(m_oCodec.m_frameBuffers[iEncodeBuffer].m_v,
				m_oCodec.m_frameBuffers[iEncodeBuffer].m_v, iWidth>>1, iHeight>>1, oFrameInfo.m_iQuality);

			if(oFrameInfo.m_iFrameType == P_Frame)
			{
				add_buffer(
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_y,
					m_oCodec.m_frameBuffers[iPreviousBuffer].m_y,
					m_oCodec.m_frameBuffers[iEncodeBuffer].m_y,
					iWidth,
					iHeight);

				add_buffer(
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_u,
					m_oCodec.m_frameBuffers[iPreviousBuffer].m_u,
					m_oCodec.m_frameBuffers[iEncodeBuffer].m_u,
					iWidth>>1,
					iHeight>>1);

				add_buffer(
					m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_v,
					m_oCodec.m_frameBuffers[iPreviousBuffer].m_v,
					m_oCodec.m_frameBuffers[iEncodeBuffer].m_v,
					iWidth>>1,
					iHeight>>1);
			}

			// insert key/intraframe every N frames
			m_oCodec.m_iIFrameCounter = (m_oCodec.m_iIFrameCounter+1) % m_oCodec.m_iIFrameLoop;
		}
		icinfo->lpbiOutput->biSizeImage = DWORD(out_end) - DWORD(out);
		return ICERR_OK;
	}
	else if (outtype == 1)
	{    // RGB24->YUY2
		//ConvertRGB24toYUY2((unsigned char*)icinfo->lpInput, (unsigned char*)icinfo->lpOutput,
		//	icinfo->lpbiInput->biWidth, icinfo->lpbiInput->biHeight);
		icinfo->lpbiOutput->biSizeImage = (icinfo->lpbiOutput->biWidth * icinfo->lpbiOutput->biHeight * GetBitCount(icinfo->lpbiOutput)) >> 3;
		return ICERR_OK;
	}
	else
		return ICERR_BADFORMAT;
}


DWORD CodecInst::CompressEnd()
{
	ReleaseBuffers();
	return ICERR_OK;
}


/********************************************************************
********************************************************************/

static bool CanDecompress(LPBITMAPINFOHEADER lpbiIn)
{
	int intype = GetBitmapType(lpbiIn);
	return intype == 1;
	//if (intype < 0)
	//	return IsLegalMethod(GetMethod(lpbiIn), intype != -1);
	//else
	//	return (intype == 1 || intype == 2);
}


static bool CanDecompress(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	if (!lpbiOut)
		return CanDecompress(lpbiIn);

	// must be 1:1 (no stretching)
	if (lpbiOut && (lpbiOut->biWidth != lpbiIn->biWidth || lpbiOut->biHeight != lpbiIn->biHeight))
		return false;

	int intype = GetBitmapType(lpbiIn);
	int outtype = GetBitmapType(lpbiOut);

	if (intype < 0) {
		//if (!IsLegalMethod(GetMethod(lpbiIn), intype != -1))
			return false;
	}

	switch (intype) {
case -1:
	// YUY2, RGB-24, RGB-32 output for compressed YUY2
	return (outtype == 1 || outtype == 3 || outtype == 4);
case -2: case 1: case 2:
	// RGB-24, RGB-32 output only for YUY2/UYVY and compressed RGB
	return (outtype == 3); // || outtype == 4
case -3:
	// RGB-32 output for compressed RGBA
	return (outtype == 4);
default:
	return false;
	}
}

/********************************************************************
********************************************************************/


DWORD CodecInst::DecompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	Msg("DecompressQuery: input = %s; output = %s\n", &PrintBitmapType(lpbiIn), &PrintBitmapType(lpbiOut));
	return CanDecompress(lpbiIn, lpbiOut) ? ICERR_OK : ICERR_BADFORMAT;
}


// This function should return "the output format which preserves the most
// information."  However, I now provide the option to return RGB format
// instead, since some programs treat the default format as the ONLY format.

DWORD CodecInst::DecompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	Msg("DecompressGetFormat: input = %s; output = %s\n", &PrintBitmapType(lpbiIn), &PrintBitmapType(lpbiOut));

	// if lpbiOut == NULL, then return the size required to hold an output format struct
	if (lpbiOut == NULL)
		return sizeof(BITMAPINFOHEADER);

	if (!CanDecompress(lpbiIn))
		return ICERR_BADFORMAT;

	*lpbiOut = *lpbiIn;
	lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
	lpbiOut->biPlanes = 1;

	int intype = GetBitmapType(lpbiIn);
	if (intype == -3)
	{
		lpbiOut->biBitCount = 32;   // RGBA
		lpbiOut->biCompression = 0;
		lpbiOut->biSizeImage = lpbiIn->biWidth * lpbiIn->biHeight * 4;
	} else if (intype == -2 || intype == 1 || intype == 2 || SuggestRGB())
	{
		lpbiOut->biBitCount = 24;   // RGB
		lpbiOut->biCompression = 0;
		lpbiOut->biSizeImage = lpbiIn->biWidth * lpbiIn->biHeight * 3;
	} else
	{
		lpbiOut->biBitCount = 16;       // YUY2
		lpbiOut->biCompression = FOURCC_LGTH;
		lpbiOut->biSizeImage = lpbiIn->biWidth * lpbiIn->biHeight * 2;
	}

	return ICERR_OK;
}


DWORD CodecInst::DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut) {
	Msg("DecompressBegin: input = %s; output = %s\n", &PrintBitmapType(lpbiIn), &PrintBitmapType(lpbiOut));

	DecompressEnd();  // free resources if necessary

	if (!CanDecompress(lpbiIn, lpbiOut))
		return ICERR_BADFORMAT;

	decompressing = true;

	int intype = GetBitmapType(lpbiIn);
	int outtype = GetBitmapType(lpbiOut);
	int method = GetMethod(lpbiIn);

	if (intype < 0)
	{
		//InitializeDecodeTables(GetEmbeddedHuffmanTable(lpbiIn));
		//decode_table_owner = this;
	}

	// allocate buffer if decompressing HFYU->YUY2->RGB
	if (intype == -1 && outtype >= 3)
		decompress_yuy2_buffer = new unsigned char[lpbiIn->biWidth * lpbiIn->biHeight * 2];

	if (intype == 1 && outtype==3)
	{
		AllocBuffers(lpbiIn->biWidth, lpbiIn->biHeight);
	}


	swapfields = !!GetPrivateProfileInt("debug", "decomp_swap_fields", false, "lighthaar.ini");

	return ICERR_OK;
}


DWORD CodecInst::Decompress(ICDECOMPRESS* icinfo, DWORD dwSize)
{
	// If you insert a Huffyuv-compressed AVI to a Premiere project and then
	// drag it on to the timeline, the following dialogue occurs:
	//
	// 1. Premiere calls ICDecompressBegin, asking Huffyuv to decompress
	//    to a bitmap with different dimensions than the compressed frame.
	//
	// 2. Huffyuv can't resize, so it returns ICERR_BADFORMAT.
	//
	// 3. Premiere calls ICDecompress without making another call to
	//    ICDecompressBegin.
	//
	// Therefore I now check for this case and compensate for Premiere's
	// negligence by making the DecompressBegin call myself.

	//if (!decompressing) {
	//	DWORD retval = DecompressBegin(icinfo->lpbiInput, icinfo->lpbiOutput);
	//	if (retval != ICERR_OK)
	//		return retval;
	//}

	icinfo->lpbiOutput->biSizeImage = (icinfo->lpbiOutput->biWidth * icinfo->lpbiOutput->biHeight * icinfo->lpbiOutput->biBitCount) >> 3;

	int intype = GetBitmapType(icinfo->lpbiInput);
	/*	if (intype < 0)
	{
	int method = GetMethod(icinfo->lpbiInput);
	int outtype = GetBitmapType(icinfo->lpbiOutput);

	if (decode_table_owner != this)
	{
	InitializeDecodeTables(GetEmbeddedHuffmanTable(icinfo->lpbiInput));
	decode_table_owner = this;
	}

	const unsigned long* const in = (unsigned long*)icinfo->lpInput;
	unsigned char* const out = (unsigned char*)icinfo->lpOutput;

	if (intype == -1) {   // decompressing HFYU16
	int stride = icinfo->lpbiOutput->biWidth * 2;
	const int size = stride * icinfo->lpbiOutput->biHeight;
	if (icinfo->lpbiOutput->biHeight > 288) stride *= 2;      // if image is interlaced, double stride so fields are treated separately

	unsigned char* const yuy2 = outtype>1 ? decompress_yuy2_buffer : out;

	if (method == methodMedian) {
	//        asm_DecompressHFYU16(in, yuy2, yuy2+size);
	//        asm_MedianRestore(yuy2, yuy2+size, stride);
	}
	else
	{
	//        asm_DecompressHFYU16Delta(in, yuy2, yuy2+size);
	if (method == methodGrad)
	{
	//          mmx_RowAccum(yuy2, yuy2+size, stride);
	}
	}

	if (outtype>1)    // HFYU16->RGB
	ConvertYUY2toRGB(icinfo->lpbiOutput, yuy2, out);
	}
	else {   // decompressing HFYU24/HFYU32
	int stride = (icinfo->lpbiOutput->biWidth * icinfo->lpbiOutput->biBitCount) >> 3;
	const int size = stride * icinfo->lpbiOutput->biHeight;
	if (icinfo->lpbiOutput->biHeight > 288) stride *= 2;      // if image is interlaced, double stride so fields are treated separately
	if (intype == -2) {   // HFYU24->RGB
	if (outtype == 4) {
	//if (method == methodLeft || method == methodOld)
	//            asm_DecompressHFYU24To32Delta(in, out, out+size);
	//else
	//            asm_DecompressHFYU24To32DeltaDecorrelate(in, out, out+size);
	//} else {
	//if (method == methodLeft || method == methodOld)
	//            asm_DecompressHFYU24To24Delta(in, out, out+size);
	//else
	//            asm_DecompressHFYU24To24DeltaDecorrelate(in, out, out+size);
	}
	}
	else if (intype == -3) {    // HFYU32->RGBA
	//if (method == methodLeft || method == methodOld)
	//          asm_DecompressHFYU32To32Delta(in, out, out+size);
	//else
	//          asm_DecompressHFYU32To32DeltaDecorrelate(in, out, out+size);
	}
	else
	return ICERR_BADFORMAT;

	//if ((method&~flagDecorrelate) == methodGrad)
	//        mmx_RowAccum(out, out+size, stride);
	}

	//if (swapfields && icinfo->lpbiOutput->biHeight > 288)
	//      asm_SwapFields(out, out+icinfo->lpbiOutput->biSizeImage,
	//(icinfo->lpbiOutput->biWidth * icinfo->lpbiOutput->biBitCount) >> 3);

	return ICERR_OK;
	}
	else if (intype == 1) {   // YUY2->RGB
	return ConvertYUY2toRGB(icinfo->lpbiOutput, icinfo->lpInput, icinfo->lpOutput);
	}
	else if (intype == 2) {   // UYVY->RGB
	return ConvertUYVYtoRGB(icinfo->lpbiOutput, icinfo->lpInput, icinfo->lpOutput);
	}
	else
	*/
	int outtype = GetBitmapType(icinfo->lpbiOutput);
	if (intype == 1 && outtype == 3)
	{
		const uint8_t* const in = (uint8_t*)icinfo->lpInput;
		uint8_t* out = (uint8_t*)icinfo->lpOutput;

		int iWidth = icinfo->lpbiInput->biWidth;
		int iHeight = icinfo->lpbiInput->biHeight;
		int iSize = iWidth * iHeight;
		int iSizeUV = iSize>>2;
		const uint8_t* read_in = static_cast<const uint8_t*>(icinfo->lpInput);

		FrameInfo oFrameInfo;
		memcpy(&oFrameInfo, read_in, sizeof(FrameInfo));
		read_in+=sizeof(FrameInfo);

		int iPreviousBuffer = m_oCodec.m_curFrameBuffer;
		m_oCodec.m_curFrameBuffer = (m_oCodec.m_curFrameBuffer+1) % m_oCodec.m_iFrameBufferCount;

		int iDecodeBuffer = -1;
		if(oFrameInfo.m_iFrameType == I_Frame)
		{
			iDecodeBuffer = m_oCodec.m_curFrameBuffer;
		}
		else if(oFrameInfo.m_iFrameType == P_Frame)
		{
			iDecodeBuffer = (m_oCodec.m_curFrameBuffer+1) % m_oCodec.m_iFrameBufferCount;
		}
		
		int iDisplayBuffer = (m_oCodec.m_curFrameBuffer+1) % m_oCodec.m_iFrameBufferCount;

		// first AE decoding
		int iRL0SizeY = 0;
		int iRL0SizeU = 0;
		int iRL0SizeV = 0;
		uint8_t* pRL0Buffer = new uint8_t[iSize+2*iSizeUV]; //m_oCodec.m_frameBuffers[iDecodeBuffer].m_y;
		BinStream inStream;
		inStream.Open((void*)read_in);
		inStream.SetBufferSize(icinfo->lpbiInput->biSizeImage);
		inStream.Read(&iRL0SizeY, sizeof(int32_t));
		inStream.Read(&iRL0SizeU, sizeof(int32_t));
		inStream.Read(&iRL0SizeV, sizeof(int32_t));
		ArithEncoder* ae=new ArithEncoder(1);
		ae->initialize_model();
		ae->initialize_input_bitstream();
		ae->initialize_arithmetic_decoder(&inStream, inStream.GetBufferSize());
		for(int x=0; x<(iRL0SizeY+iRL0SizeU+iRL0SizeV); x++)
		{
			SYMBOL symb;
			ae->get_symbol_scale( &symb );
			int count = ae->get_current_count( &symb );
			int c = ae->convert_symbol_to_int( count, &symb );
			ae->remove_symbol_from_stream(&inStream, &symb );
			pRL0Buffer[x]=c;
			ae->update_model( c );
		}
		read_in += inStream.m_iPosition;
		delete ae;

		// then RL0 decoding
		uint8_t* pRL0Position = pRL0Buffer;
		fromRL0((uint8_t*)m_oCodec.m_frameBuffers[iDecodeBuffer].m_y, pRL0Position, iRL0SizeY); pRL0Position+=iRL0SizeY;
		fromRL0((uint8_t*)m_oCodec.m_frameBuffers[iDecodeBuffer].m_u, pRL0Position, iRL0SizeU); pRL0Position+=iRL0SizeU;
		fromRL0((uint8_t*)m_oCodec.m_frameBuffers[iDecodeBuffer].m_v, pRL0Position, iRL0SizeV); pRL0Position+=iRL0SizeV;

		delete pRL0Buffer;

		DecodeCDF53(m_oCodec.m_frameBuffers[iDecodeBuffer].m_y,
			m_oCodec.m_frameBuffers[iDecodeBuffer].m_y, iWidth, iHeight, oFrameInfo.m_iQuality);
		DecodeCDF53(m_oCodec.m_frameBuffers[iDecodeBuffer].m_u,
			m_oCodec.m_frameBuffers[iDecodeBuffer].m_u, iWidth>>1, iHeight>>1, oFrameInfo.m_iQuality);
		DecodeCDF53(m_oCodec.m_frameBuffers[iDecodeBuffer].m_v,
			m_oCodec.m_frameBuffers[iDecodeBuffer].m_v, iWidth>>1, iHeight>>1, oFrameInfo.m_iQuality);

		if(oFrameInfo.m_iFrameType == P_Frame)
		{
			// combine with previous buffer to get current one
			add_buffer(
				m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_y, // out
				m_oCodec.m_frameBuffers[iPreviousBuffer].m_y,
				m_oCodec.m_frameBuffers[iDecodeBuffer].m_y,
				iWidth,
				iHeight);

			add_buffer(
				m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_u,
				m_oCodec.m_frameBuffers[iPreviousBuffer].m_u,
				m_oCodec.m_frameBuffers[iDecodeBuffer].m_u,
				iWidth>>1,
				iHeight>>1);

			add_buffer(
				m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_v,
				m_oCodec.m_frameBuffers[iPreviousBuffer].m_v,
				m_oCodec.m_frameBuffers[iDecodeBuffer].m_v,
				iWidth>>1,
				iHeight>>1);
		}

		// YUV conversion 
		ConvertYCrCbtoRGB24(
			out,
			m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_y,
			m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_u,
			m_oCodec.m_frameBuffers[m_oCodec.m_curFrameBuffer].m_v,
			icinfo->lpbiInput->biWidth,
			icinfo->lpbiInput->biHeight);

		return ICERR_OK;
	}
	else
		return ICERR_BADFORMAT;
}


// palette-mapped output only
DWORD CodecInst::DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	return ICERR_BADFORMAT;
}


DWORD CodecInst::DecompressEnd()
{
	ReleaseBuffers();
	decompressing = false;
	return ICERR_OK;
}

void CodecInst::AllocBuffers(int _width, int _height)
{
	if(rgb_buffer == NULL)
	{
		rgb_buffer = new uint8_t[ _width * _height * 4]; //+4 was for mem align
	}

	m_oCodec.Init(_width, _height);
}

void CodecInst::ReleaseBuffers()
{
	if (yuy2_buffer)
	{
		delete[] yuy2_buffer;
		yuy2_buffer = 0;
	}
	if (median_buffer)
	{
		delete[] median_buffer;
		median_buffer = 0;
	}
	if (rgb_buffer)
	{
		delete[] rgb_buffer;
		rgb_buffer = 0;
	}
	if (decompress_yuy2_buffer)
	{
		delete[] decompress_yuy2_buffer;
		decompress_yuy2_buffer = 0;
	}

	m_oCodec.Term();
}

