#ifndef BLE_KEYBOARD_H
#define BLE_KEYBOARD_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEHIDDevice.h>

// ============================================================================
// BleKeyboard - NimBLE-based BLE HID Keyboard
// ============================================================================
// Implements standard HID Boot Keyboard protocol for maximum compatibility.
// Advertises as: "ESP32-S3 Keyboard"
// HID descriptor: Standard 8-byte boot keyboard report
// Performance: Optimized for <50ms per HID report (10 reports in <500ms)

class BleKeyboard {
public:
    // ---- HID Report Constants ----
    static const uint8_t KEY_REPORT_SIZE = 8;         // Standard keyboard report
    static const uint16_t HID_APPEARANCE_KEYBOARD = 0x03C1;
    static const uint16_t HID_SERVICE_UUID = 0x1812;
    static const uint16_t HID_REPORT_UUID = 0x2A4D;
    static const uint16_t HID_PROTOCOL_UUID = 0x2A4E;
    static const uint16_t BATTERY_SERVICE_UUID = 0x180F;
    static const uint16_t BATTERY_LEVEL_UUID = 0x2A19;
    static const uint16_t DEVICE_INFO_SERVICE_UUID = 0x180A;
    static const uint16_t PNP_ID_UUID = 0x2A50;

    // ---- Key Modifiers (bitmask) ----
    static const uint8_t KEY_MOD_LCTRL   = 0x01;
    static const uint8_t KEY_MOD_LSHIFT  = 0x02;
    static const uint8_t KEY_MOD_LALT    = 0x04;
    static const uint8_t KEY_MOD_LGUI    = 0x08;
    static const uint8_t KEY_MOD_RCTRL   = 0x10;
    static const uint8_t KEY_MOD_RSHIFT  = 0x20;
    static const uint8_t KEY_MOD_RALT    = 0x40;
    static const uint8_t KEY_MOD_RGUI    = 0x80;

    // ---- Common Key Codes (subset) ----
    static const uint8_t KEY_NONE        = 0x00;
    static const uint8_t KEY_A           = 0x04;
    static const uint8_t KEY_B           = 0x05;
    static const uint8_t KEY_C           = 0x06;
    static const uint8_t KEY_D           = 0x07;
    static const uint8_t KEY_E           = 0x08;
    static const uint8_t KEY_F           = 0x09;
    static const uint8_t KEY_G           = 0x0A;
    static const uint8_t KEY_H           = 0x0B;
    static const uint8_t KEY_I           = 0x0C;
    static const uint8_t KEY_J           = 0x0D;
    static const uint8_t KEY_K           = 0x0E;
    static const uint8_t KEY_L           = 0x0F;
    static const uint8_t KEY_M           = 0x10;
    static const uint8_t KEY_N           = 0x11;
    static const uint8_t KEY_O           = 0x12;
    static const uint8_t KEY_P           = 0x13;
    static const uint8_t KEY_Q           = 0x14;
    static const uint8_t KEY_R           = 0x15;
    static const uint8_t KEY_S           = 0x16;
    static const uint8_t KEY_T           = 0x17;
    static const uint8_t KEY_U           = 0x18;
    static const uint8_t KEY_V           = 0x19;
    static const uint8_t KEY_W           = 0x1A;
    static const uint8_t KEY_X           = 0x1B;
    static const uint8_t KEY_Y           = 0x1C;
    static const uint8_t KEY_Z           = 0x1D;
    static const uint8_t KEY_1           = 0x1E;
    static const uint8_t KEY_2           = 0x1F;
    static const uint8_t KEY_3           = 0x20;
    static const uint8_t KEY_4           = 0x21;
    static const uint8_t KEY_5           = 0x22;
    static const uint8_t KEY_6           = 0x23;
    static const uint8_t KEY_7           = 0x24;
    static const uint8_t KEY_8           = 0x25;
    static const uint8_t KEY_9           = 0x26;
    static const uint8_t KEY_0           = 0x27;
    static const uint8_t KEY_ENTER       = 0x28;
    static const uint8_t KEY_ESC         = 0x29;
    static const uint8_t KEY_BACKSPACE   = 0x2A;
    static const uint8_t KEY_TAB         = 0x2B;
    static const uint8_t KEY_SPACE       = 0x2C;
    static const uint8_t KEY_MINUS       = 0x2D;
    static const uint8_t KEY_EQUAL       = 0x2E;
    static const uint8_t KEY_LBRACKET    = 0x2F;
    static const uint8_t KEY_RBRACKET    = 0x30;
    static const uint8_t KEY_BACKSLASH   = 0x31;
    static const uint8_t KEY_HASH        = 0x32;  // Non-US # and ~
    static const uint8_t KEY_SEMICOLON   = 0x33;
    static const uint8_t KEY_APOSTROPHE  = 0x34;
    static const uint8_t KEY_GRAVE       = 0x35;
    static const uint8_t KEY_COMMA       = 0x36;
    static const uint8_t KEY_PERIOD      = 0x37;
    static const uint8_t KEY_SLASH       = 0x38;
    static const uint8_t KEY_CAPSLOCK    = 0x39;
    static const uint8_t KEY_F1          = 0x3A;
    static const uint8_t KEY_F2          = 0x3B;
    static const uint8_t KEY_F3          = 0x3C;
    static const uint8_t KEY_F4          = 0x3D;
    static const uint8_t KEY_F5          = 0x3E;
    static const uint8_t KEY_F6          = 0x3F;
    static const uint8_t KEY_F7          = 0x40;
    static const uint8_t KEY_F8          = 0x41;
    static const uint8_t KEY_F9          = 0x42;
    static const uint8_t KEY_F10         = 0x43;
    static const uint8_t KEY_F11         = 0x44;
    static const uint8_t KEY_F12         = 0x45;
    static const uint8_t KEY_DELETE      = 0x4C;
    static const uint8_t KEY_UP          = 0x52;
    static const uint8_t KEY_LEFT        = 0x50;
    static const uint8_t KEY_DOWN        = 0x51;
    static const uint8_t KEY_RIGHT       = 0x4F;
    static const uint8_t KEY_HOME        = 0x4A;
    static const uint8_t KEY_END         = 0x4D;
    static const uint8_t KEY_INSERT      = 0x49;
    static const uint8_t KEY_PAGEUP      = 0x4B;
    static const uint8_t KEY_PAGEDOWN    = 0x4E;

private:
    NimBLEServer* _pServer;
    NimBLEHIDDevice* _pHID;
    NimBLECharacteristic* _pInput;
    NimBLECharacteristic* _pBattery;
    bool _connected;
    uint8_t _batteryLevel;
    uint8_t _report[KEY_REPORT_SIZE];

