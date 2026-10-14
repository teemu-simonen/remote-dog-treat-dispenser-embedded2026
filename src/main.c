#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>

#include "servo.h"
#include "status_led.h"
#include "ble_service.h"


LOG_MODULE_REGISTER(dog_dispenser, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Dog Treat Dispenser");

    int ret = servo_init();

    if (ret != 0) {
        LOG_WRN("Servo not available yet: %d", ret);
    }

    int ret2 = status_led_init();

    if (ret2 < 0) {
        LOG_ERR("Status LED initialization failed: %d", ret2);
        return 0;
    }


    /* Simulate waiting for a BLE connection. */
    status_led_set_state(STATUS_LED_ADVERTISING);
    k_sleep(K_SECONDS(5));

    /* Simulate a phone connecting. */
    status_led_set_state(STATUS_LED_CONNECTED);
    k_sleep(K_SECONDS(3));

    /* Simulate a command arriving from the phone. */
    status_led_command_flash();
    k_sleep(K_SECONDS(3));

    /* Return to the connected state. */
    status_led_set_state(STATUS_LED_CONNECTED);


    ret = ble_service_init();

    if (ret < 0) {
        LOG_ERR("BLE initialization failed: %d", ret);
        status_led_set_state(STATUS_LED_BOOT);
        return 0;
    }

    LOG_INF("Dispenser is advertising");


    while (1) {

        /*
         * Most of the actual work will be
         * event-driven.
         */

        k_sleep(K_MSEC(100));
    }

    return 0;
}