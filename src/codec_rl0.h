//
// RL0 (run length 0) encoder/decoder
//

#include <stdint.h>

int RL0count(const uint8_t * const buf, int pos, int len);
void toRL0(uint8_t* rl0, int* rl0size, const uint8_t* const buf, const int size);
void fromRL0(uint8_t *out, const uint8_t * const rl0, const int rl0size);