    // Connection callbacks
    class ServerCallbacks : public NimBLEServerCallbacks {
        BleKeyboard* _parent;
    public:
        ServerCallbacks(BleKeyboard* parent) : _parent(parent) {}
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    };

    // Send a raw HID report
    void sendReport(const uint8_t* report);

public:
    BleKeyboard();
    ~BleKeyboard();

    // ---- Lifecycle ----
    bool begin(const char* deviceName = "ESP32-S3 Keyboard");
    void end();

    // ---- Connection State ----
    bool isConnected() const { return _connected; }

    // ---- Key Actions ----
    void write(uint8_t key);
    void write(uint8_t modifier, uint8_t key);
    void press(uint8_t key);
    void press(uint8_t modifier, uint8_t key);
    void release(uint8_t key);
    void releaseAll();
    void write(const char* str);
    void pressModifier(uint8_t mod);
    void releaseModifier(uint8_t mod);

    // ---- Battery Service ----
    void setBatteryLevel(uint8_t level);

    // ---- Utility ----
    const uint8_t* getLastReport() const { return _report; }
};

// ============================================================================
// HID Boot Keyboard Report Descriptor
// ============================================================================

static const uint8_t _hidReportDescriptor[] PROGMEM = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    // Modifier byte
    0x05, 0x07,        //   Usage Page (Keyboard)
    0x19, 0xE0,        //   Usage Minimum (Left Control)
    0x29, 0xE7,        //   Usage Maximum (Right GUI)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    // Reserved byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Constant)
    // Key array (6 keys)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Keyboard)
    0x19, 0x00,        //   Usage Minimum (Reserved)
    0x29, 0x65,        //   Usage Maximum (Application)
    0x81, 0x00,        //   Input (Data, Array)
    // Output report (LEDs)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data, Variable, Absolute)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x01,        //   Output (Constant)
    0xC0               // End Collection
};

static const uint8_t _hidReportDescriptorSize = sizeof(_hidReportDescriptor);

// ============================================================================
// Connection Callbacks
// ============================================================================

void BleKeyboard::ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    Serial.println("BLE: Client connected");
    _parent->_connected = true;
    NimBLEDevice::stopAdvertising();
    Serial.printf("BLE: Connected to %s\n", connInfo.getAddress().toString().c_str());
}

void BleKeyboard::ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    Serial.printf("BLE: Client disconnected (reason: %d)\n", reason);
    _parent->_connected = false;
    NimBLEDevice::startAdvertising();
    Serial.println("BLE: Advertising restarted");
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

BleKeyboard::BleKeyboard()
    : _pServer(nullptr)
    , _pHID(nullptr)
    , _pInput(nullptr)
    , _pBattery(nullptr)
    , _connected(false)
    , _batteryLevel(100)
{
    memset(_report, 0, KEY_REPORT_SIZE);
}

BleKeyboard::~BleKeyboard() {
    end();
}

// ============================================================================
// begin()
// ============================================================================

