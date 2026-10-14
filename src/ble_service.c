
#include "ble_service.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include "servo.h"
#include "status_led.h"

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_INF);

/*
 * Custom 128-bit UUIDs for the dispenser service
 * and hatch-control characteristic.
 */
#define BT_UUID_DOG_DISPENSER_VAL \
    BT_UUID_128_ENCODE(0x7E400001, 0xB5A3, 0xF393, \
                       0xE0A9, 0xE50E24DCCA9E)

#define BT_UUID_HATCH_CONTROL_VAL \
    BT_UUID_128_ENCODE(0x7E400002, 0xB5A3, 0xF393, \
                       0xE0A9, 0xE50E24DCCA9E)

static struct bt_uuid_128 dog_dispenser_uuid =
    BT_UUID_INIT_128(BT_UUID_DOG_DISPENSER_VAL);

static struct bt_uuid_128 hatch_control_uuid =
    BT_UUID_INIT_128(BT_UUID_HATCH_CONTROL_VAL);

/*
 * Handle a phone writing to the Hatch Control characteristic.
 *
 * Expected command:
 *   0x01 = toggle hatch
 */
static ssize_t hatch_control_write(
    struct bt_conn *conn,
    const struct bt_gatt_attr *attr,
    const void *buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);

    /* This characteristic accepts one byte at offset zero. */
    if (offset != 0 || len != 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    const uint8_t *data = buf;

    if (data[0] != 0x01) {
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }

    LOG_INF("Hatch toggle command received");

    /*
     * The LED acknowledges receipt of a valid command.
     * This does not mean the physical hatch has moved.
     */
    status_led_command_flash();

    /*
     * Request the servo action.
     * The servo stub currently returns -ENOTSUP because
     * hardware-specific PWM has not been implemented.
     */
    int ret = servo_toggle();

    if (ret < 0) {
        LOG_WRN("Hatch action failed: %d", ret);
    }

    /*
     * The command was valid and accepted by this callback.
     * Return the number of bytes consumed, even if the
     * servo implementation is still a hardware stub.
     */
    return len;
}

/*
 * Register the service and its writable characteristic.
 */
BT_GATT_SERVICE_DEFINE(dog_dispenser_service,
    BT_GATT_PRIMARY_SERVICE(&dog_dispenser_uuid),

    BT_GATT_CHARACTERISTIC(
        &hatch_control_uuid.uuid,
        BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_WRITE,
        NULL,
        hatch_control_write,
        NULL
    )
);

/*
 * Advertising data: discoverable, connectable BLE peripheral.
 */
static const struct bt_data advertising_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS,
        (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

/*
 * Put the full device name in the scan response.
 * This avoids making the advertising packet too large.
 */
static const struct bt_data scan_response_data[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE,
        CONFIG_BT_DEVICE_NAME,
        sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/*
 * Start BLE advertising.
 */
static int start_advertising(void)
{
    int ret = bt_le_adv_start(
        BT_LE_ADV_CONN_FAST_1,
        advertising_data,
        ARRAY_SIZE(advertising_data),
        scan_response_data,
        ARRAY_SIZE(scan_response_data)
    );

    if (ret < 0) {
        LOG_ERR("Failed to start advertising: %d", ret);
        return ret;
    }

    LOG_INF("BLE advertising started");

    status_led_set_state(STATUS_LED_ADVERTISING);

    return 0;
}

/*
 * Called when a phone connects.
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
    ARG_UNUSED(conn);

    if (err != 0) {
        LOG_WRN("Connection failed: 0x%02x", err);
        return;
    }

    LOG_INF("Phone connected");

    status_led_set_state(STATUS_LED_CONNECTED);
}

/*
 * Called when the phone disconnects.
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(conn);

    LOG_INF("Phone disconnected: 0x%02x", reason);

    /*
     * Return to advertising so the phone can reconnect.
     */
    int ret = start_advertising();

    if (ret < 0) {
        LOG_ERR("Could not restart advertising: %d", ret);
    }
}

BT_CONN_CB_DEFINE(connection_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/*
 * Public initialization function.
 */
int ble_service_init(void)
{
    int ret = bt_enable(NULL);

    if (ret < 0) {
        LOG_ERR("Bluetooth initialization failed: %d", ret);
        return ret;
    }

    LOG_INF("Bluetooth initialized");

    ret = start_advertising();

    if (ret < 0) {
        return ret;
    }

    return 0;
}
