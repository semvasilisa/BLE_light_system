# Smart Home Lighting System — nRF5340 DK Project Guide

A BLE peripheral project for the nRF5340 DK that lets your phone control the DK's LEDs (power, blink pattern, brightness) over a custom GATT service — combining everything from Lessons 1–5 of the Nordic BLE Fundamentals course.

**What you'll build:** a "smart light" that advertises itself, accepts a connection from your phone, exposes a custom GATT service with characteristics for Power / Mode / Brightness / Status, notifies the phone when status changes (e.g. from a physical button press), and requires pairing before it can be controlled.

---

## 0. Prerequisites

- nRF5340 DK
- nRF Connect SDK + VS Code extension installed and working (from nRF Connect SDK Fundamentals course)
- nRF Connect for Mobile app on your phone
- Familiarity with flashing/building a sample (you did this in Lesson 1, Exercise 1)

**Starting point:** Just like the course's Lesson 4 exercises, start from the **Bluetooth: Peripheral LBS** sample (Browse Samples → search "Bluetooth LE LED Button service") as your base project, or start from your own Lesson 4 Exercise 1 code if you still have it — it already has the plumbing for a custom service.

---

## Phase 1 — Advertising (maps to Lesson 2)

**Goal:** the board advertises as a connectable device with a recognizable name and a service UUID, so your phone can find and identify it.

1. In `prj.conf`, make sure `CONFIG_BT=y` is set (enables the BLE stack).
2. Set a custom device name:
   ```
   CONFIG_BT_DEVICE_NAME="SmartLight"
   ```
3. In `main.c`, build your advertising data array (`ad[]`) with:
   - **Flags**: `BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR)` — same as Lesson 2 Exercise 1.
   - **Complete name**: `BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)`.
   - **Service UUID**: include your custom service's 128-bit UUID (defined in Phase 3 below) using `BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SMARTLIGHT_VAL)` — this lets a scanner filter specifically for "smart light" devices instead of relying on the name string, and lets nRF Connect for Mobile recognize/label your service.
4. **Manually configure advertising parameters** (don't use the `BT_LE_ADV_CONN` preset — set your own, like in Lesson 2 Exercise 2's `BT_LE_ADV_PARAM`):
   ```c
   static const struct bt_le_adv_param *adv_param =
       BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, /* connectable, undirected */
                       400,  /* 250 ms min (400*0.625ms) */
                       401,  /* ~250.6 ms max */
                       NULL);
   ```
   Note: this time use `BT_LE_ADV_OPT_CONN` (connectable) instead of `BT_LE_ADV_OPT_NONE`, since you want your phone to be able to connect — unlike the pure-beacon exercise in Lesson 2.
5. Call `bt_enable(NULL)`, then `bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0)` (no scan response needed here, unlike the beacon exercise).
6. **Test:** flash, confirm LED1 blinks (advertising), and see "SmartLight" show up in nRF Connect for Mobile's scanner with a CONNECT button available (proving it's connectable, unlike the earlier non-connectable beacon exercise).

---

## Phase 2 — Connection handling (maps to Lesson 3)

**Goal:** detect when the phone connects/disconnects, and tune connection parameters.

1. Add the connection callbacks header: `#include <zephyr/bluetooth/conn.h>`.
2. Declare a `bt_conn_cb` structure with `connected` and `disconnected` callbacks (same pattern as Lesson 3 Exercise 1), and register it with `BT_CONN_CB_DEFINE(...)` or `bt_conn_cb_register()`.
3. In your `connected()` callback: log "Connected", and optionally read the negotiated connection parameters using `bt_conn_get_info()` (Lesson 3 Exercise 2) — log the connection interval, latency, and supervision timeout so you can see what your phone's OS actually agreed to.
4. In your `disconnected()` callback: log "Disconnected", and **restart advertising** (`bt_le_adv_start(...)` again) so the light becomes discoverable again after a disconnect.
5. **Optional (peripheral latency):** since your light will mostly sit idle waiting for commands, request a peripheral latency using `bt_conn_le_param_update()` after connecting, allowing the board to skip some wake-ups when idle — good real-world use of the "mouse/keyboard" concept from the Connection Parameters lesson, just applied to a light instead.
6. **Test:** connect from nRF Connect for Mobile, confirm the connect/disconnect logs appear on your board's serial terminal, and confirm advertising resumes automatically after you disconnect.

