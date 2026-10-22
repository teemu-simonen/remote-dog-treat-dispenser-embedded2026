#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/lbs.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/settings/settings.h>
#include <zephyr/drivers/pwm.h> /* 1. Include PWM header */

#define USER_LED DK_LED1
#define DEVICE_NAME "Dog_treat_dispenser"

/* LED behavior */
static bool is_connected;
static bool is_advertising;
static bool command_flash;
static bool led_blink_state;
static int command_phase;

static void led_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(led_work, led_work_handler);

static void restart_advertising_work_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(
    restart_advertising_work,
    restart_advertising_work_handler
);

/* 2. Get the servo node from the overlay file */
static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_NODELABEL(servo));

/* SG90 pulse widths (500us = 0 deg, 1500us = 90 deg, 2500us = 180 deg) */
#define SERVO_CLOSED_PULSE PWM_USEC(600) /* ~45 degrees */
#define SERVO_OPEN_PULSE   PWM_USEC(2400) /* ~135 degrees */

/* Servo work item */
static struct k_work_delayable servo_work;

/* Store the requested pulse width */
static uint32_t servo_target_pulse;

// Bluetooth advertise
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, (sizeof(DEVICE_NAME) - 1)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed: %u\n", err);
        return;
    }

    printk("Bluetooth connected\n");

    is_connected = true;
    is_advertising = false;

    /* Prevent a pending restart from interfering */
    k_work_cancel_delayable(&restart_advertising_work);
    k_work_cancel_delayable(&led_work);

    dk_set_led(USER_LED, 1);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Bluetooth disconnected, reason: %u\n", reason);

    is_connected = false;
    is_advertising = false;

    /*
     * Do not restart advertising directly here.
     * Give Zephyr time to release the connection object.
     */
    dk_set_led(USER_LED, 0);

    k_work_reschedule(
        &restart_advertising_work,
        K_SECONDS(1)
    );
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static void led_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (command_flash) {
    if (command_phase == 0) {
        dk_set_led(USER_LED, 0);  /* OFF */
        command_phase = 1;
        k_work_reschedule(&led_work, K_MSEC(500));
    } else if (command_phase == 1) {
        dk_set_led(USER_LED, 1);  /* ON */
        command_phase = 2;
        k_work_reschedule(&led_work, K_MSEC(500));
    } else {
        command_flash = false;
        command_phase = 0;
        k_work_reschedule(&led_work, K_NO_WAIT);
    }

    return;
}

    if (is_connected) {
        /* Connected: LED stays ON */
        dk_set_led(USER_LED, 1);
    } else if (is_advertising) {
        /* Advertising: toggle LED every 500 ms */
        led_blink_state = !led_blink_state;
        dk_set_led(USER_LED, led_blink_state);
        k_work_reschedule(&led_work, K_MSEC(500));
    } else {
        dk_set_led(USER_LED, 0);
    }
}

static bool servo_stop_phase;

static void servo_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (!servo_stop_phase) {
        /* Phase 1: Start moving the servo */
        int err = pwm_set_pulse_dt(&servo, servo_target_pulse);

        if (err) {
            printk("Servo PWM start failed (err %d)\n", err);
            servo_stop_phase = false;
            return;
        }

        /* Schedule PWM shutdown after 500 ms */
        servo_stop_phase = true;
        k_work_reschedule(&servo_work, K_MSEC(500));

    } else {
        /* Phase 2: Stop sending PWM */
        int err = pwm_set_pulse_dt(&servo, 0);

        if (err) {
            printk("Servo PWM stop failed (err %d)\n", err);
        }

        servo_stop_phase = false;
    }
}

static void app_led_cb(bool led_state)
{
    printk("Command received: %s\n",
           led_state ? "OPEN" : "CLOSE");

    /* Trigger the one-second LED indication */
    command_flash = true;
    command_phase = 0;
    k_work_reschedule(&led_work, K_NO_WAIT);

    /* Choose the target servo position */
    servo_target_pulse = led_state
                       ? SERVO_OPEN_PULSE
                       : SERVO_CLOSED_PULSE;

    /* Restart the servo operation without blocking */
    servo_stop_phase = false;
    k_work_reschedule(&servo_work, K_NO_WAIT);
}

static void restart_advertising_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    int err = bt_le_adv_start(
        BT_LE_ADV_CONN_FAST_2,
        ad,
        ARRAY_SIZE(ad),
        NULL,
        0
    );

    if (err == 0) {
        printk("Advertising restarted successfully\n");

        is_advertising = true;
        led_blink_state = false;

        k_work_reschedule(&led_work, K_NO_WAIT);
    } else if (err == -EALREADY) {
        printk("Advertising is already active\n");
        is_advertising = true;
    } else {
        printk("Advertising restart failed: %d\n", err);

        /* Retry after a short delay */
        k_work_reschedule(
            &restart_advertising_work,
            K_SECONDS(1)
        );
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

    k_work_init_delayable(&servo_work, servo_work_handler);

    bt_enable(NULL);
    settings_load();

    bt_lbs_init(&lbs_callbacks);

    int err = bt_le_adv_start(
    BT_LE_ADV_CONN_FAST_2,
    ad, ARRAY_SIZE(ad),
    NULL, 0
);

if (err) {
    printk("Advertising failed (err %d)\n", err);
    return 0;
}

is_advertising = true;
is_connected = false;
led_blink_state = false;

k_work_reschedule(&led_work, K_NO_WAIT);

printk("Broadcasting...\n");
}