# Phase 1 — Advertising: Detailed Step-by-Step Implementation

Goal: your board turns on, starts advertising as a connectable device named
"SmartLight", and your phone can see it in nRF Connect for Mobile's scanner
with a CONNECT button. Nothing else yet — no connection handling, no GATT
service. Just "I exist, come find me."

This mirrors Lesson 2's structure: Exercise 1 = build advertising data +
start non-connectable advertising. Exercise 2 = manual advertising
parameters. Exercise 3 = (typically) scan response / verifying with a
sniffer or scanner app. We're combining that into one connectable
peripheral, since your end goal (Phase 2+) needs a connectable device from
the start.

---

## Step 0 — What has to exist before advertising can start

Before you call anything BLE-related, the Bluetooth subsystem itself has to
be initialized. This is `bt_enable()`. Nothing else in the whole project —
not advertising, not GATT, not pairing — works until this call succeeds.

Why this matters: `bt_enable()` starts up the Host (GAP/GATT/ATT/SMP/L2CAP)
and hands control to the Controller underneath it. If you try to advertise
before this, you'll get an error return code, not a crash — so always check
the return value.

**In `main.c`:**

```c
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

int main(void)
{
    int err;

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return err;
    }

    printk("Bluetooth initialized\n");

    /* advertising setup goes here — Step 3 */

    return 0;
}
```

The `NULL` argument is a ready-callback — you can pass a function to run
once Bluetooth is ready asynchronously, but for a simple peripheral, passing
`NULL` and checking the return code synchronously is enough (this is also
what the course does in Lesson 1/2 exercises).

**Why `printk` and not `LOG_INF`?** Either works if logging is configured,
but `printk` requires zero extra Kconfig and is what the earliest course
exercises use. You can switch to the logging subsystem later once you add
`CONFIG_LOG=y` — not needed for Phase 1.

---

## Step 1 — Build the advertising data array (the AD structures)

Recall from Lesson 2: an advertising packet's payload (`AdvData`) is made of
one or more **AD structures**, each being `Length | Type | Data`. In Zephyr,
each AD structure is one `struct bt_data` entry, and you build them with
helper macros instead of hand-packing bytes.

You need exactly two AD structures for now:

1. **Flags** — tells scanners "this device does not support BR/EDR (Classic
   Bluetooth), it's BLE-only." This is mandatory for any connectable/
   discoverable advert; every exercise in Lesson 2 includes it.
2. **Complete Local Name** — so "SmartLight" shows up in the scanner instead
   of just a MAC address.

**In `main.c`, above `main()`:**

```c
#include <zephyr/bluetooth/bluetooth.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};
```

**Why `CONFIG_BT_DEVICE_NAME` and not a literal `"SmartLight"` string
here?** You already set `CONFIG_BT_DEVICE_NAME="SmartLight"` in `prj.conf`.
Reusing that macro means the name only has to be changed in one place
(`prj.conf`) if you ever rename the device — this is the same pattern the
course uses.

**Why `sizeof(DEVICE_NAME) - 1`?** `DEVICE_NAME` is a C string, so
`sizeof()` includes the trailing `'\0'` terminator. BLE name fields don't
use null-terminators — the length byte IS the length — so you subtract 1.
Forgetting this is a classic bug: you'd advertise one garbage byte after
your name.

**A note on what's NOT here yet:** the guide's Phase 1 step 3 also mentions
including your custom service's 128-bit UUID in `ad[]`. **Skip that for
now.** You haven't defined `BT_UUID_SMARTLIGHT_VAL` yet (that's Phase 3.1) —
trying to reference it now would be a compile error. Two AD structures
(Flags + Name) are enough to prove advertising works. You'll add the UUID
AD structure as a one-line addition once Phase 3.1 exists — I'll remind you
then.

---

## Step 2 — Manually configure advertising parameters

The guide explicitly says: don't use the `BT_LE_ADV_CONN` preset, define
your own `bt_le_adv_param`, exactly like Lesson 2 Exercise 2 did with
`BT_LE_ADV_PARAM`.

**Why manual params instead of the preset?** The preset uses default
timing values you don't control or fully see. Setting them explicitly means
you know exactly what interval you're advertising at — which matters
because advertising interval is a direct tradeoff between discoverability
speed and power consumption, something you already covered in Lesson 2.

**In `main.c`, above `main()`:**

```c
static const struct bt_le_adv_param *adv_param =
    BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
                     400,  /* Min advertising interval: 400 * 0.625ms = 250 ms */
                     401,  /* Max advertising interval: 401 * 0.625ms = 250.625 ms */
                     NULL); /* No specific peer address (undirected) */
```

Breaking down each argument:

