#ifndef FONTS5X7_H
#define FONTS5X7_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 5x7 font array: printable ASCII 0x20 (space) to 0x7F (~)
// Each char is 5 bytes (columns)
extern const uint8_t font5x7[95][5];

#ifdef __cplusplus
}
#endif

#endif // FONTS5X7_H
