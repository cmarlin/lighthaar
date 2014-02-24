/**
*  dwt53.c - Fast discrete biorthogonal CDF 5/3 wavelet forward and inverse transform (lifting implementation)
*  
*  This code is provided "as is" and is given for educational purposes.
*  2007 - Gregoire Pau - gregoire.pau@ebi.ac.uk
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>
#include <algorithm>
#include "lightHaar.h"

//double *tempbank=0;

/**
*  fwt53 - Forward biorthogonal 5/3 wavelet transform (lifting implementation)
*
*  x is an input signal, which will be replaced by its output transform.
*  n is the length of the signal, and must be a power of 2.
*
*  The first half part of the output signal contains the approximation coefficients.
*  The second half part contains the detail coefficients (aka. the wavelets coefficients).
*
*  See also iwt53.
*/
void fwt53(int16_t* t, const int16_t* s,int iWidth)
{
#if 0
	assert(t!=s);

	//int a, b;
	int iDeltaLast = 0;

	int sIndex=0, tIndex=0;
	int hOffset = ((iWidth+1) >> 1);
	int i=0;
	for (i=0; i < (iWidth-2); i+=2)
	{
		int a = s[sIndex];
		int b = s[sIndex + 1];
		int c = s[sIndex + 2];
		int y = b - (a + c)/2;
		int x = a + (iDeltaLast+y)/4;
		iDeltaLast = y;
		t[tIndex] = x;//SClamp(x);
		t[tIndex + hOffset] = y;//SClamp(y);
		sIndex += 2;
		tIndex ++;
	}
	if(i==(iWidth-2))
	{
		// compute diff with previous avg				
		//int a = t[tIndex-1]; // read last avg value
		//int y = SClamp(s[sIndex] - a, SCHAR_MIN, SCHAR_MAX);
		//t[tIndex + hOffset] = y;

		//t[tIndex] = s[sIndex];

		int a = s[sIndex];
		int b = s[sIndex + 1];
		int c = a; //s[sIndex + 2];
		int y = b - (a + c)/2;
		int x = a + (iDeltaLast+y)/4;
		iDeltaLast = y;
		t[tIndex] = x;//SClamp(x);
		t[tIndex + hOffset] = y;//SClamp(y);
		//sIndex += 2;
		//tIndex ++;
	}
	else
	{
		//t[tIndex] = s[sIndex];
		int a = s[sIndex];
		int y = 0;
		int x = a + (iDeltaLast+y)/4;
		t[tIndex] = x;//SClamp(x);
	}
#endif
#if 1
	assert(t!=s);
	float a;

	// Predict 1
	a=-0.5f;
	for (int i=1; i<iWidth-2; i+=2)
	{
		t[i]=s[i]+a*(s[i-1]+s[i+1]);
	} 
	t[iWidth-1]+=2*a*s[iWidth-2];

	// Update 1
	a=0.25f;
	for (int i=2; i<iWidth; i+=2)
	{
		t[i]=s[i]+a*(s[i-1]+s[i+1]);
	}
	t[0]+=2*a*t[1];
#endif
#if 0
	float a;
	int i;

	int n = iWidth;
	float* x = (float*)malloc(sizeof(float) * n);
	for(i=0; i<n; i++)x[i]=s[i];

	// Predict 1
	a=-0.5f;
	for (i=1;i<n-2;i+=2)
	{
		x[i]+=a*(x[i-1]+x[i+1]);
	} 
	x[n-1]+=2*a*x[n-2];

	// Update 1
	a=0.25f;
	for (i=2;i<n;i+=2)
	{
		x[i]+=a*(x[i-1]+x[i+1]);
	}
	x[0]+=2*a*x[1];

	// Scale
	a=sqrtf(2.0f);
	for (i=0;i<n;i++) {
		if (i%2) x[i]*=a;
		else x[i]/=a;
	}

	// Pack
	//if (tempbank==0) tempbank=(double *)malloc(n*sizeof(double));
	for (i=0;i<n;i++) {
		if (i%2==0)
			t[i/2]=(x[i]+255)/2;
		else
			t[n/2+i/2]=(x[i]+255)/2;
	}
	free(x);
	//for (i=0;i<n;i++) x[i]=tempbank[i];
#endif
}

