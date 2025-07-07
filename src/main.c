#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>

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
    float k = 10.0f; // spiral tightness factor
    float band_width = 6.0f; // width of spiral arm

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int dx = x - cx;
            int dy = y - cy;
            float r = sqrtf(dx * dx + dy * dy);
            float angle = atan2f(dy, dx);

            float spiral_r = k * angle;
            float dist = r - spiral_r;

            if (fmodf(dist + band_width, band_width * 2) < band_width) {
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

    show_patterns_loop(display_dev, &desc); // Ultimate switch for controlling method display

    // === To see individual patterns, comment out top method and uncomment one of these methods bel ===
    // pattern_checkerboard();
    // pattern_big_checkerboard();
    // pattern_bigger_checkerboard();
    // pattern_gigantic_checkerboard();
    // pattern_grid();
    // pattern_concentric_squares();
    // pattern_polka_dots();
    // pattern_radial_stripes();
    // pattern_spiral();
    // pattern_concentric_circles();

    // === Afterwards, uncomment these two methods ===
    // pack_buffer();
    // display_write(display_dev, 0, 0, desc, buf);

    k_msleep(1000);

    while (1) {
        k_sleep(K_SECONDS(30));
    }

    return 0;
}
