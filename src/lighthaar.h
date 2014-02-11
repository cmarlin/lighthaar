#ifndef LIGHTHAAR_H
#define LIGHTHAAR_H

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


#endif // LIGHTHAAR_H
