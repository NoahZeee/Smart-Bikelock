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
#include <Adafruit_NeoPixel.h>

/** DEVICE INFO ************************/
#define DEVICE_NAME "Bike Lock"

/** PIN DEFINITIONS ************************/
#define LED_PIN 38
#define RED_PIN 5
#define GREEN_PIN 6
#define NUM_LEDS 1

/** BLE UUIDs ******************************/
#define BIKELOCK_SERVICE_UUID "1ea28c9d-23ce-4f5b-9290-8b72317b97c3"
#define COMMAND_CHAR_UUID "5a87b3d0-7c7d-4c5b-bf2c-5e7a0f1a0a01"
#define STATUS_CHAR_UUID  "5a87b3d0-7c7d-4c5b-bf2c-5e7a0f1a0a02"

/** GLOBAL VARIABLES ************************/
String storedPassword = "";
bool isLocked = false;
BLECharacteristic *statusChar;

Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Function to process incoming commands
void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("Parsed Command: ");
  Serial.println(cmd);

  if (cmd.startsWith("SET ")) {
  
    storedPassword = cmd.substring(4); // Extract password after "SET "

    isLocked = true;
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);

    statusChar->setValue("LOCKED");
    statusChar->notify();

    Serial.println("Password set and lock engaged.");
  }

  else if (cmd.startsWith("UNLOCK ")) {

    String attempt = cmd.substring(7); // Extract password after "UNLOCK "

    if (attempt == storedPassword) {
      isLocked = false;
      
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, HIGH);

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

      isLocked = true;
      digitalWrite(RED_PIN, HIGH);
      digitalWrite(GREEN_PIN, LOW);

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
}

/** BLE SERVER EVENTS *************************/
class MyServerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer *pServer) {
    pixel.setPixelColor(0, pixel.Color(0, 255, 0)); // Green
    pixel.show();
    
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer *pServer) {
    pixel.setPixelColor(0, pixel.Color(0, 0, 0)); // Off
    pixel.show();
    
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
    
    processCommand(cmd);
  }
};

/** SETUP FUNCTION *********************/
void setup() {

    Serial.begin(115200);
    delay(2000);
    Serial.println("Bike Lock BLE Server setup init...");

    // LED setup
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    pixel.begin();
    pixel.clear();
    pixel.show();

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

    // Start Service
    pService->start();

    // Advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BIKELOCK_SERVICE_UUID);
    pAdvertising->setScanResponse(true);

    BLEAdvertisementData adData;
    adData.setName(DEVICE_NAME);
    

    pAdvertising->setAdvertisementData(adData);

    BLEDevice::startAdvertising();
    Serial.println("BLE Advertising started");
}

void loop() {
 
}
