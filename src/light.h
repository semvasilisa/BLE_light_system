#ifndef LIGHT_H_
#define LIGHT_H_

#include <zephyr/bluetooth/uuid.h>

/** @brief Smart Light Service UUID. */
#define BT_UUID_SMARTLIGHT_VAL \ 
    BT_UUID_128_ENCODE(0x00002000, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Power Characteristic UUID (Write). */
#define BT_UUID_SMARTLIGHT_POWER_VAL \
    BT_UUID_128_ENCODE(0x00002001, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Brightness Characteristic UUID (Write). */
#define BT_UUID_SMARTLIGHT_BRIGHTNESS_VAL \
    BT_UUID_128_ENCODE(0x00002002, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Mode Characteristic UUID (Write). */
#define BT_UUID_SMARTLIGHT_MODE_VAL \
    BT_UUID_128_ENCODE(0x00002003, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Status Characteristic UUID (Read + Notify). */
#define BT_UUID_SMARTLIGHT_STATUS_VAL \
    BT_UUID_128_ENCODE(0x00002004, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_SMARTLIGHT            BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_VAL)
#define BT_UUID_SMARTLIGHT_POWER      BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_POWER_VAL)
#define BT_UUID_SMARTLIGHT_BRIGHTNESS BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_BRIGHTNESS_VAL)
#define BT_UUID_SMARTLIGHT_MODE       BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_MODE_VAL)
#define BT_UUID_SMARTLIGHT_STATUS     BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_STATUS_VAL)

void button_changed(uint32_t button_state, uint32_t has_changed);

#endif /* LIGHT_H_ */