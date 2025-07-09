#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>
#include "fonts5x7.h"
#include "fonts_8x12_montserrat.h"
#include "images.h"

#define DISPLAY_WIDTH      256
#define DISPLAY_HEIGHT     120
#define DISPLAY_PITCH      DISPLAY_WIDTH
#define DISPLAY_BYTE_PITCH ((DISPLAY_WIDTH + 7) / 8)
#define DISPLAY_BUF_SIZE   (DISPLAY_BYTE_PITCH * DISPLAY_HEIGHT)

LOG_MODULE_REGISTER(main);

#define PIXEL_ON 0
#define PIXEL_OFF 1
#define REFRESH_RATE 5000 // milliseconds

static uint8_t raw_buf[DISPLAY_WIDTH * DISPLAY_HEIGHT];  // 1 byte per pixel
static uint8_t buf[DISPLAY_BUF_SIZE];

#pragma region pixel buffer configuration methods

// Set pixel (x,y) to black (0) in the buffer
static void set_pixel(int x, int y) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }
    raw_buf[y * DISPLAY_WIDTH + x] = PIXEL_ON;
}

static void color_off(int x, int y) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }
    raw_buf[y * DISPLAY_WIDTH + x] = PIXEL_OFF;
}

// Fills buffer
static void pack_buffer(void) {
    memset(buf, 0xFF, sizeof(buf));  // Set all to white
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            if (raw_buf[y * DISPLAY_WIDTH + x] == PIXEL_ON) {
                int byte_index = x + (y / 8) * DISPLAY_WIDTH;
                int bit_pos = 7 - (y % 8);
                buf[byte_index] &= ~(1 << bit_pos);
                // Uncomment to debug:
                // LOG_INF("PACK: (x=%d y=%d) -> Byte %d, bit %d", x, y, byte_index, bit_pos);
            }
        }
    }
}

#pragma endregion

#pragma region 5x7_mono text
static void draw_char_5x7(int x, int y, char c) {
    const uint8_t *bitmap = NULL;

    if (c < 0x20 || c > 0x7E) {
        c = ' ';
    }

    const uint8_t *glyph = font5x7[c - 0x20];

    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                set_pixel(x + col, y + row);
            }
        }
    }
}

static void draw_string_5x7(int x, int y, const char *str) {
    while (*str) {
        draw_char_5x7(x, y, *str);
        x += 6;  // 5 columns + 1 space
        str++;
    }
}

// void draw_string_5x7_test() {
//     draw_string_5x7(10, 10, " !\"#$%&'()*+,-./012");
//     draw_string_5x7(10, 30, "3456789:;<=>?@ABCDE");
//     draw_string_5x7(10, 50, "FGHIJKLMNOPQRSTUVWX");
//     draw_string_5x7(10, 70, "YZ[\\]^_`abcdefghijk");
//     draw_string_5x7(10, 90, "lmnopqrstuvwxyz{|}~");
// }

#pragma endregion

#pragma region 8x12_montserrat text

static void draw_char_8x12_montserrat(int x, int y, char c) {
    if (c < 0x20 || c > 0x7E) {
        return; // unsupported character
    }

    const uint8_t *glyph = font_8x12_montserrat[c - 0x20];

    for (int row = 0; row < 12; row++) {
        uint8_t row_bits = glyph[row];  // 8 pixels horizontally

        for (int col = 0; col < 8; col++) {
            if (row_bits & (1 << (7 - col))) {
                set_pixel(x + col, y + row); // (x, y) is top-left of char
            }
        }
    }
}

static void draw_string_8x12_montserrat(int x, int y, const char *str) {
    while (*str) {
        draw_char_8x12_montserrat(x, y, *str);
        x += 9;
        str++;
    }
}

// void draw_string_8x12_test() {
//     draw_string_8x12(10, 10, " !\"#$%&'()*+,-./012");
//     draw_string_8x12(10, 30, "3456789:;<=>?@ABCDE");
//     draw_string_8x12(10, 50, "FGHIJKLMNOPQRSTUVWX");
//     draw_string_8x12(10, 70, "YZ[\\]^_`abcdefghijk");
//     draw_string_8x12(10, 90, "lmnopqrstuvwxyz{|}~");
// }

#pragma endregion

