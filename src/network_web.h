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
    void begin();
    
    // Shared state variables
    String wledIP;
    int matrixWidth;
    int matrixHeight;
    VideoEffect currentEffect;
    
    ControlMode currentControlMode;
    bool webStreamEnabled;
    int webCameraBrightness;

    // Load from NVS
    void loadPreferences();
    // Save to NVS
    void savePreferences();

private:
    Preferences preferences;
    AsyncWebServer* server;

    void setupWebServer();
};

extern NetworkWeb netWeb;

#endif // NETWORK_WEB_H

