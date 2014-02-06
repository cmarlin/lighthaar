//
// LightHaar codec By Cyril MARLIN & Mathieu LEGRIS
// MIT licence 2012
//


#include "lighthaar.h"
#include "resource.h"

//#include <crtdbg.h>
#include <algorithm>
#include <vector>
//#include <pair>
#include <limits.h>
#include <math.h>


namespace LightHaar
{
	static LightHaarAlloc g_pAllocFunc = NULL;
	static LightHaarFree g_pFreeFunc = NULL;
	static intptr_t g_iUserAlloc = 0;

	void SetAllocator(LightHaarAlloc pAllocFunc, LightHaarFree pFreeFunc, intptr_t iUser)
	{
		g_pAllocFunc = pAllocFunc;
		g_pFreeFunc = pFreeFunc;
		g_iUserAlloc = iUser;
	}

	void* operator new(size_t iSize)
	{
		void* ptr = (g_pAllocFunc!=NULL)?
			g_pAllocFunc(iSize, 0, g_iUserAlloc)
			:malloc(iSize);
		return ptr;
	}

	Codec::Codec()
		: m_curFrameBuffer(0)
		, m_iIFrameCounter(0)
		, m_iIFrameLoop(10) // default value
		, m_iHaarQuality(15)
	{
	}

	void Codec::Init(const int _width, const int _height)
	{
		for(int i=0; i<m_iFrameBufferCount; ++i)
		{
			if(m_frameBuffers[i].m_y == NULL)
			{
				m_frameBuffers[i].m_y = new int8_t[_width * _height];
			}
			if(m_frameBuffers[i].m_u == NULL)
			{
				m_frameBuffers[i].m_u = new int8_t[(_width>>1) * (_height>>1)];
			}
			if(m_frameBuffers[i].m_v == NULL)
			{
				m_frameBuffers[i].m_v = new int8_t[(_width>>1) * (_height>>1)];
			}
		}
	}

	void Codec::Term()
	{
		for(int i=0; i<m_iFrameBufferCount; ++i)
		{
			if(m_frameBuffers[i].m_y != NULL)
			{
				delete[] m_frameBuffers[i].m_y;
				m_frameBuffers[i].m_y = NULL;
			}
			if(m_frameBuffers[i].m_u)
			{
				delete[] m_frameBuffers[i].m_u;
				m_frameBuffers[i].m_u = NULL;
			}
			if(m_frameBuffers[i].m_v)
			{
				delete[] m_frameBuffers[i].m_v;
				m_frameBuffers[i].m_v = NULL;
			}
		}
	}
}