bool BleKeyboard::begin(const char* deviceName) {
    Serial.println("BLE: Initializing NimBLE HID Keyboard...");

    // Clear old bonds BEFORE init (otherwise corrupts BLE stack)
    NimBLEDevice::deleteAllBonds();

    // Random MAC every boot = Windows sees fresh device (no stale driver/bond issues)
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);

    NimBLEDevice::init(deviceName);

    // No bonding = connect instantly without PIN/pairing dialog
    NimBLEDevice::setSecurityAuth(false, false, false);

    NimBLEDevice::setPower(9); // +9dBm max power

    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(new ServerCallbacks(this));

    // Create HID device
    _pHID = new NimBLEHIDDevice(_pServer);

    // Set report map
    _pHID->setReportMap((uint8_t*)_hidReportDescriptor, _hidReportDescriptorSize);

    // Set HID info: country = 0 (not localized), flags = 1
    _pHID->setHidInfo(0x00, 0x01);

    // Set manufacturer
    _pHID->setManufacturer("ESP32-S3");

    // Set PnP ID: USB Spec sig=0x02, VID=0xE502, PID=0xA111, version=0x0210
    _pHID->setPnp(0x02, 0xE502, 0xA111, 0x0210);

    // Battery service (optional but good for HID)
    _pHID->setBatteryLevel(_batteryLevel);
    _pBattery = _pHID->getBatteryLevel();

    // Use boot keyboard report (no Report ID byte → standard 8-byte HID report)
    _pInput = _pHID->getBootInput();

    // Start services
    _pServer->start();

    // Configure advertising via the advertising object
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_APPEARANCE_KEYBOARD);
    pAdvertising->addServiceUUID(_pHID->getHidService()->getUUID());
    pAdvertising->enableScanResponse(true);
    pAdvertising->setMinInterval(0x20); // 20ms
    pAdvertising->setMaxInterval(0x30); // 30ms

    // Start advertising
    NimBLEDevice::startAdvertising();
    Serial.printf("BLE: Advertising as '%s'...\n", deviceName);
    Serial.println("BLE: Ready for connection.");

    return true;
}

// ============================================================================
// end()
// ============================================================================

void BleKeyboard::end() {
    if (_connected) {
        releaseAll();
    }
    NimBLEDevice::stopAdvertising();
    NimBLEDevice::deinit(true);
    _connected = false;
    Serial.println("BLE: Deinitialized");
}

// ============================================================================
// sendReport()
// ============================================================================

void BleKeyboard::sendReport(const uint8_t* report) {
    if (!_connected || !_pInput) return;
    _pInput->setValue((uint8_t*)report, KEY_REPORT_SIZE);
    _pInput->notify();
    memcpy(_report, report, KEY_REPORT_SIZE);
}

// ============================================================================
// press()
// ============================================================================

void BleKeyboard::press(uint8_t modifier, uint8_t key) {
    uint8_t report[KEY_REPORT_SIZE];
    memset(report, 0, KEY_REPORT_SIZE);
    report[0] = modifier;
    report[2] = key;
    sendReport(report);
}

void BleKeyboard::press(uint8_t key) {
    press(0, key);
}

// ============================================================================
// release() / releaseAll()
// ============================================================================

void BleKeyboard::release(uint8_t key) {
    if (!_connected) return;
    releaseAll();
}

void BleKeyboard::releaseAll() {
    if (!_connected) return;
    uint8_t report[KEY_REPORT_SIZE];
    memset(report, 0, KEY_REPORT_SIZE);
    sendReport(report);
}

// ============================================================================
// write() single key
// ============================================================================

void BleKeyboard::write(uint8_t modifier, uint8_t key) {
    if (!_connected || key == KEY_NONE) return;
    press(modifier, key);
    delayMicroseconds(800);
    releaseAll();
    delayMicroseconds(400);
}

void BleKeyboard::write(uint8_t key) {
    write(0, key);
}

// ============================================================================
// write() string via CP437 mapping
// ============================================================================

void BleKeyboard::write(const char* str) {
    if (!_connected || !str) return;

    while (*str) {
        uint8_t cp = (uint8_t)*str;
        uint8_t modifier = 0;
        uint8_t keycode = KEY_NONE;

        // Use CP437 mapping (from cp437_map.h)
        extern void cp437_to_hid(uint8_t cp, uint8_t *mod, uint8_t *key);
        cp437_to_hid(cp, &modifier, &keycode);

        if (keycode != KEY_NONE) {
            uint8_t report[KEY_REPORT_SIZE];
            memset(report, 0, KEY_REPORT_SIZE);
            report[0] = modifier;
            report[2] = keycode;
            sendReport(report);
            delayMicroseconds(1200);

            memset(report, 0, KEY_REPORT_SIZE);
            sendReport(report);
            delayMicroseconds(600);
        }

        str++;
    }
}

// ============================================================================
// Modifier Helpers
// ============================================================================

void BleKeyboard::pressModifier(uint8_t mod) {
    if (!_connected) return;
    uint8_t report[KEY_REPORT_SIZE];
    memset(report, 0, KEY_REPORT_SIZE);
    report[0] = mod;
    sendReport(report);
}

void BleKeyboard::releaseModifier(uint8_t mod) {
    releaseAll();
}

// ============================================================================
// Battery
// ============================================================================

void BleKeyboard::setBatteryLevel(uint8_t level) {
    _batteryLevel = level;
    if (_pBattery && _connected) {
        _pBattery->setValue(&_batteryLevel, 1);
        _pBattery->notify();
    }
}

#endif // BLE_KEYBOARD_H