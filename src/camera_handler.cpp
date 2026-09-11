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
    
    // Initial adjustments for better image quality (fixing washed-out look)
    sensor_t * s = esp_camera_sensor_get();
    if(s != nullptr){
        s->set_vflip(s, 1);   // Might need toggling depending on physical orientation
        s->set_hmirror(s, 1); 
        
        // --- Image Quality Tuning ---
        s->set_contrast(s, 1);     // Bump contrast to remove flat/washed-out look
        s->set_saturation(s, 1);   // Slight color boost
        s->set_aec2(s, 1);         // Enable advanced auto exposure
        s->set_awb_gain(s, 1);     // Auto White Balance gain
        s->set_wb_mode(s, 0);      // 0 = Auto White Balance
    }

    // Configure ADC pin
    pinMode(LDR_ADC_PIN, INPUT);

    return true;
}

camera_fb_t* CameraHandler::captureFrame() {
    return esp_camera_fb_get();
}

void CameraHandler::returnFrame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

int CameraHandler::getManualExposureFromADC() {
    int adcValue = analogRead(LDR_ADC_PIN); // 0 to 4095
    // Map ADC value to manual exposure time (0 to 1200)
    return map(adcValue, 0, 4095, 0, 1200);
}

void CameraHandler::applyCameraSettings(int contrast, int saturation, bool autoExp, int manualExp) {
    sensor_t * s = esp_camera_sensor_get();
    if (s != nullptr) {
        // Clamp contrast and saturation (-2 to 2)
        if (contrast < -2) contrast = -2;
        if (contrast > 2) contrast = 2;
        if (saturation < -2) saturation = -2;
        if (saturation > 2) saturation = 2;

        s->set_contrast(s, contrast);
        s->set_saturation(s, saturation);

        if (autoExp) {
            s->set_exposure_ctrl(s, 1); // Auto Exposure ON
        } else {
            s->set_exposure_ctrl(s, 0); // Auto Exposure OFF
            // Clamp manual exposure (0 to 1200)
            if (manualExp < 0) manualExp = 0;
            if (manualExp > 1200) manualExp = 1200;
            s->set_aec_value(s, manualExp); 
        }
    }
}

