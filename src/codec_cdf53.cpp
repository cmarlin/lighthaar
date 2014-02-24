#include "codec_cdf53.h"
#include "utils.h"
#include <algorithm>

#if 1

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
			_src and _dst could be the same
	@param _quality [1; 128] => [top;worst] conversion quality (reduce output values domain)
	*/
void EncodeCDF53(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality)
{
	int iIterations = HaarIterations(_width, _height);
	int16_t* buffer = new int16_t[std::max(_width,_height)];

	//filter the values
	for (int k = 0; k < iIterations; k++)
	{
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
				if(std::abs(y)<=0)
					y = 0;
				t[i/2 + hOffset] = y/_quality;
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
				if(std::abs(y)<=0)
					y = 0;
				t[i/2 + hOffset] = y/_quality;
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
				if(std::abs(y)<=0)
					y = 0;
				t[j/2 + vOffset] = y/_quality;
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
				if(std::abs(y)<=0)
					y = 0;
				t[j/2 + vOffset] = y/_quality;
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

/**
	@brief _src and _dst could be the same
	*/
void DecodeCDF53(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality)
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
				int y = s[sIndex + vOffset]*_quality;
				int x2 = s[sIndex + _width];
				int y2 = s[sIndex + _width + vOffset]*_quality;
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
				int y = s[sIndex + vOffset]*_quality;
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
				int y = s[sIndex + hOffset]*_quality;
				int x2 = s[sIndex + 1];
				int y2 = s[sIndex + 1 + hOffset]*_quality;
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
				int y = s[sIndex + hOffset]*_quality;
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

/**
	@todo add buffer alignment & test with avx optim
	*/
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


#else

void EncodeCDF53(uint8_t* _dst, const uint8_t *_src, int _width, int _height, int _quality)
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
		int fac = 2;

		const uint8_t* s;
		int16_t* t;

		int iWidth = _width;
		int iHeight = _height;
		for(int i=0; i<k; i++)
		{
			iWidth = (iWidth>>1) + (iWidth&0x1);
			iHeight = (iHeight>>1) + (iHeight&0x1);
		}

		// vertical
		s = k==0 ? _src : _dst;
		t = buffer;

		int sx = iWidth;
		int sy = iHeight;
		sx-=((sx&1)==0);
		sy-=((sy&1)==0);

		unsigned x,y,ssx=sx/2,ssy=sy/2,xx,yy;
		int c1,c2,c3,predict,n;
		for( x=0;x<sx;x++ )
		{
			for( y=0,yy=0;y<ssy;y++,yy++,yy++ )
			{
				c1 = s[yy*_width+x];
				c2 = s[(yy+2)*_width+x];
				predict = (c1+c2)/2;
				c3 = s[(yy+1)*_width+x];
				t[y+ssy+1] = c3 - predict;
				t[y] = c1;
			}
			c1 = s[yy*_width+x];
			t[y] = c1;
			for( y=1;y<ssy-1;y++ )
			{
				int n = t[y]+(t[y+ssy]+t[y+ssy+1])/4;
				t[y] = UClamp(n);
			}
			for(y=0; y<sy; ++y)
			{
				if (y>ssy)
					_dst[y*_width+x]=t[y]/fac;
				else
					_dst[y*_width+x]=t[y];
			}
		}

		// horizontal
		s = _dst;
		t = buffer;
		for( y=0;y<ssy+1;y++ )
		{
			for( x=0,xx=0;x<ssx;x++,xx++,xx++ )
			{
				c1 = s[y*_width+xx];
				c2 = s[y*_width+xx+2];
				predict=(c1+c2)/2;
				c3 = s[y*_width+xx+1];
				t[x+ssx+1]=(short)c3-(short)predict;
				t[x]=c1;
			}
			c1=s[y*_width+xx];
			t[x]=c1;
			for( x=1;x<ssx-1;x++ )
			{
				n=t[x]+(t[x+ssx] + t[x+ssx+1])/4;
				t[x]=UClamp(n);
			}
			for( x=0;x<sx;x++)
			{
				if (x>ssx)
					_dst[y*_width+x] = t[x]/fac;
				else
					_dst[y*_width+x] = t[x];
			}
		}
	}

	delete buffer;
}

// _src and _dst could be the same
void DecodeCDF53(uint8_t* _dst, const uint8_t *_src, int _width, int _height)
{
	int iIterations = HaarIterations(_width, _height);
	uint8_t* buffer = new uint8_t[std::max(_width,_height)];

	//reverse the filtering process
	for (int k = iIterations - 1; k >= 0; k--)
	{
		int fac=2;

		int iWidth = _width;
		int iHeight = _height;
		for(int i=0; i<k; i++)
		{
			iWidth = (iWidth>>1) + (iWidth&0x1);
			iHeight = (iHeight>>1) + (iHeight&0x1);
		}

		const uint8_t* s;
		uint8_t* t;

		//vertical processing
		s = k==(iIterations - 1) ? _src : _dst;
		t = buffer;

		int sx = iWidth;
		int sy = iHeight;
		sx-=((sx&1)==0);
		sy-=((sy&1)==0);

		// horizontal
		unsigned ssx=sx/2,ssy=sy/2,x,y;
		uint8_t c1,c2,c3;
		int n;
		for( y=0;y<ssy+1;y++ )
		{
			for( x=0;x<=ssx;x++ )
				t[x<<1]=s[y*_width+x];
			for( x=1;x<ssx-1;x++ )
			{
				c1=s[y*_width+x+ssx];
				c2=s[y*_width+x+ssx+1];
				n=(int)t[x<<1]-(((int)(char)c1)*fac+((int)(char)c2)*fac)/4;	
				t[x<<1]=UClamp(n);
			}
			for( x=0;x<ssx;x++ )
			{
				c1=s[y*_width+x+ssx+1];
				n=((int)(char)c1)*fac + ((int)t[x<<1] + (int)t[(x<<1)+2])/2;
				t[(x<<1)+1]=UClamp(n);
			}
			for(x=0;x<sx;x++)
			{
				_dst[y*_width+x] = t[x];
			}
		}


		for( x=0;x<sx;x++ )
		{
			for( y=0;y<=ssy;y++ )
				t[y<<1] = s[y*_width+x];
			for( y=1;y<ssy-1;y++ )
			{
				c1=s[(y+ssy)*_width+x];
				c2=s[(y+ssy+1)*_width+x];
				n=(int)t[y<<1] - (((int)(char)c1)*fac+((int)(char)c2)*fac)/4;
				t[y<<1]=UClamp(n);
			}
			for( y=0;y<ssy;y++ )
			{
				c1=s[(y+ssy+1)*_width+x];
				n=((int)(char)c1)*fac + ((int)t[y<<1] + (int)t[(y<<1)+2])/2;
				t[(y<<1)+1]=UClamp(n);
			}
			for( y=0;y<sy;y++ )
			{
				_dst[y*_width+x]=t[y];
			}
		}
	}

	delete buffer;
}
#endif