void ConvertRGB24toYCrCb(int8_t* _pY, int8_t* _pU, int8_t* _pV, const uint8_t* _pRGB, int _width, int _height)
{
	// 219 ???
	const int cyb = int(0.114f*4096.f);
	const int cyg = int(0.587f*4096.f);
	const int cyr = int(0.299f*4096.f);
	const int cu = int(0.565f*4096.f);
	const int cv = int(0.713f*4096.f);
	const int chalf = int(0.5f/255.f*4095.f);
	//Y = 0.299R + 0.587G + 0.114B
	//U'= (B-Y)*0.565
	//V'= (R-Y)*0.713
	for(int j=0; j<_height; j+=2)
	{
		for(int i=0; i<_width; i+=2)
		{
			const uint8_t* pRGB01 = _pRGB + ((j+0)*_width+i)*3;
			const uint8_t* pRGB23 = _pRGB + ((j+1)*_width+i)*3;
			uint8_t R0 = pRGB01[0];	uint8_t R1 = pRGB01[3+0];
			uint8_t G0 = pRGB01[1];	uint8_t G1 = pRGB01[3+1];
			uint8_t B0 = pRGB01[2];	uint8_t B1 = pRGB01[3+2];
			uint8_t R2 = pRGB23[0];	uint8_t R3 = pRGB23[3+0];
			uint8_t G2 = pRGB23[1];	uint8_t G3 = pRGB23[3+1];
			uint8_t B2 = pRGB23[2];	uint8_t B3 = pRGB23[3+2];

			int fxR, fxG, fxB, fxY, fxU, fxV;
			fxR = (R0<<(12-8)) + chalf;
			fxG = (G0<<(12-8)) + chalf;
			fxB = (B0<<(12-8)) + chalf;
			fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;
			fxU = ((fxB-fxY)*cu)>>12;
			fxV = ((fxR-fxY)*cv)>>12;
			int Y0 = fxY>>(12-8);
			int U0 = (fxU>>(12-8)) + 127;
			int V0 = (fxV>>(12-8)) + 127;

			fxR = (R1<<(12-8)) + chalf;
			fxG = (G1<<(12-8)) + chalf;
			fxB = (B1<<(12-8)) + chalf;
			fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;
			fxU = ((fxB-fxY)*cu)>>12;
			fxV = ((fxR-fxY)*cv)>>12;
			int Y1 = fxY>>(12-8);
			int U1 = (fxU>>(12-8)) + 127;
			int V1 = (fxV>>(12-8)) + 127;

			fxR = (R2<<(12-8)) + chalf;
			fxG = (G2<<(12-8)) + chalf;
			fxB = (B2<<(12-8)) + chalf;
			fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;
			fxU = ((fxB-fxY)*cu)>>12;
			fxV = ((fxR-fxY)*cv)>>12;
			int Y2 = fxY>>(12-8);
			int U2 = (fxU>>(12-8)) + 127;
			int V2 = (fxV>>(12-8)) + 127;

			fxR = (R3<<(12-8)) + chalf;
			fxG = (G3<<(12-8)) + chalf;
			fxB = (B3<<(12-8)) + chalf;
			fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;
			fxU = ((fxB-fxY)*cu)>>12;
			fxV = ((fxR-fxY)*cv)>>12;
			int Y3 = fxY>>(12-8);
			int U3 = (fxU>>(12-8)) + 127;
			int V3 = (fxV>>(12-8)) + 127;

			// note: shift added here (-128) to have s8 range data instead of u8 range
			// try 2/3 scale
			_pY[(j+0)*_width+i+0] = UClamp(Y0*2/3)-128;	_pY[(j+0)*_width+i+1] = UClamp(Y1*2/3)-128;
			_pY[(j+1)*_width+i+0] = UClamp(Y2*2/3)-128;	_pY[(j+1)*_width+i+1] = UClamp(Y3*2/3)-128;
			_pU[(j/2)*(_width/2)+(i/2)] = uint8_t((UClamp(U0*2/3)+UClamp(U1*2/3)+UClamp(U2*2/3)+UClamp(U3*2/3))>>2)-128;
			_pV[(j/2)*(_width/2)+(i/2)] = uint8_t((UClamp(V0*2/3)+UClamp(V1*2/3)+UClamp(V2*2/3)+UClamp(V3*2/3))>>2)-128;
			//_pY[(j+0)*_width+i+0] = UClamp(Y0)-128;	_pY[(j+0)*_width+i+1] = UClamp(Y1)-128;
			//_pY[(j+1)*_width+i+0] = UClamp(Y2)-128;	_pY[(j+1)*_width+i+1] = UClamp(Y3)-128;
			//_pU[(j/2)*(_width/2)+(i/2)] = uint8_t((UClamp(U0)+UClamp(U1)+UClamp(U2)+UClamp(U3))>>2)-128;
			//_pV[(j/2)*(_width/2)+(i/2)] = uint8_t((UClamp(V0)+UClamp(V1)+UClamp(V2)+UClamp(V3))>>2)-128;
		}
	}
}

