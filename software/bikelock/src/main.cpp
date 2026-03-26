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
#include <config.h>
#include <SPIFFS.h>
#include <esp_sleep.h>
//#include <Adafruit_NeoPixel.h>


/** GLOBAL VARIABLES ************************/
String storedPassword = "";
bool isLocked = false;
BLECharacteristic *statusChar;
unsigned long lastActivityTime = 0;
bool deviceConnected = false;

// Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void updateLockState(bool locked);

/** FLASH STORAGE FUNCTIONS *****/
void savePasswordToFlash(String password) {
  File file = SPIFFS.open("/password.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open password file for writing");
    return;
  }
  file.print(password);
  file.close();
  Serial.println("Password saved to flash");
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

// Function to process incoming commands
void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("Parsed Command: ");
  Serial.println(cmd);

  if(cmd.startsWith("SET ")) {
  
    storedPassword = cmd.substring(4); // Extract password after "SET "
    savePasswordToFlash(storedPassword); // Save to flash

    updateLockState(true); // Engage lock immediately after setting password

    statusChar->setValue("LOCKED");
    statusChar->notify();

    Serial.println("Password set and lock engaged.");
  }

  else if (cmd.startsWith("UNLOCK ")) {

    String attempt = cmd.substring(7); // Extract password after "UNLOCK "

    if (attempt == storedPassword) {
      updateLockState(false); // Disengage lock

      statusChar->setValue("UNLOCKED");
      statusChar->notify();

      Serial.println("Correct password. Lock disengaged.");
    } else {
      statusChar->setValue("WRONG_PASS");
      statusChar->notify();

      Serial.println("Incorrect password attempt.");
    }
  }
  
  else if(cmd.startsWith("LOCK")) {

      updateLockState(true); // Engage lock

      statusChar->setValue("LOCKED");
      statusChar->notify();
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
      updateLockState(true); // Lock after reset
      statusChar->setValue("RESET");
      statusChar->notify();
      Serial.println("Password reset. Device must be configured again.");
  }

  // Update last activity time
  lastActivityTime = millis();
}

/** LOCK STATE UPDATE FUNCTION ******************/
void updateLockState(bool locked) {

  isLocked = locked;

  if (isLocked) {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);

    if (statusChar) {
      statusChar->setValue("LOCKED");
      statusChar->notify();
    }

    Serial.println("Lock engaged.");
  } else {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);

    if (statusChar) {
      statusChar->setValue("UNLOCKED");
      statusChar->notify();
    }

    Serial.println("Lock disengaged.");
  }

  statusChar->notify();
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
    
    lastActivityTime = millis();
    processCommand(cmd);
  }
};

/** SETUP FUNCTION *********************/
void setup() {

  Serial.begin(115200);
  delay(2000);
  Serial.println("Bike Lock BLE Server setup init...");

  // SPIFFS setup
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Load password from flash
  storedPassword = loadPasswordFromFlash();

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

  // BLE setup
  BLEDevice::init(DEVICE_NAME);


  // Create Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Services
  BLEService *pService = pServer->createService(BIKELOCK_SERVICE_UUID);
    
  // Command Characteristic
  BLECharacteristic *commandChar = 
    pService->createCharacteristic(COMMAND_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    
  commandChar->setCallbacks(new CommandCallbacks());

  // Status Characteristic
  statusChar =
    pService->createCharacteristic(
      STATUS_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_NOTIFY
    );
    
  BLE2901 *statusDesc = new BLE2901();
  statusDesc->setDescription("Current Lock Status: [Locked] or [Unlocked]");
  statusChar->addDescriptor(statusDesc);


  BLE2901 *commandDesc = new BLE2901();
  commandDesc->setDescription("Send commands: SET [password], UNLOCK [password], LOCK, STATUS");
  commandChar->addDescriptor(commandDesc);

  // Start Service
  pService->start();

  updateLockState(false); // Start with lock disengaged

  // Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BIKELOCK_SERVICE_UUID);
  pAdvertising->setScanResponse(true);

  BLEAdvertisementData adData;
  adData.setName(DEVICE_NAME);
    
  pAdvertising->setAdvertisementData(adData);

  BLEDevice::startAdvertising();
  Serial.println("BLE Advertising started");

  // Configure GPIO pins for deep sleep wakeup
  // GPIO12 (low side of EXT0)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_WAKE_PIN_1, 1); // GPIO12, high level
  // GPIO13 (part of EXT1 bitmap)
  esp_sleep_enable_ext1_wakeup((1ULL << TOUCH_WAKE_PIN_2), ESP_EXT1_WAKEUP_ANY_HIGH); // GPIO13

  lastActivityTime = millis();
}

void loop() {
  // Check for inactivity timeout and enter deep sleep if needed
  if (deviceConnected == false && (millis() - lastActivityTime) > INACTIVITY_TIMEOUT_MS) {
    Serial.println("Inactivity timeout reached. Entering deep sleep...");
    Serial.println("To wake up, press the touch button on GPIO12 or GPIO13");
    delay(100); // Allow serial to flush

    // Enter deep sleep
    esp_deep_sleep_start();
  }

  delay(1000); // Check every second
}
