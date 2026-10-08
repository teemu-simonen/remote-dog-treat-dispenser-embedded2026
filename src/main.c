#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(dog_dispenser, LOG_LEVEL_INF);

/* -----------------------------
 * Application state
 * ----------------------------- */

static bool hatch_open = false;

/* -----------------------------
 * Servo functions
 * ----------------------------- */

static void servo_open(void)
{
    LOG_INF("Servo: opening hatch");

    /*
     * TODO:
     * Send PWM signal to servo.
     */
}

static void servo_close(void)
{
    LOG_INF("Servo: closing hatch");

    /*
     * TODO:
     * Send PWM signal to servo.
     */
}

/* -----------------------------
 * Hatch functions
 * ----------------------------- */

static void hatch_open(void)
{
    servo_open();
    hatch_open = true;

    LOG_INF("Hatch OPEN");
}

static void hatch_close(void)
{
    servo_close();
    hatch_open = false;

    LOG_INF("Hatch CLOSED");
}

static void hatch_toggle(void)
{
    if (hatch_open) {
        hatch_close();
    } else {
        hatch_open();
    }
}

/* -----------------------------
 * LED
 * ----------------------------- */

static void led_init(void)
{
    /*
     * TODO:
     * Configure status LED GPIO.
     */
}

static void led_set_connected(bool connected)
{
    /*
     * TODO:
     * Connected:
     *     LED ON
     *
     * Disconnected:
     *     LED blinking
     */
}

/* -----------------------------
 * Bluetooth
 * ----------------------------- */

static int bluetooth_init(void)
{
    /*
     * TODO:
     *
     * 1. Initialize Bluetooth.
     * 2. Create GATT service.
     * 3. Create hatch-control characteristic.
     * 4. Start advertising.
     */

    LOG_INF("Bluetooth initialized");

    return 0;
}

/* -----------------------------
 * Main
 * ----------------------------- */

int main(void)
{
    LOG_INF("Dog Treat Dispenser");

    led_init();

    bluetooth_init();

    LOG_INF("System ready");

    while (1) {

        /*
         * Most of the actual work will be
         * event-driven.
         */

        k_sleep(K_MSEC(100));
    }

    return 0;
}