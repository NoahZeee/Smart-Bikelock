#include <lock_control.h>
#include <globals.h>
#include <flash_storage.h>
#include <config.h>

/**
 * Lock Control Module Implementation
 * Stepper motor control with flash persistence
 */

void updateLockState(bool locked) {

  isLocked = locked;
  saveLockStateToFlash(locked); // Persist lock state

  String statusValue = locked ? "LOCKED" : "UNLOCKED";
  
  if (isLocked) { 
    // LOCK: Rotate stepper motor to engage lock
    Serial.println("Rotating stepper motor to engage lock...");
    stepper.step(STEPPER_LOCK_STEPS);
    Serial.print("Lock engaged. Rotated ");
    Serial.print(STEPPER_LOCK_STEPS);
    Serial.println(" steps");
  } else { 
    // UNLOCK: Rotate stepper motor to disengage lock
    Serial.println("Rotating stepper motor to disengage lock...");
    stepper.step(STEPPER_UNLOCK_STEPS);
    Serial.print("Lock disengaged. Rotated ");
    Serial.print(STEPPER_UNLOCK_STEPS);
    Serial.println(" steps");
  }
  Serial.flush();
  
  Serial.print("Lock state: ");
  Serial.println(statusValue);
  Serial.flush();
}
