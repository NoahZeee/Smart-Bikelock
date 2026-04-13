#include <wifi_server.h>
#include <http_handlers.h>
#include <WiFi.h>
#include <config.h>

/**
 * WiFi Server Module Implementation
 * Hotspot mode with HTTP REST API
 */

WebServer httpServer(HTTP_SERVER_PORT);

void initializeWiFi() {
  Serial.println("\n--- WiFi Initialization ---");
  
  // Stop any existing WiFi
  WiFi.disconnect(true); // true = turn off radio
  delay(100);
  
  // Start WiFi in AP (Access Point) mode
  WiFi.mode(WIFI_AP);
  // Use nullptr for password to create open network (no password)
  if (strlen(WIFI_PASSWORD) == 0) {
    WiFi.softAP(WIFI_SSID, nullptr);
  } else {
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  }
  
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("WiFi Hotspot Started: ");
  Serial.println(WIFI_SSID);
  if (strlen(WIFI_PASSWORD) > 0) {
    Serial.print("WiFi Password: ");
    Serial.println(WIFI_PASSWORD);
  } else {
    Serial.println("WiFi: NO PASSWORD (Open Network)");
  }
  Serial.print("Access Point IP: ");
  Serial.println(apIP);
  Serial.println("Connect to this network from your phone, then visit http://192.168.4.1\n");
}

void startHTTPServer() {
  Serial.println("--- HTTP Server Initialization ---");
  
  // Register all routes
  registerHTTPRoutes();
  
  // Start server
  httpServer.begin();
  Serial.print("HTTP Server started on port ");
  Serial.println(HTTP_SERVER_PORT);
  Serial.println("Routes registered:");
  Serial.println("  GET  /status");
  Serial.println("  POST /lock");
  Serial.println("  POST /unlock");
  Serial.println("  POST /set-password");
  Serial.println("  POST /reset");
  Serial.println("  GET  /\n");
}

void handleHTTPRequests() {
  httpServer.handleClient();
}
