/**
 * Smart Bike Lock - Web Bluetooth Client
 * Manages BLE communication with ESP32 smart lock
 */

// Configuration
const BLE_CONFIG = {
    SERVICE_UUID: '1ea28c9d-23ce-4f5b-9290-8b72317b97c3',
    COMMAND_CHAR_UUID: '5a87b3d0-7c7d-4c5b-bf2c-5e7a0f1a0a01',
    STATUS_CHAR_UUID: '5a87b3d0-7c7d-4c5b-bf2c-5e7a0f1a0a02',
    DEVICE_NAME: 'Bike Lock'
};

// Global state
let bleDevice = null;
let bleServer = null;
let bleService = null;
let commandChar = null;
let statusChar = null;
let isConnected = false;

// UI Elements
const connectBtn = document.getElementById('connectBtn');
const disconnectBtn = document.getElementById('disconnectBtn');
const controlsSection = document.getElementById('controlsSection');
const statusCard = document.getElementById('statusCard');
const statusIcon = document.getElementById('statusIcon');
const statusMessage = document.getElementById('statusMessage');
const deviceNameEl = document.getElementById('deviceName');
const messageBox = document.getElementById('messageBox');
const passwordInput = document.getElementById('passwordInput');
const lockDisplay = document.getElementById('lockDisplay');
const lockIndicator = document.getElementById('lockIndicator');
const lockStatusText = document.getElementById('lockStatusText');

// Button elements
const setPasswordBtn = document.getElementById('setPasswordBtn');
const lockBtn = document.getElementById('lockBtn');
const unlockBtn = document.getElementById('unlockBtn');
const statusBtn = document.getElementById('statusBtn');
const resetBtn = document.getElementById('resetBtn');

// Event Listeners
connectBtn.addEventListener('click', connectToDevice);
disconnectBtn.addEventListener('click', disconnectDevice);
setPasswordBtn.addEventListener('click', setPassword);
lockBtn.addEventListener('click', lockDevice);
unlockBtn.addEventListener('click', unlockDevice);
statusBtn.addEventListener('click', checkStatus);
resetBtn.addEventListener('click', resetDevice);

/**
 * Log message to the message box
 */
function addMessage(text, type = 'info') {
    const message = document.createElement('div');
    message.className = `message ${type}`;
    message.textContent = `[${new Date().toLocaleTimeString()}] ${text}`;
    messageBox.appendChild(message);
    messageBox.scrollTop = messageBox.scrollHeight;
    console.log(`[${type.toUpperCase()}] ${text}`);
}

/**
 * Update UI connection status
 */
function updateConnectionStatus(connected) {
    isConnected = connected;
    
    if (connected) {
        statusCard.classList.remove('disconnected');
        statusCard.classList.add('connected');
        statusIcon.textContent = '✅';
        statusMessage.textContent = 'Connected';
        controlsSection.style.display = 'flex';
        connectBtn.style.display = 'none';
        addMessage('Device connected successfully', 'success');
    } else {
        statusCard.classList.add('disconnected');
        statusCard.classList.remove('connected');
        statusIcon.textContent = '🔌';
        statusMessage.textContent = 'Disconnected';
        controlsSection.style.display = 'none';
        connectBtn.style.display = 'flex';
        deviceNameEl.textContent = '';
        addMessage('Device disconnected', 'error');
    }
}

/**
 * Connect to the Bluetooth device
 */
async function connectToDevice() {
    try {
        // Check if Web Bluetooth is supported
        if (!navigator.bluetooth) {
            addMessage('Web Bluetooth is not supported on this device', 'error');
            return;
        }

        addMessage('Scanning for Bike Lock devices...', 'info');
        connectBtn.disabled = true;

        // Request Bluetooth device
        bleDevice = await navigator.bluetooth.requestDevice({
            filters: [
                { name: BLE_CONFIG.DEVICE_NAME },
                { services: [BLE_CONFIG.SERVICE_UUID] }
            ],
            optionalServices: [BLE_CONFIG.SERVICE_UUID]
        });

        if (!bleDevice) {
            addMessage('No device selected', 'error');
            connectBtn.disabled = false;
            return;
        }

        addMessage(`Found device: ${bleDevice.name}`, 'success');
        deviceNameEl.textContent = `Device: ${bleDevice.name}`;

        // Set up disconnect handler
        bleDevice.addEventListener('gattserverdisconnected', onDisconnected);

        // Connect to GATT server
        addMessage('Connecting to GATT server...', 'info');
        bleServer = await bleDevice.gatt.connect();
        addMessage('Connected to GATT server', 'success');

        // Get primary service
        bleService = await bleServer.getPrimaryService(BLE_CONFIG.SERVICE_UUID);
        addMessage('Got primary service', 'success');

        // Get characteristics
        commandChar = await bleService.getCharacteristic(BLE_CONFIG.COMMAND_CHAR_UUID);
        statusChar = await bleService.getCharacteristic(BLE_CONFIG.STATUS_CHAR_UUID);
        addMessage('Got BLE characteristics', 'success');

        // Set up notifications
        await statusChar.startNotifications();
        statusChar.addEventListener('characteristicvaluechanged', onStatusUpdate);
        addMessage('Enabled status notifications', 'success');

        // Update UI
        updateConnectionStatus(true);
        connectBtn.disabled = false;

    } catch (error) {
        addMessage(`Connection error: ${error.message}`, 'error');
        connectBtn.disabled = false;
        updateConnectionStatus(false);
    }
}

