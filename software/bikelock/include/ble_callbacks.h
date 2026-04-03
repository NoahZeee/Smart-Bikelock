#pragma once

#include <BLEDevice.h>

/**
 * BLE Callbacks Module
 * Handles BLE server and characteristic events
 */

/**
 * BLE Server event callbacks
 * Handles client connection/disconnection events
 */
class MyServerCallbacks : public BLEServerCallbacks {
public:
  void onConnect(BLEServer *pServer);
  void onDisconnect(BLEServer *pServer);
};

/**
 * Command characteristic callbacks
 * Handles incoming commands from BLE clients
 */
class CommandCallbacks : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic *pChar);
};
