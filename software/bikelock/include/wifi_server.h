#pragma once

#include <Arduino.h>
#include <WebServer.h>

/**
 * WiFi Server Module
 * Manages WiFi hotspot and HTTP REST API
 */

/**
 * Initialize WiFi in hotspot mode
 * Creates an access point that devices can connect to
 */
void initializeWiFi();

/**
 * Start HTTP server for REST API
 * Handles requests for lock control
 */
void startHTTPServer();

/**
 * Handle HTTP requests (called in main loop)
 */
void handleHTTPRequests();

/**
 * Get HTTP server instance for registering routes
 */
extern WebServer httpServer;
