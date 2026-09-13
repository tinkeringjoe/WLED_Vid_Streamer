#include "network_web.h"
#include "web_html.h"
#include "config.h"
#include "wled_streamer.h"
#include "camera_handler.h"
#include <ESPmDNS.h>

NetworkWeb netWeb;

NetworkWeb::NetworkWeb() : ws("/ws") {}

void NetworkWeb::broadcastFrame(uint8_t* frame, int width, int height) {
    if (ws.count() > 0) {
        ws.binaryAll(frame, width * height * 3);
    }
}

void NetworkWeb::cleanupClients() {
    ws.cleanupClients();
}

void NetworkWeb::loadPreferences() {
    preferences.begin("wled_stream", false);
    wledIP = preferences.getString("wledIP", DEFAULT_WLED_IP);
    matrixWidth = preferences.getInt("matrixWidth", DEFAULT_MATRIX_W);
    matrixHeight = preferences.getInt("matrixHeight", DEFAULT_MATRIX_H);
    currentEffect = (VideoEffect)preferences.getInt("effect", EFFECT_NORMAL);
    ddpColorOrder = preferences.getInt("ddpOrder", 0);
    
    currentControlMode = (ControlMode)preferences.getInt("ctrlMode", CONTROL_WEB);
    webStreamEnabled = preferences.getBool("webStream", false); // Default to false to prevent UDP flood on boot
    webPreviewEnabled = preferences.getBool("preview", true);
    
    webCameraBrightness = preferences.getInt("webBrt", 255);
    
    cameraVFlip = preferences.getBool("vflip", false);
    cameraHMirror = preferences.getBool("hmirror", false);
    cameraContrast = preferences.getInt("contrast", 2);
    cameraSaturation = preferences.getInt("saturation", 2);
    targetFPS = preferences.getInt("fps", 10);
    
    enableBgSub = preferences.getBool("bgSub", false);
    bgThreshold = preferences.getInt("bgThresh", 60);
    trailAmount = preferences.getInt("trail", 0);
    attractTimeout = preferences.getInt("attract", 0);
    effectMask = preferences.getUChar("effMask", 127);
    
    preferences.end();
    
    camHandler.updateSettings(cameraVFlip, cameraHMirror, cameraContrast, cameraSaturation);
    
    // FORCE Web Control Mode!
    // If the user previously saved Hardware Mode to memory, the floating LDR pin 
    // will violently strobe the software brightness and ruin the picture. 
    currentControlMode = CONTROL_WEB;
}

