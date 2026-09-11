#ifndef CAMERA_HANDLER_H
#define CAMERA_HANDLER_H

#include <Arduino.h>
#include "esp_camera.h"

class CameraHandler {
public:
    bool begin();
    camera_fb_t* captureFrame();
    void returnFrame(camera_fb_t* fb);
    void setBrightness(int brightness);
    int getBrightnessFromADC();
    unsigned long lastAdcRead = 0;
};

extern CameraHandler camHandler;

#endif // CAMERA_HANDLER_H

