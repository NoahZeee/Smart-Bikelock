/*
 * Smart Bike Lock - ESP32 BLE Server
 * 
 * A secure bike lock system with:
 * - Web Bluetooth API connectivity
 * - SPIFFS-based password and state persistence
 * - Deep sleep after 30s inactivity (GPIO12 button wake)
 * - LED status indicators and optional stepper motor control
 * 
 * Authors: Noah Z, Peter S, Kirollos M
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLE2901.h>
#include <BLE2902.h>
#include <config.h>
#include <SPIFFS.h>
#include <esp_sleep.h>

// Module headers
#include <globals.h>
#include <flash_storage.h>
#include <lock_control.h>
#include <commands.h>
#include <ble_callbacks.h>

/**
 * Initialize the device: SPIFFS, BLE, GPIO, and deep sleep configuration
 */
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n==================================");
  Serial.println("Bike Lock BLE Server - Initializing");
  Serial.println("==================================\n");

  // Check if waking from deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println(">>> WOKE UP FROM DEEP SLEEP (External Pin)");
  } else if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED) {
    Serial.print(">>> WOKE UP FROM DEEP SLEEP (Reason: ");
    Serial.print((int)wakeup_reason);
    Serial.println(")");
  }

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Load persistent state from flash
  storedPassword = loadPasswordFromFlash();
  bool savedLockState = loadLockStateFromFlash();

  // Configure GPIO pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  stepper.setSpeed(STEPPER_SPEED); // Set stepper motor speed (RPM)

  // Initialize BLE
  Serial.println("\n--- BLE Initialization ---");
  BLEDevice::init(DEVICE_NAME);
  Serial.println("BLE Device initialized");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  Serial.println("BLE Server created");

  // Create service and characteristics
  BLEService *pService = pServer->createService(BIKELOCK_SERVICE_UUID);
  Serial.println("BLE Service created");

  // Command characteristic (write-only)
  BLECharacteristic *commandChar = 
    pService->createCharacteristic(COMMAND_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
  commandChar->setCallbacks(new CommandCallbacks());
  BLE2901 *commandDesc = new BLE2901();
  commandDesc->setDescription("Send commands: SET [password], UNLOCK [password], LOCK, STATUS, RESET");
  commandChar->addDescriptor(commandDesc);
  Serial.println("Command characteristic created");

  // Status characteristic (read + notify)
  statusChar = pService->createCharacteristic(
    STATUS_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  statusChar->setValue("INITIALIZING");
  BLE2901 *statusDesc = new BLE2901();
  statusDesc->setDescription("Current Lock Status: [LOCKED] or [UNLOCKED]");
  statusChar->addDescriptor(statusDesc);
  
  // Add CCCD descriptor for notifications
  BLE2902 *statusCCCD = new BLE2902();
  statusChar->addDescriptor(statusCCCD);
  Serial.println("Status characteristic created with CCCD descriptor");

  // Start service and advertising
  pService->start();
  Serial.println("BLE Service started");

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BIKELOCK_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEAdvertisementData adData;
  adData.setName(DEVICE_NAME);
  pAdvertising->setAdvertisementData(adData);
  BLEDevice::startAdvertising();
  Serial.println("BLE Advertising started");
  Serial.print("Service UUID: ");
  Serial.println(BIKELOCK_SERVICE_UUID);

  // Set initial lock state from saved state
  updateLockState(savedLockState);

  // Configure deep sleep
  Serial.println("\n--- Deep Sleep Configuration ---");
  pinMode(TOUCH_WAKE_PIN_1, INPUT_PULLDOWN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_WAKE_PIN_1, 1);
  Serial.println("Deep sleep configured:");
  Serial.print("  Wake pin: GPIO");
  Serial.print(TOUCH_WAKE_PIN_1);
  Serial.println(" (HIGH level trigger)");
  Serial.print("  Inactivity timeout: ");
  Serial.print(INACTIVITY_TIMEOUT_MS / 1000);
  Serial.println(" seconds");

  lastActivityTime = millis();
  Serial.println("\n==================================");
  Serial.println("Initialization Complete - Ready!");
  Serial.println("==================================\n");
}

/**
 * Main event loop: Monitor inactivity and manage deep sleep
 */
void loop() {
  // Check for inactivity timeout and enter deep sleep if needed
  if (!deviceConnected && (millis() - lastActivityTime) > INACTIVITY_TIMEOUT_MS) {
    Serial.println("\n========== ENTERING DEEP SLEEP ==========");
    Serial.print("Last activity: ");
    Serial.print(millis() - lastActivityTime);
    Serial.println(" ms ago");
    Serial.print("To wake up: Press button on GPIO");
    Serial.println(TOUCH_WAKE_PIN_1);
    Serial.println("========================================\n");
    Serial.flush();
    delay(200); // Allow serial to flush completely

    esp_deep_sleep_start();
  }

  delay(1000); // Check every second
}
