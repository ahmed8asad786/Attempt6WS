#ifndef MY_IMAGE_H
#define MY_IMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MY_IMAGE_WIDTH 120
#define MY_IMAGE_HEIGHT 120

// 1-bit-per-pixel packed bitmap image
extern const uint8_t my_image_data[];

#ifdef __cplusplus
}
#endif

#endif // MY_IMAGE_H