- **`BT_LE_ADV_OPT_CONN`** — this is the option flag that makes the advert
  connectable (equivalent to `ADV_IND` from Lesson 2's advertising types).
  Compare this to a beacon exercise using `BT_LE_ADV_OPT_NONE`, which
  produces `ADV_NONCONN_IND` — not connectable. This one flag is the
  difference between "broadcast only" and "peripheral phones can connect
  to."
- **Min/Max interval (400/401)** — BLE advertising interval units are
  0.625 ms each (this is a fixed unit from the spec, same as connection
  interval's 1.25ms unit you learned in Lesson 3 — different base unit,
  same idea of "raw value × fixed tick"). `400 × 0.625ms = 250ms`. Giving a
  min/max range instead of one fixed value lets the Controller add small
  random jitter to avoid periodic collisions with other devices — this is
  also spec-mandated behavior, not a Zephyr quirk.
- **`NULL` (last arg)** — this is the peer address, only used for directed
  advertising (`ADV_DIRECT_IND`, aimed at one known device). You're doing
  undirected advertising (any scanner can see it), so it's `NULL` — same as
  Lesson 2's undirected exercises.

---

## Step 3 — Start advertising

**Back in `main()`, after the `bt_enable()` block:**

```c
    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return err;
    }

    printk("Advertising successfully started as %s\n", DEVICE_NAME);
```

Arguments explained:

- `adv_param` — the struct from Step 2.
- `ad, ARRAY_SIZE(ad)` — your advertising data array and its length. Zephyr
  packs these into the actual `AdvData` bytes for you — you never touch raw
  byte offsets, unlike doing this by hand.
- `NULL, 0` — this is the **scan response data** slot (data + length). You
  have none, because scan response is optional and only relevant for
  `ADV_SCAN_IND`/`ADV_IND` when a scanner sends a `SCAN_REQ` for extra data.
  Not needed for a basic connectable advert.

At this point your full `main.c` should have: includes → `ad[]` array →
`adv_param` → `main()` calling `bt_enable()` then `bt_le_adv_start()`.

---

## Step 4 — Build and flash

1. Build for `nrf5340dk/nrf5340/cpuapp` (application core — BLE runs on the
   app core using the built-in Zephyr BLE stack via the network core's
   controller; you don't need to touch the net core image manually for this
   sample-style project, the build system handles that).
2. Flash to the DK.
3. Open the serial terminal (e.g. via `west build -t debug` or a terminal
   emulator on the DK's VCOM port) — confirm you see:
   ```
   Bluetooth initialized
   Advertising successfully started as SmartLight
   ```

---

## Step 5 — Verify with nRF Connect for Mobile

1. Open nRF Connect for Mobile, tap Scan.
2. Find "SmartLight" in the list.
3. Confirm there's a **CONNECT** button available — this proves
   `BT_LE_ADV_OPT_CONN` worked (a non-connectable advert shows no connect
   option).
4. Tap on the device entry (before connecting) to inspect the raw
   advertising data — you should see the Flags AD structure and the
   Complete Local Name AD structure listed. This is a good sanity check
   that your `ad[]` array actually produced what you intended — you're
   reading back the exact AD structures from Lesson 2, just now generated
   by your own code instead of a hex dump exercise.

**Do not tap CONNECT yet** — Phase 2 (connection callbacks) isn't
implemented, so nothing bad will happen if you do, but there's nothing to
observe yet either. Save that test for Phase 2.

---

## What else is needed besides advertising code itself (general checklist)

- **`prj.conf`**: `CONFIG_BT=y`, `CONFIG_BT_PERIPHERAL=y`,
  `CONFIG_BT_DEVICE_NAME="SmartLight"` — you already have these. Nothing
  else is required specifically for Phase 1.
- **`CMakeLists.txt`**: only needs `src/main.c` for this phase — `light.c`/
  `light.h` aren't used yet (that starts in Phase 3), but there's no harm
  having the empty files already in the tree.
- **No `light.h`/`light.c` content needed yet.** Advertising is entirely
  self-contained in `main.c` for this phase. Don't pull in service UUIDs
  yet — see the note at the end of Step 1.
- **LED1 behavior**: on the nRF5340 DK, the Bluetooth advertising state is
  usually reflected by LED1 blinking, *if* you've wired that up via
  `dk_leds_init()` + a state-change callback — but that is NOT automatic
  from `bt_le_adv_start()` alone. If the guide's Step 6 test ("confirm LED1
  blinks") is important to you now, you'd need `dk_leds_init()` from the DK
  library and a small blink loop — but I'd suggest treating that as
  optional/cosmetic and relying on the serial log + phone scanner as your
  real verification for Phase 1, since LED1 blinking isn't part of the BLE
  stack itself.

---

## Self-check before moving to Phase 2

Answer these to yourself (or ask me if unsure):

1. What would happen (functionally) if you swapped `BT_LE_ADV_OPT_CONN` for
   `BT_LE_ADV_OPT_NONE`? What advertising type would that produce, and
   would "SmartLight" still be visible in a scanner?
2. Why does the min/max advertising interval use a *range* (400, 401)
   instead of Zephyr just taking one fixed value?
3. If you forgot `- 1` in `DEVICE_NAME_LEN`, would the build fail, or would
   it compile and misbehave at runtime? Why?
