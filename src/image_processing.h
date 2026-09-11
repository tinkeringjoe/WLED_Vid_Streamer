#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <Arduino.h>
#include "esp_camera.h"

enum VideoEffect {
    EFFECT_NORMAL = 0,
    EFFECT_SPOOKY,
    EFFECT_GRAYSCALE,
    EFFECT_INVERT,
    EFFECT_SEPIA,
    EFFECT_PSYCHEDELIC,
    EFFECT_MAX // Used to wrap around
};

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    // Returns a pointer to an internal RGB888 buffer of size (matrixW * matrixH * 3)
    // The caller can then send this buffer via UDP.
    uint8_t* processFrame(camera_fb_t* fb, int matrixW, int matrixH, VideoEffect effect);

    // Get current effect name as string (for Web UI)
    const char* getEffectName(VideoEffect effect);

private:
    uint8_t* outputBuffer = nullptr;
    int currentBufferW = 0;
    int currentBufferH = 0;
    
    // Internal helper to convert RGB565 to RGB888
    void rgb565ToRgb888(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b);
    
    // Nearest neighbor downscaling + RGB conversion + Effects
    void processNearestNeighbor(camera_fb_t* fb, int targetW, int targetH, VideoEffect effect);
};

extern ImageProcessor imgProcessor;

#endif // IMAGE_PROCESSING_H

