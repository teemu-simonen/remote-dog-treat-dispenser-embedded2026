
#include "servo.h"

#include <errno.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(servo, LOG_LEVEL_INF);

/*
 * Software state.
 *
 * Initially, assume the hatch is closed.
 * This is only an assumption until the physical
 * hatch position can be established.
 */
static bool hatch_open = false;

/*
 * Hardware placeholder.
 *
 * Later, this module will configure PWM and send
 * the appropriate control signal to the servo.
 */
int servo_init(void)
{
    LOG_INF("Initializing servo module");

    /*
     * TODO:
     * 1. Get the PWM device from devicetree.
     * 2. Check that the PWM device is ready.
     * 3. Configure the servo control output.
     *
     * No physical servo is controlled yet.
     */

    LOG_WRN("Servo hardware is not configured");

    return -ENOTSUP;
}

int servo_open(void)
{
    LOG_INF("Open requested");

    /*
     * TODO:
     * Command the servo to the calibrated
     * open position.
     *
     * Do not update hatch_open until the
     * command has been accepted.
     */

    LOG_WRN("Servo movement is not implemented");

    return -ENOTSUP;
}

int servo_close(void)
{
    LOG_INF("Close requested");

    /*
     * TODO:
     * Command the servo to the calibrated
     * closed position.
     */

    LOG_WRN("Servo movement is not implemented");

    return -ENOTSUP;
}

int servo_toggle(void)
{
    if (hatch_open) {
        return servo_close();
    }

    return servo_open();
}

bool servo_is_open(void)
{
    return hatch_open;
}