---

## Phase 3 — Custom GATT service: Smart Light Service (maps to Lesson 4)

**Goal:** define your own service and characteristics, following the exact pattern of the `my_lbs` example from Lesson 4.

### 3.1 Define UUIDs (`smart_light.h`)

Generate your own 128-bit base UUID (or reuse Nordic's `my_lbs` pattern, incrementing the field for each item), e.g.:

```c
/** @brief Smart Light Service UUID. */
#define BT_UUID_SMARTLIGHT_VAL \
    BT_UUID_128_ENCODE(0x00002000, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Power Characteristic UUID (Write). */
#define BT_UUID_SMARTLIGHT_POWER_VAL \
    BT_UUID_128_ENCODE(0x00002001, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Brightness Characteristic UUID (Write). */
#define BT_UUID_SMARTLIGHT_BRIGHTNESS_VAL \
    BT_UUID_128_ENCODE(0x00002002, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Mode Characteristic UUID (Write) — 0=steady, 1=slow blink, 2=fast blink. */
#define BT_UUID_SMARTLIGHT_MODE_VAL \
    BT_UUID_128_ENCODE(0x00002003, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Status Characteristic UUID (Read + Notify). */
#define BT_UUID_SMARTLIGHT_STATUS_VAL \
    BT_UUID_128_ENCODE(0x00002004, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_SMARTLIGHT           BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_VAL)
#define BT_UUID_SMARTLIGHT_POWER     BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_POWER_VAL)
#define BT_UUID_SMARTLIGHT_BRIGHTNESS BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_BRIGHTNESS_VAL)
#define BT_UUID_SMARTLIGHT_MODE      BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_MODE_VAL)
#define BT_UUID_SMARTLIGHT_STATUS    BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_STATUS_VAL)
```

### 3.2 Your attribute table (design on paper first — like the `my_lbs` table in Lesson 4)

| Characteristic | Properties | Permissions | Purpose |
|---|---|---|---|
| Power | Write | Write | 0 = off, 1 = on |
| Brightness | Write | Write | 0–255, drives PWM duty cycle |
| Mode | Write | Write | 0 = steady, 1 = slow blink, 2 = fast blink |
| Status | Read, Notify | Read | Current combined state; pushed to phone on change (incl. physical button override) |

This directly mirrors the Button/LED/MySensor pattern from Lesson 4's `my_lbs` table — Power/Brightness/Mode are like the LED characteristic (Write-only, no CCCD needed), Status is like the Button characteristic (needs a CCCD since it supports Notify).

### 3.3 Declare the service (`smart_light.c`, following the GATT service macros pattern)

```c
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
```

Note the `BT_GATT_CCC(...)` line right after Status — this is exactly the CCCD attribute from the Attribute Table lesson, automatically generated by this macro since Status supports Notify. Power/Brightness/Mode don't get one, just like the LED characteristic in `my_lbs` didn't.

### 3.4 Implement the write callbacks — wiring GATT writes to real hardware

```c
static ssize_t write_power(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    uint8_t val = ((uint8_t *)buf)[0];
    light_state.power = val;
    apply_light_state();       /* updates the physical LED(s) */
    update_status_notify(conn); /* push new Status to phone */
    return len;
}
```

Write similar functions for `write_brightness` (store 0–255, feed into a PWM duty cycle if using `pwm_set_dt()`) and `write_mode` (store enum, drive a k_timer-based blink pattern).

**`apply_light_state()`** is where you translate your stored `power` / `brightness` / `mode` values into actual hardware:
- **Power off** → LED off / PWM duty 0%, ignore mode.
- **Power on, mode = steady** → LED on at set brightness continuously.
- **Power on, mode = slow/fast blink** → start/restart a `k_timer` that toggles the LED at the chosen rate.

### 3.5 Implement the Status Read + Notify

```c
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

void update_status_notify(struct bt_conn *conn)
{
    bt_gatt_notify(conn, &smart_light_svc.attrs[/* status value attr index */],
                   &light_state, sizeof(light_state));
}
```

`status_ccc_cfg_changed` fires when the phone subscribes/unsubscribes — this is the exact CCCD write mechanism you learned about (client writes to the CCCD's Value field to flip the Notify bit). This matches Lesson 4 Exercise 2's "Adding notification support."

### 3.6 Add the physical button override (ties Lesson 2 Exercise 2's button-callback pattern into this project)

1. Init buttons with `dk_buttons_init(button_changed)` as in Lesson 2 Exercise 2.
2. In `button_changed()`, toggle `light_state.power` locally when Button 1 is pressed, call `apply_light_state()`, and call `update_status_notify(conn)` — this is your reason to actually use Notify: the phone's UI reflects a change made physically at the board, not just from app-driven writes.

### 3.7 Test in nRF Connect for Mobile

- Connect, find "Smart Light Service" (nRF Connect for Mobile will just show the raw UUID unless you're using a recognized one — that's fine, expected for a fully custom service).
- Write `01` to Power → confirm an LED turns on.
- Write a value 0–255 to Brightness → confirm dimming (if PWM wired) or at least a log line confirming receipt.
- Write `00`/`01`/`02` to Mode → confirm steady/slow-blink/fast-blink behavior.
- Enable notifications on Status (tap the bell/arrow icon) → press the physical button on the DK → confirm the phone receives a live update.

---

## Phase 4 — Security (maps to Lesson 5)

**Goal:** require pairing before the light can be controlled, so a random nearby phone can't just connect and flip your light.

1. **Require encryption on Power** (mirrors Lesson 5 Exercise 1 exactly): change the Power characteristic's permission from `BT_GATT_PERM_WRITE` to `BT_GATT_PERM_WRITE_ENCRYPT`. Now writing requires the link to be encrypted (Security Level 2) — an unpaired device gets rejected.
2. **Add pairing support**: register a `bt_conn_auth_cb` structure. For simplest testing, use Just Works (no callbacks needed beyond basic ones) — or, to go further like Lesson 5 Exercise 1's second part, implement a `passkey_display` callback that logs a 6-digit passkey, and enter it in nRF Connect for Mobile when prompted, upgrading you to Security Level 3/4 (authenticated).
3. **Require authentication specifically** (optional, stronger): change Power's permission to `BT_GATT_PERM_WRITE_AUTHEN` instead of `_ENCRYPT` — this forces Security Level 3 or 4 (must use an authenticated pairing method, not just Just Works).
4. **Enable bonding** (Lesson 5 Exercise 2): set `CONFIG_BT_BONDABLE=y` in `prj.conf`. After pairing once, your phone and board save the keys — reconnecting later won't require re-pairing.
5. **Optional — Filter Accept List**: after bonding once, configure the board to only accept connections from the bonded phone's address (using the identity keys exchanged during bonding), so no other device can even connect, not just "can't write."
6. **Test:** try writing to Power before pairing → should fail/be rejected. Pair (accept on phone, or enter passkey if you implemented that), then try again → should succeed. Disconnect and reconnect → should work without re-pairing (bonding), and check that the connection log shows "Security changed ... level X" matching what you configured.

---

## Suggested build/test order

1. Phase 1 only — confirm advertising + connectable, visible in nRF Connect for Mobile.
2. Phase 2 — confirm connect/disconnect logs and advertising restart.
3. Phase 3.1–3.4 — Power characteristic only, wired to one LED. Get this fully working before adding more characteristics.
4. Phase 3 (rest) — add Brightness, Mode, and finally Status + Notify + button override.
5. Phase 4 — add security last, once all GATT logic already works over an open connection (security bugs are much easier to debug when you're sure the underlying reads/writes are correct).

---

## What this project demonstrates (cross-reference to the course)

| Course topic | Where it appears in this project |
|---|---|
| Advertising types, flags, packet structure (L2) | Phase 1 |
| Manual advertising parameters (L2 Ex2) | Phase 1 step 4 |
| Connection process & callbacks (L3) | Phase 2 |
| Connection parameters, peripheral latency (L3) | Phase 2 step 5 |
| GATT operations: Write, Read, Notify (L4) | Phase 3.4–3.5 |
| Services/characteristics/attribute table structure (L4) | Phase 3.2–3.3 |
| CCCD (L4) | Phase 3.3 (`BT_GATT_CCC`), 3.5 (`status_ccc_cfg_changed`) |
| Pairing process & phases (L5) | Phase 4 steps 1–2 |
| Security levels & permissions (L5) | Phase 4 steps 1, 3, 6 |
| Bonding & Filter Accept List (L5) | Phase 4 steps 4–5 |
