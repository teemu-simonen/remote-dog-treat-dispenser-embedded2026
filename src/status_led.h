
#ifndef STATUS_LED_H
#define STATUS_LED_H

/*
 * LED states for the dog treat dispenser.
 */
typedef enum {
    STATUS_LED_BOOT,
    STATUS_LED_ADVERTISING,
    STATUS_LED_CONNECTED
} status_led_state_t;

/*
 * Initialize the status LED hardware.
 *
 * Returns 0 on success, negative error on failure.
 */
int status_led_init(void);

/*
 * Set the normal operating state of the LED.
 */
void status_led_set_state(status_led_state_t state);

/*
 * Flash the LED briefly when a hatch command
 * is received and accepted.
 */
void status_led_command_flash(void);

#endif /* STATUS_LED_H */
