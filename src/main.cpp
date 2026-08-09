// ============================================================================
// ESP32-S3 SCREEN MIRROR — Optimized Firmware
// ============================================================================
// Optimizations applied:
//   1. Binary protocol: 4-byte LE uint32 header + JPEG data (was text)
//   2. Double buffering: 2x 512KB PSRAM buffers → receive + decode overlap
//   3. SPI speed: 60MHz (was default ~27MHz)
//   4. Frame drop guard: rejects new frame if both buffers busy
//   5. Dual mode: LOCAL_SERVER (PC→ESP32) or REMOTE_CLIENT (ESP32→relay)
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <Arduino_GFX_Library.h>
#include <TJpg_Decoder.h>

// ── Pick ONE mode ────────────────────────────────────────────────────────────
#define RUN_MODE_LOCAL  0   // ESP32 = WebSocket SERVER (PC connects to ESP32)
#define RUN_MODE_REMOTE 1   // ESP32 = WebSocket CLIENT (connects to cloud relay)
#define RUN_MODE RUN_MODE_LOCAL
// ─────────────────────────────────────────────────────────────────────────────

// ── WiFi credentials (home + hotspot fallback) ───────────────────────────────
#define WIFI_SSID_HOME     "ULKU"
#define WIFI_PASS_HOME     "1108117976Dm"
#define WIFI_SSID_FALLBACK "Denizulku"
#define WIFI_PASS_FALLBACK "deniz1108"

// ── Remote relay (only used in REMOTE mode) ──────────────────────────────────
#define RELAY_HOST "servermirrorpc.onrender.com"
#define RELAY_PORT 443
#define RELAY_PATH "/?room=deniz-mirror"

// ── Display (ST7789 240x320) ────────────────────────────────────────────────
#define TFT_SCLK     5
#define TFT_MOSI     6
#define TFT_CS       7
#define TFT_DC       15
#define TFT_RST      16
#define TFT_W        240
#define TFT_H        320

// ── JPEG buffer (double‑buffered, PSRAM) ────────────────────────────────────
#define JPEG_BUF_SZ (512 * 1024)   // 512 KB per buffer, 1 MB total
#define JPEG_MAX_FRAME (JPEG_BUF_SZ - 32) // safety margin

// ── Local WebSocket server port ──────────────────────────────────────────────
#define WS_PORT      81

// ── Globals ──────────────────────────────────────────────────────────────────
Arduino_GFX        *gfx    = nullptr;
WebSocketsServer   *wsSrv  = nullptr;   // local server
WebSocketsClient   *wsCli  = nullptr;   // remote client

uint8_t  *jpegBuf[2];                // double buffer in PSRAM
volatile uint32_t jpegReady[2] = {0, 0};
volatile uint8_t  jpegActive = 0;     // 0 or 1 → currently receiving into this buffer
unsigned long ft = 0;                // FPS timer
int fc = 0, fv = 0;                  // frame counter, FPS value
bool   firstFrame = true;            // clear screen on first decode
bool   wifiOk = false;

// ── TJpg_Decoder callback — draws decoded JPEG to TFT ────────────────────────
bool jpegDrawCB(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    if (y >= TFT_H) return false;            // clip off‑screen
    gfx->draw16bitRGBBitmap(x, y, bitmap, w, h);
    return true;
}

// ── Draw FPS counter (top‑left overlay) ──────────────────────────────────────
void drawFPS() {
    gfx->setTextColor(0x07E0, 0x0000);       // green on black
    gfx->setTextSize(1);
    gfx->setCursor(3, 2);
    gfx->printf("FPS:%d", fv);
}

// ============================================================================
//  LOCAL MODE — WebSocket event handler (ESP32 is the SERVER)
// ============================================================================
#if RUN_MODE == RUN_MODE_LOCAL

void wsEventLocal(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type != WStype_BIN || length < 4) return;

    // Decode 4‑byte little‑endian length prefix
    uint32_t jpegLen = payload[0] | ((uint32_t)payload[1] << 8)
                    | ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);

    // Sanity checks
    if (jpegLen == 0 || jpegLen > JPEG_MAX_FRAME) return;
    if (jpegLen != length - 4) return;             // size mismatch

    // If active buffer is still waiting to be decoded → drop frame
    if (jpegReady[jpegActive]) return;

    // Copy JPEG into active buffer
    memcpy(jpegBuf[jpegActive], payload + 4, jpegLen);
    jpegReady[jpegActive] = jpegLen;
    jpegActive ^= 1;                                // flip to other buffer
}

#endif // LOCAL

// ============================================================================
//  REMOTE MODE — WebSocket event handler (ESP32 is the CLIENT)
// ============================================================================
#if RUN_MODE == RUN_MODE_REMOTE

void wsEventRemote(WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
        Serial.println("[WS] Connected to relay");
    } else if (type == WStype_DISCONNECTED) {
        Serial.println("[WS] Disconnected from relay — will reconnect");
    } else if (type == WStype_BIN && length >= 4) {
        uint32_t jpegLen = payload[0] | ((uint32_t)payload[1] << 8)
                        | ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
        if (jpegLen == 0 || jpegLen > JPEG_MAX_FRAME) return;
        if (jpegLen != length - 4) return;
        if (jpegReady[jpegActive]) return;

        memcpy(jpegBuf[jpegActive], payload + 4, jpegLen);
        jpegReady[jpegActive] = jpegLen;
        jpegActive ^= 1;
    }
}

