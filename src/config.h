#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// CAMERA PIN CONFIGURATION (ESP32-S3-CAM)
// ==========================================
// Freenove / Generic ESP32-S3-WROOM-CAM pinout
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

// ==========================================
// HARDWARE PINS
// ==========================================
#define EFFECT_BUTTON_PIN 47 
#define STREAM_ENABLE_PIN 48 
#define LDR_ADC_PIN       3

// ==========================================
// WLED & MATRIX DEFAULTS
// ==========================================
#define DEFAULT_WLED_IP   "192.168.1.100"
#define DEFAULT_MATRIX_W  72
#define DEFAULT_MATRIX_H  40
#define DDP_PORT          4048

#endif // CONFIG_H
