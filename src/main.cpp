#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "camera_handler.h"
#include "image_processing.h"
#include "wled_streamer.h"
#include "network_web.h"

// Button state variables
unsigned long lastButtonPress = 0;
bool lastButtonState = HIGH; // Assuming pull-up

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\nStarting WLED Video Streamer");

    // Initialize buttons and switches
    pinMode(EFFECT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STREAM_ENABLE_PIN, INPUT_PULLUP);

    // Initialize Network (Blocks until Wi-Fi connects via WiFiManager)
    netWeb.begin();

    // Initialize Camera
    if (!camHandler.begin()) {
        Serial.println("Camera initialization failed. Please check pinout.");
        // Try to continue anyway, though capture will fail
    }

    // Initialize Streamer
    wledStreamer.begin(netWeb.wledIP.c_str(), DDP_PORT);
}

void loop() {
    bool streamEnabled = false;

    // --- Control Logic (Hardware vs Web) ---
    if (netWeb.currentControlMode == CONTROL_HARDWARE) {
        // 0. Stream Toggle (Hardware)
        streamEnabled = (digitalRead(STREAM_ENABLE_PIN) == LOW);

        // 1. Handle ADC to Camera adjustments
        static unsigned long lastAdcRead = 0;
        if (millis() - lastAdcRead > 500) {
            lastAdcRead = millis();
            int hwBrt = camHandler.getBrightnessFromADC();
            camHandler.setBrightness(hwBrt);
            netWeb.webCameraBrightness = hwBrt; // Sync for status output
        }

        // 2. Handle Button for cycling effects
        bool currentButtonState = digitalRead(EFFECT_BUTTON_PIN);
        if (currentButtonState == LOW && lastButtonState == HIGH) {
            if (millis() - lastButtonPress > 200) { // 200ms debounce
                lastButtonPress = millis();
                
                int nextEffect = (int)netWeb.currentEffect + 1;
                if (nextEffect >= EFFECT_MAX) nextEffect = 0;
                netWeb.currentEffect = (VideoEffect)nextEffect;
                
                Serial.printf("Effect changed to: %s\n", imgProcessor.getEffectName(netWeb.currentEffect));
                netWeb.savePreferences(); // Save state
            }
        }
        lastButtonState = currentButtonState;

    } else {
        // --- Web Control Mode ---
        streamEnabled = netWeb.webStreamEnabled;
        
        // 1. Handle Web-based Camera adjustments
        static int lastWebBrt = -99;
        if (netWeb.webCameraBrightness != lastWebBrt) {
            camHandler.setBrightness(netWeb.webCameraBrightness);
            lastWebBrt = netWeb.webCameraBrightness;
        }
        
        // Effects are updated asynchronously by the web server
    }

    // 2.5 Periodic Status Output (Every 5 seconds)
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 5000) {
        lastStatusPrint = millis();
        Serial.println("\n--- Status Update ---");
        Serial.printf("Device IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Target WLED: %s:%d\n", netWeb.wledIP.c_str(), DDP_PORT);
        Serial.printf("Matrix Size: %dx%d\n", netWeb.matrixWidth, netWeb.matrixHeight);
        Serial.printf("Control Source: %s\n", netWeb.currentControlMode == CONTROL_HARDWARE ? "HARDWARE PINS" : "WEB UI");
        Serial.printf("Current Effect: %s\n", imgProcessor.getEffectName(netWeb.currentEffect));
        Serial.printf("Stream State: %s\n", streamEnabled ? "ACTIVE (Pushing to WLED)" : "PAUSED (WLED standard effects)");
        Serial.println("---------------------");
    }

    // 3. Capture & Process Frame (Only if streaming is enabled!)
    if (streamEnabled) {
        // Limit to ~15 FPS (66ms) to balance smooth video with network stability
        static unsigned long lastFrameTime = 0;
        if (millis() - lastFrameTime > 66) {
            lastFrameTime = millis();

            camera_fb_t* fb = camHandler.captureFrame();
            if (fb) {
                // 4. Process Frame
                uint8_t* outputFrame = imgProcessor.processFrame(
                    fb, 
                    netWeb.matrixWidth, 
                    netWeb.matrixHeight, 
                    netWeb.currentEffect
                );

                // 5. Send to WLED
                if (outputFrame) {
                    wledStreamer.sendFrame(outputFrame, netWeb.matrixWidth, netWeb.matrixHeight);
                }

                // 6. Return Frame Buffer
                camHandler.returnFrame(fb);
            }
        }
    }
}