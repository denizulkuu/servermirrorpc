#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// ============================================================================
// Display Task - ST7789 240x320 TFT Display Management
// ============================================================================
// Manages the ST7789 display for the ESP32-S3 BLE HID Keyboard project.
// Shows:
//   - Status bar (connection state, battery)
//   - CP437 character display area (HID echo)
//   - Sent text log area
//
// Pin Configuration (from main.ino / user setup):
//   SCLK = GPIO 5
//   MOSI = GPIO 6
//   CS   = GPIO 7
//   DC   = GPIO 15
//   RST  = GPIO 16
//   BL   = -1 (optional backlight, not connected)

// ---- Display Dimensions ----
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

// ---- Color Constants (16-bit RGB565) ----
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_ORANGE      0xFD20
#define COLOR_DARK_GRAY   0x39E7
#define COLOR_LIGHT_GRAY  0xC618
#define COLOR_NAVY        0x000F
#define COLOR_DARK_GREEN  0x03E0
#define COLOR_MAROON      0x7800

// ---- Layout Constants ----
#define STATUS_BAR_HEIGHT   30
#define CHAR_AREA_Y         STATUS_BAR_HEIGHT + 2
#define CHAR_AREA_HEIGHT    180
#define LOG_AREA_Y          CHAR_AREA_Y + CHAR_AREA_HEIGHT + 2
#define LOG_AREA_HEIGHT     DISPLAY_HEIGHT - LOG_AREA_Y - 4
#define TEXT_SIZE_STATUS   2
#define TEXT_SIZE_CHAR     2   // "HID" display uses larger font for visibility
#define TEXT_SIZE_LOG      1
#define CHARS_PER_ROW      20
#define MAX_CHAR_ROWS      8

// ============================================================================
// BLE Connection State Enum
// ============================================================================

enum BLEState : uint8_t {
    BLE_DISCONNECTED = 0,
    BLE_ADVERTISING,
    BLE_CONNECTED,
    BLE_SENDING
};

// ============================================================================
// Display Manager Class
// ============================================================================

class DisplayManager {
private:
    Arduino_GFX* _gfx;
    Arduino_DataBus* _bus;
    
    BLEState _currentState;
    BLEState _lastState;
    
    // Character display cursor
    uint8_t _cursorX;
    uint8_t _cursorY;
    
    // Log buffer (simple circular text buffer)
    static const uint8_t LOG_LINES = 6;
    char _logBuffer[LOG_LINES][CHARS_PER_ROW + 1];
    uint8_t _logIndex;
    
    // Sent text tracking
    char _lastSent[CHARS_PER_ROW + 1];
    uint8_t _sentCount;
    
    // Helpers
    void drawStatusBar();
    void drawCharArea();
    void drawLogArea();
    void clearStatusBar();
    void clearCharArea();
    void clearLogArea();
    void addLogLine(const char* line);

public:
    DisplayManager();
    ~DisplayManager();
    
    // ---- Lifecycle ----
    // Initialize the display hardware and draw initial UI
    bool begin();
    
    // ---- State Management ----
    // Update the displayed BLE connection state
    void setState(BLEState state);
    BLEState getState() const { return _currentState; }
    
    // ---- Character Display ----
    // Show a CP437 character on the HID display area (character-by-character)
    void showChar(uint8_t cp);
    
    // Show a string character-by-character in the HID area
    void showString(const char* str);
    
    // Clear the character display area
    void clearDisplay();
    
    // Set cursor for character display
    void setCursor(uint8_t col, uint8_t row);
    
    // ---- Status Updates ----
    // Update battery percentage display
    void setBattery(uint8_t percent);
    
    // Show a status message on the status bar
    void setStatus(const char* status);
    
    // ---- Sent Text Feedback ----
    // Show what's currently being sent via BLE
    void showSentText(const char* text);
    
    // ---- Raw Display Access ----
    Arduino_GFX* getGFX() { return _gfx; }
    
    // ---- Logging ----
    void log(const char* format, ...);
};

// ============================================================================
// Global Display Instance (defined in main.cpp)
// ============================================================================

extern DisplayManager display;

