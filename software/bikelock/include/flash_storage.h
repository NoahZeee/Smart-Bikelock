#pragma once

#include <Arduino.h>

/**
 * Flash Storage Module
 * Manages SPIFFS operations for password and lock state persistence
 */

/**
 * Save password to flash memory
 * @param password The password to save
 */
void savePasswordToFlash(String password);

/**
 * Load password from flash memory
 * @return The stored password, or empty string if not found
 */
String loadPasswordFromFlash();

/**
 * Delete password from flash memory
 */
void deletePasswordFromFlash();

/**
 * Save lock state to flash memory
 * @param locked true for locked, false for unlocked
 */
void saveLockStateToFlash(bool locked);

/**
 * Load lock state from flash memory
 * @return true if locked, false if unlocked
 */
bool loadLockStateFromFlash();