// todo: rewrite to use 4096 fixed flot + shift
void ConvertYCrCbtoRGB24(uint8_t* _pRGB, const int8_t* _pY, const int8_t* _pU, const int8_t* _pV, int _width, int _height)
{
	// TODO: use 255<<4 (4080)instead of 4096... or 4095? 
	const int cvr = int(1.403f*4096.f);
	const int cug = int(0.344f*4096.f);
	const int cvg = int(0.714f*4096.f);
	const int cub = int(1.770f*4096.f);
	const int chalf = int(0.5f/255.f*4095.f);
	//R = Y + 1.403V'
	//G = Y - 0.344U' - 0.714V'
	//B = Y + 1.770U'
	for(int j=0; j<_height; j+=2)
	{
		for(int i=0; i<_width; i+=2)
		{
			// note: shift added here (+128) to have s8 range data input to u8 range for conversion
			// try 2/3 scale
			uint8_t Y0 = UClamp((_pY[(j+0)*_width+i+0]+128)*3/2);	uint8_t Y1 = UClamp((_pY[(j+0)*_width+i+1]+128)*3/2);
			uint8_t Y2 = UClamp((_pY[(j+1)*_width+i+0]+128)*3/2);	uint8_t Y3 = UClamp((_pY[(j+1)*_width+i+1]+128)*3/2);
			uint8_t U = UClamp((_pU[(j/2)*(_width/2)+(i/2)]+128)*3/2);
			uint8_t V = UClamp((_pV[(j/2)*(_width/2)+(i/2)]+128)*3/2);
			//uint8_t Y0 = _pY[(j+0)*_width+i+0]+128;	uint8_t Y1 = _pY[(j+0)*_width+i+1]+128;
			//uint8_t Y2 = _pY[(j+1)*_width+i+0]+128;	uint8_t Y3 = _pY[(j+1)*_width+i+1]+128;
			//uint8_t U = _pU[(j/2)*(_width/2)+(i/2)]+128;
			//uint8_t V = _pV[(j/2)*(_width/2)+(i/2)]+128;

			int fxY, fxU, fxV, fxR, fxG, fxB, fxVR, fxUB, fxUVG;

			fxU = ((U-127)<<(12-8)) + chalf;
			fxV = ((V-127)<<(12-8)) + chalf;
			fxVR = ((cvr*fxV)>>12);
			fxUVG = ((cug*fxU) + (cvg*fxV))>>12;
			fxUB = ((cub*fxU)>>12);

			fxY = (Y0<<(12-8)) + chalf;
			fxR = fxY + fxVR;
			fxG = fxY - fxUVG;
			fxB = fxY + fxUB;
			int R0 = fxR>>(12-8);
			int G0 = fxG>>(12-8);
			int B0 = fxB>>(12-8);

			fxY = (Y1<<(12-8)) + chalf;
			fxR = fxY + fxVR;
			fxG = fxY - fxUVG;
			fxB = fxY + fxUB;
			int R1 = fxR>>(12-8);
			int G1 = fxG>>(12-8);
			int B1 = fxB>>(12-8);

			fxY = (Y2<<(12-8)) + chalf;
			fxR = fxY + fxVR;
			fxG = fxY - fxUVG;
			fxB = fxY + fxUB;
			int R2 = fxR>>(12-8);
			int G2 = fxG>>(12-8);
			int B2 = fxB>>(12-8);

			fxY = (Y3<<(12-8)) + chalf;
			fxR = fxY + fxVR;
			fxG = fxY - fxUVG;
			fxB = fxY + fxUB;
			int R3 = fxR>>(12-8);
			int G3 = fxG>>(12-8);
			int B3 = fxB>>(12-8);

			uint8_t* pRGB01 = _pRGB + ((j+0)*_width+i)*3;
			uint8_t* pRGB23 = _pRGB + ((j+1)*_width+i)*3;
			pRGB01[0] = UClamp(R0);	pRGB01[3+0] = UClamp(R1);
			pRGB01[1] = UClamp(G0);	pRGB01[3+1] = UClamp(G1);
			pRGB01[2] = UClamp(B0);	pRGB01[3+2] = UClamp(B1);
			pRGB23[0] = UClamp(R2);	pRGB23[3+0] = UClamp(R3);
			pRGB23[1] = UClamp(G2);	pRGB23[3+1] = UClamp(G3);
			pRGB23[2] = UClamp(B2);	pRGB23[3+2] = UClamp(B3);
		}
	}
}

