
#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

/*
 * Initialize Bluetooth, register the custom GATT service,
 * and start advertising so a phone can find the device.
 *
 * Returns 0 on success, or a negative error code on failure.
 */
int ble_service_init(void);

#endif /* BLE_SERVICE_H */

