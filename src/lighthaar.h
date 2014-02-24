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
		int			m_iQuality;
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


#endif // LIGHTHAAR_H
