#ifndef BLE_MODE_H
#define BLE_MODE_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEHIDDevice.h>
#include "../cp437_map.h"

class BleMode {
private:
    Arduino_GFX* _gfx;
    NimBLEServer* _pServer;
    NimBLEHIDDevice* _pHID;
    NimBLECharacteristic* _pInput;
    NimBLECharacteristic* _pBattery;
    bool _connected;
    uint8_t _batteryLevel;
    uint8_t _report[8];

    bool _autoSendTriggered;
    uint8_t _autoSendIndex;
    bool _autoSendComplete;
    unsigned long _connectedAt;

    void sendReport(const uint8_t* r) {
        if (!_connected || !_pInput) return;
        _pInput->setValue((uint8_t*)r, 8);
        _pInput->notify();
        memcpy(_report, r, 8);
    }
    void press(uint8_t m, uint8_t k) { uint8_t r[8]={0}; r[0]=m; r[2]=k; sendReport(r); }
    void releaseAll() { if(_connected) { uint8_t r[8]={0}; sendReport(r); } }
    void writeKey(uint8_t m, uint8_t k) {
        if(!_connected || k==0) return;
        press(m,k); delayMicroseconds(1200); releaseAll(); delayMicroseconds(600);
    }

    class CB : public NimBLEServerCallbacks {
        BleMode* _p;
    public:
        CB(BleMode* p):_p(p){}
        void onConnect(NimBLEServer*, NimBLEConnInfo&) override {
            Serial.println("[BLE] Connected");
            _p->_connected=true;
            NimBLEDevice::stopAdvertising();
        }
        void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int r) override {
            Serial.printf("[BLE] Disconnected (%d)\n", r);
            _p->_connected=false; _p->_autoSendTriggered=false; _p->_autoSendComplete=false;
            NimBLEDevice::startAdvertising();
        }
    };

    static const uint8_t _desc[];
    static const uint8_t _descSize;

public:
    BleMode(Arduino_GFX* g):_gfx(g),_pServer(0),_pHID(0),_pInput(0),_pBattery(0),
        _connected(0),_batteryLevel(100),_autoSendTriggered(0),_autoSendIndex(0),
        _autoSendComplete(0),_connectedAt(0){memset(_report,0,8);}
    ~BleMode(){end();}

    bool begin() {
        Serial.println("[BLE] Init...");
        NimBLEDevice::deleteAllBonds();
        NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
        NimBLEDevice::init("ESP32-S3 Keyboard");
        NimBLEDevice::setSecurityAuth(false,false,false);
        NimBLEDevice::setPower(9);
        _pServer=NimBLEDevice::createServer();
        _pServer->setCallbacks(new CB(this));
        _pHID=new NimBLEHIDDevice(_pServer);
        _pHID->setReportMap((uint8_t*)_desc, _descSize);
        _pHID->setHidInfo(0,1); _pHID->setManufacturer("ESP32-S3");
        _pHID->setPnp(2,0xE502,0xA111,0x0210);
        _pHID->setBatteryLevel(_batteryLevel);
        _pBattery=_pHID->getBatteryLevel();
        _pInput=_pHID->getBootInput();
        _pServer->start();
        auto* adv=NimBLEDevice::getAdvertising();
        adv->setAppearance(0x03C1);
        adv->addServiceUUID(_pHID->getHidService()->getUUID());
        adv->enableScanResponse(true);
        NimBLEDevice::startAdvertising();
        _gfx->fillScreen(BLACK);
        _gfx->setTextColor(CYAN); _gfx->setTextSize(2);
        _gfx->setCursor(10,10); _gfx->println("BLE MODE");
        _gfx->setTextColor(WHITE); _gfx->setTextSize(1);
        _gfx->setCursor(10,50); _gfx->println("Advertising...");
        return true;
    }

    void end() {
        if(_connected) releaseAll();
        NimBLEDevice::stopAdvertising();
        NimBLEDevice::deinit(true);
        _connected=false;
    }

    void loop() {
        static bool was=0;
        if(_connected&&!was){was=1;_connectedAt=millis();_autoSendTriggered=0;_autoSendComplete=0;
            _gfx->fillScreen(BLACK);_gfx->setTextColor(GREEN);_gfx->setTextSize(2);
            _gfx->setCursor(10,10);_gfx->println("BLE MODE");
            _gfx->setTextColor(WHITE);_gfx->setTextSize(1);
            _gfx->setCursor(10,50);_gfx->println("Connected!");}
        else if(!_connected&&was){was=0;
            _gfx->fillScreen(BLACK);_gfx->setTextColor(CYAN);_gfx->setTextSize(2);
            _gfx->setCursor(10,10);_gfx->println("BLE MODE");
            _gfx->setTextColor(WHITE);_gfx->setTextSize(1);
            _gfx->setCursor(10,50);_gfx->println("Advertising...");}
        if(_connected&&!_autoSendTriggered&&!_autoSendComplete&&millis()-_connectedAt>=1000){
            _autoSendTriggered=1;_autoSendIndex=0;
        }
        if(_autoSendTriggered&&!_autoSendComplete){
            struct{uint8_t m,k; uint16_t d;}seq[]={
                {0,0,300},{0x08,0x15,200},{0,0,500},
                {0,0x11,50},{0,0x12,50},{0,0x17,50},{0,0x08,50},{0,0x13,50},{0,0x04,50},{0,0x07,50},{0,0,200},
                {0,0x28,800},
                {0x02,0x0B,50},{0,0x08,50},{0,0x0F,50},{0,0x0F,50},{0,0x12,50},{0,0x2C,50},{0,0x17,50},{0,0x0B,50},{0,0x08,50},{0,0x15,50},{0,0x08,50},{0x02,0x1E,50},
                {0,0x28,100},{0,0,0}};
            int n=sizeof(seq)/sizeof(seq[0]);
            if(_autoSendIndex<n){
                auto& s=seq[_autoSendIndex];
                if(s.k==0&&s.m==0&&s.d==0){_autoSendTriggered=0;_autoSendComplete=1;}
                else{if(s.k){writeKey(s.m,s.k);} if(s.d>0)delay(s.d);}
                _autoSendIndex++;
            }
        }
    }
};

const uint8_t BleMode::_desc[] PROGMEM = {
    0x05,0x01,0x09,0x06,0xA1,0x01,
    0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,
    0x95,0x01,0x75,0x08,0x81,0x01,
    0x95,0x06,0x75,0x08,0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,
    0x95,0x05,0x75,0x01,0x05,0x08,0x19,0x01,0x29,0x05,0x91,0x02,
    0x95,0x01,0x75,0x03,0x91,0x01,
    0xC0
};
const uint8_t BleMode::_descSize = sizeof(BleMode::_desc);

#endif
