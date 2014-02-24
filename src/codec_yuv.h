#include "stdint.h"

void ConvertRGB24toYCrCb(int8_t* _pY, int8_t* _pU, int8_t* _pV, const uint8_t* _pRGB, int _width, int _height);
void ConvertYCrCbtoRGB24(uint8_t* _pRGB, const int8_t* _pY, const int8_t* _pU, const int8_t* _pV, int _width, int _height);