int HaarIterations(int _width, int _height)
{
	int iImgWidthLog2 = Log2(_width) + (IsPowerOf2(_width) ? 0 : 1);
	int iImgHeightLog2 = Log2(_height) + (IsPowerOf2(_height) ? 0 : 1);
	//int iIterations = std::max(std::min(iImgWidthLog2, iImgHeightLog2)-4, 0); // stop before too small
	//int iIterations = std::max(std::min(iImgWidthLog2, iImgHeightLog2)-1, 0); // stop before too small
	int iIterations = 0;
	while(_width>16 && _height>16)
	{
		iIterations++;
		_width>>=1;
		_height>>=1;
	}
	//int iIterations = 1;
	return iIterations;
}

/**
	@brief in fact this version doesn't use strict haar method but a more like cdf53 method
	@param _quality [1; 128] => [top;worst] conversion quality (reduce output values domain)
*/
const int iSqrt2 = int(4096.f);
const int iISqrt2 = int(4096.f);
//const int iSqrt2 = int(2.f*4096.f);
//const int iISqrt2 = int(0.5f*4096.f);
// @brief _src and _dst could be the same
void EncodeHaar(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality)
{
	int iIterations = HaarIterations(_width, _height);
	int16_t* buffer = new int16_t[std::max(_width,_height)];
	static int _floor = 0;

	//filter the values
	for (int k = 0; k < iIterations; k++)
	{
		int iShift = std::max(_quality-(1<<(2*k)), 1);
		//int iShift = 1;
		int iFloor = _floor;

		const int8_t* s;
		int16_t* t;

		int iWidth = _width;
		int iHeight = _height;
		for(int i=0; i<k; i++)
		{
			iWidth = (iWidth>>1) + (iWidth&0x1);
			iHeight = (iHeight>>1) + (iHeight&0x1);
		}

		//horizontal processing
		s = k==0 ? _src : _dst;
		t = buffer;

		int hOffset = (iWidth+1) >> 1;
		for (int j = 0; j < iHeight; j++)
		{
			int sIndex, tIndex;
			sIndex = tIndex = j * _width;
			int i;
			int iDeltaLast = 0;
			for (i=0; i < iWidth-2; i+=2)
			{
				int a = s[sIndex];
				int b = s[sIndex + 1];
				int c = s[sIndex + 2];
				int y = b - (a + c)/2;
				int x = a + (iDeltaLast+y)/4;
				iDeltaLast = y;
				t[i/2] = x;
				if(std::abs(y)<=iFloor)
					y = 0;
				t[i/2 + hOffset] = (((y*iISqrt2)/4096/iShift)*iShift);
				sIndex += 2;
				//tIndex ++;
			}
			if(i==(iWidth-2)) // remaining data
			{
				int a = s[sIndex];
				int b = s[sIndex + 1];
				int c = a; //s[sIndex + 2];
				int y = b - (a + c)/2;
				//int x = a + (iDeltaLast+y)/4;
				int x = a;
				iDeltaLast = y;
				t[i/2] = x;
				if(std::abs(y)<=iFloor)
					y = 0;
				t[i/2 + hOffset] = (((y*iISqrt2)/4096/iShift)*iShift);
			}
			else
			{
				int a = s[sIndex];
				int y = 0;
				int x = a + (iDeltaLast+y)/4;
				t[i/2] = x;
			}
			// copy back data
			for(i=0; i<iWidth; ++i)_dst[tIndex+i]=(int8_t)SClamp(t[i]);
		}

		//vertical processing
		s = _dst;
		t = buffer;

		int vOffset = ((iHeight+1) >> 1);
		for (int i = 0; i < iWidth; i++)
		{
			int sIndex, tIndex;
			sIndex = tIndex = i;
			int iDeltaLast = 0;
			int j;
			for (j = 0; j < (iHeight-2); j += 2)
			{
				int a = s[sIndex];
				int b = s[sIndex + _width];
				int c = s[sIndex + (_width<<1)];
				// high-pass filter
				int y = b - (a + c)/2;
				// low-pass filter
				int x = a + ((iDeltaLast+y)/4); // clip must be useless...
				//int x = a;
				iDeltaLast = y;
				t[j/2] = x;
				if(std::abs(y)<=iFloor)
					y = 0;
				t[j/2 + vOffset] = (((y*iISqrt2)/4096/iShift)*iShift);
				sIndex += _width << 1;
				//tIndex += _width;
			}
			if(j==(iHeight-2)) // non-even number
			{
				int a = s[sIndex];
				int b = s[sIndex + _width];
				int c = a;
				int y = b - (a + c)/2; // high-pass filter
				// low-pass filter
				//int x = a + ((iDeltaLast+y)/4); // clip must be useless...
				int x = a;
				iDeltaLast = y;
				t[j/2] = x;
				if(std::abs(y)<=iFloor)
					y = 0;
				t[j/2 + vOffset] = (((y*iISqrt2)/4096/iShift)*iShift);
			}
			else
			{
				int a = s[sIndex];
				int y = 0;
				int x = a + (iDeltaLast+y)/4;
				t[j/2] = x;
			}
			for(j=0; j<iHeight; ++j)_dst[tIndex+j*_width] = (int8_t)SClamp(t[j]);
		}
	}

	delete buffer;
}

