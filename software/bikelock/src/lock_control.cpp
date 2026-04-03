#include <lock_control.h>
#include <globals.h>
#include <flash_storage.h>
#include <config.h>

/**
 * Lock Control Module Implementation
 * Centralized lock state management with LED feedback and BLE sync
 */

void updateLockState(bool locked) {

  isLocked = locked;
  saveLockStateToFlash(locked); // Persist lock state

  String statusValue = locked ? "LOCKED" : "UNLOCKED";
  
  if (isLocked) { // engage lock
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    Serial.println("Lock engaged.");
  } else { // disengage lock
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    Serial.println("Lock disengaged.");
  }
  Serial.flush();
  
  // Update BLE characteristic value
  if (statusChar) {
    Serial.print("Setting BLE characteristic to: ");
    Serial.println(statusValue);
    statusChar->setValue(statusValue.c_str());
    Serial.println("BLE characteristic value set successfully");
  } else {
    Serial.println("ERROR: statusChar is NULL!");
  }
  Serial.flush();

  // Send notification once at the end
  if (statusChar) {
    statusChar->notify();
    Serial.println("BLE notification sent");
  }
  Serial.flush();
}
