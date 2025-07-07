#ifndef FONTS8X12_MONTSERRAT_H
#define FONTS8X12_MONTSERRAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 8x12 Montserrat font array: printable ASCII 0x30 ('0') to 0x5A ('Z')
// Each char is 12 bytes (rows), 8 pixels wide
// Characters included: '0'..'9', 'A'..'Z' (total 10 + 26 = 36 glyphs)
extern const uint8_t font_8x12_montserrat[95][12];

#ifdef __cplusplus
}
#endif

#endif // FONTS8X12_MONTSERRAT_H