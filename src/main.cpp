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

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// FreeRTOS Handles for Dual-Core Networking
TaskHandle_t networkTaskHandle = NULL;
SemaphoreHandle_t frameSemaphore = NULL;
uint8_t* core0Buffer = nullptr;
int core0BufferSize = 0;
volatile int sharedNetW = 0;
volatile int sharedNetH = 0;
volatile bool core0Busy = false;

// Network Task (Pinned to Core 0)
// This task handles all UDP transmission and WebSocket broadcasting.
// It waits for a signal from Core 1, sends the data, and goes back to sleep.
void networkTask(void *pvParameters) {
    for(;;) {
        // Wait for Core 1 to signal that a new frame is ready
        if (xSemaphoreTake(frameSemaphore, portMAX_DELAY) == pdTRUE) {
            core0Busy = true; // Lock buffer
            
            // 1. Send to WLED via UDP (Highest Priority, runs every frame)
            wledStreamer.sendFrame(core0Buffer, sharedNetW, sharedNetH);
            
            // 2. Broadcast to Web UI via WebSockets (Throttled to max 5 FPS to prevent TCP choking)
            if (netWeb.webPreviewEnabled) {
                static unsigned long lastWsSend = 0;
                if (millis() - lastWsSend > 200) {
                    lastWsSend = millis();
                    netWeb.broadcastFrame(core0Buffer, sharedNetW, sharedNetH);
                }
            }
            
            // Cleanup any disconnected WebSockets
            netWeb.cleanupClients();
            
            core0Busy = false; // Unlock buffer
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Give serial monitor time to connect
    Serial.println("\n\n--- Starting WLED Video Streamer ---");

    // Initialize buttons and switches
    pinMode(EFFECT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STREAM_ENABLE_PIN, INPUT_PULLUP);

    // Initialize Camera FIRST!
    Serial.println("Initializing Camera...");
    if (!camHandler.begin()) {
        Serial.println("Camera initialization failed. Please check pinout.");
    }

    // Initialize Network & Web Server
    Serial.println("Initializing Network & Web Server...");
    netWeb.begin();

    // Initialize Streamer
    wledStreamer.begin(netWeb.wledIP.c_str(), DDP_PORT);
    
    // --- Initialize Dual-Core Processing ---
    frameSemaphore = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(
        networkTask,      // Function to implement the task
        "NetworkTask",    // Name of the task
        8192,             // Stack size in words (8KB is safe for network stack)
        NULL,             // Task input parameter
        1,                // Priority of the task
        &networkTaskHandle, // Task handle
        0                 // Pin task to Core 0 (Loop runs on Core 1)
    );

    Serial.println("Setup Complete!");
}

void loop() {
    bool streamEnabled = false;
    int targetSoftwareBrightness = 255;

    // --- Control Logic (Hardware vs Web) ---
    if (netWeb.currentControlMode == CONTROL_HARDWARE) {
        streamEnabled = (digitalRead(STREAM_ENABLE_PIN) == LOW);

        static unsigned long lastAdcRead = 0;
        static int hwBrt = 255;
        if (millis() - lastAdcRead > 500) {
            lastAdcRead = millis();
            int adcValue = analogRead(LDR_ADC_PIN);
            hwBrt = map(adcValue, 0, 4095, 0, 255);
            netWeb.webCameraBrightness = hwBrt;
        }

        bool currentButtonState = digitalRead(EFFECT_BUTTON_PIN);
        if (currentButtonState == LOW && lastButtonState == HIGH) {
            if (millis() - lastButtonPress > 200) {
                lastButtonPress = millis();
                netWeb.lastInteractionTime = millis();
                
                int nextEffect = (int)netWeb.currentEffect;
                for (int i = 0; i < EFFECT_MAX; i++) {
                    nextEffect++;
                    if (nextEffect >= EFFECT_MAX) nextEffect = 0;
                    if (netWeb.effectMask & (1 << nextEffect)) break;
                }
                netWeb.currentEffect = (VideoEffect)nextEffect;
                
                netWeb.savePreferences();
            }
        }
        lastButtonState = currentButtonState;
        targetSoftwareBrightness = hwBrt;
    } else {
        streamEnabled = netWeb.webStreamEnabled;
        targetSoftwareBrightness = netWeb.webCameraBrightness;
    }

    // --- Attract Mode (Auto-Cycle Effects) ---
    if (netWeb.attractTimeout > 0 && (millis() - netWeb.lastInteractionTime > (netWeb.attractTimeout * 1000UL))) {
        static unsigned long lastAutoChange = 0;
        if (millis() - lastAutoChange > 15000) { // Cycle every 15s in attract mode
            lastAutoChange = millis();
            int nextEffect = (int)netWeb.currentEffect;
            for (int i = 0; i < EFFECT_MAX; i++) {
                nextEffect++;
                if (nextEffect >= EFFECT_MAX) nextEffect = 0;
                if (netWeb.effectMask & (1 << nextEffect)) break;
            }
            netWeb.currentEffect = (VideoEffect)nextEffect;
        }
    }

    // 3. Capture & Process Frame (Only if streaming is enabled!)
    if (streamEnabled) {
        // Limit FPS based on user preference to balance smoothness and network stability
        static unsigned long lastFrameTime = 0;
        unsigned long frameDelay = (netWeb.targetFPS > 0) ? (1000 / netWeb.targetFPS) : 100;
        if (millis() - lastFrameTime > frameDelay) {
            lastFrameTime = millis();

            camera_fb_t* fb = camHandler.captureFrame();
            if (fb) {
                // 4. Process Frame on Core 1
                uint8_t* outputFrame = imgProcessor.processFrame(
                    fb, 
                    netWeb.matrixWidth, 
                    netWeb.matrixHeight, 
                    netWeb.currentEffect,
                    targetSoftwareBrightness
                );

                // 5. Hand-off to Core 0 for Network Transmission
                if (outputFrame) {
                    int requiredSize = netWeb.matrixWidth * netWeb.matrixHeight * 3;
                    
                    // Reallocate shared buffer if matrix size changed
                    if (core0BufferSize != requiredSize) {
                        if (core0Buffer) free(core0Buffer);
                        core0Buffer = (uint8_t*)malloc(requiredSize);
                        core0BufferSize = requiredSize;
                    }
                    
                    // Only pass frame to Core 0 if it isn't currently choked sending the last one.
                    // This acts as a dynamic frame-dropper, ensuring Core 1 NEVER stalls.
                    if (core0Buffer && !core0Busy) {
                        memcpy(core0Buffer, outputFrame, requiredSize);
                        sharedNetW = netWeb.matrixWidth;
                        sharedNetH = netWeb.matrixHeight;
                        
                        // Signal Core 0 to wake up and send
                        xSemaphoreGive(frameSemaphore);
                    }
                }

                // 6. Return Frame Buffer
                camHandler.returnFrame(fb);
            }
        }
    } else {
        // If streaming is off, still cleanup WebSockets periodically
        // since Core 0 networkTask won't be triggered.
        static unsigned long lastCleanup = 0;
        if (millis() - lastCleanup > 1000) {
            lastCleanup = millis();
            netWeb.cleanupClients();
        }
    }

    // Tiny delay to keep the FreeRTOS watchdog happy
    delay(1);
}