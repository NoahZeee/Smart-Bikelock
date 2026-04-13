#pragma once

#include <Arduino.h>
#include <Stepper.h>
#include <config.h>

/**
 * Global variables shared across modules
 */

extern String storedPassword;
extern bool isLocked;
extern unsigned long lastActivityTime;
extern Stepper stepper;

// Forward declarations
void updateLockState(bool locked);
void processCommand(String cmd);
