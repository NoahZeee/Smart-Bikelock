# Smart Bike Lock - Software Architecture

## System Architecture Diagram

```mermaid
graph TB
    subgraph Hardware["🔧 Hardware Layer"]
        GPIO["GPIO Pins"]
        LED["LED<br/>GPIO 2"]
        STEPPER["Stepper Motor<br/>GPIO 13,25,14,26"]
        WAKE["Touch Wake Pin<br/>GPIO 18,15"]
        USB["USB CDC<br/>Serial"]
        WIFI["WiFi Radio<br/>802.11 b/g/n"]
    end
    
    subgraph Storage["💾 Storage Layer"]
        SPIFFS["SPIFFS<br/>File System"]
        FLASH["Flash Memory<br/>4MB N4"]
        PASSWORD["Stored Password"]
        LOCKSTATE["Lock State"]
    end
    
    subgraph Core["🎯 Core Modules"]
        CONFIG["config.h<br/>Pin & WiFi Defs"]
        GLOBALS["globals.h<br/>Shared Variables"]
        LOCK_CTRL["lock_control.cpp<br/>Motor & State"]
        FLASH_STG["flash_storage.cpp<br/>Read/Write"]
        COMMANDS["commands.cpp<br/>CLI Commands"]
    end
    
    subgraph Network["🌐 Network Stack"]
        WIFI_SRV["wifi_server.cpp<br/>AP Mode"]
        HTTP["http_handlers.cpp<br/>REST API"]
        ROUTES["Routes<br/>/status /lock /unlock<br/>/set-password /reset"]
    end
    
    subgraph Power["⚡ Power Management"]
        SLEEP["esp_sleep.h<br/>Deep Sleep Control"]
        INACTIVITY["Inactivity Timer<br/>1 minute"]
        WAKEUP["External Interrupt<br/>GPIO18 HIGH"]
    end
    
    subgraph Main["▶️ Main Loop"]
        SETUP["setup()"]
        LOOP["loop()"]
    end
    
    SETUP -->|Initialize| CONFIG
    SETUP -->|Mount| SPIFFS
    SETUP -->|Load| FLASH_STG
    SETUP -->|Configure GPIO| GPIO
    SETUP -->|Enable AP| WIFI_SRV
    SETUP -->|Register Routes| HTTP
    
    LOOP -->|Monitor| INACTIVITY
    LOOP -->|Handle| HTTP
    LOOP -->|Check| SLEEP
    
    HTTP -->|GET| LOCK_CTRL
    HTTP -->|POST| LOCK_CTRL
    ROUTES -->|Authenticate| FLASH_STG
    
    LOCK_CTRL -->|Control| STEPPER
    LOCK_CTRL -->|Persist| FLASH_STG
    LOCK_CTRL -->|Indicate| LED
    
    FLASH_STG -->|Read/Write| SPIFFS
    SPIFFS -->|Store| FLASH
    FLASH -->|Hold| PASSWORD
    FLASH -->|Hold| LOCKSTATE
    
    WIFI_SRV -->|Communicate| WIFI
    HTTP -->|Transport| WIFI
    
    SLEEP -->|Trigger| WAKEUP
    WAKEUP -->|Wake from| SLEEP
    INACTIVITY -->|Activate| SLEEP
    
    USB -->|Debug Output| SETUP
    USB -->|Debug Output| LOOP
```

## User Interaction Flow

```mermaid
graph LR
    USER["📱 Mobile App<br/>or Browser"]
    
    subgraph Device["ESP32-S3 Bike Lock"]
        WIFI_AP["WiFi AP<br/>BikelockAP"]
        REST["REST API<br/>Port 80"]
        AUTH["Password<br/>Authentication"]
        LOCK_SM["Lock State<br/>Machine"]
        MOTOR["Stepper<br/>Motor"]
    end
    
    USER -->|Connect to AP| WIFI_AP
    USER -->|HTTP Request| REST
    REST -->|Verify Password| AUTH
    AUTH -->|/lock<br/>/unlock| LOCK_SM
    LOCK_SM -->|Rotate| MOTOR
    REST -->|/status| LOCK_SM
    REST -->|/set-password| AUTH
    REST -->|/reset| LOCK_SM
    
    LOCK_SM -->|Save State| FLASH["Flash Storage"]
    AUTH -->|Save Password| FLASH
```

## How to Export These Diagrams

### Option 1: View Online (Easiest)
1. Go to https://mermaid.live
2. Copy the Mermaid code from the code blocks above
3. Paste into the editor
4. Click "Export" → "Download SVG/PNG"

### Option 2: Use VS Code Extension
1. Install "Markdown Preview Mermaid Support" extension
2. Open this file in preview mode (Ctrl+Shift+V)
3. Right-click on diagram → "Save Image As"

### Option 3: Use Command Line (Requires npm)
```bash
npm install -g @mermaid-js/mermaid-cli
mmdc -i ARCHITECTURE.md -o architecture.svg
```

## Architecture Overview

### Hardware Layer
- **LED (GPIO 2)**: Connection indicator
- **Stepper Motor (GPIO 13, 25, 14, 26)**: Physical lock mechanism
- **Wake Pins (GPIO 18, 15)**: External interrupt for deep sleep wakeup
- **USB CDC**: Native serial communication over USB
- **WiFi Radio**: 802.11 b/g/n access point

### Storage Layer
- **SPIFFS**: File-based storage on flash
- **Flash Memory**: 4MB N4 variant (192KB SPIFFS, 1.75MB app code)
- **Stored Password**: Hashed password for authentication
- **Lock State**: Persisted lock status (locked/unlocked)

### Core Modules
- **config.h**: Pin definitions and WiFi configuration constants
- **globals.h**: Shared global variables and extern declarations
- **lock_control.cpp**: Motor control and lock state management
- **flash_storage.cpp**: SPIFFS read/write operations
- **commands.cpp**: CLI command processing

### Network Stack
- **wifi_server.cpp**: Access point (AP) mode initialization
- **http_handlers.cpp**: REST API endpoint handlers
- **Routes**: 5 HTTP endpoints for control and status

### Power Management
- **esp_sleep.h**: Deep sleep configuration
- **Inactivity Timer**: 1 minute of inactivity triggers sleep
- **External Wakeup**: GPIO18 pulled HIGH wakes from sleep

### Main Loop
- **setup()**: Initializes all modules and peripherals
- **loop()**: Handles HTTP requests and monitors inactivity for sleep

## Data Flow

### Lock Operation
```
HTTP Request → Authenticate Password → updateLockState() 
→ Control Stepper Motor → LED Indicator → Save to Flash
```

### Query Status
```
HTTP GET /status → Read Lock State from Memory → Return JSON
```

### Change Password
```
HTTP POST /set-password → Validate → Store Hashed Password → Lock Device
```

### Power Saving
```
Inactivity Detected (1 min) → Disable WiFi → Enter Deep Sleep 
→ GPIO18 HIGH → External Interrupt → Wake & Reinit
```