// SPI Pin Definitions (matching user's main.ino setup)
#define TFT_SCLK 5
#define TFT_MOSI 6
#define TFT_CS   7
#define TFT_DC   15
#define TFT_RST  16
#define TFT_BL   -1

// ============================================================================
// Singleton accessor - creates the display instance
// ============================================================================

inline Arduino_GFX* createDisplay() {
    Arduino_DataBus* bus = new Arduino_ESP32SPI(
        TFT_DC,
        TFT_CS,
        TFT_SCLK,
        TFT_MOSI,
        GFX_NOT_DEFINED   // MISO not connected
    );
    
    Arduino_GFX* gfx = new Arduino_ST7789(
        bus,
        TFT_RST,
        0,      // rotation (0 = portrait)
        true,   // IPS display
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT
    );
    
    return gfx;
}

// ============================================================================
// DisplayManager Implementation
// ============================================================================

DisplayManager::DisplayManager()
    : _gfx(nullptr)
    , _bus(nullptr)
    , _currentState(BLE_DISCONNECTED)
    , _lastState(BLE_DISCONNECTED)
    , _cursorX(0)
    , _cursorY(0)
    , _logIndex(0)
    , _sentCount(0)
{
    memset(_logBuffer, 0, sizeof(_logBuffer));
    memset(_lastSent, 0, sizeof(_lastSent));
}

DisplayManager::~DisplayManager() {
    if (_gfx) delete _gfx;
    if (_bus) delete _bus;
}

// ---- begin() ----

bool DisplayManager::begin() {
    Serial.println("Display: Initializing ST7789...");
    
    _bus = new Arduino_ESP32SPI(
        TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED
    );
    
    _gfx = new Arduino_ST7789(
        _bus, TFT_RST, 0, true, DISPLAY_WIDTH, DISPLAY_HEIGHT
    );
    
    _gfx->begin();
    _gfx->fillScreen(COLOR_BLACK);
    
    // Initial screen
    _gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
    _gfx->setTextSize(2);
    _gfx->setCursor(20, 80);
    _gfx->println("ESP32-S3 BLE");
    _gfx->setCursor(20, 110);
    _gfx->println("HID Keyboard");
    _gfx->setTextSize(1);
    _gfx->setCursor(20, 150);
    _gfx->println("Waiting for BLE connection...");
    
    // Draw border
    _gfx->drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, COLOR_DARK_GRAY);
    _gfx->drawRect(1, 1, DISPLAY_WIDTH-2, DISPLAY_HEIGHT-2, COLOR_NAVY);
    
    // Draw initial status bar background
    drawStatusBar();
    drawCharArea();
    drawLogArea();
    
    Serial.println("Display: ST7789 initialized successfully");
    return true;
}

// ---- drawStatusBar() ----

void DisplayManager::drawStatusBar() {
    // Status bar background
    _gfx->fillRect(0, 0, DISPLAY_WIDTH, STATUS_BAR_HEIGHT, COLOR_NAVY);
    _gfx->drawFastHLine(0, STATUS_BAR_HEIGHT, DISPLAY_WIDTH, COLOR_CYAN);
    
    // State label
    _gfx->setTextSize(TEXT_SIZE_STATUS - 1);
    _gfx->setTextColor(COLOR_CYAN, COLOR_NAVY);
    _gfx->setCursor(4, 4);
    
    const char* stateStr;
    uint16_t stateColor;
    
    switch (_currentState) {
        case BLE_DISCONNECTED:
            stateStr = "DISCONNECTED";
            stateColor = COLOR_RED;
            break;
        case BLE_ADVERTISING:
            stateStr = "ADVERTISING..";
            stateColor = COLOR_YELLOW;
            break;
        case BLE_CONNECTED:
            stateStr = "CONNECTED";
            stateColor = COLOR_GREEN;
            break;
        case BLE_SENDING:
            stateStr = "SENDING...";
            stateColor = COLOR_ORANGE;
            break;
        default:
            stateStr = "UNKNOWN";
            stateColor = COLOR_RED;
    }
    
    _gfx->setTextColor(stateColor, COLOR_NAVY);
    _gfx->print("STATUS: ");
    _gfx->println(stateStr);
    
    // Battery indicator (right side)
    _gfx->setTextColor(COLOR_LIGHT_GRAY, COLOR_NAVY);
    _gfx->setCursor(DISPLAY_WIDTH - 60, 4);
    _gfx->print("BAT:100%");
}

