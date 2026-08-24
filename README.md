# Smart Home Lighting System — BLE Peripheral for nRF5340 DK

This project was implemented to apply the knowledge acquired in Nordic's
Dev Academy "Bluetooth Low Energy Fundamentals" course. Beyond the basic
creation of a BLE peripheral, several custom characteristics were added.

The project implements a BLE peripheral on the nRF5340 DK that lets a
phone control the DK's LED — power, blink pattern, and brightness — over
a custom GATT service.

## Prerequisites

- nRF5340 DK
- nRF Connect SDK + VS Code extension installed and working (from the nRF
  Connect SDK Fundamentals course)
- nRF Connect for Mobile app on your phone

---

## Phase 1 — Advertising

**Goal:** the board advertises as a connectable device with a recognizable
name and a service UUID, so the phone can find and identify it.

`bt_enable()` starts the Host, which is responsible for GAP, GATT, ATT,
SMP, and L2CAP. Once the Host is active, we build the advertising data
array. The advertising packet includes two AD structures:

- **Flags** — tells scanners this device is BLE-only (does not support
  BR/EDR).
- **Complete Local Name** — tells the scanner the board's name.

Next, we configure the advertising parameters, specifying that the advert
is connectable and setting the minimum/maximum advertising interval.
Giving a min/max range instead of a single fixed value lets the Controller
add small random jitter to avoid periodic collisions with other devices.

With that in place, we start advertising via `bt_le_adv_start()`. Result:
the board is visible in nRF Connect for Mobile with a **Connect** button,
confirming it's connectable — though connecting isn't yet possible, since
connection callbacks haven't been implemented.

---

## Phase 2 — Connection Handling

**Goal:** detect when the phone connects/disconnects, and read the
negotiated connection parameters.

At this point the board is visible with a Connect button, but nothing
handles a connection attempt yet. The goal of this phase is to make
pressing Connect succeed: the board detects the connection, logs it,
reads back the connection parameters the phone's OS negotiated, detects
disconnection, and automatically restarts advertising.

Rather than continuously polling "is anyone connected?", the BLE stack
calls registered callback functions when a connection event occurs. The
`bt_conn_cb` structure tells the stack which functions to call for each
event:

```c
struct bt_conn_cb connection_callbacks = {
    .connected = on_connected,
    .disconnected = on_disconnected,
};
```

Once these callbacks are implemented, connecting and disconnecting work
as expected, and the board automatically resumes advertising after a
disconnect.

---

## Phase 3 — Custom GATT Service

**Goal:** define a custom service and its characteristics.

After connecting, the client doesn't yet know what services the server
offers, so it performs **service discovery** — asking the server "what do
you have?" and building a map of its services and characteristics.

For the phone to know, for example, which attribute on the board
represents the LED, each attribute is identified by a **UUID** — a unique
identifier for that piece of functionality:

```
UUID A → "Power characteristic"
UUID B → "Button characteristic"
UUID C → "Custom service"
```

```
Service:
UUID = ABC123...
→ "This is the custom service"

Characteristic:
UUID = ABC124...
→ "This is the button"

Characteristic:
UUID = ABC125...
→ "This is the LED"
```

A UUID simply identifies what the attribute represents. The Bluetooth SIG
defines standardized 16-bit UUIDs for common services (Heart Rate Service,
Battery Service, Device Information Service, etc.). Since this project
defines its own, non-standard service, it uses **128-bit UUIDs** instead.

Building the service can be thought of as creating a Bluetooth interface
for the board, so the phone knows how to interact with it. The process:

1. **Assign identities** — define a UUID for each attribute (service and
   characteristics).
2. **Build the service** — register an (initially empty) service in the
   board's GATT table.
3. **Add characteristics** — place characteristics inside the service and
   define how the phone is permitted to interact with each one.
4. **Define callbacks** — specify what code runs when the phone reads
   from or writes to each characteristic.

Each UUID exists in two forms:

```
BT_UUID_SMARTLIGHT_POWER_VAL
        ↓
    raw UUID value

        ↓ BT_UUID_DECLARE_128()

BT_UUID_SMARTLIGHT_POWER
        ↓
    Bluetooth UUID object/pointer
```

```c
#define BT_UUID_SMARTLIGHT_POWER_VAL \
    BT_UUID_128_ENCODE(0x00002001, 0x1212, 0xefde, 0x1523, 0x785feabcd123)
```

The `_VAL` definition is the raw 128-bit UUID value for the Power
characteristic.

```c
#define BT_UUID_SMARTLIGHT_POWER \
    BT_UUID_DECLARE_128(BT_UUID_SMARTLIGHT_POWER_VAL)
```

This wraps the raw value into a usable Bluetooth UUID object — the form
the Zephyr Bluetooth API expects. The `_VAL` form is used in advertising
data (raw bytes); the non-`_VAL` form is used when defining the GATT
service itself.

### Device capabilities

1. **Power control** — phone writes `0` or `1`; the board turns the light
   off/on.
2. **Brightness control** — phone writes `0–255`; the board drives a PWM
   duty cycle for dimming.
3. **Mode control** — phone writes `0`/`1`/`2`; the board switches between
   steady / slow blink / fast blink.
