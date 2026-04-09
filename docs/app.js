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
let lockState = null; // Track current lock state (true=locked, false=unlocked)

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
        statusIcon.textContent = '◉';  // Connected indicator
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

        // Read initial status to update UI immediately
        let initialStatus = await statusChar.readValue();
        let initialDecodedStatus = new TextDecoder().decode(initialStatus).replace(/\0/g, '').trim().toUpperCase();
        console.log('Initial read from device:', initialDecodedStatus);
        
        // If we got INITIALIZING, wait a moment and try again
        if (initialDecodedStatus === 'INITIALIZING') {
            addMessage('Device is initializing, waiting...', 'info');
            await new Promise(resolve => setTimeout(resolve, 500));
            initialStatus = await statusChar.readValue();
            initialDecodedStatus = new TextDecoder().decode(initialStatus).replace(/\0/g, '').trim().toUpperCase();
            console.log('Second read after init wait:', initialDecodedStatus);
        }
        
        // Manually trigger status update with the initial read
        onStatusUpdate({ target: { value: initialStatus } });
        
        // Request fresh status from device to ensure we're in sync
        addMessage('Requesting fresh status from device...', 'info');
        await sendCommand('STATUS');

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
    
    // Safely decode the BLE value, removing any null bytes or artifacts
    let decodedStatus = new TextDecoder().decode(value);
    
    // Remove null bytes and non-printable characters
    decodedStatus = decodedStatus.replace(/\0/g, '').trim();
    
    // Remove any non-ASCII characters that might interfere
    decodedStatus = decodedStatus.replace(/[^\x20-\x7E]/g, '').trim();
    
    const status = decodedStatus.toUpperCase();
    
    if (!status) {
        addMessage('WARNING: Received empty status from device', 'error');
        console.log('Raw BLE value:', value);
        return;
    }
    
    console.log('Raw BLE value bytes:', Array.from(new Uint8Array(value.buffer)).map(b => '0x' + b.toString(16).padStart(2, '0')).join(' '));
    addMessage(`Device Status: ${status}`, 'info');
    updateLockDisplay(status);
}

/**
 * Update the lock display based on status (CLEAN STATE MACHINE)
 * All status messages must update lockState to prevent deadlocks
 */
function updateLockDisplay(status) {
    // Normalize status to handle any whitespace or case issues
    status = status.trim().toUpperCase();
    
    console.log('updateLockDisplay called with status:', status, 'current lockState:', lockState);
    
    // Determine lock state from status message
    let newLockState = null;
    let displayText = '';
    
    if (status.includes('LOCKED') && !status.includes('UNLOCKED')) {
        // Make sure it says LOCKED but not UNLOCKED (to avoid matching UNLOCKED as LOCKED)
        newLockState = true;
        displayText = 'LOCKED';
    } else if (status.includes('UNLOCKED')) {
        newLockState = false;
        displayText = 'UNLOCKED';
    } else if (status.includes('RESET')) {
        newLockState = false;  // Device always unlocked after reset
        displayText = 'RESET - Ready to Configure';
    } else if (status.includes('INITIALIZING')) {
        // Device just started - don't update state, just show message
        addMessage('Device initializing...', 'info');
        console.log('Ignoring INITIALIZING status, keeping current lockState:', lockState);
        return;
    } else if (status.includes('WRONG_PASS')) {
        // Wrong password attempted - maintain current lock state, just show error
        addMessage('Incorrect password!', 'error');
        lockStatusText.textContent = 'Wrong Password - Try Again';
        console.log('WRONG_PASS received, maintaining lockState:', lockState);
        // DON'T change lockState here - keep current state
        // But still update button states in case something got out of sync
        updateButtonStates();
        return;  // Exit early, don't update display styling
    } else if (status.includes('LOCKED_CANNOT_SET')) {
        newLockState = true;  // Device is locked
        displayText = 'LOCKED';
        addMessage('Cannot set password while locked. Unlock first!', 'error');
    } else if (status.includes('ERROR_NO_PASSWORD')) {
        // Cannot lock without password - maintain current state
        addMessage('Cannot lock device without a password. Use SET password first!', 'error');
        lockStatusText.textContent = 'Password Required';
        console.log('ERROR_NO_PASSWORD received, maintaining lockState:', lockState);
        // Don't change lockState, just show error and update buttons
        updateButtonStates();
        return;  // Exit early
    } else {
        // Unknown status - don't change state, try to query device
        addMessage(`Unknown status received: "${status}" - Requesting device status...`, 'error');
        console.log('Unknown status, requesting status from device');
        // Attempt to query status to get back in sync
        checkStatus();
        return;
    }
    
    // Update lock state (this is CRITICAL for all paths to prevent deadlock)
    if (newLockState !== null) {
        console.log(`State change: ${lockState} -> ${newLockState}`);
        lockState = newLockState;
    }
    
    // Update UI display based on lock state
    if (lockState === true) {
        // LOCKED
        lockDisplay.classList.add('locked');
        lockDisplay.classList.remove('unlocked');
        lockIndicator.textContent = '⬤';
        lockStatusText.textContent = displayText || 'LOCKED';
        console.log('UI updated to LOCKED state');
    } else if (lockState === false) {
        // UNLOCKED
        lockDisplay.classList.remove('locked');
        lockDisplay.classList.add('unlocked');
        lockIndicator.textContent = '○';
        lockStatusText.textContent = displayText || 'UNLOCKED';
        console.log('UI updated to UNLOCKED state');
    } else {
        console.log('ERROR: lockState is null after status update!');
    }
    
    // Update button states based on new lock state
    updateButtonStates();
}

/**
 * Update button states based on current lock state
 */
function updateButtonStates() {
    if (lockState === null) {
        // Not yet determined
        setPasswordBtn.disabled = true;
        lockBtn.disabled = true;
        unlockBtn.disabled = true;
    } else if (lockState === true) {
        // Device is LOCKED
        setPasswordBtn.disabled = true;  // Cannot set password while locked
        lockBtn.disabled = true;          // Already locked, disable to prevent double-locking
        unlockBtn.disabled = false;       // Can unlock
    } else {
        // Device is UNLOCKED
        setPasswordBtn.disabled = false;  // Can set password while unlocked
        lockBtn.disabled = false;         // Can lock
        unlockBtn.disabled = true;        // Already unlocked, disable to prevent double-unlocking
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
    await sendCommand(command);
    // Don't show success message here - let the status update from device confirm it
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