// ---- drawCharArea() ----

void DisplayManager::drawCharArea() {
    // Character display area border
    _gfx->drawRect(2, CHAR_AREA_Y - 1, DISPLAY_WIDTH - 4, CHAR_AREA_HEIGHT + 1, COLOR_DARK_GRAY);
    
    // Title
    _gfx->setTextSize(1);
    _gfx->setTextColor(COLOR_LIGHT_GRAY, COLOR_BLACK);
    _gfx->setCursor(6, CHAR_AREA_Y);
    _gfx->print("HID ECHO:");
}

// ---- drawLogArea() ----

void DisplayManager::drawLogArea() {
    // Log area border
    _gfx->drawRect(2, LOG_AREA_Y - 1, DISPLAY_WIDTH - 4, LOG_AREA_HEIGHT + 1, COLOR_DARK_GRAY);
    
    // Title
    _gfx->setTextSize(1);
    _gfx->setTextColor(COLOR_LIGHT_GRAY, COLOR_BLACK);
    _gfx->setCursor(6, LOG_AREA_Y);
    _gfx->print("LOG:");
}

// ---- setState() ----

void DisplayManager::setState(BLEState state) {
    _lastState = _currentState;
    _currentState = state;
    
    if (_currentState != _lastState) {
        // Only redraw status if state changed
        _gfx->fillRect(0, 0, DISPLAY_WIDTH, STATUS_BAR_HEIGHT, COLOR_NAVY);
        drawStatusBar();
        
        // Handle state transitions
        switch (_currentState) {
            case BLE_CONNECTED:
                // Clear the "waiting" text
                _gfx->fillRect(0, 140, DISPLAY_WIDTH, 30, COLOR_BLACK);
                _gfx->setTextColor(COLOR_GREEN, COLOR_BLACK);
                _gfx->setTextSize(1);
                _gfx->setCursor(20, 150);
                _gfx->println("Connected! Auto-sending...");
                break;
            case BLE_DISCONNECTED:
                _gfx->fillRect(0, 140, DISPLAY_WIDTH, 30, COLOR_BLACK);
                _gfx->setTextColor(COLOR_RED, COLOR_BLACK);
                _gfx->setTextSize(1);
                _gfx->setCursor(20, 150);
                _gfx->println("Disconnected. Advertising...");
                break;
            default:
                break;
        }
    }
}

// ---- showChar() ----

void DisplayManager::showChar(uint8_t cp) {
    if (!_gfx) return;
    
    // Calculate pixel position in character area
    uint8_t charWidth = 12;   // Approximate width for text size
    uint8_t charHeight = 16;  // Approximate height for text size
    uint8_t col = _cursorX % CHARS_PER_ROW;
    uint8_t row = _cursorY;
    
    uint16_t x = 8 + (col * charWidth);
    uint16_t y = CHAR_AREA_Y + 14 + (row * (charHeight + 2));
    
    // If past display area, reset
    if (y + charHeight > CHAR_AREA_Y + CHAR_AREA_HEIGHT) {
        // Clear area and reset
        _gfx->fillRect(4, CHAR_AREA_Y + 10, DISPLAY_WIDTH - 8, CHAR_AREA_HEIGHT - 12, COLOR_BLACK);
        _cursorX = 0;
        _cursorY = 0;
        col = 0;
        row = 0;
        x = 8;
        y = CHAR_AREA_Y + 14;
    }
    
    // Display the character
    _gfx->setTextSize(2);
    _gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
    
    // Only print printable characters; show hex for non-printable
    if (cp >= 0x20 && cp != 0x7F) {
        _gfx->setCursor(x, y);
        _gfx->write((char)cp);
    } else if (cp == 0x0A || cp == 0x0D) {
        // Newline: advance cursor to next row
        _cursorX = 0;
        _cursorY++;
        return;
    } else if (cp == 0x08) {
        // Backspace: move cursor back
        if (_cursorX > 0) _cursorX--;
        return;
    } else if (cp == 0x09) {
        // Tab: advance to next tab stop
        _cursorX = ((_cursorX / 4) + 1) * 4;
        return;
    }
    
    // Advance cursor
    _cursorX++;
    if (_cursorX >= CHARS_PER_ROW) {
        _cursorX = 0;
        _cursorY++;
    }
}

