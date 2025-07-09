#ifndef MY_IMAGE_H
#define MY_IMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Image metadata + pixel data container
typedef struct {
    uint16_t width;         // in pixels
    uint16_t height;        // in pixels
    const uint8_t *data;    // pointer to packed 1bpp pixel data
    uint8_t bytes_per_row;
} Image;

extern const Image my_image;
extern const uint8_t my_image_data[];

extern const Image mab_labs;
extern const uint8_t mab_labs_data[];


#ifdef __cplusplus
}
#endif

#endif // MY_IMAGE_H
