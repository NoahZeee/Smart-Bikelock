# Smart Bike Lock - Web App Setup & Deployment Guide

## Overview
This web app connects to your ESP32-based Bike Lock via Web Bluetooth API. It provides a mobile-friendly interface to control your lock remotely.

## Features
✅ **Direct Bluetooth Connection** - No backend relay needed  
✅ **Mobile-First Design** - Optimized for phone use  
✅ **Real-Time Status Updates** - Live lock status via BLE notifications  
✅ **Secure Commands** - SET, UNLOCK, LOCK, STATUS, RESET  
✅ **Responsive Layout** - Works on all screen sizes  
✅ **Dark Mode Support** - Automatic based on system preferences  

## Requirements
- **ESP32 Firmware**: Must be flashed with the updated main.cpp (includes SPIFFS and deep sleep support)
- **Browser Support**: Chrome, Edge, or similar (Chromium-based) on:
  - Android (native support)
  - macOS/iOS (iOS 17+, needs app wrapper)
  - Desktop Linux/Windows (requires compatible BLE adapter)
- **Permissions**: Your phone will need to allow Bluetooth access

## Deployment Options

### Option 1: Static Hosting (Recommended)
Host the web app files on a static file server. Users visit a URL to access it.

#### Services:
- **GitHub Pages** (free): Upload to a gh-pages branch
- **Firebase Hosting** (free tier available)
- **Netlify** (free tier available)
- **AWS S3 + CloudFront** (pay-as-you-go)
- Public Dropbox link or similar

#### Steps:
1. Upload `index.html`, `style.css`, and `app.js` to your hosting service
2. Share the URL with users
3. Users open the URL in their browser on a phone/device with Bluetooth and the capability

**Example GitHub Pages:**
```bash
# In your bikelock repository root
mkdir docs
cp web/* docs/
git add docs/
git commit -m "Add web app"
git push origin main

# Then enable GitHub Pages in Settings → Pages → Deploy from a branch → docs folder
```

### Option 2: Local Testing (for development)
Run a simple HTTP server on your computer:

```bash
# From the web/ directory
python -m http.server 8000

# Then visit: http://localhost:8000
```

> **Note**: Web Bluetooth requires HTTPS or localhost. Public HTTP sites won't have BLE access.

### Option 3: Serve from ESP32 (Advanced)
Embed the web app files directly on the ESP32 and serve via HTTP.

This requires additional libraries (ESPAsyncWebServer) and SPIFFS modifications. Recommended for later iteration.

## Usage Instructions

### For End Users:

1. **Power on the ESP32** - Device will start advertising as "Bike Lock"
2. **Open the web app** - Visit the hosted URL on your phone browser
3. **Click "Connect to Lock"** - Browser will show a Bluetooth device picker
4. **Select "Bike Lock"** - Confirm the pairing
5. **Set a password** - Enter a secure 4+ character password and click "Set Password"
6. **Control your lock** - Use LOCK/UNLOCK buttons with your password

### Commands Reference:
| Command | Function |
|---------|----------|
| **Set Password** | Initialize or change the lock password |
| **Lock** | Engage the lock (no password needed after setup) |
| **Unlock** | Disengage the lock (requires correct password) |
| **Check Status** | Query current lock state |
| **Reset Device** | Clear password and reset device |

## Web App Files

- **index.html** - UI structure (buttons, inputs, status display)
- **style.css** - Mobile-first responsive styling + dark mode
- **app.js** - Web Bluetooth client logic and command handling

## BLE Protocol

### Service UUID
```
1ea28c9d-23ce-4f5b-9290-8b72317b97c3
```

### Characteristics

**Command Characteristic** (WRITE-only)
- UUID: `5a87b3d0-7c7d-4c5b-bf2c-5e7a0f1a0a01`
- Send: `SET [password]`, `UNLOCK [password]`, `LOCK`, `STATUS`, `RESET`

**Status Characteristic** (READ + NOTIFY)
- UUID: `5a87b3d0-7c7d-4c5b-bf2c-5e7a0f1a0a02`
- Receive: `LOCKED`, `UNLOCKED`, `WRONG_PASS`, `RESET`

## Troubleshooting

### Web app opens but no "Connect" button responds
- **Cause**: Web Bluetooth not available on this device/browser
- **Fix**: Use Chrome/Edge on Android or macOS 10.15+

### Can't find the Bike Lock device
- **Cause**: ESP32 not powered or not advertising
- **Fix**: Check ESP32 serial output; verify device is within Bluetooth range (~10m)

### Connection drops frequently
- **Cause**: Interference or range issue
- **Fix**: Move closer to the device; reduce obstacles

### Wrong password keeps showing
- **Cause**: Typo or device not set up yet
- **Fix**: Re-run "Set Password" to initialize

### Device goes to sleep and can't reconnect
- **Cause**: 30-second inactivity timer engaged; device in deep sleep
- **Fix**: Press GPIO12 or GPIO13 touch button to wake; reconnect

## Security Notes

⚠️ **Current Implementation**: Password stored in **plain text** on ESP32 flash  
🔐 **Future Improvement**: Consider AES encryption for production use  

## ESP32 Deep Sleep & Wake

The ESP32 will enter deep sleep after **30 seconds of inactivity** (no BLE commands).

### To Wake the Device:
- **Press GPIO12 touch pin** OR
- **Press GPIO13 touch pin**  
- Device will resume BLE advertising within 1-2 seconds

### Power Consumption:
- **Active**: ~100mA
- **Sleep**: <1mA

This enables months of battery operation if powered by a coin cell or small rechargeable battery.

## Future Enhancements

- [ ] Add support for motor control (future replacement for LED)
- [ ] Implement encrypted password storage (AES)
- [ ] Multi-device support (device directory)
- [ ] Backend system for lost device tracking
- [ ] Mobile app wrapper (React Native or Flutter)
- [ ] Activity logging and audit trail

## Support

For issues, check:
1. ESP32 serial console output
2. Browser console (F12 → Console tab)
3. Bluetooth device list (Settings → Bluetooth)

## Files Modified in Main Project

- `platformio.ini` - Added SPIFFS library
- `include/config.h` - Added flash storage & sleep constants
- `src/main.cpp` - Added flash persistence, deep sleep, RESET command
- `/web/` - All new web app files

---

**Happy locking! 🚲🔒**
