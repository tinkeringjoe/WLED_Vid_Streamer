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
    delay(1000); // Give serial monitor time to connect
    Serial.println("\n\n--- Starting WLED Video Streamer ---");

    // Initialize buttons and switches
    pinMode(EFFECT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STREAM_ENABLE_PIN, INPUT_PULLUP);

    // Initialize Camera FIRST!
    // The camera requires massive, contiguous blocks of PSRAM. If we start WiFi first, 
    // the network stack fragments the memory pool and causes a kernel panic!
    Serial.println("Initializing Camera...");
    if (!camHandler.begin()) {
        Serial.println("Camera initialization failed. Please check pinout.");
    }

    // Initialize Network & Web Server
    Serial.println("Initializing Network & Web Server...");
    netWeb.begin();

    // Initialize Streamer
    wledStreamer.begin(netWeb.wledIP.c_str(), DDP_PORT);
    Serial.println("Setup Complete!");
}

void loop() {
    bool streamEnabled = false;
    int targetSoftwareBrightness = 255;

    // --- Control Logic (Hardware vs Web) ---
    if (netWeb.currentControlMode == CONTROL_HARDWARE) {
        // 0. Stream Toggle (Hardware)
        streamEnabled = (digitalRead(STREAM_ENABLE_PIN) == LOW);

        static unsigned long lastAdcRead = 0;
        static int hwBrt = 255; // Default 1.0x
        if (millis() - lastAdcRead > 500) {
            lastAdcRead = millis();
            int adcValue = analogRead(LDR_ADC_PIN);
            hwBrt = map(adcValue, 0, 4095, 0, 255); // Map to 0-255 Software Brightness
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
                
                netWeb.savePreferences(); // Save state
            }
        }
        lastButtonState = currentButtonState;

        // The target brightness is our hardware pot
        targetSoftwareBrightness = hwBrt;

    } else {
        // --- Web Control Mode ---
        streamEnabled = netWeb.webStreamEnabled;
        targetSoftwareBrightness = netWeb.webCameraBrightness; // Values 0-255
    }

    // Periodic Status Output removed for performance optimization

    // 3. Capture & Process Frame (Only if streaming is enabled!)
    if (streamEnabled) {
        // Limit FPS based on user preference to balance smoothness and network stability
        static unsigned long lastFrameTime = 0;
        unsigned long frameDelay = (netWeb.targetFPS > 0) ? (1000 / netWeb.targetFPS) : 100;
        if (millis() - lastFrameTime > frameDelay) {
            lastFrameTime = millis();

            camera_fb_t* fb = camHandler.captureFrame();
            if (fb) {
                // 4. Process Frame
                uint8_t* outputFrame = imgProcessor.processFrame(
                    fb, 
                    netWeb.matrixWidth, 
                    netWeb.matrixHeight, 
                    netWeb.currentEffect,
                    targetSoftwareBrightness // Apply infinite software brightness
                );

                // 5. Send to WLED
                if (outputFrame) {
                    wledStreamer.sendFrame(outputFrame, netWeb.matrixWidth, netWeb.matrixHeight);

                    // Clone the exact same processed frame to the Web UI via WebSockets
                    netWeb.broadcastFrame(outputFrame, netWeb.matrixWidth, netWeb.matrixHeight);
                }

                // 6. Return Frame Buffer
                camHandler.returnFrame(fb);
            }
        }
    }
    
    // Cleanup any disconnected WebSockets so we don't leak memory
    netWeb.cleanupClients();

    // Tiny delay to keep the FreeRTOS watchdog happy and ensure Wi-Fi task isn't starved
    // when streaming is disabled and the loop spins freely.
    delay(1);
}