#pragma once

/** DEVICE INFO ************************/
#define DEVICE_NAME "Bike Lock"

/** PIN DEFINITIONS ************************/
#define LED_PIN 2
#define RED_PIN 25
#define GREEN_PIN 26
#define NUM_LEDS 1

/** TOUCH WAKE PINS (for deep sleep) *****/
#define TOUCH_WAKE_PIN_1 12
#define TOUCH_WAKE_PIN_2 13

/** SLEEP CONFIGURATION *****/
#define INACTIVITY_TIMEOUT_MS (30 * 1000) // 30 seconds

/** SPIFFS CONFIGURATION *****/
#define SPIFFS_MOUNT_POINT "/spiffs"
#define PASSWORD_KEY "bike_lock_pwd"

/** BLE UUIDs ******************************/
#define BIKELOCK_SERVICE_UUID "1ea28c9d-23ce-4f5b-9290-8b72317b97c3"
#define COMMAND_CHAR_UUID "5a87b3d0-7c7d-4c5b-bf2c-5e7a0f1a0a01"
#define STATUS_CHAR_UUID  "5a87b3d0-7c7d-4c5b-bf2c-5e7a0f1a0a02"