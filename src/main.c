#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/lbs.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/settings/settings.h>
#include <zephyr/drivers/pwm.h> /* 1. Include PWM header */

#define USER_LED DK_LED1
#define DEVICE_NAME "Led_Remote_Switch"

/* 2. Get the servo node from the overlay file */
static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_NODELABEL(servo));

/* SG90 pulse widths (500us = 0 deg, 1500us = 90 deg, 2500us = 180 deg) */
#define SERVO_CLOSED_PULSE PWM_USEC(1000) /* ~45 degrees */
#define SERVO_OPEN_PULSE   PWM_USEC(2000) /* ~135 degrees */

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

// LED + Servo logic
static void app_led_cb(bool led_state) {
    dk_set_led(USER_LED, led_state);
    printk("Turned LED1 %s\n", led_state ? "ON" : "OFF");

    /* 3. Move the servo based on the Bluetooth LED toggle */
    uint32_t pulse = led_state ? SERVO_OPEN_PULSE : SERVO_CLOSED_PULSE;
    int err = pwm_set_pulse_dt(&servo, pulse);
    if (err) {
        printk("PWM set failed (err %d)\n", err);
    }
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