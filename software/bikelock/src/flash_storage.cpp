#include <flash_storage.h>
#include <SPIFFS.h>

/**
 * Flash Storage Module Implementation
 * SPIFFS-based persistence for password and lock state
 */

void savePasswordToFlash(String password) {
  File file = SPIFFS.open("/password.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open password file for writing");
    Serial.flush();
    return;
  }
  file.print(password);
  file.close();
  Serial.println("Password saved to flash");
  Serial.flush();
}

String loadPasswordFromFlash() {
  if (!SPIFFS.exists("/password.txt")) {
    Serial.println("Password file does not exist");
    return "";
  }
  
  File file = SPIFFS.open("/password.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open password file for reading");
    return "";
  }
  
  String password = file.readString();
  file.close();
  Serial.print("Password loaded from flash: ");
  Serial.println(password);
  return password;
}

void deletePasswordFromFlash() {
  if (SPIFFS.remove("/password.txt")) {
    Serial.println("Password deleted from flash");
  } else {
    Serial.println("Failed to delete password file");
  }
}

void saveLockStateToFlash(bool locked) {
  File file = SPIFFS.open("/lock_state.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open lock state file for writing");
    Serial.flush();
    return;
  }
  file.print(locked ? "1" : "0");
  file.close();
  Serial.print("Lock state saved to flash: ");
  Serial.println(locked ? "LOCKED" : "UNLOCKED");
  Serial.flush();
}

bool loadLockStateFromFlash() {
  if (!SPIFFS.exists("/lock_state.txt")) {
    Serial.println("Lock state file does not exist, defaulting to UNLOCKED");
    return false;
  }
  
  File file = SPIFFS.open("/lock_state.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open lock state file for reading");
    return false;
  }
  
  String state = file.readString();
  file.close();
  bool locked = (state == "1");
  Serial.print("Lock state loaded from flash: ");
  Serial.println(locked ? "LOCKED" : "UNLOCKED");
  Serial.flush();
  return locked;
}
