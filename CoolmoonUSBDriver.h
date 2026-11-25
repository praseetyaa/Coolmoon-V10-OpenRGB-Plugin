#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "hidapi.h"

// Identitas hardware USBFAN (Coolmoon USB ARGB Controller)
#define COOLMOON_VID 0x1A86
#define COOLMOON_PID 0xFE05

struct RGBColorData {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct CoolmoonPort {
    uint8_t     channel;      // Port index (0..9)
    std::string name;         // Zone name
    unsigned    leds_max;     // Max LEDs for Edit Zone
    unsigned    leds_default; // Active LED count
};

class CoolmoonUSBDriver {
public:
    CoolmoonUSBDriver(hid_device* handle, std::string path);
    ~CoolmoonUSBDriver();

    std::string GetLocation() const { return device_path; }

    // Auto-read active ports and LED count from MCU
    std::vector<CoolmoonPort> ReadFanConfig();

    // Per-port, max 16 LED: protocol 0x52 0x42
    void SendChannelLEDs(uint8_t channel, const std::vector<RGBColorData>& leds);
    // Broadcast all ports, unlimited LED: protocol 0x88 (mubu_chai)
    void SendBroadcastLEDs(const std::vector<RGBColorData>& all_leds);
    void CommitFrame();

private:
    hid_device* dev;
    std::string device_path;
    void WritePacket64(const uint8_t payload[64]);
};
