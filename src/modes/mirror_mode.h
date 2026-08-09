#ifndef MIRROR_MODE_H
#define MIRROR_MODE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Arduino_GFX_Library.h>
#include <TJpg_Decoder.h>

#define MIRROR_SSID       "ULKU"
#define MIRROR_PASSWORD   "1108117976Dm"
#define MIRROR_WS_PORT    81
#define JPEG_BUFFER_SIZE  (50 * 1024)

class MirrorMode {
private:
    Arduino_GFX* _gfx;
    WebSocketsServer* _webSocket;
    uint8_t* _jpegBuffer;
    uint32_t _jpegBufferPos;
    uint32_t _expectedJpegSize;
    bool _wifiConnected;
    bool _relayConnected;
    int _frameCount;
    static MirrorMode* _instance;
    static void _wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

    bool connectWiFi() {
        _gfx->fillScreen(BLACK);
        _gfx->setTextColor(WHITE); _gfx->setTextSize(2);
        _gfx->setCursor(10,10); _gfx->println("SCREEN MIRROR");
        _gfx->setTextSize(1);
        _gfx->setCursor(10,50); _gfx->printf("SSID: %s", MIRROR_SSID);
        _gfx->setCursor(10,65); _gfx->print("Connecting");

        WiFi.mode(WIFI_STA);
        WiFi.begin(MIRROR_SSID, MIRROR_PASSWORD);
        int a=0;
        while(WiFi.status()!=WL_CONNECTED && a<30){delay(500);_gfx->print(".");a++;}
        if(WiFi.status()!=WL_CONNECTED){
            _gfx->fillScreen(BLACK);_gfx->setTextColor(RED);_gfx->setCursor(10,10);
            _gfx->println("WIFI FAILED");_wifiConnected=false;return false;
        }
        _wifiConnected=true;
        return true;
    }

public:
    MirrorMode(Arduino_GFX* g):_gfx(g),_webSocket(0),_jpegBuffer(0),_jpegBufferPos(0),
        _expectedJpegSize(0),_wifiConnected(0),_relayConnected(0),_frameCount(0){_instance=this;}
    ~MirrorMode(){end();_instance=0;}

    bool begin() {
        _jpegBuffer=(uint8_t*)ps_malloc(JPEG_BUFFER_SIZE);
        if(!_jpegBuffer){
            _gfx->fillScreen(BLACK);_gfx->setTextColor(RED);
            _gfx->setCursor(10,10);_gfx->println("PSRAM FULL");return false;
        }

        TJpgDec.setJpgScale(1);
        TJpgDec.setSwapBytes(true);
        TJpgDec.setCallback([](int16_t x,int16_t y,uint16_t w,uint16_t h,uint16_t* b){
            if(!_instance||!_instance->_gfx) return false;
            _instance->_gfx->draw16bitRGBBitmap(x,y,b,w,h);
            return true;
        });

        if(!connectWiFi()) return false;
        _webSocket=new WebSocketsServer(MIRROR_WS_PORT);
        _webSocket->begin();
        _webSocket->onEvent(_wsEvent);

        _gfx->fillScreen(BLACK);
        _gfx->setTextColor(GREEN);_gfx->setTextSize(2);
        _gfx->setCursor(10,10);_gfx->println("MIRROR READY");
        _gfx->setTextColor(WHITE);_gfx->setTextSize(1);
        _gfx->setCursor(10,50);_gfx->printf("IP: %s",WiFi.localIP().toString().c_str());
        _gfx->setCursor(10,65);_gfx->printf("Port: %d",MIRROR_WS_PORT);
        return true;
    }

    void end() {
        if(_webSocket){_webSocket->close();delete _webSocket;_webSocket=0;}
        WiFi.disconnect(true);WiFi.mode(WIFI_OFF);
        if(_jpegBuffer){free(_jpegBuffer);_jpegBuffer=0;}
        _wifiConnected=false;_relayConnected=false;
    }

    void loop() {if(_webSocket) _webSocket->loop();}
};

MirrorMode* MirrorMode::_instance=0;

void MirrorMode::_wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    MirrorMode* s=_instance;
    if(!s) return;
    switch(type){
    case WStype_DISCONNECTED:s->_relayConnected=false;break;
    case WStype_CONNECTED:s->_relayConnected=true;s->_webSocket->sendTXT(num,"ESP32-S3-ROOM-01");break;
    case WStype_TEXT:{
        String t((char*)payload);
        if(t.startsWith("JPEG_FRAME_SIZE:")){s->_expectedJpegSize=t.substring(16).toInt();s->_jpegBufferPos=0;
            if(s->_expectedJpegSize>JPEG_BUFFER_SIZE) s->_expectedJpegSize=0;}
        break;}
    case WStype_BIN:
        if(s->_expectedJpegSize>0&&s->_jpegBufferPos+length<=s->_expectedJpegSize)
            memcpy(s->_jpegBuffer+s->_jpegBufferPos,payload,length),s->_jpegBufferPos+=length;
        if(s->_expectedJpegSize>0&&s->_jpegBufferPos>=s->_expectedJpegSize){
            TJpgDec.drawJpg(0,0,s->_jpegBuffer,s->_expectedJpegSize);
            s->_frameCount++; s->_expectedJpegSize=0;
        }
        break;
    }
}

#endif