// _src and _dst could be the same
void DecodeHaar(int8_t* _dst, const int8_t *_src, int _width, int _height)
{
	int iIterations = HaarIterations(_width, _height);
	int16_t* buffer = new int16_t[std::max(_width,_height)];

	//reverse the filtering process
	for (int k = iIterations - 1; k >= 0; k--)
	{
		int iWidth = _width;
		int iHeight = _height;
		for(int i=0; i<k; i++)
		{
			iWidth = (iWidth>>1) + (iWidth&0x1);
			iHeight = (iHeight>>1) + (iHeight&0x1);
		}

		const int8_t* s;
		int16_t* t;

		//vertical processing
		s = k==(iIterations - 1) ? _src : _dst;
		t = buffer;
		int vOffset = ((iHeight+1) >> 1) * _width;
		for (int i = 0; i < iWidth; i++)
		{
			int sIndex, tIndex;
			sIndex = tIndex = i;
			int iDeltaLast = 0;
			int j;
			for (j = 0; j < (iHeight-3); j += 2)
			{
				int x = s[sIndex];
				int y = (s[sIndex + vOffset]*iSqrt2)/4096;
				int x2 = s[sIndex + _width];
				int y2 = (s[sIndex + _width + vOffset]*iSqrt2)/4096;
				int a = x - (iDeltaLast+y)/4;
				int a2 = x2 - (y+y2)/4;
				//int a = x;
				//int a2 = x2;
				int b = y + (a+a2)/2 ;
				iDeltaLast = y;
				t[j] = a;
				t[j + 1] = b;
				sIndex += _width;
				//tIndex += _width << 1;
			}
				int x = s[sIndex];
				int y = (s[sIndex + vOffset]*iSqrt2)/4096;
				int x2 = s[sIndex + _width];
				int y2 = 0;
				//int a = x - (iDeltaLast+y)/4;
				//int a2 = x2 - ((y+y2)/4);
				int a = x;
				int a2 = x2;
				if (j+2>=iHeight)
					a2 = a;
				int b = y + (a+a2)/2;
				t[j] = a;
				t[j + 1] = b;
				sIndex += _width;
				//tIndex += _width << 1;
				j+=2;

			if(j<iHeight) // non even value
			{
				t[j] = a2;
			}
			for(j=0; j<iHeight; ++j)_dst[tIndex+j*_width] = (int8_t)SClamp(t[j]);
		}

		//horizontal processing
		s = _dst;
		t = buffer;

		int hOffset = (iWidth+1) >> 1;
		for (int j = 0; j < iHeight; j++)
		{
			int sIndex, tIndex;
			tIndex = sIndex = j * _width;
			int iDeltaLast = 0;
			int i;
			for (i = 0; i < (iWidth-3); i+=2)
			{
				int x = s[sIndex];
				int y = (s[sIndex + hOffset]*iSqrt2)/4096;
				int x2 = s[sIndex + 1];
				int y2 = (s[sIndex + 1 + hOffset]*iSqrt2)/4096;
				int a = x - (iDeltaLast+y)/4;
				int a2 = x2 - (y+y2)/4;
				int b = y + (a+a2)/2;
				iDeltaLast = y;
				t[i] = a;
				t[i + 1] = b;
				sIndex ++;
				//tIndex += 2;
			}
				int x = s[sIndex];
				int y = (s[sIndex + hOffset]*iSqrt2)/4096;
				int x2 = s[sIndex + 1];
				int y2 = 0;
				//int a = x - (iDeltaLast+y)/4;
				//int a2 = x2 - ((y+y2)/4);
				int a = x;
				int a2 = x2;
				if (i+2>=iWidth)
					a2 = a;
				int b = y + (a+a2)/2;
				t[i] = a;
				t[i + 1] = b;
				sIndex ++;
				//tIndex += 2;
				i+=2;

			if(i<iWidth) // non power of 2
			{
				t[i] = a2;
			}
			for(i=0; i<iWidth; ++i)_dst[tIndex+i]=(int8_t)SClamp(t[i]);
		}
	}

	delete buffer;
}

