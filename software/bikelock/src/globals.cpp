#include <globals.h>

/**
 * Global Variables Definition
 * Shared state across all modules
 */

String storedPassword = "";
bool isLocked = false;

BLECharacteristic *statusChar = nullptr;
unsigned long lastActivityTime = 0;
bool deviceConnected = false;

Stepper stepper(STEPS_PER_REV, STEPPER_PIN_1, STEPPER_PIN_2, STEPPER_PIN_3, STEPPER_PIN_4);
