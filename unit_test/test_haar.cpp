#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <stdint.h>

#include "lightHaar.h"
#include "utils.h"


//#define LOSSLESS	// check lossless/lossy version
#define COEF_LOSS 4	// for lossy: how many bits to lose ?
#define EPSILON 255

// if encode is true:
// x & y are inputs to code; a & b hold haar-coded values 
// x & y are haar inputs to decode; a & b holds output
void haar(int8_t& _a, int8_t& _b, const int8_t& _x, const int8_t& _y, const int8_t _previous, bool _encode)
{
	// ok
#if 0
	if(encode) {
		*_a = _x;
		*_b = _y;
	} else {
		*_a = _x;
		*_b = _y;
	}
#endif

#if 0
	// fail
	if(encode) {
		*a = (x+y)/2;
		*b = (x-y)/2;
	} else {
		*a = x + y;
		*b = x - y;
	}
#endif

#if 0
	if(encode) { // ok
		a = x;
		b = (y-x) & 255;
	} else {
		a = x;
		b = (x + y) & 255;
	}
#endif

#if 0
	if(encode) { // fail => delta in [-255;255] range, bigger than u8
		int delta = (y-x);
		int offset = delta/2;
		b = (delta) & 255;
		a = x + offset;
	} else {
		int delta = char(y);
		int offset = delta/2;
		a = x - (offset);
		b = (a + y) & 255;
	}
#endif

#if 0
	if(encode) { // ok!
		int delta = (y-x) & 255;
		int sdelta = char(delta);
		int offset = sdelta/2;
		b = (delta);
		a = x + offset;
	} else {
		int delta = y;
		int sdelta = char(delta);
		int offset = sdelta/2;
		a = x - (offset);
		b = (a + y) & 255;
	}
#endif

#if 0
	if(_encode) { // ok
		uint8_t delta = (_y-_x);
		_b = (delta);
		_a = _x + char(delta)/2;
	} else {
		_a = (_x - char(_y)/2);
		_b = (_a + _y);
	}
#endif

#if 1
	if(_encode) { // ok
		_b = SClamp((_y-_x)/2);
		//_a = (_x + _y)/2;
		_a = _x + _b;
	} else {
		_a = SClamp(_x - _y);
		_b = SClamp(_x + _y);
	}
#endif

#if 0
	if(_encode) { // ok
		//uint8_t delta = Clip(_y - _x, SCHAR_MIN, SCHAR_MAX); // avoid ring effect
		int delta = _y - _x;
		_a = _x + delta/2;
		int predict = (_a*2+_previous)/3;
		int delta_predict = (_a-predict)*2;
		_b = Clip(delta - delta_predict, SCHAR_MIN, SCHAR_MAX);
	} else {
		int predict = (_x*2+_previous)/3;
		int Y = _y+(_x-predict)*2;
		_a = Clip(_x - Y/2, SCHAR_MIN, SCHAR_MAX);
		_b = Clip(_a + Y, SCHAR_MIN, SCHAR_MAX);
		//_a = (_x - char(_y)/2);
		//_b = (_a + _y);
	}
#endif
}

int main()
{
	bool bSuccess = true;
	int iSuccess[256] = {0};
	int iError[512] = {0};
	long long shift = 0;
	for(int k=SCHAR_MIN; k<=SCHAR_MAX; ++k)
	{
		int8_t z = k;
		for(int j=SCHAR_MIN; j<=SCHAR_MAX; ++j)
		{
			for(int i=SCHAR_MIN; i<=SCHAR_MAX; ++i)
			{
				int8_t a, b;
				int8_t x = i;
				int8_t y = j;
				haar(a, b, x, y, z,true);

#if 1 // defined(LOSSLESS)
				int8_t a_prim = a;
				int8_t b_prim = b;
#else
				int8_t a_prim = ((a>>COEF_LOSS)<<COEF_LOSS);
				int8_t b_prim = ((b>>COEF_LOSS)<<COEF_LOSS);
#endif
				int8_t x_out, y_out;
				haar(x_out, y_out, a_prim, b_prim, z, false);

				iError[abs(x_out-x)+abs(y_out-y)]++;

#if defined(LOSSLESS)
				bool bResult = (x_out==x) && (y_out==y);
#else
				//const int iEpsilon = (1<<COEF_LOSS);
				bool bResult = (abs(x_out-x) + abs(y_out-y))<EPSILON;
#endif
				if(bResult)
				{
					iSuccess[j]++;
				}
				else
				{
					int z=0;
				}

				shift += (x_out-x) + (y_out-y);

				/*if(i==92 && j==32)
				{
				int z=0;
				}*/
				/*
				printf("[%03d;%03d] -> [%03d;%03d] -> [%03d;%03d] %s\n",
				i, j,
				a, b,
				x, y,
				bResult?"ok":"err");
				*/
				bSuccess &= bResult;
			}
		}
	}

	printf("shift :%f\n", shift/(256.f*256.f));

	for(int j=0; j<512; j++)
	{
		//printf("[%03d] -> %03d\n", j, iSuccess[j]);
		printf("[%03d] -> %03d\n", j, iError[j]);
	}
	printf("Result:%s\n", bSuccess?"Success!":"Fail!");
	return 0;
}