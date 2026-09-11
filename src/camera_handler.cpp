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

int CameraHandler::getBrightnessFromADC() {
    int adcValue = analogRead(LDR_ADC_PIN); // 0 to 4095
    // Map ADC value to camera brightness (-2 to 2)
    return map(adcValue, 0, 4095, -2, 2);
}

void CameraHandler::setBrightness(int brightness) {
    sensor_t * s = esp_camera_sensor_get();
    if (s != nullptr) {
        // clamp between -2 and 2
        if (brightness < -2) brightness = -2;
        if (brightness > 2) brightness = 2;
        
        // Instead of setting "brightness" (which applies a flat digital offset and washes out the image),
        // we set the Auto Exposure Target Level (ae_level). This tells the hardware shutter to let in
        // more or less light, preserving contrast and color depth!
        s->set_ae_level(s, brightness); 
    }
}

