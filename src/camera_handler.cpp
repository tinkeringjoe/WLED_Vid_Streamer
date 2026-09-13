#include "camera_handler.h"
#include "config.h"

CameraHandler camHandler;

bool CameraHandler::begin() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QQVGA; // 160x120 is plenty for a 72x40 target
    config.pixel_format = PIXFORMAT_RGB565;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12; // Not used for RGB565
    config.fb_count = 2; // Double buffering for smoother streaming

    // Initialize the camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }
    
    // Initial adjustments
    sensor_t * s = esp_camera_sensor_get();
    if(s != nullptr){
        // Defaults, will be overridden by Web UI load
        s->set_vflip(s, 0);
        s->set_hmirror(s, 0); 
        
        // The OV3660's "Advanced Auto Exposure" (AEC2) is notoriously buggy and 
        // often washes out the picture or turns it completely gray. Disable it to use standard AEC.
        s->set_aec2(s, 0); 
        
        s->set_contrast(s, 2);
        s->set_saturation(s, 2);
        s->set_brightness(s, -1);
    }
    analogReadResolution(12); // 0-4095
    return true;
}

void CameraHandler::updateSettings(bool vflip, bool hmirror, int contrast, int saturation) {
    sensor_t * s = esp_camera_sensor_get();
    if (s != nullptr) {
        s->set_vflip(s, vflip ? 1 : 0);
        s->set_hmirror(s, hmirror ? 1 : 0);
        s->set_contrast(s, contrast);
        s->set_saturation(s, saturation);
    }
}

camera_fb_t* CameraHandler::captureFrame() {
    return esp_camera_fb_get();
}

void CameraHandler::returnFrame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}