#pragma region image

void draw_my_image(const Image *img, int x_offset, int y_offset) {
    int width = img->width;
    int height = img->height;
    int bytes_per_row = img->bytes_per_row;

    for (int y = 0; y < height; y++) {
        for (int byte_index = 0; byte_index < bytes_per_row; byte_index++) {
            uint8_t byte = img->data[y * bytes_per_row + byte_index];
            for (int bit = 0; bit < 8; bit++) {
                int x = byte_index * 8 + bit;
                if (x >= width) break;  // Ignore bits outside width

                // Check if bit is set (pixel on)
                if (byte & (0x80 >> bit)) {
                    set_pixel(x + x_offset, y + y_offset);
                }
            }
        }
    }
}

#pragma endregion

#pragma region pattern methods

void pattern_checkerboard(void) {
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            if ((x + y) % 2 == 0) set_pixel(x, y);
        }
    }
}

void pattern_big_checkerboard(void) {
    int block_size = 8;
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int block_x = x / block_size;
            int block_y = y / block_size;
            if ((block_x + block_y) % 2 == 0) set_pixel(x, y);
        }
    }
}

void pattern_bigger_checkerboard(void) {
    int block_size = 16;
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int block_x = x / block_size;
            int block_y = y / block_size;
            if ((block_x + block_y) % 2 == 0) set_pixel(x, y);
        }
    }
}

void pattern_gigantic_checkerboard(void) {
    int block_size = 32;
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int block_x = x / block_size;
            int block_y = y / block_size;
            if ((block_x + block_y) % 2 == 0) set_pixel(x, y);
        }
    }
}

void pattern_grid(void) {
    for (int y = 0; y < DISPLAY_HEIGHT; y += 8) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) set_pixel(x, y);
    }
    for (int x = 0; x < DISPLAY_WIDTH; x += 8) {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) set_pixel(x, y);
    }
}

void pattern_concentric_squares(void) {
    for (int layer = 0; layer < DISPLAY_HEIGHT / 2; layer += 8) {
        for (int x = layer; x < DISPLAY_WIDTH - layer; x++) {
            set_pixel(x, layer);
            set_pixel(x, DISPLAY_HEIGHT - 1 - layer);
        }
        for (int y = layer; y < DISPLAY_HEIGHT - layer; y++) {
            set_pixel(layer, y);
            set_pixel(DISPLAY_WIDTH - 1 - layer, y);
        }
    }
}

void pattern_polka_dots(void) {
    const int dot_spacing_x = 16, dot_spacing_y = 16, dot_radius = 2;
    for (int cy = dot_radius; cy < DISPLAY_HEIGHT; cy += dot_spacing_y) {
        for (int cx = dot_radius; cx < DISPLAY_WIDTH; cx += dot_spacing_x) {
            for (int y = -dot_radius; y <= dot_radius; y++) {
                for (int x = -dot_radius; x <= dot_radius; x++) {
                    if (x * x + y * y <= dot_radius * dot_radius) {
                        set_pixel(cx + x, cy + y);
                    }
                }
            }
        }
    }
}

void pattern_radial_stripes(void) {
    double M_PI = 3.14159265358979323846;
    int cx = DISPLAY_WIDTH / 2, cy = DISPLAY_HEIGHT / 2;
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int dx = x - cx, dy = y - cy;
            float angle = atan2f(dy, dx);
            if (((int)((angle + M_PI) / (M_PI / 8))) % 2 == 0) {
                set_pixel(x, y);
            }
        }
    }
}

void pattern_spiral(void) {
    int cx = DISPLAY_WIDTH / 2;
    int cy = DISPLAY_HEIGHT / 2;
    float k = 10.0f;           // Spiral tightness (higher = tighter spiral)
    float band_width = 6.0f;   // Width of each spiral arm
    float r_min = 4.0f;        // Minimum radius to start drawing (avoids center blob)

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int dx = x - cx;
            int dy = y - cy;
            float r = sqrtf(dx * dx + dy * dy);

            if (r < r_min) {
                continue; // Skip drawing near the center to avoid the dark spot
            }

            float angle = atan2f(dy, dx); // Range: -π to π
            float spiral_r = k * angle;

            float dist = r - spiral_r;

            if (fmodf(dist + band_width, band_width * 2.0f) < band_width) {
                set_pixel(x, y);
            }
        }
    }
}

