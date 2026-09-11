#include "wled_streamer.h"

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
    }
}

void WLEDStreamer::sendFrame(uint8_t* frameBuffer, int width, int height) {
    if (!frameBuffer || strlen(wledIP) == 0) return;

    int totalPixels = width * height;
    int totalBytes = totalPixels * 3; // RGB
    
    // Standard MTU is 1500, UDP payload ~1472. 
    // DDP Header is 10 bytes. Let's use 1440 data bytes per packet (480 pixels).
    const int MAX_DATA_PER_PACKET = 1440; 
    
    int bytesSent = 0;
    bool isFinalPacket = false;

    while (bytesSent < totalBytes) {
        int bytesToSend = totalBytes - bytesSent;
        if (bytesToSend > MAX_DATA_PER_PACKET) {
            bytesToSend = MAX_DATA_PER_PACKET;
        } else {
            isFinalPacket = true;
        }

        // Construct DDP Header
        DDPHeader header;
        // 0x40 = Ver 1, Push. 0x01 = Sync (only if final packet)
        header.flags1 = 0x40 | (isFinalPacket ? 0x01 : 0x00);
        header.flags2 = sequenceNumber;
        header.type = 1; // 1 = RGB byte order
        header.id = 1;   // Destination display ID (default 1)
        
        // Offset is in bytes
        uint32_t offset = bytesSent;
        // Needs network byte order (Big Endian) for offset and length
        header.offset = htonl(offset);
        header.length = htons(bytesToSend);

        udp.beginPacket(wledIP, wledPort);
        // Write header
        udp.write((uint8_t*)&header, sizeof(header));
        // Write pixel chunk
        udp.write(frameBuffer + bytesSent, bytesToSend);
        udp.endPacket();

        bytesSent += bytesToSend;
        
        // Brief yield to allow the Wi-Fi task to dispatch the UDP packet
        // This prevents ENOMEM (Error 12) from filling up the TX buffers
        delay(1); 
    }
    
    // Increment sequence (1-15 per DDP spec)
    sequenceNumber++;
    if (sequenceNumber > 15) sequenceNumber = 1;
}

