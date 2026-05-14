# Smart Bike Lock – Design Logbook

**Project**: Smart Bike Lock  
**Team**: Noah Zacharia, Peter Luster, Kirollos Melek  
**Repository**: `NoahZeee/Smart-Bikelock`

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Phase 1 – Concept & Requirements](#2-phase-1--concept--requirements)
3. [Phase 2 – RFID Concept (Abandoned)](#3-phase-2--rfid-concept-abandoned)
4. [Phase 3 – Bluetooth Low Energy (BLE) Implementation](#4-phase-3--bluetooth-low-energy-ble-implementation)
5. [Phase 4 – Pivot to WiFi Hotspot](#5-phase-4--pivot-to-wifi-hotspot)
6. [Phase 5 – Hardware Design & Custom PCB](#6-phase-5--hardware-design--custom-pcb)
7. [Phase 6 – Firmware Architecture & Refinement](#7-phase-6--firmware-architecture--refinement)
8. [Phase 7 – Power Management & Deep Sleep](#8-phase-7--power-management--deep-sleep)
9. [Phase 8 – Testing & Validation](#9-phase-8--testing--validation)
10. [Lessons Learned](#10-lessons-learned)
11. [Open Issues & Future Work](#11-open-issues--future-work)

---

## 1. Project Overview

The Smart Bike Lock is an embedded security device built on an ESP32 microcontroller. It controls a 28BYJ-48 stepper motor to physically lock and unlock a bike, and provides wireless access via a mobile-friendly web interface. The lock operates as a self-contained WiFi Access Point — no internet connection or cloud backend is required.

**Final Feature Set:**
- WiFi hotspot (`BikelockAP`) served by the ESP32 itself
- Responsive web UI embedded directly in firmware
- Password-protected lock/unlock via HTTP REST API
- SPIFFS-based persistence (password + lock state survive power cycles)
- Deep sleep after 1 minute of inactivity; wakes on external GPIO interrupt
- Custom PCB with battery management, buck-boost regulator, and stepper driver

---

## 2. Phase 1 – Concept & Requirements

### What We Wanted
- A wireless bike lock that could be controlled from a smartphone
- No need for physical keys or codes on the lock body itself
- Low power consumption for battery operation
- Works outdoors without internet infrastructure

### Initial Requirements Defined
| Requirement | Detail |
|---|---|
| Wireless control | Lock/unlock from a phone |
| Password protection | Prevent unauthorized unlocking |
| Persistent state | Lock stays locked across power cycles and sleep |
| Battery-friendly | Deep sleep mode to extend battery life |
| Offline operation | No cloud dependency |
| Physical actuation | Stepper motor to move lock bolt |

### What Worked
- Clearly defining offline-first operation early saved time — we never had to build or maintain a backend server.
- Targeting a stepper motor (28BYJ-48 with ULN2003A driver) was a cost-effective choice that gave precise control over lock position.

---

## 3. Phase 2 – RFID Concept (Abandoned)

### What Was Tried
Early in the design process, RFID was explored as the primary authentication mechanism. The KiCad schematic files are still named `RFID_bikelock.*`, a relic of this phase.

The idea was to use an RFID card reader to authenticate the user at the lock itself — swipe a card, the motor turns.

### Why It Was Abandoned
- **Single-user limitation**: RFID cards are tied to a specific lock; sharing access or revoking it requires physical card management.
- **No remote control**: RFID only works when you are standing at the lock. There is no way to let someone in remotely.
- **Added complexity**: Adding an RFID module meant more hardware, more wiring, and more code for marginal gain over a simpler PIN-based or phone-based system.
- **Wireless is better**: Using the ESP32's built-in WiFi/BLE radio for authentication leverages existing hardware and enables remote access.

### Lesson
> Validate whether a feature delivers real user value before designing hardware around it. RFID solved physical access but not remote access, so it was cut early.

---

## 4. Phase 3 – Bluetooth Low Energy (BLE) Implementation

### What Was Built
The first complete wireless implementation used **BLE (Bluetooth Low Energy)** via the Web Bluetooth API. A separate hosted web app (HTML + CSS + JavaScript) ran in the phone's browser and connected to the ESP32 over GATT.

**BLE Architecture:**
- ESP32 advertised as `"Bike Lock"` with a custom GATT service
- Service UUID: `1ea28c9d-23ce-4f5b-9290-8b72317b97c3`
- Command characteristic (WRITE-only): send `SET`, `UNLOCK`, `LOCK`, `STATUS`, `RESET`
- Status characteristic (READ + NOTIFY): receive `LOCKED`, `UNLOCKED`, `WRONG_PASS`, `RESET`, `INITIALIZING`

**Web app features (BLE phase):**
- Mobile-first, dark-mode-aware UI
- Real-time status updates via BLE notifications
- Disconnect/reconnect handling
- `INITIALIZING` state handling (wait 500 ms and retry read)
- Deep sleep timeout: 30 seconds of BLE inactivity

### What Worked
- The BLE command/notify protocol was clean and responsive.
- The state machine in `app.js` (`updateLockDisplay`) correctly handled all status codes including `WRONG_PASS` and `ERROR_NO_PASSWORD` without corrupting lock state.
- The Web Bluetooth API worked well on Android Chrome and macOS Chrome.
- SPIFFS persistence was already in place for password and lock state.

### What Did Not Work

| Problem | Detail |
|---|---|
| **iOS incompatibility** | Web Bluetooth API is not natively supported on iOS Safari. Works only on iOS 17+ with an app wrapper, which most users don't have. |
| **HTTPS requirement** | Web Bluetooth requires a secure (HTTPS) origin or localhost. Hosting the app on plain HTTP blocks BLE access in the browser. |
| **30-second sleep too aggressive** | After going to sleep, the BLE stack needed to fully reinitialize. Users had to press a wakeup button and wait ~1–2 seconds before the app could reconnect, causing UX friction. |
| **BLE range** | Effective range was ~10 m outdoors, which is acceptable, but obstacles (metal bike frames, walls) degraded reliability. |
| **`INITIALIZING` status race** | On first connect, the status characteristic briefly returned `INITIALIZING` before the device was ready. Required a polling workaround in the web app. |
| **BLE module firmware size** | Including the full BLE stack in the ESP32 firmware consumed significant flash, limiting available space for other features. |

### Lesson
> Platform coverage is a hard constraint that should be evaluated before choosing a wireless protocol. Web Bluetooth's iOS gap meant the product would not work for ~50% of smartphone users without additional native app work. WiFi HTTP is universally supported.

---

## 5. Phase 4 – Pivot to WiFi Hotspot

### The Decision
After the BLE phase hit the iOS wall and HTTPS hosting complexity, the team pivoted to a **WiFi Access Point (AP)** model. The ESP32 runs its own WiFi hotspot (`BikelockAP`), and users connect to it directly and visit `http://192.168.4.1` in any browser.

### Why WiFi Hotspot
- **Universal browser support**: Works on iOS Safari, Android Chrome, any desktop browser — no special APIs required.
- **No internet dependency**: The phone connects directly to the lock's WiFi network. No router, no cloud.
- **HTTP REST is simple**: `GET /status`, `POST /unlock`, `POST /lock`, `POST /set-password`, `POST /reset` — trivial to implement and debug.
- **Embedded web UI**: The entire UI can be stored as a C string in firmware (`web_ui.h`, served from `PROGMEM`), eliminating the need to host a separate web app.

### What Changed
- Removed BLE stack entirely (`btStop()` called on sleep, no BLE init in setup).
- Added `WiFi.softAP()` for AP mode.
- Added `WebServer` (ESP32 Arduino library) on port 80.
- All HTTP handlers registered in `wifi_server.cpp` and `http_handlers.cpp`.
- Web UI inlined in `include/web_ui.h` as a raw string literal in `PROGMEM`.
- Manual JSON parsing implemented in `http_handlers.cpp` (no ArduinoJson library) to save flash.

### What Worked
- Open WiFi network (no password on the AP itself) makes it easy to join — users just connect and visit the IP.
- Embedded web UI loads instantly; no external hosting needed.
- REST API is easy to test with `curl` during development.
- iOS, Android, and desktop all worked on the first try.

### What Did Not Work / Required Tuning

| Problem | Detail |
|---|---|
| **Firmware size** | With WiFi stack + Web UI + SPIFFS, firmware reached **1.01 MB (77% of 4 MB flash)**. Had to remove ArduinoJson to stay within limits. |
| **Manual JSON parsing fragility** | The hand-rolled JSON parser (`indexOf("\"password\"")`) breaks on unusual input formatting (e.g., extra spaces, escaped characters). |
| **Open AP security** | Anyone within WiFi range can connect to `BikelockAP`. The only protection is the lock password. If an attacker is close enough, they can attempt brute-force over HTTP. |
| **Deep sleep + WiFi restart** | After waking from deep sleep, the WiFi AP and HTTP server must fully reinitialize, which takes ~1–2 seconds. Users may see a brief "no connection" period. |
| **IP address hardcoded** | The AP IP (`192.168.4.1`) is the ESP32 default and works, but users must remember or bookmark it. |

### Lesson
> When the target device diversity is broad (iOS + Android + desktop), HTTP over WiFi is almost always a safer default than BLE or NFC, even if it requires the user to switch WiFi networks.

---

## 6. Phase 5 – Hardware Design & Custom PCB

### What Was Designed
A custom PCB was designed in **KiCad** to move the project from breadboard prototype to a more reliable form factor. All Gerber files are in `hardware/manufacturing/`.

**Key Components on the PCB (from BOM):**

| Ref | Part | Purpose |
|---|---|---|
| U2 | ESP32-S3-WROOM-1-N4 | Main microcontroller (240 MHz, 4 MB flash, WiFi/BLE) |
| U1 | ULN2003A | Darlington transistor array to drive stepper motor |
| U4 | TPS63002 | Buck-boost DC-DC converter (battery to 3.3 V/5 V) |
| U6 | MCP73871-2CCI | Li-Po battery charger with power-path management |
| U3 | ADPL40502AUJZ-3.3 | LDO 3.3 V regulator |
| J1 | 2-pin header | Battery input |
| J2 | Micro-USB | Programming / charging port |
| J3 | 5-pin header | Stepper motor output |
| SW1 | EN button | ESP32 enable/reset |
| SW2 | BOOT button | ESP32 boot mode for programming |
| SW3 | WAKE button | External wakeup from deep sleep |
| D3/D5/D6/D7 | LEDs | ESP-PWR, Pwr_Good, STAT1, STAT2 indicators |
| TP1–TP4 | Test points | Vsys, GND, 3V3, 5V for debugging |

### What Worked
- Using the ESP32-S3-WROOM-1 (SMD module) kept the footprint compact while maintaining full WiFi capability.
- The TPS63002 buck-boost ensured stable voltage as the battery drains, preventing brownout resets.
- The MCP73871 handled both USB charging and load sharing — the lock runs off USB when connected and switches to battery seamlessly.
- Adding four labeled test points (Vsys, GND, 3V3, 5V) saved significant debugging time during bring-up.
- Dedicated BOOT, EN, and WAKE buttons on the PCB removed the need for jumper wires during testing.

### What Did Not Work / Challenges

| Problem | Detail |
|---|---|
| **Schematic name mismatch** | Files are named `RFID_bikelock.*` from the abandoned RFID phase. This caused confusion when sharing files. |
| **ULN2003A package** | The ULN2003A was placed in a CERDIP-16 through-hole package. This is large and limits PCB miniaturization in future revisions. Consider switching to a smaller SMD driver. |
| **NPTH vs PTH drill files** | Both plated (PTH) and non-plated (NPTH) drill files are present; ensuring the PCB manufacturer uses both correctly required explicit instruction. |
| **Hand-solder pads** | Some capacitor footprints use `_HandSolder` variants. This was deliberate for prototype assembly but not ideal for future automated pick-and-place. |

### Lesson
> Add test points to every power rail and signal line you plan to debug. The time spent adding four solder pads to the PCB was paid back immediately during first power-on.

---

## 7. Phase 6 – Firmware Architecture & Refinement

### Modular Code Structure
The firmware was deliberately split into small, single-responsibility modules to make testing and debugging easier:

```
src/
├── main.cpp          — Setup/loop, deep sleep logic
├── wifi_server.cpp   — AP mode init, HTTP server start
├── http_handlers.cpp — REST endpoint handlers
├── commands.cpp      — Command parsing and execution
├── lock_control.cpp  — Stepper motor control + state persistence
├── flash_storage.cpp — SPIFFS read/write for password and lock state
└── globals.cpp       — Global variable definitions

include/
├── config.h          — All pin numbers, timing, WiFi config
├── globals.h         — Shared extern declarations
├── web_ui.h          — Entire HTML/CSS/JS UI as a PROGMEM string
└── ...               — One .h per .cpp module
```

### Design Decisions Made

**Removed ArduinoJson:**  
ArduinoJson is a popular C++ JSON library for Arduino. It was initially considered but removed because including it pushed firmware size over target. Manual `indexOf`-based parsing was implemented instead. This saved ~40–80 KB of flash.

**PROGMEM for Web UI:**  
The full web UI (HTML + CSS + JavaScript) is stored in `PROGMEM` as a raw string literal in `web_ui.h`. This keeps it in flash rather than RAM, which is important because the ESP32 has only 320 KB of RAM.

**SPIFFS Partition Configuration:**  
`platformio.ini` defines custom SPIFFS start and size:
```ini
board_build.spiffs_start = 0x2B0000
board_build.spiffs_size  = 0x150000   ; ~1.3 MB for SPIFFS
```

**Security Guard on `SET` Command:**  
The `commands.cpp` module enforces: the password can only be changed while the lock is **unlocked**. This prevents an attacker who gains temporary HTTP access from changing the password while the owner is locked out.

### What Worked
- Splitting modules made it easy to isolate bugs (e.g., flash storage issues were isolated to `flash_storage.cpp` without touching motor code).
- Embedding the UI in `web_ui.h` eliminated the SPIFFS file-serving complexity and made the firmware self-contained.
- `config.h` as a single configuration header meant all pin assignments and timeouts could be changed in one place.

### What Did Not Work / Technical Debt

| Issue | Detail |
|---|---|
| **Password stored in plain text** | `/password.txt` on SPIFFS contains the raw password string. Acknowledged in the BLE-era docs: *"Future Improvement: Consider AES encryption for production use."* |
| **`processCommand` converts to uppercase** | `cmd.toUpperCase()` in `commands.cpp` means passwords are case-insensitive at the command level, which reduces the effective password space. |
| **No CORS headers** | The HTTP server does not set `Access-Control-Allow-Origin` headers. This is fine for same-origin requests from the embedded UI, but would break cross-origin requests from other apps. |
| **Deep sleep wakeup not fully tested** | A `// TODO test deep sleep wakeup functionality` comment remains in `main.cpp`. The wakeup path was configured (EXT0 on GPIO18, HIGH trigger) but integration testing was incomplete. |
| **`WiFi.softAP(WIFI_SSID, nullptr)`** | Passing `nullptr` as the password creates an open network, which is correct for usability but is a known security trade-off. |

### Lesson
> Technical debt comments (`// TODO`) should include a ticket/issue number. A bare TODO comment with no owner or tracking rarely gets resolved before release.

---

## 8. Phase 7 – Power Management & Deep Sleep

### Goal
Extend battery life by putting the ESP32 into deep sleep when no user is interacting with the lock. Deep sleep current is <1 mA vs ~100 mA active — a 100× improvement.

### Implementation
- **Inactivity timer**: `lastActivityTime` is reset on every HTTP request. After `INACTIVITY_TIMEOUT_MS` (1 minute, configurable in `config.h`) with no requests, the device enters deep sleep.
- **Pre-sleep**: WiFi is turned off (`WiFi.mode(WIFI_OFF)`), Bluetooth stopped (`btStop()`), serial flushed.
- **Sleep entry**: `esp_deep_sleep_start()`.
- **Wakeup**: `esp_sleep_enable_ext0_wakeup(GPIO18, HIGH)` — pulling GPIO18 high (button press) wakes the device.
- **Wake reason logging**: On boot, `esp_sleep_get_wakeup_cause()` is checked and logged to serial.

### Timeout Iterations

| Iteration | Timeout | Reason for Change |
|---|---|---|
| BLE phase | 30 seconds | Aggressive to save battery; caused too many unwanted disconnects |
| Early WiFi phase | 5 minutes | Noted in README as default; comfortable UX margin |
| Architecture doc | 1 minute | Noted in `config.h` and architecture diagram; current setting |

The discrepancy between the README (5 minutes) and the actual `config.h` (1 minute) indicates the timeout was tuned during development and documentation was not kept in sync.

### What Worked
- `INACTIVITY_TIMEOUT_MS` defined as a macro makes it trivial to adjust without hunting through code.
- WiFi-off + btStop before sleep reliably reduced deep sleep current.
- EXT0 wakeup on GPIO18 is straightforward and works without any application code running (hardware-level wakeup).

### What Did Not Work

| Problem | Detail |
|---|---|
| **Wakeup not fully tested** | `// TODO test deep sleep wakeup functionality` in `main.cpp`. The wakeup configuration looks correct but end-to-end testing was deferred. |
| **RTC memory for WiFi config** | README mentions "Uses RTC memory to preserve WiFi config across sleep" but this is not visible in the committed code. May have been planned and not yet implemented. |
| **Documentation mismatch** | README says 5-minute timeout; `config.h` says 1 minute. Needs reconciliation. |
| **Stepper motor current during sleep** | The stepper motor coils retain current when held at a position. If the ULN2003A is not explicitly powered off, the motor draws current even when the ESP32 is sleeping. Motor power management during sleep was not addressed in the current firmware. |

### Lesson
> Test deep sleep and wakeup as early as possible — it is the most disruptive change to the execution model. Code that runs fine in a continuous loop may behave differently after a full hardware reset (which is what deep sleep wakeup is).

---

## 9. Phase 8 – Testing & Validation

### Test Artifacts
The `design_docs/` directory contains three documents from the testing phase:
- `Smart Bike Lock Logbook.docx` — ongoing team logbook
- `Midterm Presentation Notes.docx` — notes from midterm design review
- `Midterm testing.docx` — testing results and observations

### Known Test Results

**HTTP API (manual testing with a browser/curl):**
- `GET /status` → returns `{"status":"LOCKED"}` or `{"status":"UNLOCKED"}` ✅
- `POST /unlock` with correct password → motor rotates, state changes to UNLOCKED ✅
- `POST /unlock` with wrong password → motor does not rotate, state stays LOCKED ✅
- `POST /lock` after password is set → motor rotates, state changes to LOCKED ✅
- `POST /set-password` while unlocked → password saved, device locks ✅
- `POST /reset` → password cleared, device unlocks ✅
- `POST /lock` with no password set → returns error `"No password set"` ✅

**Persistent State:**
- Power cycle after locking: device boots, loads state from SPIFFS, motor rotates to locked position ✅
- Password survives power cycle ✅

**Web UI:**
- Loaded on iOS Safari (after WiFi hotspot connection) ✅
- Loaded on Android Chrome ✅
- Button states (lock/unlock disabled appropriately) ✅
- 5-second auto-refresh of status ✅

**Known Failures / Untested:**
- Deep sleep wakeup via GPIO18 — deferred ⚠️
- Motor coil power during sleep ⚠️
- Password brute-force protection — no rate limiting ⚠️
- Firmware upload after WiFi-off (requires COM port reconnection) — noted in troubleshooting ⚠️

### Troubleshooting Encountered During Development

| Symptom | Root Cause | Fix |
|---|---|---|
| Web UI not loading | Phone not connected to `BikelockAP` | Connect to hotspot first, then visit `192.168.4.1` |
| Unlock button does nothing | No password set yet | Use "Set Password" first |
| Firmware upload fails | Serial monitor still open, blocking COM port | Close serial monitor before upload |
| SPIFFS errors on first boot | Partition not formatted | `SPIFFS.begin(true)` with `formatOnFail=true` handles this automatically |
| Motor rotates wrong direction | Wiring polarity on stepper connector | Swap coil wires; `STEPPER_UNLOCK_STEPS` uses negative value (`-1024`) |

---

## 10. Lessons Learned

### L1 – Validate Platform Coverage Before Choosing a Protocol
**What happened**: The team spent significant effort building a polished BLE interface with a well-structured state machine, only to discover that ~50% of target users (iOS Safari) could not use it without a native app wrapper.  
**Lesson**: Before committing to a wireless protocol, list the target devices and browsers and confirm the protocol is natively supported on all of them. Web Bluetooth is Chromium-only. HTTP is universal.

---

### L2 – Abandon Early Concepts Without Sentimentality
**What happened**: RFID was an early concept that made it into the KiCad schematic file names (`RFID_bikelock.*`) even after the concept was dropped. This caused ongoing confusion.  
**Lesson**: When a design concept is abandoned, rename affected files and clean up references immediately. Stale names become misleading documentation.

---

### L3 – Embedded Firmware Size Is a Real Constraint
**What happened**: ArduinoJson had to be removed because the firmware was too large. Manual JSON parsing was written as a workaround.  
**Lesson**: On constrained targets (ESP32 with 4 MB flash, but 77% already used), evaluate library footprints before adding dependencies. For simple fixed-format JSON, a few lines of `indexOf` is often sufficient and saves significant space.

---

### L4 – Never Store Passwords in Plain Text
**What happened**: The password is saved to `/password.txt` on SPIFFS as a plain string. The BLE-era documentation explicitly flagged this as a known security issue ("Future Improvement: Consider AES encryption").  
**Lesson**: Password hashing should be implemented from the start, not deferred. On ESP32, SHA-256 is available in the mbedTLS library that ships with the ESP-IDF. The fix is straightforward and should not be deferred to a "future improvement."

---

### L5 – Password Case-Sensitivity Is Lost by `toUpperCase()`
**What happened**: `processCommand` calls `cmd.toUpperCase()` before parsing, which converts the entire command string — including the password — to uppercase. A password of `"myPass123"` becomes `"MYPASS123"`, reducing the effective character set.  
**Lesson**: When parsing commands that include user-provided strings (passwords, names), only uppercase the command keyword, not the full argument. This is a correctness bug with security implications.

---

### L6 – Test Deep Sleep and Wakeup Early
**What happened**: Deep sleep was one of the last things integrated, and the `// TODO test deep sleep wakeup functionality` comment was never resolved before the repository was submitted.  
**Lesson**: Deep sleep fundamentally changes the execution model (full hardware reset on wake). Any code that assumes persistent state in RAM (not SPIFFS or RTC memory) will break. This should be tested in the first hardware bring-up session, not deferred to the end.

---

### L7 – Keep Documentation in Sync with Code
**What happened**: The README states a 5-minute inactivity timeout; `config.h` has it set to 1 minute. The discrepancy suggests the code was changed without updating the README.  
**Lesson**: Configuration constants that are user-visible (like sleep timeout) should be documented with a single source of truth, ideally by referencing `config.h` in the README rather than duplicating the value.

---

### L8 – Add Test Points to Every Rail You Will Debug
**What happened**: The custom PCB includes test points TP1 (Vsys), TP2 (GND), TP3 (3V3), and TP4 (5V). This was a deliberate design choice.  
**Lesson**: This was the right call. First PCB bring-up almost always requires probing power rails to confirm regulators are working before any firmware runs. Test points cost almost nothing (a pad on the PCB) and save hours of debug time.

---

### L9 – The Stepper Motor Holds Current When Not Explicitly Released
**What happened**: The 28BYJ-48 stepper motor holds its last coil state after a `stepper.step()` call. During deep sleep, the ESP32 powers down but the ULN2003A output state may remain, keeping motor coils energized and draining the battery.  
**Lesson**: Before entering deep sleep, explicitly zero out the stepper motor coil outputs (drive all motor GPIO pins LOW) to cut the holding current. This is a non-obvious hardware interaction that should be in the power management checklist.

---

### L10 – Open WiFi + HTTP = Low-Cost Attack Surface
**What happened**: The lock AP (`BikelockAP`) has no WiFi password. Anyone nearby can connect and attempt HTTP requests. There is no rate limiting on `/unlock`, so password brute-force is possible if the attacker is within WiFi range.  
**Lesson**: For a security device, consider: (1) adding a WiFi AP password so only the owner can even reach the HTTP server, and (2) implementing a failed-attempt lockout (e.g., 5 wrong passwords → 60-second lockout). Both are straightforward additions.

---

## 11. Open Issues & Future Work

| # | Issue | Priority |
|---|---|---|
| 1 | Test and validate deep sleep wakeup via GPIO18 | High |
| 2 | Drive stepper coils to zero before entering deep sleep | High |
| 3 | Replace plain-text password with SHA-256 hash (mbedTLS) | High |
| 4 | Fix `toUpperCase()` bug — only uppercase command keyword, not full string | High |
| 5 | Add rate limiting / lockout on `/unlock` after failed attempts | Medium |
| 6 | Add WiFi AP password option (or document the security trade-off) | Medium |
| 7 | Reconcile README timeout (5 min) with `config.h` (1 min) | Low |
| 8 | Rename KiCad files from `RFID_bikelock.*` to `smart_bikelock.*` | Low |
| 9 | Replace `ULN2003A` with SMD equivalent for PCB miniaturization | Low |
| 10 | Add mobile app (React Native / Flutter) as alternative to browser UI | Low |
| 11 | Implement backend for lost-device tracking (future) | Low |

---

*Logbook last updated: 2026-05-14*
