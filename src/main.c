#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/lbs.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/settings/settings.h>
#include <zephyr/drivers/pwm.h> /* 1. Include PWM header */

#define USER_LED DK_LED1
#define DEVICE_NAME "Dog_treat_dispenser"

/* 2. Get the servo node from the overlay file */
static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_NODELABEL(servo));

/* SG90 pulse widths (500us = 0 deg, 1500us = 90 deg, 2500us = 180 deg) */
#define SERVO_CLOSED_PULSE PWM_USEC(600) /* ~45 degrees */
#define SERVO_OPEN_PULSE   PWM_USEC(2400) /* ~135 degrees */

// Bluetooth advertise
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, (sizeof(DEVICE_NAME) - 1)),
};

// Connect events
static void connected(struct bt_conn *conn, uint8_t err) {
    printk("Device connected\n");
}   

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Device disconnected\n");
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

// LED + Servo logic with auto-detach to prevent brownouts
static void app_led_cb(bool led_state) {
    dk_set_led(USER_LED, led_state);
    printk("Turned LED1 %s\n", led_state ? "ON" : "OFF");

    /* 1. Pick the pulse width based on phone toggle state */
    uint32_t pulse = led_state ? SERVO_OPEN_PULSE : SERVO_CLOSED_PULSE;
    
    /* 2. Move the servo */
    int err = pwm_set_pulse_dt(&servo, pulse);
    if (err) {
        printk("PWM set failed (err %d)\n", err);
        return;
    }

    /* 3. Wait 500ms for the SG90 to reach its destination */
    k_msleep(500);

    /* 4. Cut the PWM signal (width = 0) so the motor stops holding/buzzing.
          This prevents stall current when it hits the box limit, 
          stopping the board from browning out and resetting! */
    pwm_set_pulse_dt(&servo, 0);
}
static struct bt_lbs_cb lbs_callbacks = {
    .led_cb = app_led_cb
};

int main(void) {
    dk_leds_init();

    /* 4. Verify the PWM hardware is ready */
    if (!pwm_is_ready_dt(&servo)) {
        printk("Error: PWM device is not ready\n");
        return 0;
    }

    bt_enable(NULL);
    settings_load();

    bt_lbs_init(&lbs_callbacks);

    bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), NULL, 0);

    printk("Broadcasting...\n");
    return 0;
}