void NetworkWeb::savePreferences() {
    preferences.begin("wled_stream", false);
    preferences.putString("wledIP", wledIP);
    preferences.putInt("matrixWidth", matrixWidth);
    preferences.putInt("matrixHeight", matrixHeight);
    preferences.putInt("effect", (int)currentEffect);
    preferences.putInt("ddpOrder", ddpColorOrder);
    
    preferences.putInt("ctrlMode", (int)currentControlMode);
    preferences.putBool("webStream", webStreamEnabled);
    preferences.putBool("preview", webPreviewEnabled);
    
    preferences.putInt("webBrt", webCameraBrightness);
    
    preferences.putBool("vflip", cameraVFlip);
    preferences.putBool("hmirror", cameraHMirror);
    preferences.putInt("contrast", cameraContrast);
    preferences.putInt("saturation", cameraSaturation);
    preferences.putInt("fps", targetFPS);
    
    preferences.putBool("bgSub", enableBgSub);
    preferences.putInt("bgThresh", bgThreshold);
    preferences.putInt("trail", trailAmount);
    preferences.putInt("attract", attractTimeout);
    preferences.putUChar("effMask", effectMask);
    
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
    if(var == "S6") return netWeb.currentEffect == 6 ? "selected" : "";
    
    if(var == "M0") return (netWeb.effectMask & (1<<0)) ? "checked" : "";
    if(var == "M1") return (netWeb.effectMask & (1<<1)) ? "checked" : "";
    if(var == "M2") return (netWeb.effectMask & (1<<2)) ? "checked" : "";
    if(var == "M3") return (netWeb.effectMask & (1<<3)) ? "checked" : "";
    if(var == "M4") return (netWeb.effectMask & (1<<4)) ? "checked" : "";
    if(var == "M5") return (netWeb.effectMask & (1<<5)) ? "checked" : "";
    if(var == "M6") return (netWeb.effectMask & (1<<6)) ? "checked" : "";
    
    if(var == "C0") return netWeb.ddpColorOrder == 0 ? "selected" : "";
    if(var == "C1") return netWeb.ddpColorOrder == 1 ? "selected" : "";
    if(var == "C2") return netWeb.ddpColorOrder == 2 ? "selected" : "";
    if(var == "C3") return netWeb.ddpColorOrder == 3 ? "selected" : "";
    if(var == "C4") return netWeb.ddpColorOrder == 4 ? "selected" : "";
    if(var == "C5") return netWeb.ddpColorOrder == 5 ? "selected" : "";
    
    if(var == "CTRL_HW") return netWeb.currentControlMode == CONTROL_HARDWARE ? "selected" : "";
    if(var == "CTRL_WEB") return netWeb.currentControlMode == CONTROL_WEB ? "selected" : "";
    if(var == "STREAM_CHK") return netWeb.webStreamEnabled ? "checked" : "";
    if(var == "PREVIEW_CHK") return netWeb.webPreviewEnabled ? "checked" : "";
    
    if(var == "BRIGHTNESS") return String(netWeb.webCameraBrightness);
    
    if(var == "VFLIP_CHK") return netWeb.cameraVFlip ? "checked" : "";
    if(var == "HMIRROR_CHK") return netWeb.cameraHMirror ? "checked" : "";
    if(var == "CONTRAST") return String(netWeb.cameraContrast);
    if(var == "SATURATION") return String(netWeb.cameraSaturation);
    
    if(var == "F1") return netWeb.targetFPS == 1 ? "selected" : "";
    if(var == "F2") return netWeb.targetFPS == 2 ? "selected" : "";
    if(var == "F3") return netWeb.targetFPS == 3 ? "selected" : "";
    if(var == "F4") return netWeb.targetFPS == 4 ? "selected" : "";
    if(var == "F5") return netWeb.targetFPS == 5 ? "selected" : "";
    if(var == "F6") return netWeb.targetFPS == 6 ? "selected" : "";
    if(var == "F7") return netWeb.targetFPS == 7 ? "selected" : "";
    if(var == "F8") return netWeb.targetFPS == 8 ? "selected" : "";
    if(var == "F9") return netWeb.targetFPS == 9 ? "selected" : "";
    if(var == "F10") return netWeb.targetFPS == 10 ? "selected" : "";
    if(var == "F12") return netWeb.targetFPS == 12 ? "selected" : "";
    if(var == "F15") return netWeb.targetFPS == 15 ? "selected" : "";
    if(var == "F20") return netWeb.targetFPS == 20 ? "selected" : "";
    
    if(var == "BGS") return netWeb.enableBgSub ? "checked" : "";
    if(var == "BGT") return String(netWeb.bgThreshold);
    if(var == "TRL") return String(netWeb.trailAmount);
    if(var == "ATT") return String(netWeb.attractTimeout);
    
    return String();
}

