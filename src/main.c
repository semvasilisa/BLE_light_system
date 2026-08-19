#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include "light.h"

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// what we will send in the advertising packet
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// configuration for advertising parameters
static const struct bt_le_adv_param *adv_param =
    BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, // no special advertising options, makes the advert connectable
                     400,  // min advertising interval: 400 * 0.625ms = 250 ms 
                     401,  // max advertising interval: 401 * 0.625ms = 250.625 ms
                     NULL); // no specific device is targeted (undirected) 

// a device is connected, get info about this connection
static void connected(struct bt_conn *conn, uint8_t err) // conn tells which BLE connection
{
    if (err) {
        LOG_ERR("Connection failed (err %u)\n", err);
        return;
    }

    LOG_INF("Connected\n");

    struct bt_conn_info info; // all info about the connection 
    err = bt_conn_get_info(conn, &info);
    if (err) {
        LOG_ERR("Failed to get connection info (err %d)\n", err);
        return;
    }

    // return connection interval, peripheral latency, and supervision timeout
    LOG_INF("Connection interval: %u units (%u ms)\n",
            info.le.interval, info.le.interval * 5 / 4);
    LOG_INF("Peripheral latency: %u\n", info.le.latency);
    LOG_INF("Supervision timeout: %u units (%u ms)\n",
            info.le.timeout, info.le.timeout * 10);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)\n", reason);

    // restart advertising after disconnection
    // int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    // if (err) {
    //     LOG_ERR("Advertising failed to restart (err %d)\n", err);
    // }
}

static void recycled(void)
{
    int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);

    if (err) {
        LOG_ERR("Advertising failed to restart (err %d)", err);
    } else {
        LOG_INF("Advertising restarted");
    }
}

static struct bt_conn_cb connection_callbacks = {
        .connected = connected,
        .disconnected = disconnected,
        .recycled = recycled,
};

int main(void)
{
    int err;

    err = bt_enable(NULL); // starts the ble host
    if (err) {
        LOG_INF("Bluetooth init failed (err %d)\n", err);
        return err;
    }

    LOG_INF("Bluetooth initialized\n");

    bt_conn_cb_register(&connection_callbacks);
    // start advertising
    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)\n", err);
        return err;
    }

    printk("Advertising successfully started as %s\n", DEVICE_NAME);

    return 0;
}