#include "stdint.h"

void EncodeCDF53(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality);
void DecodeCDF53(int8_t* _dst, const int8_t *_src, int _width, int _height, int _quality);