#include <http_handlers.h>
#include <wifi_server.h>
#include <globals.h>
#include <commands.h>
#include <config.h>
#include <SPIFFS.h>

/**
 * HTTP Request Handlers Implementation
 * REST API for lock control - Minimal size version, no JSON library
 */

// Simple JSON response helpers
String jsonStatus() {
  return "{\"status\":\"" + String(isLocked ? "LOCKED" : "UNLOCKED") + "\"}";
}

String jsonError(const char* message) {
  return "{\"error\":\"" + String(message) + "\"}";
}

void registerHTTPRoutes() {
  httpServer.on("/", HTTP_GET, handleRoot);
  httpServer.on("/status", HTTP_GET, handleStatus);
  httpServer.on("/lock", HTTP_POST, handleLock);
  httpServer.on("/unlock", HTTP_POST, handleUnlock);
  httpServer.on("/set-password", HTTP_POST, handleSetPassword);
  httpServer.on("/reset", HTTP_POST, handleReset);
}

void handleRoot() {
  // Serve index.html from SPIFFS
  if (SPIFFS.exists("/index.html")) {
    File file = SPIFFS.open("/index.html", "r");
    if (file) {
      httpServer.streamFile(file, "text/html");
      file.close();
      return;
    }
  }
  
  // Fallback if file not found
  httpServer.send(200, "text/html", "<h1>Smart Bike Lock</h1><p>Web UI not available</p>");
}

void handleStatus() {
  lastActivityTime = millis();  // Keep device awake on HTTP activity
  httpServer.send(200, "application/json", jsonStatus());
}

void handleLock() {
  lastActivityTime = millis();  // Keep device awake
  if (storedPassword.length() == 0) {
    httpServer.send(400, "application/json", jsonError("No password set"));
    return;
  }
  
  processCommand("LOCK");
  httpServer.send(200, "application/json", jsonStatus());
}

void handleUnlock() {
  lastActivityTime = millis();  // Keep device awake
  if (!httpServer.hasArg("plain")) {
    httpServer.send(400, "application/json", jsonError("No data"));
    return;
  }
  
  String body = httpServer.arg("plain");
  
  // Simple JSON parsing for {"password":"value"}
  int pwdStart = body.indexOf("\"password\"");
  if (pwdStart == -1) {
    httpServer.send(400, "application/json", jsonError("Invalid format"));
    return;
  }
  
  int start = body.indexOf("\"", pwdStart + 10) + 1;
  int end = body.indexOf("\"", start);
  
  if (start > 0 && end > start) {
    String password = body.substring(start, end);
    processCommand("UNLOCK " + password);
  }
  
  httpServer.send(200, "application/json", jsonStatus());
}

void handleSetPassword() {
  lastActivityTime = millis();  // Keep device awake
  if (!httpServer.hasArg("plain")) {
    httpServer.send(400, "application/json", jsonError("No data"));
    return;
  }
  
  String body = httpServer.arg("plain");
  
  // Simple JSON parsing for {"password":"value"}
  int pwdStart = body.indexOf("\"password\"");
  if (pwdStart == -1) {
    httpServer.send(400, "application/json", jsonError("Invalid format"));
    return;
  }
  
  int start = body.indexOf("\"", pwdStart + 10) + 1;
  int end = body.indexOf("\"", start);
  
  if (start > 0 && end > start) {
    String password = body.substring(start, end);
    processCommand("SET " + password);
  }
  
  httpServer.send(200, "application/json", jsonStatus());
}

void handleReset() {
  lastActivityTime = millis();  // Keep device awake
  processCommand("RESET");
  httpServer.send(200, "application/json", jsonStatus());
}
