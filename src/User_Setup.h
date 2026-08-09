// User_Setup.h for ESP32-S3 + ST7789 240x320 IPS Display
// Matches the exact pinout from main.ino (Arduino_GFX setup)

#define USER_SETUP_INFO "ESP32-S3 ST7789 240x320"

// ---- Driver ----
#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ---- SPI Pins (match main.ino) ----
#define TFT_MISO -1   // Not connected
#define TFT_MOSI 6
#define TFT_SCLK 5
#define TFT_CS   7
#define TFT_DC   15
#define TFT_RST  16

// ---- Backlight (not connected) ----
#define TFT_BL   -1
#define TFT_BACKLIGHT_ON HIGH

// ---- SPI Settings ----
#define SPI_FREQUENCY       27000000   // 27MHz (safe for ST7789)
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// ---- Display Settings ----
#define TFT_INVERSION_ON    // IPS display needs inverted colors
#define TFT_ROTATION 0       // Portrait (matching Arduino_GFX default)

// ---- Fonts ----
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
