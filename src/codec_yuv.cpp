#include "codec_yuv.h"
#include "utils.h"

void ConvertRGB24toYCrCb(int8_t* _pY, int8_t* _pU, int8_t* _pV, const uint8_t* _pRGB, int _width, int _height)
{
	// 219 ???
	const int cyb = int(0.114f*4096.f);
	const int cyg = int(0.587f*4096.f);
	const int cyr = int(0.299f*4096.f);
	const int cu = int(0.565f*4096.f);
	const int cv = int(0.713f*4096.f);
	const int chalf = int(0.5f/255.f*4095.f);
	//Y = 0.299R + 0.587G + 0.114B
	//U'= (B-Y)*0.565
	//V'= (R-Y)*0.713
	for(int j=0; j<_height; j+=2)
	{
		for(int i=0; i<_width; i+=2)
		{
			const uint8_t* pRGB01 = _pRGB + ((j+0)*_width+i)*3;
			const uint8_t* pRGB23 = _pRGB + ((j+1)*_width+i)*3;
			uint8_t R0 = pRGB01[0];	uint8_t R1 = pRGB01[3+0];
			uint8_t G0 = pRGB01[1];	uint8_t G1 = pRGB01[3+1];
			uint8_t B0 = pRGB01[2];	uint8_t B1 = pRGB01[3+2];
			uint8_t R2 = pRGB23[0];	uint8_t R3 = pRGB23[3+0];
			uint8_t G2 = pRGB23[1];	uint8_t G3 = pRGB23[3+1];
			uint8_t B2 = pRGB23[2];	uint8_t B3 = pRGB23[3+2];

			int fxR, fxG, fxB, fxY, fxU, fxV;
			fxR = (R0<<(12-8)) + chalf;
			fxG = (G0<<(12-8)) + chalf;
			fxB = (B0<<(12-8)) + chalf;
			fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;
			fxU = ((fxB-fxY)*cu)>>12;
			fxV = ((fxR-fxY)*cv)>>12;
			int Y0 = fxY>>(12-8);
			int U0 = (fxU>>(12-8)) + 127;
			int V0 = (fxV>>(12-8)) + 127;

			fxR = (R1<<(12-8)) + chalf;
			fxG = (G1<<(12-8)) + chalf;
			fxB = (B1<<(12-8)) + chalf;
			fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;
			fxU = ((fxB-fxY)*cu)>>12;
			fxV = ((fxR-fxY)*cv)>>12;
			int Y1 = fxY>>(12-8);
			int U1 = (fxU>>(12-8)) + 127;
			int V1 = (fxV>>(12-8)) + 127;

			fxR = (R2<<(12-8)) + chalf;
			fxG = (G2<<(12-8)) + chalf;
			fxB = (B2<<(12-8)) + chalf;
			fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;
			fxU = ((fxB-fxY)*cu)>>12;
			fxV = ((fxR-fxY)*cv)>>12;
			int Y2 = fxY>>(12-8);
			int U2 = (fxU>>(12-8)) + 127;
			int V2 = (fxV>>(12-8)) + 127;

			fxR = (R3<<(12-8)) + chalf;
			fxG = (G3<<(12-8)) + chalf;
			fxB = (B3<<(12-8)) + chalf;
			fxY = ((cyr*fxR) + (cyg*fxG) + (cyb*fxB))>>12;
			fxU = ((fxB-fxY)*cu)>>12;
			fxV = ((fxR-fxY)*cv)>>12;
			int Y3 = fxY>>(12-8);
			int U3 = (fxU>>(12-8)) + 127;
			int V3 = (fxV>>(12-8)) + 127;

			// note: shift added here (-128) to have s8 range data instead of u8 range
			// try 2/3 scale
			_pY[(j+0)*_width+i+0] = UClamp(Y0*2/3)-128;	_pY[(j+0)*_width+i+1] = UClamp(Y1*2/3)-128;
			_pY[(j+1)*_width+i+0] = UClamp(Y2*2/3)-128;	_pY[(j+1)*_width+i+1] = UClamp(Y3*2/3)-128;
			_pU[(j/2)*(_width/2)+(i/2)] = uint8_t((UClamp(U0*2/3)+UClamp(U1*2/3)+UClamp(U2*2/3)+UClamp(U3*2/3))>>2)-128;
			_pV[(j/2)*(_width/2)+(i/2)] = uint8_t((UClamp(V0*2/3)+UClamp(V1*2/3)+UClamp(V2*2/3)+UClamp(V3*2/3))>>2)-128;
			//_pY[(j+0)*_width+i+0] = UClamp(Y0)-128;	_pY[(j+0)*_width+i+1] = UClamp(Y1)-128;
			//_pY[(j+1)*_width+i+0] = UClamp(Y2)-128;	_pY[(j+1)*_width+i+1] = UClamp(Y3)-128;
			//_pU[(j/2)*(_width/2)+(i/2)] = uint8_t((UClamp(U0)+UClamp(U1)+UClamp(U2)+UClamp(U3))>>2)-128;
			//_pV[(j/2)*(_width/2)+(i/2)] = uint8_t((UClamp(V0)+UClamp(V1)+UClamp(V2)+UClamp(V3))>>2)-128;
		}
	}
}

