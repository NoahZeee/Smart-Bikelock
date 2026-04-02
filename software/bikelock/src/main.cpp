/*
 * Smart Bike Lock - ESP32 BLE Server
 * 
 * This code implements a BLE server on an ESP32 that simulates a smart bike lock. 
 * It allows clients to set a password, lock/unlock the bike, and check the lock status via BLE commands.
 * 
 * Commands sent from phone (lightblue app):
 * - Set Password: SET [password]
 * - Unlock: UNLOCK [attempted password]
 * - Lock: LOCK
 * - Status: STATUS
 * 
 * LED Indicators:
 * - Red LED ON: Bike is locked
 * - Green LED ON: Bike is unlocked
 * 
 * Authors: Noah Z, Peter S, Kirollos M
 * 
 */
 
/* INCLUDES ************************/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLE2901.h>
#include <BLE2902.h>
#include <config.h>
#include <SPIFFS.h>
#include <esp_sleep.h>
#include <Stepper.h>
//#include <Adafruit_NeoPixel.h>


/** GLOBAL VARIABLES ************************/
String storedPassword = "";
bool isLocked = false;

BLECharacteristic *statusChar;
unsigned long lastActivityTime = 0;
bool deviceConnected = false;

Stepper stepper(STEPS_PER_REV, STEPPER_PIN_1, STEPPER_PIN_2, STEPPER_PIN_3, STEPPER_PIN_4);

// Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void updateLockState(bool locked);

/** FLASH STORAGE FUNCTIONS *****/
void savePasswordToFlash(String password) {
  File file = SPIFFS.open("/password.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open password file for writing");
    Serial.flush();
    return;
  }
  file.print(password);
  file.close();
  Serial.println("Password saved to flash");
  Serial.flush();
}

String loadPasswordFromFlash() {
  if (!SPIFFS.exists("/password.txt")) {
    Serial.println("Password file does not exist");
    return "";
  }
  
  File file = SPIFFS.open("/password.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open password file for reading");
    return "";
  }
  
  String password = file.readString();
  file.close();
  Serial.print("Password loaded from flash: ");
  Serial.println(password);
  return password;
}

void deletePasswordFromFlash() {
  if (SPIFFS.remove("/password.txt")) {
    Serial.println("Password deleted from flash");
  } else {
    Serial.println("Failed to delete password file");
  }
}

void saveLockStateToFlash(bool locked) {
  File file = SPIFFS.open("/lock_state.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open lock state file for writing");
    Serial.flush();
    return;
  }
  file.print(locked ? "1" : "0");
  file.close();
  Serial.print("Lock state saved to flash: ");
  Serial.println(locked ? "LOCKED" : "UNLOCKED");
  Serial.flush();
}

bool loadLockStateFromFlash() {
  if (!SPIFFS.exists("/lock_state.txt")) {
    Serial.println("Lock state file does not exist, defaulting to UNLOCKED");
    return false;
  }
  
  File file = SPIFFS.open("/lock_state.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open lock state file for reading");
    return false;
  }
  
  String state = file.readString();
  file.close();
  bool locked = (state == "1");
  Serial.print("Lock state loaded from flash: ");
  Serial.println(locked ? "LOCKED" : "UNLOCKED");
  Serial.flush();
  return locked;
}

// Function to process incoming commands
void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("Parsed Command: ");
  Serial.println(cmd);
  Serial.flush();

  if(cmd.startsWith("SET ")) {
  
    // Security: Only allow setting password when UNLOCKED
    if (isLocked) {
      statusChar->setValue("LOCKED_CANNOT_SET");
      statusChar->notify();
      Serial.println("ERROR: Cannot set password while locked. Unlock device first.");
      Serial.flush();
    } else {
      storedPassword = cmd.substring(4); // Extract password after "SET "
      savePasswordToFlash(storedPassword); // Save to flash

      updateLockState(true); // Engage lock immediately after setting password (handles setValue + notify)

      Serial.println("Password set and lock engaged.");
      Serial.flush();
    }
  }

  else if (cmd.startsWith("UNLOCK ")) {

    String attempt = cmd.substring(7); // Extract password after "UNLOCK "

    if (attempt == storedPassword) {
      updateLockState(false); // Disengage lock (handles setValue + notify)

      Serial.println("Correct password. Lock disengaged.");
      Serial.flush();
    } else {
      statusChar->setValue("WRONG_PASS");
      statusChar->notify();

      Serial.println("Incorrect password attempt.");
      Serial.flush();
    }
  }
  
  else if(cmd.startsWith("LOCK")) {
      // Security: Only allow locking if a password has been set
      if (storedPassword.length() == 0) {
        statusChar->setValue("ERROR_NO_PASSWORD");
        statusChar->notify();
        Serial.println("ERROR: Cannot lock device without a password set. Use SET [password] first.");
        Serial.flush();
      } else {
        updateLockState(true); // Engage lock (handles setValue + notify)
      }
  }

  else if(cmd.startsWith("STATUS")) {

      if(isLocked)
          statusChar->setValue("LOCKED");
      else
          statusChar->setValue("UNLOCKED");

      statusChar->notify();
  }

  else if(cmd.startsWith("RESET")) {
      deletePasswordFromFlash();
      storedPassword = "";
      updateLockState(false); // Unlock after reset - this calls setValue("UNLOCKED") + notify()
      // NOTE: Don't send RESET separately - updateLockState already notified with UNLOCKED
      Serial.println("Password reset. Device unlocked and ready for reconfiguration.");
      Serial.flush();
  }

  // Update last activity time
  lastActivityTime = millis();
}

