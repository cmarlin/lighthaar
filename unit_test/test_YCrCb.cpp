#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <stdint.h>

#include "lightHaar.h"
#include "utils.h"


static void RGB2YCrCb(int& _y, int& _u, int& _v, const int _r, const int _g, const int _b)
{
#if 0
	// YIQ color model
	int Y = (299*_r + 587*_g + 114*_b ) / 1000;
	int I = (596*_r - 275*_g - 321*_b ) / 1192 + 127;
	int Q = (212*_r - 523*_g + 311*_b ) / 1046 + 127;
	_y=((int)Y-127)*2/3+127;
	_u=((int)I-127)*2/3+127;
	_v=((int)Q-127)*2/3+127;
#endif
#if 1
	const int cyb = int(0.114f*4096.f);
	const int cyg = int(0.587f*4096.f);
	const int cyr = int(0.299f*4096.f);
	const int cu = int(0.565f*4096.f);
	const int cv = int(0.713f*4096.f);
	int fxR = (_r<<(12-8)) + int(0.5f/255.f*4095.f);
	int fxG = (_g<<(12-8)) + int(0.5f/255.f*4095.f);
	int fxB = (_b<<(12-8)) + int(0.5f/255.f*4095.f);
	int fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;	//Y = 0.299R + 0.587G + 0.114B
	int fxU = ((fxB-fxY)*cu)>>12; //U'= (B-Y)*0.565
	int fxV = ((fxR-fxY)*cv)>>12; //V'= (R-Y)*0.713
	int y=(299*_r+587*_g+114*_b)/1000;
	_y = fxY>>(12-8);
	_u = (fxU>>(12-8)) + 127;
	_v = (fxV>>(12-8)) + 127;
#endif
#if 0
	float fy = 0.299f*(_r+0.5f)+ 0.587f*(_g+0.5f) + 0.114f*(_b+0.5f);
	_y = int(fy);
	_u = int(((_b+0.5f)-fy)*0.565f);
	_v = int(((_r+0.5f)-fy)*0.713f);
	_u += 127;
	_v += 127;
#endif
}

static void YCrCb2RGB(int& _r, int& _g, int& _b, const int _y, const int _u, const int _v)
{
#if 0
	// YIQ color model
	int u,I,Q;
	I=((int)_u-127)*166/127;
	Q=((int)_v-127)*148/127;
	u=(int)_y+(956*I+621*Q)/1000;
	u=(u-127)*3/2+127;
	_r=u<0?0:u>255?255:u;
	u=(int)_y-(272*I+647*Q)/1000;
	u=(u-127)*3/2+127;
	_g=u<0?0:u>255?255:u;
	u=(int)_y+(1703*Q-1106*I)/1000;
	u=(u-127)*3/2+127;
	_b=u<0?0:u>255?255:u;
#endif
#if 1
	const int cvr = int(1.403f*4096.f);
	const int cug = int(0.344f*4096.f);
	const int cvg = int(0.714f*4096.f);
	const int cub = int(1.770f*4096.f);
	//const int half = (1<<((12-8)-1)); // add half value to better round ?
	//int fxY = _y<<(12-8) + (1<<(12-8-1))-1;
	int fxY = (_y<<(12-8)) + int(0.5f/255.f*4095.f);
	int fxU = ((_u-127)<<(12-8)) + int(0.5f/255.f*4095.f);
	int fxV = ((_v-127)<<(12-8)) + int(0.5f/255.f*4095.f);
	int fxR = fxY + ((cvr*fxV)>>12);
	int fxG = fxY - (((cug*fxU) + (cvg*fxV))>>12);
	int fxB = fxY + ((cub*fxU)>>12);
	_r = fxR>>(12-8);
	_g = fxG>>(12-8);
	_b = fxB>>(12-8);
	//R = Y + 1.403V'
	//G = Y - 0.344U' - 0.714V'
	//B = Y + 1.770U'
#endif
#if 0
	float fy = (_y+0.5f);
	float fu = ((_u-127)+0.5f);
	float fv = ((_v-127)+0.5f);
	float cvr = 1.403f*fv;
	float cuvg = 0.344f*fu + 0.714f*fv;
	float cub = 1.770*fu;
	_r = int(fy + cvr);
	_g = int(fy - cuvg);
	_b = int(fy + cub);
#endif
}

int main()
{
	int Ymax = -10000; int Ymin = 10000;
	int Umax = -10000; int Umin = 10000;
	int Vmax = -10000; int Vmin = 10000;
	int Rmax = -10000; int Rmin = 10000;
	int Gmax = -10000; int Gmin = 10000;
	int Bmax = -10000; int Bmin = 10000;

	bool bSuccess = true;
	int iSuccess[256] = {0};
	int iError[256*2][3] = {0};
	int iCount = 0;
	for(int r=0; r<=UCHAR_MAX; ++r)
	{
		for(int g=0; g<=UCHAR_MAX; ++g)
		{
			for(int b=0; b<=UCHAR_MAX; ++b)
			{
				int y,u,v;
				RGB2YCrCb(y, u, v,  r, g, b);

				Ymax = std::max(Ymax, y);
				Ymin = std::min(Ymin, y);
				Umax = std::max(Umax, u);
				Umin = std::min(Umin, u);
				Vmax = std::max(Vmax, v);
				Vmin = std::min(Vmin, v);

				int r_out,g_out,b_out;
				YCrCb2RGB(r_out,g_out,b_out,UClamp(y),UClamp(u),UClamp(v));

				Rmax = std::max(Rmax, r_out);
				Rmin = std::min(Rmin, r_out);
				Gmax = std::max(Gmax, g_out);
				Gmin = std::min(Gmin, g_out);
				Bmax = std::max(Bmax, b_out);
				Bmin = std::min(Bmin, b_out);

				int iErrVal = 0;
				iErrVal = r_out-UClamp(r)+256;
				iError[iErrVal][0]++;
				iErrVal = g_out-UClamp(g)+256;
				iError[iErrVal][1]++;
				iErrVal = b_out-UClamp(b)+256;
				iError[iErrVal][2]++;
/*
				bool bResult = (r_out==r) && (g_out==g) && (b_out==b);

				if(bResult)
				{
					iSuccess[j]++;
				}
				else
				{
					int z=0;
				}
*/
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
				//bSuccess &= bResult;
				iCount++;
			}
		}
	}

	float fAccError[3] = {0.f};
	for(int j=(256-32); j<(256+32); j++)
	{
		//printf("[%03d] -> %03d\n", j, iSuccess[j]);
		int iErrVal = j-256;
		fAccError[0] += float(iError[j][0])*abs(iErrVal);
		fAccError[1] += float(iError[j][1])*abs(iErrVal);
		fAccError[2] += float(iError[j][2])*abs(iErrVal);
		printf("[%03d] -> %03d, %03d, %03d\n", iErrVal, iError[j][0], iError[j][1], iError[j][2]);
	}
	//printf("Result:%s\n", bSuccess?"Success!":"Fail!");

	printf("Y[%3d; %3d] U[%3d; %3d] V[%3d; %3d]\n",
		Ymin, Ymax, Umin, Umax, Vmin, Vmax);
	printf("R[%3d; %3d] G[%3d; %3d] B[%3d; %3d]\n",
		Rmin, Rmax, Gmin, Gmax, Bmin, Bmax);

	printf("AvgErr: %f, %f, %f\n", fAccError[0]/iCount, fAccError[1]/iCount, fAccError[2]/iCount);
	return 0;
}