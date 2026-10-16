#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/lbs.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/settings/settings.h>

#define USER_LED DK_LED1
#define DEVICE_NAME "Dog_Treat_Dispenser"

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

// LED logic
static void app_led_cb(bool led_state) {
	dk_set_led(USER_LED, led_state);
	printk("Turned LED1 %s\n", led_state ? "ON" : "OFF");
}

static struct bt_lbs_cb lbs_callbacks = {
	.led_cb = app_led_cb
};

int main(void) {
	dk_leds_init();

	bt_enable(NULL);
	settings_load();

	bt_lbs_init(&lbs_callbacks);

	bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), NULL, 0);

	printk("Broadcasting...\n");
	return 0;
}