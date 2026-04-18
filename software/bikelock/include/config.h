#pragma once

/** DEVICE INFO ************************/
#define DEVICE_NAME "Bike Lock"

/** STATUS LED PIN (connection indicator) ************************/
#define LED_PIN 2

/** STEPPER MOTOR CONFIGURATION ************************/
#define STEPPER_PIN_1 13
#define STEPPER_PIN_2 25
#define STEPPER_PIN_3 14
#define STEPPER_PIN_4 26
#define STEPS_PER_REV 2048

// Stepper motor rotation amounts
// Adjust these values based on your lock mechanism
// Positive values = clockwise, negative = counterclockwise
#define STEPPER_LOCK_STEPS 1024      // Steps to rotate when locking (adjust as needed)
#define STEPPER_UNLOCK_STEPS -1024   // Steps to rotate when unlocking (negative of lock)
#define STEPPER_SPEED 10            // RPM speed for stepper motor

/** TOUCH WAKE PINS (for deep sleep) *****/
#define TOUCH_WAKE_PIN_1 18
#define TOUCH_WAKE_PIN_2 15

/** SLEEP CONFIGURATION *****/
#define INACTIVITY_TIMEOUT_MS (1 * 60 * 1000) // 1 minute (60 seconds)

/** SPIFFS CONFIGURATION *****/
#define SPIFFS_MOUNT_POINT "/spiffs"
#define PASSWORD_KEY "bike_lock_pwd"

/** WIFI HOTSPOT CONFIGURATION ************************/
#define WIFI_SSID "BikelockAP"           // WiFi network name
#define WIFI_PASSWORD ""                // WiFi password (empty = open network)
#define WIFI_MODE_ENABLED true           // Enable WiFi hotspot (true/false)
#define HTTP_SERVER_PORT 80              // HTTP port (80 is standard)