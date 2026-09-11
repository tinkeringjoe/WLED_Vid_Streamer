#ifndef WLED_STREAMER_H
#define WLED_STREAMER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include "config.h"

class WLEDStreamer {
public:
    WLEDStreamer();
    
    // Call once to setup UDP
    void begin(const char* targetIP, uint16_t port = DDP_PORT);
    
    // Update target IP dynamically
    void setTargetIP(const char* targetIP);

    // Send a full RGB frame to WLED via DDP protocol
    // Handles packet chunking automatically
    void sendFrame(uint8_t* frameBuffer, int width, int height);

private:
    WiFiUDP udp;
    char wledIP[16];
    uint16_t wledPort;
    uint8_t sequenceNumber = 1;
    
    // DDP Header definition
    struct DDPHeader {
        uint8_t flags1;
        uint8_t flags2;
        uint8_t type;
        uint8_t id;
        uint32_t offset;
        uint16_t length;
    } __attribute__((packed));
};

extern WLEDStreamer wledStreamer;

#endif // WLED_STREAMER_H