/**
*  iwt53 - Inverse biorthogonal 5/3 wavelet transform
*
*  This is the inverse of fwt53 so that iwt53(fwt53(x,n),n)=x for every signal x of length n.
*
*  See also fwt53.
*/
void iwt53(int16_t* t, const int16_t* s, int iWidth)
{
#if 1
	assert(t!=s);

	int hOffset = ((iWidth+1) >> 1);

	int sIndex=0, tIndex=0;
	int iDeltaLast = 0;
	int i=0;
	for (i = 0; i < (iWidth-3); i+=2)
	{
		int x = s[sIndex];
		int y = s[sIndex + hOffset];

		int x2 = s[sIndex + 1];
		int y2 = s[sIndex + hOffset + 1];

		int a = x - (iDeltaLast+y)/4;
		int a2 = x2 - ((y+y2)/4);
		int b = y + ((a+a2)/2);
		iDeltaLast = y;
		t[tIndex] = a;//SClamp(a);
		t[tIndex + 1] = b;//SClamp(b);
		sIndex ++;
		tIndex += 2;
	}
	int x = s[sIndex];
	int y = s[sIndex + hOffset];
	int x2 = s[sIndex + 1];
	//int y2 = (i+3<iWidth) ? s[sIndex + hOffset + 1] : 0;
	int y2 = 0;
	int a = x - (iDeltaLast+y)/4;
	int a2 = x2 - ((y+y2)/4);
	if (i+2>=iWidth)
		a2 = a;
	int b = y + (a+a2)/2 ;
	iDeltaLast = y;
	t[tIndex] = a; //SClamp(a);
	t[tIndex + 1] = b; //SClamp(b);
	sIndex ++;
	tIndex += 2;
	if((i+2)<iWidth)
	{
		//t[tIndex] = s[sIndex];
		t[tIndex] = a2;
		//int x = s[sIndex];
		//int y = 0;
		//int a = x - (iDeltaLast+y)/4;
		//t[tIndex] = SClamp(a);
	}
#else
	float a;
	int i;

	// Unpack
	int n = iWidth;
	float* x=(float *)malloc(n*sizeof(float));
	for (i=0;i<n/2;i++)
	{
		x[i*2]=s[i]*2-255;
		x[i*2+1]=s[i+n/2]*2-255;
	}
	//for (i=0;i<n;i++) x[i]=tempbank[i];

	// Undo scale
	a=1/sqrtf(2.0f);
	for (i=0;i<n;i++)
	{
		if (i%2)
			x[i]*=a;
		else
			x[i]/=a;
	}

	// Undo update 1
	a=-0.25f;
	for (i=2;i<n;i+=2)
	{
		x[i]+=a*(x[i-1]+x[i+1]);
	}
	x[0]+=2*a*x[1];

	// Undo predict 1
	a=0.5f;
	for (i=1;i<n-2;i+=2)
	{
		t[i]=x[i]+a*(x[i-1]+x[i+1]);
	}
	t[n-1]=x[n-1]+2*a*x[n-2];
	free(x);
#endif
}

const int iSampleCount = 4096;
int main()
{
	int16_t x[iSampleCount], coef[iSampleCount], tmp[iSampleCount], y[iSampleCount];
	int i;

	// Makes a fancy cubic signal
	//for (i=0;i<32;i++) x[i]=i; //5+i+0.4*i*i-0.02*i*i*i;
	// random noise with gaussian filter
	srand(0);
	for (i=0;i<iSampleCount;i++) x[i] = int8_t(rand());
	//x[iSampleCount-2] = 127;
	//x[iSampleCount-1] = 127;
	//for (i=0;i<(iSampleCount-1);i++) x[i] = (x[i] + x[i+1])/2;

	//for(i=0; i<(iSampleCount*1/3); i++) x[i] = 255;
	//for(i=0; i<(iSampleCount*3/3); i++) x[i] = 0;
	//for(; i<(iSampleCount*3/3); i++) x[i] = 255;
	//x[0] = 255;

	// Prints original sigal x
	printf("Original signal:\n");
	//for (i=0;i<iSampleCount;i++) printf("x[%d]=%d\n",i,x[i]);
	printf("\n");

	// Do the forward 5/3 transform
	fwt53(tmp, x, iSampleCount);
	//int iPassSample = iSampleCount;
	//int iLoopCount = 0;
	//memcpy(coef, x, iPassSample);
	//while(iPassSample>=4)
	//{
	//	fwt53(tmp, coef, iPassSample);
	//	memcpy(coef, tmp, iSampleCount);
	//	iPassSample>>=1;
	//	iLoopCount++;
	//}

	// Prints the wavelet coefficients
	printf("Wavelets coefficients:\n");
	//for (i=0;i<iSampleCount;i++) printf("wc[%d]=%d\n",i,coef[i]);
	printf("\n");

	// Do the inverse 5/3 transform
	iwt53(y,tmp,iSampleCount);
	/*memcpy(tmp, coef, iSampleCount);
	while(iLoopCount)
	{
		iPassSample = iSampleCount;
		for(int i=0; i<iLoopCount; i++)
		{
			iPassSample >>= 1;
		}
		iwt53(y, tmp, iPassSample);
		memcpy(tmp, y, iSampleCount);
		iLoopCount --;
	}*/

	// Prints the reconstructed signal
	int iError = 0;
	int iBias = 0;
	int iBiasLowFactors = 0;
	int iBiasHighFactors = 0;
	int iErrorMax = 0;
	int iErrorMaxIndex = 0;
	printf("Reconstructed signal:\n");
	for (i=0;i<iSampleCount;i++)
	{
		int iAvg = tmp[i/2];
		int iDelta = tmp[i/2+(iSampleCount/2)];
		iError += std::abs(y[i]-x[i]);
		iBias += (y[i] - x[i]);
		if(std::abs(y[i]-x[i]) > 10)
		{
			if(i%2)
				printf("xx[%d]= (%3d => %3d => %3d)\n",i,x[i], iDelta, y[i]);
			else
				printf("xx[%d]= (%3d => [%3d] => %3d)\n",i,x[i], iAvg, y[i]);
		}
		if(i%2)
		{
			iBiasHighFactors += (y[i] - x[i]);
		}
		else
		{
			iBiasLowFactors += (y[i] - x[i]);
		}
		if(std::abs(y[i]-x[i]) > iErrorMax)
		{
			iErrorMax = std::abs(y[i]-x[i]);
			iErrorMaxIndex = i;
		}
	}

	printf("Error: %f (Max:%d[%d : %d=>%d])\n", float(iError)/iSampleCount, iErrorMax, iErrorMaxIndex, x[iErrorMaxIndex], y[iErrorMaxIndex]);
	printf("Bias: %f (Low:%f / High:%f)\n"
		,float(iBias)/iSampleCount
		,float(iBiasLowFactors)/(iSampleCount/2)
		,float(iBiasHighFactors)/(iSampleCount/2));
	//calculer l'err max!
}

