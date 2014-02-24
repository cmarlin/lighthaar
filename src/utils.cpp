#include "utils.h"
#include <math.h>

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
	for(int j=0; j<_height; ++j)
	{
		for(int i=0; i<_width; ++i)
		{
			int iOffset = j*_width+i;
			_dst[iOffset] = SClamp((_a[iOffset] - _b[iOffset]), SCHAR_MIN, SCHAR_MAX);
		}
	}
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
			//_dst[iOffset] = clamp<int8_t>(_a[iOffset] + (_b[iOffset]));
		}
	}
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

double PSNR(uint8_t *p1, uint8_t *p2, int _size)
{
    double d;
    int i;
    d = 0;
    for (i = 0; i < _size; i++)
    {
        d += ((*p1) - (*p2)) * ((*p1) - (*p2));
        p1++;
        p2++;
    }
    //printf ("Y: %5.2f, ", 10 * log10(255 * 255 / d * _size));
	return 10 * log10(255 * 255 / d * _size);
}

double SHIFT(uint8_t *p1, uint8_t *p2, int _size)
{
	double d;
	d = 0;
    for (int i = 0; i < _size; i++)
    {
		d += ((*p2) - (*p1));
        p1++;
        p2++;
	}
	return d/_size;
}