#pragma GCC optimize ("O2")

#include "image_processing.h"
#include "network_web.h"
#include "esp_heap_caps.h"

ImageProcessor imgProcessor;

ImageProcessor::ImageProcessor() {}

ImageProcessor::~ImageProcessor() {
    if (outputBuffer) free(outputBuffer);
    if (bgBuffer) free(bgBuffer);
    if (trailBuffer) free(trailBuffer);
    if (xMap) free(xMap);
    if (yMap) free(yMap);
}

void ImageProcessor::rgb565ToRgb888(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
    color = (color >> 8) | (color << 8); // Swap bytes for Big-Endian OV3660 DMA
    uint8_t r5 = (color >> 11) & 0x1F;
    uint8_t g6 = (color >> 5) & 0x3F;
    uint8_t b5 = color & 0x1F;
    
    // Replicate bits to perfectly scale to 8-bit
    r = (r5 << 3) | (r5 >> 2);
    g = (g6 << 2) | (g6 >> 4);
    b = (b5 << 3) | (b5 >> 2);
    
    // Note: We DO NOT apply Gamma Correction here because WLED applies its own 
    // gamma curve to incoming DDP streams by default! Applying it twice turns the video to trash.
}

struct PaletteColor { uint8_t r, g, b; };

const PaletteColor retroPalette[] = {
    {0,0,0}, {255,0,0}, {0,255,0}, {0,0,255},
    {255,255,0}, {255,0,255}, {0,255,255}, {255,255,255},
    {128,128,128}, {128,0,0}, {0,128,0}, {0,0,128},
    {128,128,0}, {128,0,128}, {0,128,128}, {192,192,192}
};

const PaletteColor cyberpunkPalette[] = {
    {0,0,0}, {18,6,32}, {45,15,65}, {104,18,111},
    {224,27,121}, {254,74,144}, {255,142,180},
    {7,21,38}, {16,52,89}, {20,105,142},
    {36,220,212}, {158,255,238},
    {255,221,51}, {255,140,0}
};

void snapToPalette(uint8_t& r, uint8_t& g, uint8_t& b, const PaletteColor* pal, int palSize) {
    int bestDist = 200000;
    uint8_t bestR=0, bestG=0, bestB=0;
    for(int k=0; k<palSize; k++) {
        int dr = r - pal[k].r;
        int dg = g - pal[k].g;
        int db = b - pal[k].b;
        int dist = dr*dr + dg*dg + db*db;
        if(dist < bestDist) {
            bestDist = dist;
            bestR = pal[k].r; bestG = pal[k].g; bestB = pal[k].b;
        }
    }
    r = bestR; g = bestG; b = bestB;
}

