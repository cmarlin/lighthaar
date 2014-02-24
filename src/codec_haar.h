#include "stdint.h"

/**
	@brief __src and _dst could overlap
	*/
void EncodeHaarLossLess(int8_t* _dst, const int8_t *_src, int _width, int _height, int /*_quality*/);
void DecodeHaarLossLess(int8_t* _dst, const int8_t *_src, int _width, int _height, int /*_quality*/);

void EncodeHaar(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality);
void DecodeHaar(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality);