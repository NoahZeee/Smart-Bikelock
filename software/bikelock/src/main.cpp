/*
 * Smart Bike Lock - ESP32 WiFi Hotspot Server
 * 
 * A secure bike lock system with:
 * - WiFi hotspot connectivity for mobile apps
 * - SPIFFS-based password and state persistence
 * - Deep sleep after 30s inactivity
 * - Stepper motor control
 * 
 * Authors: Noah Z, Peter S, Kirollos M
 */

#include <Arduino.h>
#include <config.h>
#include <SPIFFS.h>
#include <esp_sleep.h>

// Module headers
#include <globals.h>
#include <flash_storage.h>
#include <lock_control.h>
#include <commands.h>
#include <wifi_server.h>
#include <http_handlers.h>

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
  
  // Debug: List SPIFFS contents
  Serial.println("SPIFFS Contents:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  int fileCount = 0;
  while (file) {
    Serial.print("  - ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");
    fileCount++;
    file = root.openNextFile();
  }
  if (fileCount == 0) {
    Serial.println("  (No files found in SPIFFS!)");
  }

  // Load persistent state from flash
  storedPassword = loadPasswordFromFlash();
  bool savedLockState = loadLockStateFromFlash();

  // Configure GPIO pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  stepper.setSpeed(STEPPER_SPEED); // Set stepper motor speed (RPM)

  // BLE removed - using WiFi hotspot instead
  Serial.println("\n--- WiFi Mode Enabled (No BLE)");

  // Set initial lock state from saved state
  updateLockState(savedLockState);

  // Configure deep sleep
  Serial.println("\n--- Deep Sleep Configuration ---");
  
  // Configure GPIO18 for external wakeup (HIGH level trigger)
  pinMode(TOUCH_WAKE_PIN_1, INPUT_PULLDOWN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_WAKE_PIN_1, HIGH);
  
  Serial.println("Deep sleep configured:");
  Serial.print("  Primary wake pin: GPIO");
  Serial.print(TOUCH_WAKE_PIN_1);
  Serial.println(" (HIGH level trigger)");
  Serial.print("  Inactivity timeout: ");
  Serial.print(INACTIVITY_TIMEOUT_MS / 1000);
  Serial.println(" seconds");

  // Initialize WiFi in hotspot mode (if enabled)
  if (WIFI_MODE_ENABLED) {
    initializeWiFi();
    startHTTPServer();
  }

  lastActivityTime = millis();
  Serial.println("\n==================================");
  Serial.println("Initialization Complete - Ready!");
  Serial.println("==================================\n");
}

/**
 * Main event loop: Monitor inactivity and manage deep sleep
 */
void loop() {
  // Handle HTTP requests (if WiFi is enabled)
  if (WIFI_MODE_ENABLED) {
    handleHTTPRequests();
  }

  // Check for inactivity timeout and enter deep sleep if needed
  if ((millis() - lastActivityTime) > INACTIVITY_TIMEOUT_MS) {
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
