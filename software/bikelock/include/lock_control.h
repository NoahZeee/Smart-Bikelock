#pragma once

#include <Arduino.h>

/**
 * Lock Control Module
 * Manages lock state, LED indicators, and state persistence
 */

/**
 * Update lock state and notify BLE clients
 * Handles:
 * - LED indicator updates
 * - Lock state persistence to flash
 * - BLE characteristic value update
 * - BLE notification broadcast
 * 
 * @param locked true to engage lock, false to disengage
 */
void updateLockState(bool locked);
