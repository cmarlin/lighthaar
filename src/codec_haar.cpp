#include "codec_haar.h"
#include "utils.h"
#include <algorithm>
#include <assert.h>

void EncodeHaarLossLess(int8_t* _dst, const int8_t *_src, int _width, int _height, int /*_quality*/)
{
	int iImgWidthLog2 = Log2(_width) + (IsPowerOf2(_width) ? 0 : 1);
	int iImgHeightLog2 = Log2(_height) + (IsPowerOf2(_height) ? 0 : 1);
	int iIterations = std::min<int>(iImgWidthLog2, iImgHeightLog2);
	//int iIterations = 1; // test
	int8_t* buff = new int8_t[std::max(_width,_height)];

	//filter the values
	for (int k = 0; k < iIterations; k++)
	{
		const int8_t* s;

		int iWidth = _width;
		int iHeight = _height;
		for(int i=0; i<k; i++)
		{
			iWidth = (iWidth>>1) + (iWidth&0x1);
			iHeight = (iHeight>>1) + (iHeight&0x1);
		}

		//horizontal processing
		s = k==0 ? _src : _dst;

		int hOffset = (iWidth+1) >> 1;
		for (int j = 0; j < iHeight; j++)
		{
			int sIndex, tIndex;
			tIndex = sIndex = j * _width;
			int i;
			for (i = 0; i < (iWidth-1); i+=2)
			{
				int a = s[sIndex];
				int b = s[sIndex + 1];
				int8_t y = b-a;
				int8_t x = a + int8_t(y)/2;
				buff[i/2] = x;
				buff[i/2 + hOffset] = y;
				sIndex += 2;
			}
			if(i<iWidth) // non even size
			{
				buff[i/2] = s[sIndex];
			}
			// copy back data
			for(i=0; i<iWidth; ++i)_dst[tIndex+i]=buff[i];
		}

		//vertical processing
		s = _dst;
		int vOffset = (iHeight+1) >> 1;
		// note: rewrite to be more cache friendly
		for (int i = 0; i < iWidth; i++)
		{
			int sIndex, tIndex;
			sIndex = tIndex = i;
			int j;
			for (j = 0; j < (iHeight-1); j += 2)
			{
				int a = s[sIndex];
				int b = s[sIndex + _width];
				int8_t y = b-a;
				int8_t x = a + int8_t(y)/2;
				buff[j/2] = x;
				buff[j/2 + vOffset] = y;
				sIndex += _width << 1;
			}
			if(j<iHeight) // non-even number
			{
				buff[j/2] = s[sIndex];
			}
			for(j=0; j<iHeight; ++j)_dst[tIndex+j*_width] = buff[j];
		}
	}

	delete[] buff;
}

void DecodeHaarLossLess(int8_t* _dst, const int8_t *_src, int _width, int _height, int /*_quality*/)
{
	int iImgWidthLog2 = Log2(_width) + (IsPowerOf2(_width) ? 0 : 1);
	int iImgHeightLog2 = Log2(_height) + (IsPowerOf2(_height) ? 0 : 1);
	int iIterations = std::min<int>(iImgWidthLog2, iImgHeightLog2);
	//int iIterations = 1;
	int8_t* buff = new int8_t[std::max(_width,_height)];

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
		s = k==0 ? _src : _dst;

		//vertical processing
		int vOffset = ((iHeight+1) >> 1) * _width;
		for (int i = 0; i < iWidth; i++)
		{
			int sIndex, tIndex;
			sIndex = tIndex = i;
			int j;
			for (j = 0; j < (iHeight-1); j += 2)
			{
				int8_t x = s[sIndex];
				int8_t y = s[sIndex + vOffset];
				int8_t a = (x - int8_t(y)/2);
				int8_t b = a + y;
				buff[j] = a;
				buff[j + 1] = b;
				sIndex += _width;
			}
			if(j<iHeight) // non even value
			{
				buff[j] = s[sIndex];
			}
			for(j=0; j<iHeight; ++j)_dst[tIndex+j*_width] = buff[j];
		}

		//horizontal processing
		s = _dst;
		int hOffset = (iWidth+1) >> 1;
		for (int j = 0; j < iHeight; j++)
		{
			int sIndex, tIndex;
			tIndex = sIndex = j * _width;
			int i;
			for (i = 0; i < (iWidth-1); i+=2)
			{
				int8_t x = s[sIndex];
				int8_t y = s[sIndex + hOffset];
				int8_t a = (x - int8_t(y)/2);
				int8_t b = a + y;
				buff[i] = a;
				buff[i + 1] = b;
				sIndex ++;
			}
			if(i<iWidth) // non even value
			{
				buff[i] = s[sIndex];
			}
			for(i=0; i<iWidth; ++i)_dst[tIndex+i]=buff[i];
		}
	}

	delete[] buff;
}

