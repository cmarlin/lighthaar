//
// LightHaar codec By Cyril MARLIN & Mathieu LEGRIS
// MIT licence 2012-14
//


#include "lighthaar.h"
#include "utils.h"
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
