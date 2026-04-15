# Smart Bike Lock - WiFi Hotspot Edition

A secure, wireless bike lock system controlled via a mobile web interface. Connect from any iOS or Android device to unlock/lock your bike through a dedicated WiFi hotspot.

## Features

- **📱 Mobile Web Interface**: Responsive web UI works on iOS, Android, and any browser
- **🔒 Password Protected**: Set custom passwords for lock control
- **📡 WiFi Hotspot**: Creates a local WiFi network (no internet required)
- **🔋 Deep Sleep**: Automatically sleeps after 5 minutes of inactivity for battery efficiency
- **💾 Persistent State**: Password and lock state saved to flash memory
- **⚡ Fast Control**: HTTP REST API with instant response times
- **🤖 Offline Operation**: No backend server or cloud connection required

## Hardware

- **Microcontroller**: ESP32 DOIT DevKit V1 (240MHz, 320KB RAM, 4MB Flash)
- **Motor**: 28BYJ-48 stepper motor with ULN2003A driver
- **Wakeup**: GPIO18 external interrupt (HIGH level trigger) or button press
- **GPIO Pins**:
  - Motor: 13, 25, 14, 26
  - Wakeup: 18
  - USB: COM6 (configurable in platformio.ini)

## Getting Started

### Prerequisites

- PlatformIO CLI or VS Code with PlatformIO extension
- USB cable for ESP32 upload
- Your smartphone on WiFi

### Building & Uploading

```bash
# Build firmware
platformio run

# Upload to ESP32
platformio run -t upload

# Monitor serial output
platformio run -t monitor
```

### Initial Setup

1. Serial monitor will display initialization sequence
2. Connect to WiFi network: `BikelockAP` (open network, no password)
3. Visit `http://192.168.4.1` from your phone
4. Set a password in the web UI (device will lock automatically)
5. Now you can lock/unlock with your password!

## Web UI Controls

- **Status Display**: Shows current lock state (🔒 LOCKED / 🔓 UNLOCKED)
- **Password Field**: Enter your password
- **Unlock Button**: Unlock the lock (requires correct password)
- **Lock Button**: Lock the device (only works when unlocked)
- **Set Password**: Set a new password and lock device
- **Refresh**: Manually refresh lock status
- **Reset Device**: Factory reset (clears password)

## REST API

All endpoints are available at `http://192.168.4.1`

### GET /status
Returns current lock state
```json
{"status": "LOCKED"}  // or "UNLOCKED"
```

### POST /unlock
Unlock with password
```json
Request: {"password": "your_password"}
Response: {"status": "UNLOCKED"}  // or stays "LOCKED" if password wrong
```

### POST /lock
Lock the device (requires password already set)
```json
Response: {"status": "LOCKED"}
```

### POST /set-password
Set new password and lock device
```json
Request: {"password": "new_password"}
Response: {"status": "LOCKED"}
```

### POST /reset
Factory reset device (clears password, unlocks)
```json
Response: {"status": "UNLOCKED"}
```

## Configuration

Edit `include/config.h` to customize:

- **WiFi Network**: `WIFI_SSID` and `WIFI_PASSWORD` (empty = open network)
- **Deep Sleep Timeout**: `INACTIVITY_TIMEOUT_MS` (default: 5 minutes)
- **Motor Rotation**: `STEPPER_LOCK_STEPS` and `STEPPER_UNLOCK_STEPS`
- **GPIO Pins**: All pin definitions
- **SPIFFS**: Flash partition configuration

## Project Structure

```
src/
├── main.cpp              # Setup and main event loop
├── wifi_server.cpp       # WiFi hotspot initialization
├── http_handlers.cpp     # HTTP endpoint handlers
├── commands.cpp          # Command processing logic
├── lock_control.cpp      # Stepper motor control
├── flash_storage.cpp     # SPIFFS persistence
└── globals.cpp           # Global variable definitions

include/
├── config.h              # Central configuration
├── globals.h             # Global declarations
├── web_ui.h              # Embedded HTML/CSS/JS UI
├── wifi_server.h         # WiFi function declarations
├── http_handlers.h       # HTTP handler declarations
├── commands.h            # Command processing declarations
├── lock_control.h        # Motor control declarations
└── flash_storage.h       # Storage function declarations
```

## How It Works

### Initialization Flow
1. ESP32 boots and mounts SPIFFS filesystem
2. Loads stored password and lock state from flash
3. Initializes stepper motor and rotates to match stored state
4. Enables GPIO18 for external wakeup (5V HIGH trigger)
5. Starts WiFi in Access Point (hotspot) mode
6. HTTP server listening on port 80
7. Enters main loop checking for inactivity

### Unlock Flow
1. User enters password in web UI
2. Browser sends POST to `/unlock` with JSON body
3. Server validates password matches stored value
4. If correct: rotates motor to unlock position, saves state
5. If incorrect: returns current state (still LOCKED)
6. Browser receives response and updates UI accordingly

### Deep Sleep
- Device sleeps after 5 minutes with no WiFi activity
- Can be woken by:
  - GPIO18 going HIGH (button press or external trigger)
  - Web request (resets 5-minute timer)
- Uses RTC memory to preserve WiFi config across sleep

## Firmware Optimization

- **Size**: 1.01 MB (77% of 4MB flash)
- **Memory**: Manual JSON parsing (no ArduinoJson)
- **UI**: Embedded in firmware (no SPIFFS file serving)
- **Removed**: BLE module (replaced with WiFi)

## Troubleshooting

### Web UI not loading
- Ensure you're connected to `BikelockAP` WiFi
- Check that IP is `192.168.4.1`
- Verify serial monitor shows "HTTP Server started on port 80"

### Unlock not working
- Verify password is correct (serial shows "Incorrect password" on failure)
- Check that a password has been set first
- Try Reset Device if needed

### Device not waking from sleep
- Verify GPIO18 connection (should go HIGH on button press)
- Check serial monitor for "Wake reason" message
- May need to press button longer or adjust trigger level

### Firmware upload fails
- Close serial monitor first
- Try `platformio run --target clean` then rebuild
- Check COM port in `platformio.ini`

## Future Enhancements

- [ ] OTA (Over-The-Air) firmware updates
- [ ] Multiple user passwords/permissions
- [ ] Activity logging/history
- [ ] Optional BLE for nearby notifications
- [ ] Tamper detection
- [ ] Location tracking integration

## License

[Your License Here]

## Authors

Noah Zacharia, Peter Luster, Kirollos Melek