void pattern_concentric_circles(void) {
    int cx = DISPLAY_WIDTH / 2, cy = DISPLAY_HEIGHT / 2;
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int dx = x - cx, dy = y - cy;
            int r = (int)sqrtf(dx * dx + dy * dy);
            if ((r / 5) % 2 == 0) set_pixel(x, y);
        }
    }
}

void pattern_mab_labs(void) {
    draw_string_5x7(10, 10, "Interns");
    draw_string_5x7(10, 30, "at Mab-labs");
    draw_string_5x7(10, 50, "present --->");

    // ONLY LOOKS NICE WITH THE MAB-LABS logo or any 120x120 image 
    draw_my_image(&mab_labs, (DISPLAY_WIDTH / 2) - 30, (DISPLAY_HEIGHT / 2) - 60);
}

#pragma endregion


// Method that shows every programmed pattern in an infinite loop
void show_patterns_loop(const struct device *display_dev, struct display_buffer_descriptor *desc) {
    typedef void (*pattern_func_t)(void);
    pattern_func_t patterns[] = {
        pattern_checkerboard,
        pattern_big_checkerboard,
        pattern_bigger_checkerboard,
        pattern_gigantic_checkerboard,
        pattern_grid,
        pattern_concentric_squares,
        pattern_polka_dots,
        pattern_radial_stripes,
        pattern_spiral,
        pattern_concentric_circles,
        pattern_mab_labs,
    };
    const int pattern_count = sizeof(patterns) / sizeof(patterns[0]);

    while (1) {
        for (int i = 0; i < pattern_count; i++) {
            memset(raw_buf, 1, sizeof(raw_buf));  // Clear to white
            patterns[i]();
            pack_buffer();
            display_write(display_dev, 0, 0, desc, buf);
            if(display_write(display_dev, 0, 0, &desc, buf) != 0) {
                LOG_ERR("display_write failed (%d)");
            }
            k_msleep(REFRESH_RATE);
        }
    }
}


int main(void)
{

#pragma region setup
    const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    const struct device *reset_dev   = device_get_binding("GPIO_1");
    const struct device *spi_dev     = DEVICE_DT_GET(DT_NODELABEL(spi0));
    const struct device *gpio_dev    = DEVICE_DT_GET(DT_NODELABEL(gpio1));

    if (reset_dev) {
        gpio_pin_configure(reset_dev, 3, GPIO_OUTPUT);
        gpio_pin_set(reset_dev, 3, 0);
        k_sleep(K_MSEC(10));
        gpio_pin_set(reset_dev, 3, 1);
        k_sleep(K_MSEC(10));
    }

    LOG_INF("SPI ready? %d", device_is_ready(spi_dev));
    LOG_INF("GPIO1 ready? %d", device_is_ready(gpio_dev));
    LOG_INF("Chosen display node: %s", DT_NODE_FULL_NAME(DT_CHOSEN(zephyr_display)));

    LOG_INF("Delaying for display power up");
    k_msleep(100);

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return -1;
    }

    LOG_INF("DEVICE IS READY!!!");
    LOG_INF("Display device name: %s", display_dev->name);

#pragma endregion

    display_blanking_off(display_dev);
    memset(raw_buf, 1, sizeof(raw_buf));  // White pixels

    // buffer that will be programmed through pattern methods and passed to be written to ePaper
    struct display_buffer_descriptor desc = {
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .pitch = DISPLAY_PITCH,
        .buf_size = DISPLAY_BUF_SIZE,
    };

    show_patterns_loop(display_dev, &desc);    

    // draw_my_image(&my_image, 0, 0);
    // int x_displacement = 70;
    // draw_string_5x7(x_displacement, 30, "\"Without math,");
    // draw_string_5x7(x_displacement, 38, " we're dead in");
    // draw_string_5x7(x_displacement, 46, " the water\"");
    // draw_string_5x7(x_displacement + 10, 60, "- Ken Chung");

    // pack_buffer();
    display_write(display_dev, 0, 0, &desc, buf);

    k_msleep(1000);

    while (1) {
        k_sleep(K_SECONDS(30));
    }

    return 0;
}