/**
@todo:
- corriger le shift
- éviter le SClamp
  * Decode: la moyenne peut bouger lors des decodage successifs
*/
void EncodeHaar(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality)
{
	/*if(1 == _quality)
	{
		EncodeHaarLossLess(_dst, _src, _width, _haight, _quality);
	}*/

	int iImgWidthLog2 = Log2(_width) + (IsPowerOf2(_width) ? 0 : 1);
	int iImgHeightLog2 = Log2(_height) + (IsPowerOf2(_height) ? 0 : 1);
	int iIterations = std::min<int>(iImgWidthLog2, iImgHeightLog2);
	//int iIterations = 1; // test
	int8_t* buff = new int8_t[std::max(_width,_height)];

	//filter the values
	for (int k = 0; k < iIterations; k++)
	{
		const int8_t* s;

		int iWidth = _width;
		int iHeight = _height;
		for(int i=0; i<k; i++)
		{
			iWidth = (iWidth>>1) + (iWidth&0x1);
			iHeight = (iHeight>>1) + (iHeight&0x1);
		}

		//horizontal processing
		s = k==0 ? _src : _dst;

		int hOffset = (iWidth+1) >> 1;
		for (int j = 0; j < iHeight; j++)
		{
			int sIndex, tIndex;
			tIndex = sIndex = j * _width;
			int i;
			for (i = 0; i < (iWidth-1); i+=2)
			{
				int a = s[sIndex];
				int b = s[sIndex + 1];
				int8_t y = SClamp((b-a)/2); 
				int8_t x = a + y;
				buff[i/2] = x;
				buff[i/2 + hOffset] = y/_quality;
				sIndex += 2;
			}
			if(i<iWidth) // non even size
			{
				buff[i/2] = s[sIndex];
			}
			// copy back data
			for(i=0; i<iWidth; ++i)_dst[tIndex+i]=buff[i];
		}

		//vertical processing
		s = _dst;
		
		int vOffset = (iHeight+1) >> 1;
		// note: rewrite to be more cache friendly
		for (int i = 0; i < iWidth; i++)
		{
			int sIndex, tIndex;
			sIndex = tIndex = i;
			int j;
			for (j = 0; j < (iHeight-1); j += 2)
			{
				int a = s[sIndex];
				int b = s[sIndex + _width];
				int8_t y = SClamp((b-a)/2);
				int8_t x = a + y;
				buff[j/2] = x;
				buff[j/2 + vOffset] = y/_quality;
				sIndex += _width << 1;
			}
			if(j<iHeight) // non-even number
			{
				buff[j/2] = s[sIndex];
			}
			for(j=0; j<iHeight; ++j)_dst[tIndex+j*_width] = buff[j];
		}
		
	}

	delete[] buff;
}

void DecodeHaar(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality)
{
	/*if(1 == _quality)
	{
		DecodeHaarLossLess(_dst, _src, _width, _haight, _quality);
	}*/
	int iImgWidthLog2 = Log2(_width) + (IsPowerOf2(_width) ? 0 : 1);
	int iImgHeightLog2 = Log2(_height) + (IsPowerOf2(_height) ? 0 : 1);
	int iIterations = std::min<int>(iImgWidthLog2, iImgHeightLog2);
	//int iIterations = 1;
	int8_t* buff = new int8_t[std::max(_width,_height)];

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
		s = k==0 ? _src : _dst;

		//vertical processing
		int vOffset = ((iHeight+1) >> 1) * _width;
		for (int i = 0; i < iWidth; i++)
		{
			int sIndex, tIndex;
			sIndex = tIndex = i;
			int j;
			for (j = 0; j < (iHeight-1); j += 2)
			{
				int x = s[sIndex];
				int y = s[sIndex + vOffset]*_quality;
				int8_t a = SClamp(x - y);
				int8_t b = SClamp(x + y);
				buff[j] = a;
				buff[j + 1] = b;
				sIndex += _width;
			}
			if(j<iHeight) // non even value
			{
				buff[j] = s[sIndex];
			}
			for(j=0; j<iHeight; ++j)_dst[tIndex+j*_width] = buff[j];
		}

		//horizontal processing
		s = _dst;
		int hOffset = (iWidth+1) >> 1;
		for (int j = 0; j < iHeight; j++)
		{
			int sIndex, tIndex;
			tIndex = sIndex = j * _width;
			int i;
			for (i = 0; i < (iWidth-1); i+=2)
			{
				int x = s[sIndex];
				int y = s[sIndex + hOffset]*_quality;
				int8_t a = SClamp(x - y);
				int8_t b = SClamp(x + y);
				buff[i] = a;
				buff[i + 1] = b;
				sIndex ++;
			}
			if(i<iWidth) // non even value
			{
				buff[i] = s[sIndex];
			}
			for(i=0; i<iWidth; ++i)_dst[tIndex+i]=buff[i];
		}
		
	}

	delete[] buff;
}
