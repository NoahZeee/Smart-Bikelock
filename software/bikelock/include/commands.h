#pragma once

#include <Arduino.h>

/**
 * Commands Module
 * Processes and executes bike lock control commands
 * 
 * Supported Commands:
 * - SET [password]      : Set new password and lock device (only when unlocked)
 * - UNLOCK [password]   : Unlock device with correct password
 * - LOCK                : Lock device (requires password to be set first)
 * - STATUS              : Query current lock status
 * - RESET               : Clear password and unlock device
 */

/**
 * Process incoming command from BLE client
 * @param cmd The command string to process
 */
void processCommand(String cmd);
