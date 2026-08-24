#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/drivers/pwm.h>
#include <dk_buttons_and_leds.h>
#include "light.h"

LOG_MODULE_REGISTER(light, LOG_LEVEL_INF);

#define LIGHT_LED DK_LED1

static const struct pwm_dt_spec led_pwm = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

extern struct bt_conn *current_conn;

static void update_status_notify(struct bt_conn *conn);

// variable to preserve current state, three together because read_status returns three values as one
struct light_state {
    uint8_t power;      // 0 = off, 1 = on 
    uint8_t brightness; // 0-255 
    uint8_t mode;        // 0 = steady, 1 = slow blink, 2 = fast blink 
};

static struct light_state light_state = {
    .power = 0,
    .brightness = 255,
    .mode = 0,
};

static bool led_is_on = false;

void apply_brightness(void){
    
    uint32_t pulse_width = ((uint64_t)light_state.brightness * led_pwm.period) / 255;

    int err = pwm_set_pulse_dt(&led_pwm, pulse_width);
    if (err) {
        LOG_WRN("Failed to set brightness (err %d)", err);
    }
}

void apply_led_off(void)
{
    int err = pwm_set_pulse_dt(&led_pwm, 0);
    if (err) {
        LOG_WRN("Failed to turn off LED (err %d)", err);
    }
}

void toggle_light_led(void)
{
    if (led_is_on) {
        apply_led_off();
        led_is_on = false;
    } else {
        apply_brightness();
        led_is_on = true;
    }
}

static void led_timer_expiry(struct k_timer *timer)
{
    toggle_light_led();
}

K_TIMER_DEFINE(led_timer, led_timer_expiry, NULL);

static void apply_light_state(void)
{
    k_timer_stop(&led_timer);

    if (!light_state.power) {
        apply_led_off();
        return;
    }

    /* Power is on */
    if (light_state.mode == 0) {
        apply_brightness();
    }
   
    if (light_state.mode == 1) {
        k_timer_start(&led_timer, K_MSEC(1000), K_MSEC(1000));
    }
    else if (light_state.mode == 2) {
        k_timer_start(&led_timer, K_MSEC(500), K_MSEC(500));
    }
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
        BT_GATT_PERM_WRITE_AUTHEN,
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