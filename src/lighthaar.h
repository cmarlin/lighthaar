//
// LightHaar codec By Cyril MARLIN & Mathieu LEGRIS
// MIT licence 2012
//
// @brief LighHaar library interface
//


#include <stdint.h>
#pragma hdrstop

////
/// User part
//
namespace LightHaar
{
	typedef void* (*LightHaarAlloc)(intptr_t iUser, size_t iSize, size_t iAlign);
	typedef void  (*LightHaarFree)(intptr_t iUser, void* ptr);
	void SetAllocator(LightHaarAlloc pAllocFunc, LightHaarFree pFreeFunc, intptr_t iUser);

#define ALIGN_PREDECL(x) __declspec(align(x))

	enum FrameType
	{
		I_Frame=0,		// intra (fully descriptive)
		P_Frame=1,		// progressive

		FrameTypeInvalid = 0xffffffff
	};

	////
	/// Internal part (put to another header file)
	//
		void* operator new(size_t iSize);
		void* operator new[](size_t iSize);
		void operator delete(void*);
		void operator delete[](void*);

	struct BaseObj
	{
		// placement new
		//void* operator new(void*);
		//void* operator new[](void*);
		//void operator delete(void*);
		//void operator delete[](void*);
	};

	struct FrameInfo : BaseObj
	{
		FrameType	m_iFrameType;
	};

	struct FrameBuffer : BaseObj
	{
		FrameBuffer()
			:m_y(NULL)
			,m_u(NULL)
			,m_v(NULL)
		{
		}

		int8_t* m_y;
		int8_t* m_u;
		int8_t* m_v;
	};

	struct Codec : BaseObj
	{
		// buffers
		static const int m_iFrameBufferCount = 3;
		FrameBuffer m_frameBuffers[m_iFrameBufferCount];
		int m_curFrameBuffer;	/// current buffer to use for encoding, loop in m_iFrameBufferCount

		int m_iIFrameCounter;	/// current counter to insert I frame
		int m_iIFrameLoop;		/// I frame insertion frequency

		int m_iHaarQuality;		/// haar conversion quality [1;128], default 10

		Codec();

		void Init(const int _width, const int _height);
		void Term();

		/*struct CompressInputParam
		{
		};

		struct CompressOutputParam
		{
		};

		void Compress(CompressOutputParam* _out, const CompressInputParam* _in);
		*/
	};
}

/// todo: add pitch, move to -yuv- files
void ConvertRGB24toYCrCb(int8_t* _pY, int8_t* _pU, int8_t* _pV, const uint8_t* _pRGB, int _width, int _height);
void ConvertYCrCbtoRGB24(uint8_t* _pRGB, const int8_t* _pY, const int8_t* _pU, const int8_t* _pV, int _width, int _height);

int HaarIterations(int _width, int _height);
void EncodeHaar(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality=1);
void DecodeHaar(int8_t* _dst, const int8_t *_src, int _width, int _height);
int64_t EnergyHaarCoeff(const int8_t * const _src, int _width, int _height);

void sub_buffer(
				ALIGN_PREDECL(4) int8_t* __restrict _Dest,
				ALIGN_PREDECL(4) const int8_t * const _A,
				ALIGN_PREDECL(4) const int8_t * const _B,
				const int _width,
				const int _height);

void add_buffer(
				ALIGN_PREDECL(4) int8_t* __restrict _Dest,
				ALIGN_PREDECL(4) const int8_t * __restrict const _A,
				ALIGN_PREDECL(4) const int8_t * __restrict const _B, 
				const int _width,
				const int _height);

void shift_u8_to_s8(uint8_t* _ImgData, int _width, int _height);
void shift_s8_to_u8(uint8_t* _ImgData, int _width, int _height);


template<typename t> inline t clamp(t x, t min=0, t max=UCHAR_MAX)
{
	return (x<max)?(x>=min?x:min):max;
}
/*
template<> inline uint8_t clamp<uint8_t>(uint8_t x, uint8_t min, uint8_t max)
{
	return (x<UCHAR_MAX)?((x>=0)?x:0):UCHAR_MAX;
}

template<> inline int8_t clamp<int8_t>(int8_t x, int8_t min=SCHAR_MAX, int8_t max=SCHAR_MAX)
{
	return (x<SCHAR_MAX)?((x>=SCHAR_MIN)?x:SCHAR_MIN):SCHAR_MAX;
}
*/
//////////////////////////////////////////////////////////////////////////
template<typename t> t SClamp(t x, t min=SCHAR_MIN, t max=SCHAR_MAX) 
{
	return (x<max)?(x>=min?x:min):max;
}

//////////////////////////////////////////////////////////////////////////
template<typename t> t UClamp(t x, t min=0, t max=UCHAR_MAX) 
{
	return (x<max)?(x>=min?x:min):max;
}

//////////////////////////////////////////////////////////////////////////
template<typename t> bool IsPowerOf2(t x)
{
	return ((x & (x-1))==0);
}

//////////////////////////////////////////////////////////////////////////
template<typename t> t Log2(t n)
{
	//sizeof(u32) * CHAR_BIT
	t ret = 0;
	while(n>>=1)
	{
		ret++;
	};
	return ret;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Get the next larger power of 2 (res > n).
template<typename t> t NextLargerPowerOf2(t n)
{
	for (unsigned int i = 1; i < sizeof(t) * CHAR_BIT; i <<= 1)
	{
		n |= (n >> 1);
	}
	return n + 1;
}
