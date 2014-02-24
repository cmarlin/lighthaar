#ifndef UTILS_H
#define UTILS_H

#include "stdint.h"


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

//////////////////////////////////////////////////////////////////////////
#define ALIGN_PREDECL(x) __declspec(align(x))

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
void shift_u8_to_s8(uint8_t* _ImgData, int _width, int _height);
void shift_s8_to_u8(uint8_t* _ImgData, int _width, int _height);

//////////////////////////////////////////////////////////////////////////
double PSNR (uint8_t *p1, uint8_t *p2, int _size);
double SHIFT(uint8_t *p1, uint8_t *p2, int _size);

#endif //UTILS_H