/** LOCK STATE UPDATE FUNCTION ******************/
void updateLockState(bool locked) {

  isLocked = locked;
  saveLockStateToFlash(locked); // Persist lock state

  String statusValue = locked ? "LOCKED" : "UNLOCKED";
  
  if (isLocked) { // engage lock
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    Serial.println("Lock engaged.");
  } else { // disengage lock
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    Serial.println("Lock disengaged.");
  }
  Serial.flush();
  
  // Update BLE characteristic value
  if (statusChar) {
    Serial.print("Setting BLE characteristic to: ");
    Serial.println(statusValue);
    statusChar->setValue(statusValue.c_str());
    Serial.println("BLE characteristic value set successfully");
  } else {
    Serial.println("ERROR: statusChar is NULL!");
  }
  Serial.flush();

  // Send notification once at the end
  if (statusChar) {
    statusChar->notify();
    Serial.println("BLE notification sent");
  }
  Serial.flush();
}

/** BLE SERVER EVENTS *************************/
class MyServerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer *pServer) {
    // pixel.setPixelColor(0, pixel.Color(0, 255, 0)); // Green
    // pixel.show();
    digitalWrite(LED_PIN, HIGH);
    deviceConnected = true;
    lastActivityTime = millis();

    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer *pServer) {
    // pixel.setPixelColor(0, pixel.Color(0, 0, 0)); // Off
    // pixel.show();
    digitalWrite(LED_PIN, LOW);
    deviceConnected = false;
    lastActivityTime = millis();

    Serial.println("Client disconnected");
    
    BLEDevice::startAdvertising(); // Restart advertising so new clients can connect
  }
};

/** COMMAND CHARACTERISTIC ******************/
class CommandCallbacks : public BLECharacteristicCallbacks {
  
  void onWrite(BLECharacteristic *pChar) {

    String cmd = String(pChar->getValue().c_str());
    cmd.trim();

    if (cmd.length() == 0) return; // Ignore empty writes

    Serial.print("Received Raw Command: ");
    Serial.println(cmd);
    Serial.flush();
    
    lastActivityTime = millis();
    processCommand(cmd);
  }
};

/** SETUP FUNCTION *********************/
void setup() {

  Serial.begin(115200);
  delay(2000);
  Serial.println("Bike Lock BLE Server setup init...");

  // Check if waking from deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println(">>> WOKE UP FROM DEEP SLEEP (External Pin)");
  } else if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED) {
    Serial.print(">>> WOKE UP FROM DEEP SLEEP (Reason: ");
    Serial.print((int)wakeup_reason);
    Serial.println(")");
  }

  // SPIFFS setup
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Load password from flash
  storedPassword = loadPasswordFromFlash();

  // Load lock state from flash (or default to unlocked if not set)
  bool savedLockState = loadLockStateFromFlash();

  // LED setup
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  // pixel.begin();
  // pixel.clear();
  // pixel.show();

  stepper.setSpeed(10); // Set stepper speed

  // BLE setup
  BLEDevice::init(DEVICE_NAME);
  Serial.println("BLE Device initialized");

  // Create Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  Serial.println("BLE Server created");

  // Services
  BLEService *pService = pServer->createService(BIKELOCK_SERVICE_UUID);
  Serial.println("BLE Service created");
    
  // Command Characteristic
  BLECharacteristic *commandChar = 
    pService->createCharacteristic(COMMAND_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    
  commandChar->setCallbacks(new CommandCallbacks());
  Serial.println("Command characteristic created");

  // Status Characteristic
  statusChar =
    pService->createCharacteristic(
      STATUS_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_NOTIFY
    );
  
  // Set initial value for status characteristic (will be overridden by updateLockState in setup)
  statusChar->setValue("INITIALIZING");
  Serial.println("Status characteristic created with initial value: INITIALIZING");
    
  BLE2901 *statusDesc = new BLE2901();
  statusDesc->setDescription("Current Lock Status: [Locked] or [Unlocked]");
  statusChar->addDescriptor(statusDesc);

  // Add CCCD descriptor to enable notifications
  BLE2902 *statusCCCD = new BLE2902();
  statusChar->addDescriptor(statusCCCD);
  Serial.println("BLE2902 CCCD descriptor added to status characteristic");

  BLE2901 *commandDesc = new BLE2901();
  commandDesc->setDescription("Send commands: SET [password], UNLOCK [password], LOCK, STATUS");
  commandChar->addDescriptor(commandDesc);

  // Start Service
  pService->start();
  Serial.println("BLE Service started");

  // Set initial lock state (load from flash)
  updateLockState(savedLockState);

  // Advertising
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

  // Configure GPIO pins for deep sleep wakeup
  // Set GPIO12 as input with pull-down
  pinMode(TOUCH_WAKE_PIN_1, INPUT_PULLDOWN);
  // Use EXT0 on GPIO12 - wakes on HIGH (button press pulls pin high)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_WAKE_PIN_1, 1);
  
  Serial.println("Deep sleep configured:");
  Serial.print("  Wake pin: GPIO");
  Serial.println(TOUCH_WAKE_PIN_1);
  Serial.println("  Trigger: HIGH level (button press)");

  lastActivityTime = millis();
}

void loop() {
  // Check for inactivity timeout and enter deep sleep if needed
  if (deviceConnected == false && (millis() - lastActivityTime) > INACTIVITY_TIMEOUT_MS) {
    Serial.println("\n========== ENTERING DEEP SLEEP ==========");
    Serial.print("Last activity: ");
    Serial.print(millis() - lastActivityTime);
    Serial.println(" ms ago");
    Serial.print("To wake up: Press button on GPIO");
    Serial.println(TOUCH_WAKE_PIN_1);
    Serial.println("========================================\n");
    Serial.flush();
    delay(200); // Allow serial to flush completely

    // Enter deep sleep
    esp_deep_sleep_start();
  }

  delay(1000); // Check every second
}