4. **Status reporting** — the board reports its combined state
   (power + brightness + mode) back to the phone, both on request (Read)
   and proactively whenever state changes (Notify) — including when the
   change originates from the physical button on the DK, not just from
   the app.

Three write-only controls plus one read/notify status feed. Only the
Status characteristic has a CCCD, since a CCCD is only needed for
characteristics that support Notify/Indicate and therefore require client
subscription.

The CCCD immediately follows the characteristic it applies to, per GATT's
position-based grouping rule: **a CCCD must come after the characteristic
Value it describes**, since otherwise it would have no valid characteristic
to attach to.

An important distinction: in GATT terms, an "attribute" refers to every
row in the table, including declarations and CCCDs. This project defines
four device capabilities, each mapping to exactly one characteristic — the
Service Declaration, Characteristic Declarations, and the CCCD are not
capabilities themselves; they are the protocol scaffolding that makes the
four capabilities discoverable and usable by the phone.

UUIDs are defined in a separate `light.h` file, included from `main.c`
alongside the corresponding callback declarations. The 10 resulting
attributes are registered as an actual GATT service using
`BT_GATT_SERVICE_DEFINE`, which creates the service and adds it to the
server's GATT table.

### Attribute Table

| # | Attribute | Type (0x28xx) | Perms | Why it exists |
|---|---|---|---|---|
| 1 | Primary Service Declaration | 0x2800 | Read | Marks the start of the service; every service begins with exactly one of these |
| 2 | Power Characteristic Declaration | 0x2803 | Read | Declares "a characteristic follows," including its properties (Write) |
| 3 | Power Value | (custom UUID) | Write | The actual attribute the client writes 0/1 to |
| 4 | Brightness Characteristic Declaration | 0x2803 | Read | Same pattern as #2 |
| 5 | Brightness Value | (custom UUID) | Write | 0–255 |
| 6 | Mode Characteristic Declaration | 0x2803 | Read | Same pattern |
| 7 | Mode Value | (custom UUID) | Write | 0/1/2 |
| 8 | Status Characteristic Declaration | 0x2803 | Read | Same pattern, but properties include both Read and Notify |
| 9 | Status Value | (custom UUID) | Read | The actual current-state bytes a client reads |
| 10 | Status CCCD | 0x2902 | Read, Write | Only Status gets this — it's the subscribe switch for Notify |

### Declaring the service in code

Callback functions are implemented in a separate `light.c` file. Their
parameters differ depending on the operation:

- **Write callbacks** — receive `buf` (the data the phone sent), `len`
  (number of bytes sent), `offset` (where in the data to start, for
  writes split across multiple packets), and `flags` (information about
  how the write was performed). Returning `len` tells the stack that all
  sent bytes were accepted, allowing it to send the appropriate response
  back to the phone.
- **Read callbacks** — receive an empty `buf` that the function must
  fill, along with `len` (the maximum bytes the client's buffer can hold)
  and `offset`. `bt_gatt_attr_read()` is a helper that correctly copies
  the source data into `buf`.
- **`status_ccc_cfg_changed`** — fires when the phone writes to the CCCD
  to subscribe or unsubscribe from Status notifications (Notify = on/off).
  This callback checks which of the two just happened.

Once the callbacks are defined, they're assembled into a single service
using `BT_GATT_SERVICE_DEFINE`, with each characteristic declared via
`BT_GATT_CHARACTERISTIC`. Each `BT_GATT_CHARACTERISTIC` call produces two
attributes at once — a Characteristic Declaration and a Value attribute
— where:

- `BT_GATT_CHRC_WRITE` describes the characteristic's **properties**
  (e.g., "this characteristic supports the write operation").
- `BT_GATT_PERM_WRITE` describes the **permission** on the characteristic's
  value (whether the client is allowed to write to it).

**Result:** after connecting to the board in nRF Connect for Mobile, all
four characteristics are visible and usable:
- Turn power on/off
- Set brightness level
- Choose blinking mode
- Subscribe to Status notifications

---

## Phase 4 — Security

**Goal:** require pairing before the light can be controlled, so a random
nearby phone can't connect and control it.

Initially, the Power characteristic allows `BT_GATT_PERM_WRITE`, meaning
any connected phone can write to it — unsecured. The goal is to only
allow a phone that has paired with the board to control it.

We change Power's permission to `BT_GATT_PERM_WRITE_ENCRYPT`. This
doesn't encrypt anything by itself — it's a requirement on that one
attribute, telling the GATT server: "before allowing a write to this
attribute, check that the connection is already encrypted; if not, reject
the write."

**What actually makes the connection encrypted?** Encryption is the
result of **pairing** — a separate procedure the BLE stack runs,
involving key exchange between the board and the phone.

Flow: pairing starts → the stack calls `auth_passkey_display()`, which
generates a 6-digit passkey for the session → the passkey is entered on
the phone in nRF Connect → pairing completes → writing to Power is now
permitted.

**Result:** after connecting to the board and attempting to write `01` to
Power, the BLE Host requests pairing. The passkey is printed on the
board's serial terminal; once entered on the phone, the connection becomes
encrypted, and characteristic values can be written successfully.
