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
#include "my_image.h"

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

static void pattern_background() {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            set_pixel(x, y);
        }
    }
}

void draw_my_image(int x_offset, int y_offset) {
    int width = 120;
    int height = 120;
    int bytes_per_row = 15;  // (100 + 7) / 8

    for (int y = 0; y < height; y++) {
        for (int byte_index = 0; byte_index < bytes_per_row; byte_index++) {
            uint8_t byte = my_image_data[y * bytes_per_row + byte_index];
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



int main(void)
{
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

    display_blanking_off(display_dev);
    memset(raw_buf, 1, sizeof(raw_buf));  // White pixels

    // buffer that will be programmed through pattern methods and passed to be written to ePaper
    struct display_buffer_descriptor desc = {
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .pitch = DISPLAY_PITCH,
        .buf_size = DISPLAY_BUF_SIZE,
    };

    // draw_string_5x7(10, 10, " !\"#$%&'()*+,-./012");
    // draw_string_5x7(10, 30, "3456789:;<=>?@ABCDE");
    // draw_string_5x7(10, 50, "FGHIJKLMNOPQRSTUVWX");
    // draw_string_5x7(10, 70, "YZ[\\]^_`abcdefghijk");
    // draw_string_5x7(10, 90, "lmnopqrstuvwxyz{|}~");

    // draw_string_5x7(10, 10, "Hello world!");
    // pattern_background();
    draw_string_5x7(10, 10, "Interns");
    draw_string_5x7(10, 30, "at Mab-labs");
    draw_string_5x7(10, 50, "present ...");

    draw_my_image(100, (DISPLAY_HEIGHT / 2) - 60);

    pack_buffer();
    display_write(display_dev, 0, 0, &desc, buf);

    k_msleep(1000);

    while (1) {
        k_sleep(K_SECONDS(30));
    }

    return 0;
}
