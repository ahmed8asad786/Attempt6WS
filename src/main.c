#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/printk.h>
#include <lvgl.h>
#include <zephyr/drivers/gpio.h>  // Add this!

int main(void)
{
    const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    printk("Display device name: %s\n", display_dev->name);

    const struct device *reset_dev = device_get_binding("GPIO_1");
    if (reset_dev) {
        gpio_pin_configure(reset_dev, 3, GPIO_OUTPUT);
        gpio_pin_set(reset_dev, 3, 0);
        k_sleep(K_MSEC(10));
        gpio_pin_set(reset_dev, 3, 1);
        k_sleep(K_MSEC(10));
    }

    printk("Device is ready? %d\n", device_is_ready(display_dev));
    if (!device_is_ready(display_dev)) {
        printk("Display device not ready\n");
        return -1;
    }
    else {
        printk("DEVICE IS READY!!!\n");
    }

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello World!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    display_blanking_off(display_dev);
    lv_task_handler();

    k_sleep(K_SECONDS(2));

    while (1) {
        lv_task_handler();
        k_sleep(K_SECONDS(30));
    }

    return 0;  // Never reached
}