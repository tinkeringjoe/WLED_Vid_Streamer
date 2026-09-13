#include "wled_streamer.h"
#include "network_web.h"

WLEDStreamer wledStreamer;

WLEDStreamer::WLEDStreamer() {
    strncpy(wledIP, DEFAULT_WLED_IP, sizeof(wledIP) - 1);
    wledPort = DDP_PORT;
}

void WLEDStreamer::begin(const char* targetIP, uint16_t port) {
    setTargetIP(targetIP);
    wledPort = port;
    // udp.begin() not strictly needed for sending, but good practice
}

void WLEDStreamer::setTargetIP(const char* targetIP) {
    if (targetIP && strlen(targetIP) > 0) {
        strncpy(wledIP, targetIP, sizeof(wledIP) - 1);
        wledIP[sizeof(wledIP) - 1] = '\0';
        parsedIP.fromString(wledIP); // Parse once so we don't block on DNS lookups!
    }
}

// Array removed

void WLEDStreamer::sendFrame(uint8_t* frameBuffer, int width, int height) {
    if (!frameBuffer || strlen(wledIP) == 0) return;
    
    int totalPixels = width * height;
    int pixelsLeft = totalPixels;
    int offset = 0;
    
    // DDP headers support max 480 pixels (1440 bytes) per packet to stay under typical MTU (1500)
    int maxPixelsPerPacket = 480;

    while (pixelsLeft > 0) {
        int pixelsToSend = (pixelsLeft > maxPixelsPerPacket) ? maxPixelsPerPacket : pixelsLeft;
        int bytesToSend = pixelsToSend * 3;
        
        uint8_t packet[10 + 1440]; 
        
        packet[0] = 0x41; // Flags: V1
        packet[1] = sequenceNumber & 0x0F;
        packet[2] = 1; // Type: 1 = RGB
        packet[3] = 1; // ID: 1
        
        // Offset in bytes (32-bit big endian)
        uint32_t byteOffset = offset * 3;
        packet[4] = (byteOffset >> 24) & 0xFF;
        packet[5] = (byteOffset >> 16) & 0xFF;
        packet[6] = (byteOffset >> 8) & 0xFF;
        packet[7] = byteOffset & 0xFF;
        
        // Length in bytes (16-bit big endian)
        packet[8] = (bytesToSend >> 8) & 0xFF;
        packet[9] = bytesToSend & 0xFF;
        
        // Copy pixel data directly. We removed Gamma correction here because WLED 
        // applies its own color/gamma mapping internally. Double-gamma severely corrupts colors!
        for (int i = 0; i < pixelsToSend; i++) {
            int srcIdx = (offset + i) * 3;
            int dstIdx = 10 + (i * 3);
            
            uint8_t r = frameBuffer[srcIdx];
            uint8_t g = frameBuffer[srcIdx + 1];
            uint8_t b = frameBuffer[srcIdx + 2];

            if (netWeb.ddpColorOrder == 1) {
                // GRB
                packet[dstIdx] = g; packet[dstIdx + 1] = r; packet[dstIdx + 2] = b;
            } else if (netWeb.ddpColorOrder == 2) {
                // BGR
                packet[dstIdx] = b; packet[dstIdx + 1] = g; packet[dstIdx + 2] = r;
            } else if (netWeb.ddpColorOrder == 3) {
                // RBG
                packet[dstIdx] = r; packet[dstIdx + 1] = b; packet[dstIdx + 2] = g;
            } else if (netWeb.ddpColorOrder == 4) {
                // GBR
                packet[dstIdx] = g; packet[dstIdx + 1] = b; packet[dstIdx + 2] = r;
            } else if (netWeb.ddpColorOrder == 5) {
                // BRG
                packet[dstIdx] = b; packet[dstIdx + 1] = r; packet[dstIdx + 2] = g;
            } else {
                // Default: RGB
                packet[dstIdx] = r; packet[dstIdx + 1] = g; packet[dstIdx + 2] = b;
            }
        }

        udp.beginPacket(parsedIP, wledPort);
        udp.write(packet, 10 + bytesToSend);
        udp.endPacket();

        offset += pixelsToSend;
        pixelsLeft -= pixelsToSend;
    }
    
    // Increment sequence (1-15 per DDP spec)
    sequenceNumber++;
    if (sequenceNumber > 15) sequenceNumber = 1;
}
