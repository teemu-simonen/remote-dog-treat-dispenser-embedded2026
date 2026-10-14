
#include "status_led.h"

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(status_led, LOG_LEVEL_INF);

/*
 * Use the first LED defined by the board's devicetree.
 * For the nRF52833 DK, this is normally LED1 on the board.
 */
static const struct gpio_dt_spec status_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Timing values in milliseconds. */
#define BLINK_INTERVAL_MS 500
#define COMMAND_FLASH_MS  150

/* Current normal operating state. */
static status_led_state_t current_state = STATUS_LED_BOOT;

/* True while the temporary command flash is active. */
static bool flash_active;

/* Current physical output level: 0 = off, 1 = on. */
static bool led_on;

/* Work items allow timed LED changes without busy-waiting. */
static struct k_work_delayable blink_work;
static struct k_work_delayable flash_work;

/*
 * Apply a physical LED output level.
 */
static void led_write(bool on)
{
    int ret = gpio_pin_set_dt(&status_led, on ? 1 : 0);

    if (ret < 0) {
        LOG_ERR("Failed to set status LED: %d", ret);
        return;
    }

    led_on = on;
}

/*
 * Return the LED to the normal operating state.
 */
static void apply_normal_state(void)
{
    if (flash_active) {
        return;
    }

    switch (current_state) {
    case STATUS_LED_BOOT:
        (void)k_work_cancel_delayable(&blink_work);
        led_write(false);
        break;

    case STATUS_LED_ADVERTISING:
        /*
         * Begin the advertising pattern.
         * The work handler will toggle the LED.
         */
        led_write(true);
        (void)k_work_reschedule(
            &blink_work,
            K_MSEC(BLINK_INTERVAL_MS));
        break;

    case STATUS_LED_CONNECTED:
        (void)k_work_cancel_delayable(&blink_work);
        led_write(true);
        break;

    default:
        (void)k_work_cancel_delayable(&blink_work);
        led_write(false);
        break;
    }
}

/*
 * Blink handler.
 */
static void blink_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (flash_active ||
        current_state != STATUS_LED_ADVERTISING) {
        return;
    }

    led_write(!led_on);

    (void)k_work_reschedule(
        &blink_work,
        K_MSEC(BLINK_INTERVAL_MS));
}

/*
 * End the temporary command flash.
 */
static void flash_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    flash_active = false;
    apply_normal_state();
}

/*
 * Initialize GPIO and delayed work.
 */
int status_led_init(void)
{
    if (!gpio_is_ready_dt(&status_led)) {
        LOG_ERR("Status LED GPIO device is not ready");
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(
        &status_led,
        GPIO_OUTPUT_INACTIVE);

    if (ret < 0) {
        LOG_ERR("Failed to configure status LED: %d", ret);
        return ret;
    }

    k_work_init_delayable(
        &blink_work,
        blink_work_handler);

    k_work_init_delayable(
        &flash_work,
        flash_work_handler);

    current_state = STATUS_LED_BOOT;
    flash_active = false;
    led_on = false;

    led_write(false);

    LOG_INF("Status LED initialized");

    return 0;
}

/*
 * Change the normal operating state.
 */
void status_led_set_state(status_led_state_t state)
{
    current_state = state;

    /*
     * A new connection state should cancel any
     * temporary command flash.
     */
    flash_active = false;
    (void)k_work_cancel_delayable(&flash_work);

    apply_normal_state();
}

/*
 * Briefly turn the LED on to acknowledge a command.
 */
void status_led_command_flash(void)
{
    flash_active = true;

    /*
     * Pause the advertising blink while the flash
     * is active. The normal state is preserved.
     */
    (void)k_work_cancel_delayable(&blink_work);

    led_write(true);

    (void)k_work_reschedule(
        &flash_work,
        K_MSEC(COMMAND_FLASH_MS));
}
