#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <Stepper.h>
#include <config.h>

/**
 * Global variables shared across modules
 */

extern String storedPassword;
extern bool isLocked;
extern BLECharacteristic *statusChar;
extern unsigned long lastActivityTime;
extern bool deviceConnected;
extern Stepper stepper;

// Forward declarations
void updateLockState(bool locked);
void processCommand(String cmd);