void NetworkWeb::setupWebServer() {
    server = new AsyncWebServer(80);

    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Web Server: Client requested /");
        request->send(200, "text/html", index_html, processor);
    });

    server->on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        Serial.println("Web Server: Client requested /save");
        netWeb.lastInteractionTime = millis();
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
        if(request->hasParam("prv", true)) {
            netWeb.webPreviewEnabled = request->getParam("prv", true)->value() == "1";
        }
        if(request->hasParam("e", true)) {
            netWeb.currentEffect = (VideoEffect)request->getParam("e", true)->value().toInt();
        }
        if(request->hasParam("b", true)) {
            netWeb.webCameraBrightness = request->getParam("b", true)->value().toInt();
        }
        if(request->hasParam("ddp", true)) {
            netWeb.ddpColorOrder = request->getParam("ddp", true)->value().toInt();
        }
        if(request->hasParam("vf", true)) {
            netWeb.cameraVFlip = request->getParam("vf", true)->value() == "1";
        }
        if(request->hasParam("hm", true)) {
            netWeb.cameraHMirror = request->getParam("hm", true)->value() == "1";
        }
        
        uint8_t newMask = 0;
        if(request->hasParam("m0", true) && request->getParam("m0", true)->value() == "1") newMask |= (1<<0);
        if(request->hasParam("m1", true) && request->getParam("m1", true)->value() == "1") newMask |= (1<<1);
        if(request->hasParam("m2", true) && request->getParam("m2", true)->value() == "1") newMask |= (1<<2);
        if(request->hasParam("m3", true) && request->getParam("m3", true)->value() == "1") newMask |= (1<<3);
        if(request->hasParam("m4", true) && request->getParam("m4", true)->value() == "1") newMask |= (1<<4);
        if(request->hasParam("m5", true) && request->getParam("m5", true)->value() == "1") newMask |= (1<<5);
        if(request->hasParam("m6", true) && request->getParam("m6", true)->value() == "1") newMask |= (1<<6);
        if(newMask == 0) newMask = 1; // Fallback to Normal effect if all disabled
        netWeb.effectMask = newMask;
        
        if(request->hasParam("con", true)) {
            netWeb.cameraContrast = request->getParam("con", true)->value().toInt();
        }
        if(request->hasParam("sat", true)) {
            netWeb.cameraSaturation = request->getParam("sat", true)->value().toInt();
        }
        if(request->hasParam("fps", true)) {
            netWeb.targetFPS = request->getParam("fps", true)->value().toInt();
        }
        if(request->hasParam("bgs", true)) {
            netWeb.enableBgSub = request->getParam("bgs", true)->value() == "1";
        }
        if(request->hasParam("bgt", true)) {
            netWeb.bgThreshold = request->getParam("bgt", true)->value().toInt();
        }
        if(request->hasParam("trl", true)) {
            netWeb.trailAmount = request->getParam("trl", true)->value().toInt();
        }
        if(request->hasParam("att", true)) {
            netWeb.attractTimeout = request->getParam("att", true)->value().toInt();
        }
        
        netWeb.savePreferences();
        camHandler.updateSettings(netWeb.cameraVFlip, netWeb.cameraHMirror, netWeb.cameraContrast, netWeb.cameraSaturation);
        request->send(200, "text/plain", "OK");
    });
    
    server->on("/capture_bg", HTTP_POST, [](AsyncWebServerRequest *request){
        Serial.println("Web Server: Client requested /capture_bg");
        netWeb.lastInteractionTime = millis();
        imgProcessor.triggerBgCapture = true;
        request->send(200, "text/plain", "OK");
    });

    server->on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Rebooting...");
        delay(500);
        ESP.restart();
    });
    
    server->on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        int rssi = WiFi.RSSI();
        String json = "{\"rssi\":" + String(rssi) + "}";
        request->send(200, "application/json", json);
    });

    server->addHandler(&ws);

    server->begin();
    Serial.println("Async Web Server is now listening on port 80");
}

void NetworkWeb::begin() {
    loadPreferences();

    // Set a friendly hostname for DHCP client lists/Routers BEFORE connecting
    WiFi.setHostname("wled-vid-streamer");
    
    // --- Mesh Network Sticky Client Fix ---
    // By default, ESP32 connects to the FIRST node it hears in a mesh network, 
    // even if it's the weakest one. This forces it to scan all channels and connect 
    // to the node with the absolute strongest RSSI (closest router).
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);

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