// todo: rewrite to use 4096 fixed flot + shift
void ConvertYCrCbtoRGB24(uint8_t* _pRGB, const int8_t* _pY, const int8_t* _pU, const int8_t* _pV, int _width, int _height)
{
	// TODO: use 255<<4 (4080)instead of 4096... or 4095? 
	const int cvr = int(1.403f*4096.f);
	const int cug = int(0.344f*4096.f);
	const int cvg = int(0.714f*4096.f);
	const int cub = int(1.770f*4096.f);
	const int chalf = int(0.5f/255.f*4095.f);
	//R = Y + 1.403V'
	//G = Y - 0.344U' - 0.714V'
	//B = Y + 1.770U'
	for(int j=0; j<_height; j+=2)
	{
		for(int i=0; i<_width; i+=2)
		{
			// note: shift added here (+128) to have s8 range data input to u8 range for conversion
			// try 2/3 scale
			uint8_t Y0 = UClamp((_pY[(j+0)*_width+i+0]+128)*3/2);	uint8_t Y1 = UClamp((_pY[(j+0)*_width+i+1]+128)*3/2);
			uint8_t Y2 = UClamp((_pY[(j+1)*_width+i+0]+128)*3/2);	uint8_t Y3 = UClamp((_pY[(j+1)*_width+i+1]+128)*3/2);
			uint8_t U = UClamp((_pU[(j/2)*(_width/2)+(i/2)]+128)*3/2);
			uint8_t V = UClamp((_pV[(j/2)*(_width/2)+(i/2)]+128)*3/2);
			//uint8_t Y0 = _pY[(j+0)*_width+i+0]+128;	uint8_t Y1 = _pY[(j+0)*_width+i+1]+128;
			//uint8_t Y2 = _pY[(j+1)*_width+i+0]+128;	uint8_t Y3 = _pY[(j+1)*_width+i+1]+128;
			//uint8_t U = _pU[(j/2)*(_width/2)+(i/2)]+128;
			//uint8_t V = _pV[(j/2)*(_width/2)+(i/2)]+128;

			int fxY, fxU, fxV, fxR, fxG, fxB, fxVR, fxUB, fxUVG;

			fxU = ((U-127)<<(12-8)) + chalf;
			fxV = ((V-127)<<(12-8)) + chalf;
			fxVR = ((cvr*fxV)>>12);
			fxUVG = ((cug*fxU) + (cvg*fxV))>>12;
			fxUB = ((cub*fxU)>>12);

			fxY = (Y0<<(12-8)) + chalf;
			fxR = fxY + fxVR;
			fxG = fxY - fxUVG;
			fxB = fxY + fxUB;
			int R0 = fxR>>(12-8);
			int G0 = fxG>>(12-8);
			int B0 = fxB>>(12-8);

			fxY = (Y1<<(12-8)) + chalf;
			fxR = fxY + fxVR;
			fxG = fxY - fxUVG;
			fxB = fxY + fxUB;
			int R1 = fxR>>(12-8);
			int G1 = fxG>>(12-8);
			int B1 = fxB>>(12-8);

			fxY = (Y2<<(12-8)) + chalf;
			fxR = fxY + fxVR;
			fxG = fxY - fxUVG;
			fxB = fxY + fxUB;
			int R2 = fxR>>(12-8);
			int G2 = fxG>>(12-8);
			int B2 = fxB>>(12-8);

			fxY = (Y3<<(12-8)) + chalf;
			fxR = fxY + fxVR;
			fxG = fxY - fxUVG;
			fxB = fxY + fxUB;
			int R3 = fxR>>(12-8);
			int G3 = fxG>>(12-8);
			int B3 = fxB>>(12-8);

			uint8_t* pRGB01 = _pRGB + ((j+0)*_width+i)*3;
			uint8_t* pRGB23 = _pRGB + ((j+1)*_width+i)*3;
			pRGB01[0] = UClamp(R0);	pRGB01[3+0] = UClamp(R1);
			pRGB01[1] = UClamp(G0);	pRGB01[3+1] = UClamp(G1);
			pRGB01[2] = UClamp(B0);	pRGB01[3+2] = UClamp(B1);
			pRGB23[0] = UClamp(R2);	pRGB23[3+0] = UClamp(R3);
			pRGB23[1] = UClamp(G2);	pRGB23[3+1] = UClamp(G3);
			pRGB23[2] = UClamp(B2);	pRGB23[3+2] = UClamp(B3);
		}
	}
}

