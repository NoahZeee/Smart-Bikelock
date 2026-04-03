#include <commands.h>
#include <globals.h>
#include <flash_storage.h>
#include <lock_control.h>

/**
 * Commands Module Implementation
 * BLE command parsing and execution with security checks
 */

void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("Parsed Command: ");
  Serial.println(cmd);
  Serial.flush();

  if(cmd.startsWith("SET ")) {
    // Security: Only allow setting password when UNLOCKED
    if (isLocked) {
      statusChar->setValue("LOCKED_CANNOT_SET");
      statusChar->notify();
      Serial.println("ERROR: Cannot set password while locked. Unlock device first.");
      Serial.flush();
    } else {
      storedPassword = cmd.substring(4); // Extract password after "SET "
      savePasswordToFlash(storedPassword); // Save to flash
      updateLockState(true); // Engage lock immediately after setting password
      Serial.println("Password set and lock engaged.");
      Serial.flush();
    }
  }
  else if (cmd.startsWith("UNLOCK ")) {
    String attempt = cmd.substring(7); // Extract password after "UNLOCK "

    if (attempt == storedPassword) {
      updateLockState(false); // Disengage lock
      Serial.println("Correct password. Lock disengaged.");
      Serial.flush();
    } else {
      statusChar->setValue("WRONG_PASS");
      statusChar->notify();
      Serial.println("Incorrect password attempt.");
      Serial.flush();
    }
  }
  else if(cmd.startsWith("LOCK")) {
    // Security: Only allow locking if a password has been set
    if (storedPassword.length() == 0) {
      statusChar->setValue("ERROR_NO_PASSWORD");
      statusChar->notify();
      Serial.println("ERROR: Cannot lock device without a password set. Use SET [password] first.");
      Serial.flush();
    } else {
      updateLockState(true); // Engage lock
    }
  }
  else if(cmd.startsWith("STATUS")) {
    if(isLocked)
      statusChar->setValue("LOCKED");
    else
      statusChar->setValue("UNLOCKED");
    statusChar->notify();
  }
  else if(cmd.startsWith("RESET")) {
    deletePasswordFromFlash();
    storedPassword = "";
    updateLockState(false); // Unlock after reset
    Serial.println("Password reset. Device unlocked and ready for reconfiguration.");
    Serial.flush();
  }

  // Update last activity time for sleep tracking
  lastActivityTime = millis();
}