/**
 * Disconnect from the device
 */
function disconnectDevice() {
    if (bleDevice) {
        bleDevice.gatt.disconnect();
    }
}

/**
 * Handle device disconnection
 */
function onDisconnected() {
    addMessage('Device disconnected', 'error');
    updateConnectionStatus(false);
    bleDevice = null;
    bleServer = null;
    bleService = null;
    commandChar = null;
    statusChar = null;
}

/**
 * Handle status characteristic updates
 */
function onStatusUpdate(event) {
    const value = event.target.value;
    const status = new TextDecoder().decode(value);
    
    addMessage(`Status: ${status}`, 'info');
    updateLockDisplay(status);
}

/**
 * Update the lock display based on status
 */
function updateLockDisplay(status) {
    if (status.includes('LOCKED')) {
        lockDisplay.classList.add('locked');
        lockDisplay.classList.remove('unlocked');
        lockIndicator.textContent = '🔒';
        lockStatusText.textContent = 'Locked';
    } else if (status.includes('UNLOCKED')) {
        lockDisplay.classList.remove('locked');
        lockDisplay.classList.add('unlocked');
        lockIndicator.textContent = '🔓';
        lockStatusText.textContent = 'Unlocked';
    } else if (status.includes('WRONG_PASS')) {
        addMessage('Wrong password!', 'error');
        lockStatusText.textContent = 'Wrong Password';
    } else if (status.includes('RESET')) {
        addMessage('Device has been reset', 'warning');
        lockStatusText.textContent = 'Reset';
    }
}

/**
 * Send command to the device
 */
async function sendCommand(command) {
    if (!isConnected || !commandChar) {
        addMessage('Device not connected', 'error');
        return false;
    }

    try {
        addMessage(`Sending: ${command}`, 'info');
        await commandChar.writeValue(new TextEncoder().encode(command));
        return true;
    } catch (error) {
        addMessage(`Send error: ${error.message}`, 'error');
        return false;
    }
}

/**
 * Set password command
 */
async function setPassword() {
    const password = passwordInput.value.trim();
    
    if (!password) {
        addMessage('Please enter a password', 'error');
        return;
    }

    if (password.length < 4) {
        addMessage('Password must be at least 4 characters', 'error');
        return;
    }

    const command = `SET ${password}`;
    if (await sendCommand(command)) {
        addMessage('Password set successfully', 'success');
    }
}

/**
 * Lock command
 */
async function lockDevice() {
    if (await sendCommand('LOCK')) {
        addMessage('Lock command sent', 'success');
    }
}

/**
 * Unlock command
 */
async function unlockDevice() {
    const password = passwordInput.value.trim();
    
    if (!password) {
        addMessage('Please enter the password to unlock', 'error');
        return;
    }

    const command = `UNLOCK ${password}`;
    if (await sendCommand(command)) {
        addMessage('Unlock command sent', 'success');
    }
}

/**
 * Check status command
 */
async function checkStatus() {
    if (await sendCommand('STATUS')) {
        addMessage('Status requested', 'success');
    }
}

/**
 * Reset device command
 */
async function resetDevice() {
    if (!confirm('Are you sure you want to reset the device? This will clear the password.')) {
        return;
    }

    if (await sendCommand('RESET')) {
        addMessage('Device reset command sent', 'success');
        passwordInput.value = '';
    }
}

/**
 * Initialize on page load
 */
window.addEventListener('load', () => {
    addMessage('Smart Bike Lock Web App loaded', 'success');
    addMessage('Click "Connect to Lock" to get started', 'info');
    
    // Update initial status
    updateConnectionStatus(false);
});

/**
 * Handle page unload
 */
window.addEventListener('beforeunload', () => {
    if (isConnected) {
        disconnectDevice();
    }
});
