#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <Arduino.h>
#include "esp_camera.h"

enum VideoEffect {
    EFFECT_NORMAL = 0,
    EFFECT_PSYCHEDELIC = 1,
    EFFECT_RETRO_8BIT = 2,
    EFFECT_CYBERPUNK = 3,
    EFFECT_THERMAL = 4,
    EFFECT_MATRIX = 5,
    EFFECT_EDGE_GLOW = 6,
    EFFECT_MAX
};

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    // Returns a pointer to an internal RGB888 buffer of size (matrixW * matrixH * 3)
    // The caller can then send this buffer via UDP.
    uint8_t* processFrame(camera_fb_t* fb, int targetW, int targetH, VideoEffect effect, int softwareBrightness);

    // Get current effect name as string (for Web UI)
    const char* getEffectName(VideoEffect effect);

    bool triggerBgCapture = false;

private:
    uint8_t* outputBuffer = nullptr;
    uint8_t* bgBuffer = nullptr;
    uint8_t* trailBuffer = nullptr;
    int currentBufferW = 0;
    int currentBufferH = 0;
    
    // Lookup tables for ultra-fast scaling
    int* xMap = nullptr;
    int* yMap = nullptr;

    // Internal helper to convert RGB565 to RGB888
    void rgb565ToRgb888(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b);
    
    // Nearest neighbor downscaling + RGB conversion + Effects
    void processNearestNeighbor(camera_fb_t* fb, int targetW, int targetH, VideoEffect effect);
};

extern ImageProcessor imgProcessor;

#endif // IMAGE_PROCESSING_H

