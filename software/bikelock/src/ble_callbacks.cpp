#include <ble_callbacks.h>
#include <globals.h>
#include <commands.h>
#include <config.h>

/**
 * BLE Callbacks Module Implementation
 * Event handlers for BLE connection, disconnection, and command receipt
 */

void MyServerCallbacks::onConnect(BLEServer *pServer) {
  digitalWrite(LED_PIN, HIGH);
  deviceConnected = true;
  lastActivityTime = millis();
  Serial.println("Client connected");
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer) {
  digitalWrite(LED_PIN, LOW);
  deviceConnected = false;
  lastActivityTime = millis();
  Serial.println("Client disconnected");
  BLEDevice::startAdvertising(); // Restart advertising for new clients
}

void CommandCallbacks::onWrite(BLECharacteristic *pChar) {
  String cmd = String(pChar->getValue().c_str());
  cmd.trim();

  if (cmd.length() == 0) return; // Ignore empty writes

  Serial.print("Received Raw Command: ");
  Serial.println(cmd);
  Serial.flush();
  
  lastActivityTime = millis();
  processCommand(cmd);
}
