#include "network_web.h"
#include "web_html.h"
#include "config.h"
#include "wled_streamer.h"
#include <ESPmDNS.h>

NetworkWeb netWeb;


void NetworkWeb::loadPreferences() {
    preferences.begin("wled_stream", false);
    wledIP = preferences.getString("wledIP", DEFAULT_WLED_IP);
    matrixWidth = preferences.getInt("matrixWidth", DEFAULT_MATRIX_W);
    matrixHeight = preferences.getInt("matrixHeight", DEFAULT_MATRIX_H);
    currentEffect = (VideoEffect)preferences.getInt("effect", EFFECT_NORMAL);
    
    currentControlMode = (ControlMode)preferences.getInt("ctrlMode", CONTROL_HARDWARE);
    webStreamEnabled = preferences.getBool("webStream", true);
    
    webContrast = preferences.getInt("webCont", 0);
    webSaturation = preferences.getInt("webSat", 0);
    webAutoExposure = preferences.getBool("webAE", true);
    webExposureVal = preferences.getInt("webExpV", 300);
    
    preferences.end();
}

void NetworkWeb::savePreferences() {
    preferences.begin("wled_stream", false);
    preferences.putString("wledIP", wledIP);
    preferences.putInt("matrixWidth", matrixWidth);
    preferences.putInt("matrixHeight", matrixHeight);
    preferences.putInt("effect", (int)currentEffect);
    
    preferences.putInt("ctrlMode", (int)currentControlMode);
    preferences.putBool("webStream", webStreamEnabled);
    
    preferences.putInt("webCont", webContrast);
    preferences.putInt("webSat", webSaturation);
    preferences.putBool("webAE", webAutoExposure);
    preferences.putInt("webExpV", webExposureVal);
    
    preferences.end();
}

String processor(const String& var) {
    if(var == "IP") return netWeb.wledIP;
    if(var == "WIDTH") return String(netWeb.matrixWidth);
    if(var == "HEIGHT") return String(netWeb.matrixHeight);
    if(var == "S0") return netWeb.currentEffect == 0 ? "selected" : "";
    if(var == "S1") return netWeb.currentEffect == 1 ? "selected" : "";
    if(var == "S2") return netWeb.currentEffect == 2 ? "selected" : "";
    if(var == "S3") return netWeb.currentEffect == 3 ? "selected" : "";
    if(var == "S4") return netWeb.currentEffect == 4 ? "selected" : "";
    if(var == "S5") return netWeb.currentEffect == 5 ? "selected" : "";
    
    if(var == "CTRL_HW") return netWeb.currentControlMode == CONTROL_HARDWARE ? "selected" : "";
    if(var == "CTRL_WEB") return netWeb.currentControlMode == CONTROL_WEB ? "selected" : "";
    if(var == "STREAM_CHK") return netWeb.webStreamEnabled ? "checked" : "";
    
    if(var == "CONTRAST") return String(netWeb.webContrast);
    if(var == "SATURATION") return String(netWeb.webSaturation);
    if(var == "AE_CHK") return netWeb.webAutoExposure ? "checked" : "";
    if(var == "EXPOSURE") return String(netWeb.webExposureVal);
    
    return String();
}

void NetworkWeb::setupWebServer() {
    server = new AsyncWebServer(80);

    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, processor);
    });

    server->on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("ip", true)) {
            netWeb.wledIP = request->getParam("ip", true)->value();
            wledStreamer.setTargetIP(netWeb.wledIP.c_str());
        }
        if(request->hasParam("w", true)) {
            netWeb.matrixWidth = request->getParam("w", true)->value().toInt();
        }
        if(request->hasParam("h", true)) {
            netWeb.matrixHeight = request->getParam("h", true)->value().toInt();
        }
        if(request->hasParam("m", true)) {
            netWeb.currentControlMode = (ControlMode)request->getParam("m", true)->value().toInt();
        }
        if(request->hasParam("en", true)) {
            netWeb.webStreamEnabled = request->getParam("en", true)->value() == "1";
        }
        if(request->hasParam("e", true)) {
            netWeb.currentEffect = (VideoEffect)request->getParam("e", true)->value().toInt();
        }
        if(request->hasParam("ct", true)) {
            netWeb.webContrast = request->getParam("ct", true)->value().toInt();
        }
        if(request->hasParam("st", true)) {
            netWeb.webSaturation = request->getParam("st", true)->value().toInt();
        }
        if(request->hasParam("ae", true)) {
            netWeb.webAutoExposure = request->getParam("ae", true)->value() == "1";
        }
        if(request->hasParam("ev", true)) {
            netWeb.webExposureVal = request->getParam("ev", true)->value().toInt();
        }
        
        netWeb.savePreferences();
        request->send(200, "text/plain", "OK");
    });

    server->begin();
}

void NetworkWeb::begin() {
    loadPreferences();

    // Set a friendly hostname for DHCP client lists/Routers BEFORE connecting
    WiFi.setHostname("wled-vid-streamer");

    WiFiManager wifiManager;
    // wifiManager.resetSettings(); // Uncomment if you want to force captive portal on boot
    
    wifiManager.setConfigPortalTimeout(180); // 3 minutes timeout
    
    if (!wifiManager.autoConnect("WLED_Streamer_AP")) {
        Serial.println("Failed to connect and hit timeout. Rebooting...");
        delay(3000);
        ESP.restart();
    }

    Serial.println("Connected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Disable Wi-Fi sleep mode. This is CRITICAL for high-throughput UDP streaming
    // and prevents the Web UI from periodically dropping out or timing out.
    WiFi.setSleep(false);

    // Start mDNS responder so you can go to http://wled-vid-streamer.local
    if (MDNS.begin("wled-vid-streamer")) {
        Serial.println("MDNS responder started at http://wled-vid-streamer.local");
    }

    setupWebServer();
}

