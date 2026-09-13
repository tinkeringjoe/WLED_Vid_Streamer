#ifndef NETWORK_WEB_H
#define NETWORK_WEB_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "image_processing.h"

enum ControlMode {
    CONTROL_HARDWARE = 0,
    CONTROL_WEB = 1
};

class NetworkWeb {
public:
    NetworkWeb();
    void begin();
    
    void broadcastFrame(uint8_t* frame, int width, int height);
    void cleanupClients();

    // Shared state variables
    String wledIP;
    int matrixWidth;
    int matrixHeight;
    VideoEffect currentEffect;
    
    ControlMode currentControlMode;
    bool webStreamEnabled;
    
    // Camera Tuning
    int webCameraBrightness;
    uint8_t ddpColorOrder = 0;
    bool cameraVFlip = false;
    bool cameraHMirror = false;
    int cameraContrast = 2;
    int cameraSaturation = 2;
    int targetFPS = 10;
    
    // Magic Mirror Settings
    bool enableBgSub = false;
    int bgThreshold = 60;
    int trailAmount = 0;
    int attractTimeout = 0; // 0 = Disabled
    unsigned long lastInteractionTime = 0;

    // Load from NVS
    void loadPreferences();
    // Save to NVS
    void savePreferences();

private:
    Preferences preferences;
    AsyncWebServer* server;
    AsyncWebSocket ws;

    void setupWebServer();
    void connectWiFi();
};

extern NetworkWeb netWeb;

#endif // NETWORK_WEB_H

