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

