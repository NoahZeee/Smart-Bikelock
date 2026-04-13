#pragma once

#include <Arduino.h>

/**
 * HTTP Request Handlers
 * REST API endpoints for lock control
 */

/**
 * Register all HTTP endpoints
 */
void registerHTTPRoutes();

/**
 * GET /status - Query lock status
 */
void handleStatus();

/**
 * POST /lock - Engage lock
 */
void handleLock();

/**
 * POST /unlock - Disengage lock with password
 * Body: {"password": "your_password"}
 */
void handleUnlock();

/**
 * POST /set-password - Set new password and lock
 * Body: {"password": "new_password"}
 */
void handleSetPassword();

/**
 * POST /reset - Reset device and clear password
 */
void handleReset();

/**
 * GET / - Serve landing page
 */
void handleRoot();