void shift_u8_to_s8(uint8_t* _ImgData, int _width, int _height)
{
	// shift from [0; 255] to [-128; 127]
	for (int j = 0; j < _height; j++)
	{
		for (int i = 0; i < _width; i++)
		{
			int iValue = _ImgData[j*_width+i];
			_ImgData[j*_width+i] = (iValue - 128);
		}
	}
}

void shift_s8_to_u8(uint8_t* _ImgData, int _width, int _height)
{
	// shift from [-128; 127] to [0; 255]
	for (int j = 0; j < _height; j++)
	{
		for (int i = 0; i < _width; i++)
		{
			int iValue = _ImgData[j*_width+i];
			_ImgData[j*_width+i] = (iValue + 128);
		}
	}
}

// _Dest = _A - _B
// add
void sub_buffer(
				ALIGN_PREDECL(4) int8_t* __restrict _dst,
				ALIGN_PREDECL(4) const int8_t * const _a,
				ALIGN_PREDECL(4) const int8_t * const _b,
				const int _width,
				const int _height)
{
	//long long iPosAcc=0;
	//int iPosCount=0;
	//long long iNegAcc=0;
	//int iNegCount=0;

	for(int j=0; j<_height; ++j)
	{
		for(int i=0; i<_width; ++i)
		{
			int iOffset = j*_width+i;
			_dst[iOffset] = SClamp((_a[iOffset] - _b[iOffset]), SCHAR_MIN, SCHAR_MAX);
			//_Dest[iOffset] = SClamp((int8_t(_A[iOffset]) - int8_t(_B[iOffset]))/2, SCHAR_MIN, SCHAR_MAX);
			//if(_Dest[iOffset]>0)
			//{
			//	iPosAcc += _Dest[iOffset];
			//	iPosCount ++;
			//}
			//else if(_Dest[iOffset]<0)
			//{
			//	iNegAcc += _Dest[iOffset];
			//	iNegCount ++;
			//}
		}
	}
	//int iPosAvg = iPosAcc/iPosCount;
	//int iNegAvg = iNegAcc/iNegCount;
}
/*
void add_post_filter(
				int8_t* __restrict _dst,
				const int8_t *  ALIGN_PREDECL(16) const _a,
				const int8_t * const _b, 
				const int _width,
				const int _height);
*/
// _Dest = A+B
void add_buffer(
				ALIGN_PREDECL(4) int8_t* __restrict _dst,
				//int8_t* ALIGN_PREDECL(16) const _dst, // ok...
				ALIGN_PREDECL(4) const int8_t * __restrict ALIGN_PREDECL(16) const _a,
				ALIGN_PREDECL(4) const int8_t * __restrict const _b, 
				const int _width,
				const int _height)
{
	// todo: assert pointer alignment
	for(int j=0; j<_height; ++j)
	{
		for(int i=0; i<_width; ++i)
		{
			int iOffset = j*_width+i;
			_dst[iOffset] = SClamp(_a[iOffset] + (_b[iOffset]), SCHAR_MIN, SCHAR_MAX);
			//_dst[iOffset] = SClamp(_a[iOffset]+_b[iOffset]*2, SCHAR_MIN, SCHAR_MAX);
			_dst[iOffset] = clamp<int8_t>(_a[iOffset] + (_b[iOffset]));
		}
	}
	// test post filtering
	//static bool bPostFilter = false;
	//if(bPostFilter)
	//{
	//	add_post_filter(_dst, _dst, _b, _width, _height);
	//}
}
/*
void add_post_filter(
				int8_t* __restrict _dst,
				const int8_t *  ALIGN_PREDECL(16) const _a,
				const int8_t * const _b, 
				const int _width,
				const int _height)
{
	static int iBias = 1;
	// todo: assert pointer alignment
	for(int j=0; j<_height; ++j)
	{
		for(int i=1; i<(_width-1); ++i)
		{
			int iOffset = j*_width+i;
			int fSlope = std::abs(_b[iOffset-1] - _b[iOffset+1])/iBias/255.f;
			int iBlur = (_a[iOffset-1] + _a[iOffset+1]) / 2;

			_dst[iOffset] = _a[iOffset]*(1.f-fSlope) + (iBlur*fSlope);
		}
	}
}
*/
// todo: add buffer alignment & test with avx optim
int64_t EnergyHaarCoeff(const int8_t * const _src, int _width, int _height)
{
	int iIterations = HaarIterations(_width, _height);

	int64_t iEnergy = 0;
	int iBeginWidth = _width;
	int iEndWidth = 0;
	int iBeginHeight = _height;
	int iEndHeight = 0;
	for (int k = 0; k < iIterations; k++)
	{
		iBeginWidth = (_width>>1) + (_width&0x1);
		iEndWidth = _width;
		iBeginHeight = (_height>>1) + (_height&0x1);
		iEndHeight = _height;
		for(int i=0; i<k; i++)
		{
			iBeginWidth = (iBeginWidth>>1) + (iBeginWidth&0x1);
			iBeginHeight = (iBeginHeight>>1) + (iBeginHeight&0x1);
			iEndWidth = (iEndWidth>>1) + (iEndWidth&0x1);
			iEndHeight = (iEndHeight>>1) + (iEndHeight&0x1);
		}

		for(int j=0; j<iBeginHeight; ++j)
		{
			for(int i=iBeginWidth; i<iEndWidth; ++i)
			{
				int iOffset = j*_width+i;
				iEnergy += (1<<(2*k)) * std::abs(_src[iOffset]);
			}
		}

		for(int j=iBeginHeight; j<iEndHeight; ++j)
		{
			for(int i=0; i<iEndWidth; ++i)
			{
				int iOffset = j*_width+i;
				iEnergy += (1<<(2*k)) * std::abs(_src[iOffset]);
			}
		}
	}
	// remaining low-filter datas
	for(int j=0; j<iBeginHeight; ++j)
	{
		for(int i=0; i<iBeginWidth; ++i)
		{
			int iOffset = j*_width+i;
			// todo: code this part...
			//_dst[iOffset] = SClamp((_a[iOffset] - _b[iOffset])/iShift*iShift, SCHAR_MIN, SCHAR_MAX);
		}
	}
	return iEnergy;
}
