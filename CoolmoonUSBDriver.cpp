#include "CoolmoonUSBDriver.h"
#include <algorithm>
#include <cstring>

CoolmoonUSBDriver::CoolmoonUSBDriver(hid_device* handle, std::string path)
    : dev(handle), device_path(std::move(path)) {}

CoolmoonUSBDriver::~CoolmoonUSBDriver() {
    if (dev) {
        hid_close(dev);
        dev = nullptr;
    }
}

void CoolmoonUSBDriver::WritePacket64(const uint8_t payload[64]) {
    if (!dev) return;

    // Windows HID membutuhkan 1 byte Report ID (0x00) di depan paket 64 byte
    uint8_t report[65];
    report[0] = 0x00;
    std::memcpy(&report[1], payload, 64);
    hid_write(dev, report, sizeof(report));
}

std::vector<CoolmoonPort> CoolmoonUSBDriver::ReadFanConfig() {
    std::vector<CoolmoonPort> detected_ports;
    if (!dev) return detected_ports;

    uint8_t req[64] = {0};
    req[0] = 0x52; // 'R'
    req[1] = 0x42; // 'B'
    req[2] = 0x07;
    req[3] = 0x00;
    req[4] = 0x01;
    req[5] = 0xFF;
    uint32_t sum = 0;
    for (int i = 0; i < 6; ++i) {
        sum += req[i];
    }
    req[6] = static_cast<uint8_t>(sum & 0xFF);

    WritePacket64(req);

    uint8_t resp[64] = {0};
    int res = hid_read_timeout(dev, resp, sizeof(resp), 1500);
    if (res >= 56 && resp[0] == 0x52 && resp[1] == 0x42 && resp[4] == 0x01) {
        for (uint8_t p = 0; p < 10; ++p) {
            size_t offset = 6 + p * 5;
            uint16_t count = resp[offset] | (resp[offset + 1] << 8);
            uint8_t port_id = resp[offset + 3];

            if (count > 0) {
                CoolmoonPort port;
                port.channel = port_id;
                port.name = "Port " + std::to_string(port_id + 1) + " (" + std::to_string(count) + " LED)";
                port.leds_max = 200;
                port.leds_default = count;
                detected_ports.push_back(port);
            }
        }
    }

    return detected_ports;
}

void CoolmoonUSBDriver::SendChannelLEDs(uint8_t channel, const std::vector<RGBColorData>& leds) {
    // Protokol USBFAN (1A86:FE05):
    // Byte 0..2   : 52 42 3C ("RB" + 60 byte panjang frame aktif)
    // Byte 3..10  : Header kontrol Direct Mode (8 byte)
    // Byte 11..58 : Payload warna RGB untuk maksimal 16 LED (48 byte)
    // Byte 59     : Checksum SUM(Byte 0..58) & 0xFF
    // Byte 60..63 : Padding 0x00 (4 byte)
    const size_t max_leds_per_pkt = 16;
    const size_t total_leds = leds.size();
    uint8_t packet_idx = 0;

    for (size_t offset = 0; offset < total_leds; offset += max_leds_per_pkt) {
        uint8_t pkt[64] = {0};
        size_t chunk = std::min(max_leds_per_pkt, total_leds - offset);

        pkt[0]  = 0x52; // 'R'
        pkt[1]  = 0x42; // 'B'
        pkt[2]  = 0x3C; // Panjang frame aktif = 60 byte (indeks 0..59)
        pkt[3]  = packet_idx; // 0 untuk LED 0..15, 1 untuk LED 16..23
        pkt[4]  = 0x03; // Mode Direct Per-LED
        pkt[5]  = (channel == 0xFF) ? 0xFF : 0xFD; // 0xFD = Single Port, 0xFF = Broadcast
        pkt[6]  = channel; // Port Index (0 = Fan 1, 1 = Fan 2, ... 9 = Fan 10, 0xFF = Broadcast)
        pkt[7]  = 0x00;
        pkt[8]  = 0x03;
        pkt[9]  = 0x01;
        pkt[10] = 0x99;

        // Payload warna urutan R-G-B (indeks 11..58)
        for (size_t i = 0; i < chunk; ++i) {
            const auto& c = leds[offset + i];
            pkt[11 + (i * 3) + 0] = c.r;
            pkt[11 + (i * 3) + 1] = c.g;
            pkt[11 + (i * 3) + 2] = c.b;
        }

        // Checksum terverifikasi di Byte 59: SUM(pkt[0..58]) & 0xFF
        uint8_t checksum = 0;
        for (int b = 0; b < 59; ++b) {
            checksum = static_cast<uint8_t>(checksum + pkt[b]);
        }
        pkt[59] = checksum;

        WritePacket64(pkt);
        packet_idx++;
    }
}

void CoolmoonUSBDriver::SendBroadcastLEDs(const std::vector<RGBColorData>& all_leds) {
    if (!dev || all_leds.empty()) return;

    std::vector<uint8_t> flat;
    flat.reserve(all_leds.size() * 3);
    for (const auto& c : all_leds) {
        flat.push_back(c.r);
        flat.push_back(c.g);
        flat.push_back(c.b);
    }

    const size_t total_bytes = flat.size();
    const uint8_t z = static_cast<uint8_t>((total_bytes + 59) / 60);

    for (uint8_t i = 0; i < z; ++i) {
        uint8_t pkt[64] = {0};
        pkt[0] = 0x88;
        pkt[1] = z;
        pkt[2] = i + 1; // 1-based chunk index

        size_t start_idx = i * 60;
        size_t chunk_len = std::min(static_cast<size_t>(60), total_bytes - start_idx);
        for (size_t j = 0; j < chunk_len; ++j) {
            pkt[3 + j] = flat[start_idx + j];
        }

        uint32_t sum = 0;
        for (int b = 0; b < 63; ++b) {
            sum += pkt[b];
        }
        pkt[63] = static_cast<uint8_t>(sum & 0xFF);

        WritePacket64(pkt);
    }
}

void CoolmoonUSBDriver::CommitFrame() {
    // USBFAN menerapkan perubahan warna langsung saat paket diterima
}

void CoolmoonUSBDriver::SetFanSpeed(uint8_t speed_pct) {
    if (!dev) return;

    speed_pct = std::min<uint8_t>(speed_pct, 100);

    uint8_t pkt[64] = {0};
    pkt[0] = 0x52; // 'R'
    pkt[1] = 0x42; // 'B'
    pkt[2] = 0x07; // Frame length
    pkt[3] = 0x00;
    pkt[4] = 0xFB; // Opcode Fan Speed PWM
    pkt[5] = speed_pct;

    uint32_t sum = 0;
    for (int i = 0; i < 6; ++i) {
        sum += pkt[i];
    }
    pkt[6] = static_cast<uint8_t>(sum & 0xFF);

    WritePacket64(pkt);
}

void CoolmoonUSBDriver::SetFanControlMode(uint8_t mode) {
    if (!dev) return;

    uint8_t pkt[64] = {0};
    pkt[0] = 0x52; // 'R'
    pkt[1] = 0x42; // 'B'
    pkt[2] = 0x07; // Frame length
    pkt[3] = 0x00;
    pkt[4] = 0x08; // Opcode Fan Speed Control Authority
    pkt[5] = mode; // 0 = Motherboard PWM Sync, 1 = Manual/Software

    uint32_t sum = 0;
    for (int i = 0; i < 6; ++i) {
        sum += pkt[i];
    }
    pkt[6] = static_cast<uint8_t>(sum & 0xFF);

    WritePacket64(pkt);
}
