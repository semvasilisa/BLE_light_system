#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include "light.h"

LOG_MODULE_REGISTER(light, LOG_LEVEL_INF);

static ssize_t write_power(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_INF("write_power called, len=%u", len);
    return len;
}

static ssize_t write_brightness(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_INF("write_brightness called, len=%u", len);
    return len;
}

static ssize_t write_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_INF("write_mode called, len=%u", len);
    return len;
}

static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset)
{
    LOG_INF("read_status called");
    uint8_t dummy = 0;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &dummy, sizeof(dummy));
}

static void status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Status notifications %s", notify_enabled ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(smart_light_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_SMARTLIGHT),

    BT_GATT_CHARACTERISTIC(BT_UUID_SMARTLIGHT_POWER,
        BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_WRITE,
        NULL, write_power, NULL),

    BT_GATT_CHARACTERISTIC(BT_UUID_SMARTLIGHT_BRIGHTNESS,
        BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_WRITE,
        NULL, write_brightness, NULL),

    BT_GATT_CHARACTERISTIC(BT_UUID_SMARTLIGHT_MODE,
        BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_WRITE,
        NULL, write_mode, NULL),

    BT_GATT_CHARACTERISTIC(BT_UUID_SMARTLIGHT_STATUS,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_READ,
        read_status, NULL, NULL),
    BT_GATT_CCC(status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);