// ---- showString() ----

void DisplayManager::showString(const char* str) {
    if (!str) return;
    while (*str) {
        showChar((uint8_t)*str);
        str++;
    }
    // Store as last sent
    strncpy(_lastSent, str, CHARS_PER_ROW);
    _lastSent[CHARS_PER_ROW] = 0;
}

// ---- clearDisplay() ----

void DisplayManager::clearDisplay() {
    _gfx->fillRect(4, CHAR_AREA_Y + 10, DISPLAY_WIDTH - 8, CHAR_AREA_HEIGHT - 12, COLOR_BLACK);
    _cursorX = 0;
    _cursorY = 0;
}

// ---- setCursor() ----

void DisplayManager::setCursor(uint8_t col, uint8_t row) {
    _cursorX = col % CHARS_PER_ROW;
    _cursorY = row;
}

// ---- setBattery() ----

void DisplayManager::setBattery(uint8_t percent) {
    _gfx->setTextSize(TEXT_SIZE_STATUS - 1);
    _gfx->setTextColor(COLOR_LIGHT_GRAY, COLOR_NAVY);
    _gfx->fillRect(DISPLAY_WIDTH - 60, 4, 56, 12, COLOR_NAVY);
    _gfx->setCursor(DISPLAY_WIDTH - 60, 4);
    _gfx->printf("BAT:%d%%", percent > 100 ? 100 : percent);
}

// ---- setStatus() ----

void DisplayManager::setStatus(const char* status) {
    _gfx->setTextSize(1);
    _gfx->setTextColor(COLOR_YELLOW, COLOR_BLACK);
    _gfx->fillRect(0, DISPLAY_HEIGHT - 14, DISPLAY_WIDTH, 14, COLOR_BLACK);
    _gfx->setCursor(4, DISPLAY_HEIGHT - 12);
    _gfx->print(status);
}

// ---- showSentText() ----

void DisplayManager::showSentText(const char* text) {
    if (!text) return;
    _gfx->setTextSize(1);
    _gfx->setTextColor(COLOR_GREEN, COLOR_BLACK);
    _gfx->fillRect(0, 170, DISPLAY_WIDTH, 16, COLOR_BLACK);
    _gfx->setCursor(4, 172);
    _gfx->printf("Sent: %s", text);
}

// ---- addLogLine() ----

void DisplayManager::addLogLine(const char* line) {
    if (!line) return;
    
    // Store in circular buffer
    strncpy(_logBuffer[_logIndex], line, CHARS_PER_ROW);
    _logBuffer[_logIndex][CHARS_PER_ROW] = 0;
    _logIndex = (_logIndex + 1) % LOG_LINES;
    
    // Redraw log area
    _gfx->setTextSize(1);
    uint8_t y = LOG_AREA_Y + 14;
    _gfx->fillRect(4, LOG_AREA_Y + 10, DISPLAY_WIDTH - 8, LOG_AREA_HEIGHT - 14, COLOR_BLACK);
    
    for (uint8_t i = 0; i < LOG_LINES; i++) {
        uint8_t idx = (_logIndex + i) % LOG_LINES;
        if (_logBuffer[idx][0] != 0) {
            _gfx->setTextColor(COLOR_LIGHT_GRAY, COLOR_BLACK);
            _gfx->setCursor(6, y);
            _gfx->print(_logBuffer[idx]);
            y += 14;
        }
    }
}

// ---- log() ----

void DisplayManager::log(const char* format, ...) {
    char buffer[CHARS_PER_ROW + 1];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    addLogLine(buffer);
    // Also output to serial
    Serial.println(buffer);
}

#endif // DISPLAY_TASK_H