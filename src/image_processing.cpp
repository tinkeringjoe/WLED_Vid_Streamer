#include "image_processing.h"

ImageProcessor imgProcessor;

ImageProcessor::ImageProcessor() {}

ImageProcessor::~ImageProcessor() {
    if (outputBuffer) {
        free(outputBuffer);
    }
}

void ImageProcessor::rgb565ToRgb888(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
    // RGB565 format: RRRRRGGG GGGBBBBB
    // It's often little-endian in memory, so we swap bytes when reading from uint16_t array cast
    color = (color >> 8) | (color << 8); 
    
    r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
    g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
    b = (((color & 0x1F) * 527) + 23) >> 6;
}

uint8_t* ImageProcessor::processFrame(camera_fb_t* fb, int matrixW, int matrixH, VideoEffect effect) {
    if (!fb || fb->format != PIXFORMAT_RGB565) return nullptr;

    int requiredSize = matrixW * matrixH * 3;
    
    // Allocate or re-allocate buffer if matrix size changed
    if (!outputBuffer || currentBufferW != matrixW || currentBufferH != matrixH) {
        if (outputBuffer) free(outputBuffer);
        outputBuffer = (uint8_t*)malloc(requiredSize);
        if (!outputBuffer) return nullptr; // Out of memory
        currentBufferW = matrixW;
        currentBufferH = matrixH;
    }

    processNearestNeighbor(fb, matrixW, matrixH, effect);

    return outputBuffer;
}

void ImageProcessor::processNearestNeighbor(camera_fb_t* fb, int targetW, int targetH, VideoEffect effect) {
    uint16_t* pixels = (uint16_t*)fb->buf;
    int srcW = fb->width;
    int srcH = fb->height;

    // Time-based variables for dynamic effects
    static unsigned long frameCount = 0;
    frameCount++;

    float x_ratio = ((float)(srcW - 1)) / targetW;
    float y_ratio = ((float)(srcH - 1)) / targetH;

    for (int i = 0; i < targetH; i++) {
        for (int j = 0; j < targetW; j++) {
            int srcX = (int)(x_ratio * j);
            int srcY = (int)(y_ratio * i);
            int srcIndex = (srcY * srcW) + srcX;
            
            uint8_t r, g, b;
            rgb565ToRgb888(pixels[srcIndex], r, g, b);

            // Apply effect
            switch(effect) {
                case EFFECT_NORMAL:
                    break;
                case EFFECT_GRAYSCALE: {
                    uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
                    r = g = b = gray;
                    break;
                }
                case EFFECT_INVERT: {
                    r = 255 - r;
                    g = 255 - g;
                    b = 255 - b;
                    break;
                }
                case EFFECT_SPOOKY: {
                    // Boost reds, crush blues/greens for high contrast creepy look
                    uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
                    r = min(255, gray + 80); // Harsh red
                    g = gray / 2;            // Suppress green
                    b = gray / 4;            // Suppress blue
                    break;
                }
                case EFFECT_SEPIA: {
                    int tr = (r * 393 + g * 769 + b * 189) / 1000;
                    int tg = (r * 349 + g * 686 + b * 168) / 1000;
                    int tb = (r * 272 + g * 534 + b * 131) / 1000;
                    r = min(255, tr);
                    g = min(255, tg);
                    b = min(255, tb);
                    break;
                }
                case EFFECT_PSYCHEDELIC: {
                    // Rotate color channels based on frame count
                    uint8_t cycle = (frameCount / 2) % 3;
                    uint8_t tempR = r, tempG = g, tempB = b;
                    if (cycle == 0) { r = tempR; g = tempG; b = tempB; }
                    else if (cycle == 1) { r = tempG; g = tempB; b = tempR; }
                    else { r = tempB; g = tempR; b = tempG; }
                    break;
                }
                default:
                    break;
            }

            int destIndex = (i * targetW + j) * 3;
            outputBuffer[destIndex] = r;
            outputBuffer[destIndex + 1] = g;
            outputBuffer[destIndex + 2] = b;
        }
    }
}

const char* ImageProcessor::getEffectName(VideoEffect effect) {
    switch(effect) {
        case EFFECT_NORMAL: return "Normal";
        case EFFECT_SPOOKY: return "Spooky";
        case EFFECT_GRAYSCALE: return "Grayscale";
        case EFFECT_INVERT: return "Invert";
        case EFFECT_SEPIA: return "Sepia";
        case EFFECT_PSYCHEDELIC: return "Psychedelic";
        default: return "Unknown";
    }
}

