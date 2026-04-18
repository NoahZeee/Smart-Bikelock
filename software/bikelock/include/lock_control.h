#pragma once

#include <Arduino.h>

/**
 * Lock Control Module
 * Manages lock state, LED indicators, and state persistence
 */

/**
 * Update lock state and persist state
 * Handles:
 * - Lock state persistence to flash
 * - Stepper motor control
 * 
 * @param locked true to engage lock, false to disengage
 */
void updateLockState(bool locked);
