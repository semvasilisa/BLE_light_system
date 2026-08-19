#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

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

int main(void)
{
    int err;

    err = bt_enable(NULL); // starts the ble host
    if (err) {
        LOG_INF("Bluetooth init failed (err %d)\n", err);
        return err;
    }

    LOG_INF("Bluetooth initialized\n");

    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return err;
    }

    printk("Advertising successfully started as %s\n", DEVICE_NAME);

    return 0;
}