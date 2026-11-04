#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/lbs.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/settings/settings.h>
#include <zephyr/drivers/pwm.h> 

#define USER_LED DK_LED1
#define DEVICE_NAME "Dog_treat_dispenser"

// Get the servo pin mapping from the device tree
static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_NODELABEL(servo));

/* 
 * SG90 Servo pulse widths define the angle of the motor arm.
 * 600 microseconds = ~45 degrees (Closed lid)
 * 2400 microseconds = ~135 degrees (Open lid)
 */
#define SERVO_CLOSED_PULSE PWM_USEC(600) 
#define SERVO_OPEN_PULSE   PWM_USEC(2400) 


static bool is_connected;
static bool is_advertising;
static bool command_flash;
static bool led_blink_state;
static int command_phase;
static bool servo_stop_phase;
static uint32_t servo_target_pulse; // Stores the requested open/close angle

// Handles the visual status lights without freezing the Bluetooth radio.
static void led_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(led_work, led_work_handler);

// Waits 1 second after a phone disconnects before turning the radio back on.
static void restart_advertising_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(restart_advertising_work, restart_advertising_work_handler);

// Servo Motor Task
static struct k_work_delayable servo_work;

// Bluetooth advertising
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

    // Update state
    is_connected = true;
    is_advertising = false;

    // Cancel any pending background task so they don't interfere with the new connection
    k_work_cancel_delayable(&restart_advertising_work);
    k_work_cancel_delayable(&led_work);

    // Turn the LED on solid to confirm connection
    dk_set_led(USER_LED, 1);
}

// Runs automatically when the phone disconnects
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Bluetooth disconnected, reason: %u\n", reason);

    is_connected = false;
    is_advertising = false;

    dk_set_led(USER_LED, 0);

    k_work_reschedule(&restart_advertising_work, K_SECONDS(1));
}

// Register the connection callbacks with the Zephyr Bluetooth stack
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

// Controls the LED states
static void led_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    // Acknowledge a command from the phone by flashing LED
    if (command_flash) {
        if (command_phase == 0) {
            dk_set_led(USER_LED, 0); 
            command_phase = 1;
            k_work_reschedule(&led_work, K_MSEC(500));
        } else if (command_phase == 1) {
            dk_set_led(USER_LED, 1);
            command_phase = 2;
            k_work_reschedule(&led_work, K_MSEC(500));
        } else {
            command_flash = false;
            command_phase = 0;
            k_work_reschedule(&led_work, K_NO_WAIT);
        }
        return;
    }

    // Normal connection status
    if (is_connected) {
        dk_set_led(USER_LED, 1);
    } else if (is_advertising) {
        led_blink_state = !led_blink_state;
        dk_set_led(USER_LED, led_blink_state);
        k_work_reschedule(&led_work, K_MSEC(500));
    } else {
        dk_set_led(USER_LED, 0);
    }
}

// Controls the servo motor
static void servo_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (!servo_stop_phase) {
        // Send the PWM signal to start moving the motor
        int err = pwm_set_pulse_dt(&servo, servo_target_pulse);
        if (err) {
            printk("Servo PWM start failed (err %d)\n", err);
            servo_stop_phase = false;
            return;
        }

        // Set a 500ms timer to give the time to turn
        servo_stop_phase = true;
        k_work_reschedule(&servo_work, K_MSEC(500));

    } else {
        int err = pwm_set_pulse_dt(&servo, 0);
        if (err) {
            printk("Servo PWM stop failed (err %d)\n", err);
        }

        servo_stop_phase = false;
    }
}

// Commands the Bluetooth to start broadcasting
static void restart_advertising_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), NULL, 0);

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
        k_work_reschedule(&restart_advertising_work, K_SECONDS(1));
    }
}


// Bluetooth command task
static void app_led_cb(bool led_state)
{
    printk("Command received: %s\n", led_state ? "OPEN" : "CLOSE");

    command_flash = true;
    command_phase = 0;
    k_work_reschedule(&led_work, K_NO_WAIT);

    // 2. Choose the correct angle based on the bluetooth command
    servo_target_pulse = led_state ? SERVO_OPEN_PULSE : SERVO_CLOSED_PULSE;

    servo_stop_phase = false;
    k_work_reschedule(&servo_work, K_NO_WAIT);
}

static struct bt_lbs_cb lbs_callbacks = {
    .led_cb = app_led_cb
};


int main(void) {
    dk_leds_init();

    if (!pwm_is_ready_dt(&servo)) {
        printk("Error: PWM device is not ready\n");
        return 0;
    }

    k_work_init_delayable(&servo_work, servo_work_handler);

    // Turn on the Bluetooth
    bt_enable(NULL);
    settings_load();

    bt_lbs_init(&lbs_callbacks);

    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed (err %d)\n", err);
        return 0;
    }

    is_advertising = true;
    is_connected = false;
    led_blink_state = false;

    k_work_reschedule(&led_work, K_NO_WAIT);

    printk("Broadcasting...\n");
    
    return 0;
}