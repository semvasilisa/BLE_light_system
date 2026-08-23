#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <dk_buttons_and_leds.h>
#include "light.h"

LOG_MODULE_REGISTER(light, LOG_LEVEL_INF);

#define LIGHT_LED DK_LED1

extern struct bt_conn *current_conn;

static void update_status_notify(struct bt_conn *conn);

// variable to preserve current state, three together because read_status returns three values as one
struct light_state {
    uint8_t power;      // 0 = off, 1 = on 
    uint8_t brightness; // 0-255 
    uint8_t mode;        // 0 = steady, 1 = slow blink, 2 = fast blink 
};

static struct light_state light_state;

static void apply_light_state(void)
{
    if (!light_state.power) {
        dk_set_led_off(LIGHT_LED);
        return;
    }

    /* Power is on. For now (steady mode logic only): */
    if (light_state.mode == 0) {
        dk_set_led_on(LIGHT_LED);
    }
    /* mode == 1 (slow blink) and mode == 2 (fast blink) come in Step 5 */
}

static ssize_t write_power(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if(len != 1){ // we're expecting exactly one byte for power value
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    uint8_t val = ((uint8_t *)buf)[0]; // get the first byte from the buffer
    light_state.power = val;
    LOG_INF("Power set to %u", light_state.power);
    apply_light_state();
    update_status_notify(conn);
    // LOG_INF("write_power called, len=%u", len);
    return len;
}

static ssize_t write_brightness(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if(len != 1){ 
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    light_state.brightness = ((uint8_t *)buf)[0];
    LOG_INF("Brightness set to %u", light_state.brightness);
    apply_light_state();
    update_status_notify(conn);
    // LOG_INF("write_brightness called, len=%u", len);
    return len;
}

static ssize_t write_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
     if(len != 1){ 
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    light_state.mode = ((uint8_t *)buf)[0];
    LOG_INF("Mode set to %u", light_state.mode);
    apply_light_state();
    update_status_notify(conn);
    // LOG_INF("write_mode called, len=%u", len);
    return len;
}

static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset,
                              &light_state, sizeof(light_state));
}

static void status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Status notifications %s", notify_enabled ? "enabled" : "disabled");
}

void button_changed(uint32_t button_state, uint32_t has_changed)
{
    if (has_changed & DK_BTN1_MSK) {
        if (button_state & DK_BTN1_MSK) {
            /* Button 1 was just pressed */
            light_state.power = !light_state.power;
            LOG_INF("Button 1 pressed - power toggled to %u", light_state.power);

            apply_light_state();
            update_status_notify(current_conn);
        }
    }
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

static void update_status_notify(struct bt_conn *conn)
{
    if (!conn) {
        return;
    }

    int err = bt_gatt_notify(conn, &smart_light_svc.attrs[8],
                              &light_state, sizeof(light_state));
    if (err) {
        LOG_WRN("Status notify failed (err %d)", err);
    }
}