uint8_t* ImageProcessor::processFrame(camera_fb_t* fb, int targetW, int targetH, VideoEffect effect, int softwareBrightness) {
    if (!fb || fb->format != PIXFORMAT_RGB565) return nullptr;

    int requiredSize = targetW * targetH * 3;
    
    if (!outputBuffer || currentBufferW != targetW || currentBufferH != targetH) {
        if (outputBuffer) free(outputBuffer);
        if (bgBuffer) free(bgBuffer);
        if (trailBuffer) free(trailBuffer);
        
        outputBuffer = (uint8_t*)malloc(requiredSize);
        bgBuffer = (uint8_t*)calloc(requiredSize, 1);
        trailBuffer = (uint8_t*)calloc(requiredSize, 1);
        if (!outputBuffer) return nullptr; 
        
        if (xMap) free(xMap);
        if (yMap) free(yMap);
        xMap = (int*)malloc(targetW * sizeof(int));
        yMap = (int*)malloc(targetH * sizeof(int));
        
        if (xMap && yMap) {
            for (int i = 0; i < targetH; i++) yMap[i] = (i * fb->height) / targetH;
            for (int j = 0; j < targetW; j++) xMap[j] = (j * fb->width) / targetW;
        }
        
        currentBufferW = targetW;
        currentBufferH = targetH;
    }

    int srcW = fb->width;
    uint16_t* pixels = (uint16_t*)fb->buf;
    
    // --- PSRAM to SRAM Copy for Massive Speedup ---
    // The camera DMA dumps to PSRAM (slow external memory). Our convolution loops
    // read pixels 5 times per target pixel. Copying to internal SRAM first eliminates PSRAM cache misses.
    static uint16_t* sramFrame = nullptr;
    static int sramFrameSize = 0;
    int requiredSram = fb->width * fb->height * 2;
    if (sramFrameSize != requiredSram) {
        if (sramFrame) heap_caps_free(sramFrame);
        sramFrame = (uint16_t*)heap_caps_malloc(requiredSram, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        sramFrameSize = requiredSram;
    }
    
    if (sramFrame) {
        memcpy(sramFrame, fb->buf, requiredSram);
        pixels = sramFrame; // Point processing loop to the ultra-fast internal memory!
    }

    static unsigned long frameCount = 0;
    frameCount++;

    for (int i = 0; i < targetH; i++) {
        int rowOffset = yMap[i] * srcW;
        
        for (int j = 0; j < targetW; j++) {
            int srcIndex = rowOffset + xMap[j];
            int cx = xMap[j];
            int cy = yMap[i];
            uint8_t r, g, b;
            
            // 1. Edge Detection & Unsharp Masking on the High-Res Frame
            if (cx > 0 && cx < srcW - 1 && cy > 0 && cy < fb->height - 1) {
                uint8_t cr, cg, cb, u_r, u_g, u_b, d_r, d_g, d_b, l_r, l_g, l_b, r_r, r_g, r_b;
                rgb565ToRgb888(pixels[srcIndex], cr, cg, cb);
                rgb565ToRgb888(pixels[srcIndex - srcW], u_r, u_g, u_b);
                rgb565ToRgb888(pixels[srcIndex + srcW], d_r, d_g, d_b);
                rgb565ToRgb888(pixels[srcIndex - 1], l_r, l_g, l_b);
                rgb565ToRgb888(pixels[srcIndex + 1], r_r, r_g, r_b);
                
                if (effect == EFFECT_EDGE_GLOW) {
                    // Laplacian Edge Detection for Shadows/Silhouettes
                    int er = abs((cr * 4) - u_r - d_r - l_r - r_r);
                    int eg = abs((cg * 4) - u_g - d_g - l_g - r_g);
                    int eb = abs((cb * 4) - u_b - d_b - l_b - r_b);
                    int edge = (er + eg + eb) / 3;
                    
                    if (edge > 25) { // Sensitivity Threshold
                        r = min(255, edge * 2); // Neon pink/purple/cyan vibe
                        g = min(255, edge + 50);
                        b = 255;
                    } else {
                        r = 0; g = 0; b = 0; // Black background
                    }
                } else {
                    // Standard Unsharp Mask
                    int sr = ((cr * 6) - u_r - d_r - l_r - r_r) / 2;
                    int sg = ((cg * 6) - u_g - d_g - l_g - r_g) / 2;
                    int sb = ((cb * 6) - u_b - d_b - l_b - r_b) / 2;
                    
                    r = max(0, min(255, sr));
                    g = max(0, min(255, sg));
                    b = max(0, min(255, sb));
                }
            } else {
                rgb565ToRgb888(pixels[srcIndex], r, g, b);
                if (effect == EFFECT_EDGE_GLOW) { r = 0; g = 0; b = 0; }
            }
            
            // Apply color grading only to non-edge effects
            if (effect != EFFECT_EDGE_GLOW) {
                // 2. Software Contrast Boost
                int c_r = 128 + ((r - 128) * 3) / 2; // +150%
                int c_g = 128 + ((g - 128) * 3) / 2;
                int c_b = 128 + ((b - 128) * 3) / 2;
                c_r = max(0, min(255, c_r));
                c_g = max(0, min(255, c_g));
                c_b = max(0, min(255, c_b));
                
                // 3. Software Saturation Boost
                int luma = (c_r * 77 + c_g * 150 + c_b * 29) >> 8;
                int s_r = c_r + ((c_r - luma) * 3) / 2; // +150%
                int s_g = c_g + ((c_g - luma) * 3) / 2;
                int s_b = c_b + ((c_b - luma) * 3) / 2;
                
                r = max(0, min(255, s_r));
                g = max(0, min(255, s_g));
                b = max(0, min(255, s_b));
            }

            int destIndex = (i * targetW + j) * 3;

            // --- Background Subtraction ---
            bool isBackground = false;
            
            if (triggerBgCapture && bgBuffer) {
                bgBuffer[destIndex] = r;
                bgBuffer[destIndex + 1] = g;
                bgBuffer[destIndex + 2] = b;
            }
            
            if (netWeb.enableBgSub && bgBuffer) {
                int bgR = bgBuffer[destIndex];
                int bgG = bgBuffer[destIndex + 1];
                int bgB = bgBuffer[destIndex + 2];
                int diff = abs(r - bgR) + abs(g - bgG) + abs(b - bgB);
                if (diff < netWeb.bgThreshold) {
                    isBackground = true;
                }
            }

            if (isBackground) {
                r = 0; g = 0; b = 0;
            } else {
                // Apply Infinite Software Brightness Dimmer (0 to 255, where 255 = 100%)
                if (softwareBrightness != 255) {
                    int newR = (r * softwareBrightness) >> 8;
                    int newG = (g * softwareBrightness) >> 8;
                    int newB = (b * softwareBrightness) >> 8;
                    r = (newR > 255) ? 255 : newR;
                    g = (newG > 255) ? 255 : newG;
                    b = (newB > 255) ? 255 : newB;
                }

                switch(effect) {
                    case EFFECT_NORMAL:
                        break;
                    case EFFECT_PSYCHEDELIC: {
                        uint8_t cycle = (frameCount / 2) % 3;
                        uint8_t tempR = r, tempG = g, tempB = b;
                        if (cycle == 0) { r = tempR; g = tempG; b = tempB; }
                        else if (cycle == 1) { r = tempG; g = tempB; b = tempR; }
                        else { r = tempB; g = tempR; b = tempG; }
                        break;
                    }
                    case EFFECT_RETRO_8BIT: {
                        snapToPalette(r, g, b, retroPalette, 16);
                        break;
                    }
                    case EFFECT_CYBERPUNK: {
                        snapToPalette(r, g, b, cyberpunkPalette, 14);
                        break;
                    }
                    case EFFECT_THERMAL: {
                        uint8_t luma = (r * 77 + g * 150 + b * 29) >> 8;
                        if (luma < 85) {
                            r = luma * 3; g = 0; b = 0;
                        } else if (luma < 170) {
                            r = 255; g = (luma - 85) * 3; b = 0;
                        } else {
                            r = 255; g = 255; b = (luma - 170) * 3;
                        }
                        break;
                    }
                    case EFFECT_MATRIX: {
                        uint8_t luma = (r * 77 + g * 150 + b * 29) >> 8;
                        r = 0;
                        g = luma;
                        b = luma / 6; // Digital green with tiny cyan hint
                        break;
                    }
                    case EFFECT_EDGE_GLOW:
                        // Handled during edge detection
                        break;
                    default:
                        break;
                }
            }

            // --- Motion Trails ---
            if (netWeb.trailAmount > 0 && trailBuffer) {
                int tr = trailBuffer[destIndex];
                int tg = trailBuffer[destIndex + 1];
                int tb = trailBuffer[destIndex + 2];
                
                // Blend current pixel with history
                r = (r * (255 - netWeb.trailAmount) + tr * netWeb.trailAmount) / 255;
                g = (g * (255 - netWeb.trailAmount) + tg * netWeb.trailAmount) / 255;
                b = (b * (255 - netWeb.trailAmount) + tb * netWeb.trailAmount) / 255;
                
                trailBuffer[destIndex] = r;
                trailBuffer[destIndex + 1] = g;
                trailBuffer[destIndex + 2] = b;
            } else if (trailBuffer) {
                trailBuffer[destIndex] = r;
                trailBuffer[destIndex + 1] = g;
                trailBuffer[destIndex + 2] = b;
            }

            outputBuffer[destIndex] = r;
            outputBuffer[destIndex + 1] = g;
            outputBuffer[destIndex + 2] = b;
        }
    }
    
    if (triggerBgCapture) triggerBgCapture = false;
    
    return outputBuffer;
}

const char* ImageProcessor::getEffectName(VideoEffect effect) {
    switch(effect) {
        case EFFECT_NORMAL: return "Normal";
        case EFFECT_PSYCHEDELIC: return "Psychedelic";
        case EFFECT_RETRO_8BIT: return "Retro 8-bit";
        case EFFECT_CYBERPUNK: return "Cyberpunk";
        case EFFECT_THERMAL: return "Thermal";
        case EFFECT_MATRIX: return "Digital Matrix";
        case EFFECT_EDGE_GLOW: return "Neon Edge Glow";
        default: return "Unknown";
    }
}