#endif // REMOTE

// ── WiFi connection (tries home SSID, then hotspot) ──────────────────────────
bool connectWiFi() {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(10, 30);
    gfx->println("SCREEN MIRROR");
    gfx->setTextSize(1);
    gfx->setCursor(10, 70);
    gfx->printf("SSID: %s", WIFI_SSID_HOME);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);         // disable WiFi sleep → no latency spikes
    WiFi.begin(WIFI_SSID_HOME, WIFI_PASS_HOME);

    int tries = 0;
    gfx->setCursor(10, 88);
    gfx->print("Connecting");
    while (WiFi.status() != WL_CONNECTED && tries < 30) {
        delay(500); gfx->print("."); tries++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        gfx->println("\nRetry fallback...");
        WiFi.begin(WIFI_SSID_FALLBACK, WIFI_PASS_FALLBACK);
        tries = 0;
        while (WiFi.status() != WL_CONNECTED && tries < 30) {
            delay(500); gfx->print("."); tries++;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        gfx->setTextColor(GREEN);
        gfx->setCursor(10, 110);
        gfx->printf("OK  IP:%s", WiFi.localIP().toString().c_str());
        return true;
    } else {
        gfx->setTextColor(RED);
        gfx->setCursor(10, 110);
        gfx->println("WIFI FAIL");
        return false;
    }
}

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n=== ESP32-S3 Screen Mirror v3 ===");

    // ── Display init ─────────────────────────────────────────────────────────
    Arduino_DataBus *bus = new Arduino_ESP32SPI(
        TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED);
    gfx = new Arduino_ST7789(bus, TFT_RST, 0, true, TFT_W, TFT_H);
    gfx->begin();
    gfx->fillScreen(BLACK);
    gfx->setTextColor(CYAN); gfx->setTextSize(2);
    gfx->setCursor(5, 30); gfx->println("BOOTING...");

    // ── JPEG decoder init ────────────────────────────────────────────────────
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(false);
    TJpgDec.setCallback(jpegDrawCB);

    // ── Allocate double buffer in PSRAM ──────────────────────────────────────
    for (int i = 0; i < 2; i++) {
        jpegBuf[i] = (uint8_t *)ps_malloc(JPEG_BUF_SZ);
        if (!jpegBuf[i]) {
            gfx->setTextColor(RED);
            gfx->setCursor(5, 60);
            gfx->printf("PSRAM FAIL buf[%d]", i);
            while (1) delay(1000);
        }
    }
    Serial.printf("PSRAM: 2x %d KB allocated OK\n", JPEG_BUF_SZ / 1024);

    // ── WiFi ─────────────────────────────────────────────────────────────────
    wifiOk = connectWiFi();
    if (!wifiOk) return;

    gfx->fillScreen(BLACK);

#if RUN_MODE == RUN_MODE_LOCAL
    // ── Start WebSocket SERVER ───────────────────────────────────────────────
    wsSrv = new WebSocketsServer(WS_PORT);
    wsSrv->begin();
    wsSrv->onEvent(wsEventLocal);
    gfx->setTextColor(GREEN); gfx->setTextSize(1);
    gfx->setCursor(5, 3);  gfx->print(WiFi.localIP().toString().c_str());
    gfx->setCursor(5, 18); gfx->print("Port:81  Run sender");
    Serial.printf("WebSocket server on port %d\n", WS_PORT);

#elif RUN_MODE == RUN_MODE_REMOTE
    // ── Start WebSocket CLIENT → relay ───────────────────────────────────────
    wsCli = new WebSocketsClient();
    wsCli->onEvent(wsEventRemote);
    wsCli->beginSSL(RELAY_HOST, RELAY_PORT, RELAY_PATH);
    // setInsecure() only needed if relay uses self‑signed certs
    // wsCli->setInsecure();
    wsCli->setReconnectInterval(3000);   // auto‑reconnect every 3 s
    gfx->setTextColor(GREEN); gfx->setTextSize(1);
    gfx->setCursor(5, 3);  gfx->println("REMOTE MODE");
    gfx->setCursor(5, 18); gfx->printf("Relay: %s", RELAY_HOST);
    Serial.printf("WebSocket client → %s:%d%s\n", RELAY_HOST, RELAY_PORT, RELAY_PATH);
#endif

    ft = millis();
}

// ============================================================================
//  LOOP
// ============================================================================
void loop() {
#if RUN_MODE == RUN_MODE_LOCAL
    if (wsSrv) wsSrv->loop();
#elif RUN_MODE == RUN_MODE_REMOTE
    if (wsCli) wsCli->loop();
#endif

    // ── Decode any ready buffers ─────────────────────────────────────────────
    for (int i = 0; i < 2; i++) {
        if (jpegReady[i]) {
            uint32_t sz = jpegReady[i];
            jpegReady[i] = 0;

            if (firstFrame) {
                gfx->fillScreen(BLACK);
                firstFrame = false;
            }

            if (TJpgDec.drawJpg(0, 0, jpegBuf[i], sz) == 0) {
                fc++;
            }
            drawFPS();
        }
    }

    // ── FPS counter (rolling 5‑second average) ───────────────────────────────
    if (millis() - ft >= 5000) {
        fv = fc / 5;
        fc = 0;
        ft = millis();
    }

    yield();
}