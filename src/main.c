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



    ret = ble_service_init();


    LOG_INF("Dispenser is advertising");


    while (1) {
        printk("Servo opening\n");
        servo_open();
        k_sleep(K_SECONDS(2));

        printk("Servo closing\n");
        servo_close();
        k_sleep(K_SECONDS(2));
    }



    while (1) {

        /*
         * Most of the actual work will be
         * event-driven.
         */

        k_sleep(K_MSEC(100));
    }

    return 0;
}