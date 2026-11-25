# Blueprint & Spesifikasi Teknis: Native OpenRGB C++ Plugin untuk Coolmoon / USBFAN (`1A86:FE05`)

Dokumen ini berfungsi sebagai **Single Source of Truth (SSOT)** dan dokumentasi teknis lengkap untuk *Native OpenRGB C++ Plugin* (`.dll`) pada Windows x64 (**OpenRGB 1.0+ / Plugin API v5**).

---

## 1. Ringkasan Proyek & Target Hardware

* **Target Aplikasi:** OpenRGB 1.0+ (Windows 64-bit, Plugin API v5).
* **Target Hardware Controller:** Coolmoon USB ARGB Controller (terdeteksi di Windows sebagai `USBFAN` / `ROBOBLOQ`).
* **Identitas USB HID:**
  * **Vendor ID (`VID`):** `0x1A86`
  * **Product ID (`PID`):** `0xFE05`
  * **Usage Page:** Vendor-Defined (`0xFF00`), Usage: `0x0001`
  * **Interface Number:** `0`
* **Topologi Port & LED Fisik (Hasil Deteksi MCU):**
  * Port 0 (Order 1): 8 LED
  * Port 1 (Order 2): 8 LED
  * Port 2 (Order 3): 10 LED
  * Port 3 (Order 4): 8 LED
  * Port 4 (Order 5): 10 LED
  * Port 5..7: 0 LED (tidak terhubung / kosong)
  * Port 8 (Order 9): 72 LED (daisy-chain fan/strip)
  * Port 9 (Order 10): 18 LED (strip)
  * **Total Rantai LED Aktif:** **134 LED**
* **Fitur Utama Plugin:**
  1. **Auto-Read Hardware Config:** Otomatis membaca jumlah LED & status port dari EEPROM/MCU controller saat startup atau rescan devices.
  2. **Full Per-LED Control:** Menggunakan protokol continuous stream `0x88` (`mubu_chai`), memungkinkan kontrol mandiri per-titik LED tanpa batasan 16 LED.
  3. **Manual Zone Resize:** Tombol **Edit Zone** aktif di UI OpenRGB (`ZONE_FLAG_MANUALLY_CONFIGURABLE_SIZE`), pengguna bebas mengatur jumlah LED (1–200 LED) tiap port secara manual.

---

## 2. Spesifikasi Protokol USB HID

Controller Coolmoon berkomunikasi lewat **USB HID Report** ukuran tetap **64 byte** (di Windows ditambahkan 1 byte `Report ID = 0x00` di depan, total **65 byte**).

### 2.1. Protokol Utama: Continuous Streaming (`0x88` / `mubu_chai`)
Digunakan untuk pengiriman warna LED ke seluruh port dalam satu frame berkelanjutan. Mengatasi limit 16-LED dan bug broadcast pada protokol lama.

* **Format Paket (64 byte):**

| Offset Byte | Panjang | Nilai Hex | Keterangan |
| :--- | :--- | :--- | :--- |
| `0` | 1 Byte | `0x88` | Opcode Continuous RGB Stream |
| `1` | 1 Byte | `z` | Total paket/chunk (`ceil(total_byte / 60)`) |
| `2` | 1 Byte | `1..z` | Nomor urut chunk (1-based index) |
| `3 – 62` | s.d. 60 Byte | `R0 G0 B0 ...` | Payload warna LED mentah (maksimal 20 LED per paket) urutan **`R-G-B`** |
| `63` | 1 Byte | `SUM & 0xFF` | Checksum: `SUM(Byte 0..62) & 0xFF` |

Untuk 134 LED (402 byte payload), driver membagi payload menjadi **7 paket chunk** yang dikirim berurutan.

### 2.2. Protokol Auto-Read Konfigurasi MCU (`0x52 0x42 0x07`)
Digunakan oleh plugin saat inisialisasi untuk membaca konfigurasi port dan jumlah LED dari MCU.

* **Request Paket:**
  `[0x52, 0x42, 0x07, 0x00, 0x01, 0xFF, Checksum]` (panjang 64 byte, sisa diisi `0x00`).
* **Respon dari MCU (64 byte):**
  * `Byte 0..5`: Header `52 42 39 00 01 FF`
  * `Byte 6..55`: 10 record port (@ 5 byte per port untuk Port 0..9):
    * `Byte +0..+1`: Jumlah LED (`uint16_t` Little-Endian)
    * `Byte +2`: Tipe / Status Flag
    * `Byte +3`: Port ID (`0..9`)
    * `Byte +4`: Reserved / Padding (`0x00`)

---

## 3. Struktur Direktori Proyek

```text
coolmoon-plugin-openrgb/
├── .gitignore                              <-- Abaikan build/, *.dll, dsb.
├── .gitmodules                             <-- Referensi submodule OpenRGB SDK
├── CMakeLists.txt                          <-- Script kompilasi CMake (Qt6 + hidapi + OpenRGB SDK)
├── OpenRGBCoolmoonPlugin.json              <-- Metadata plugin Qt (API Version 5)
├── OpenRGBCoolmoonPlugin.h                 <-- Entry-point OpenRGB Plugin API v5
├── OpenRGBCoolmoonPlugin.cpp               <-- Inisialisasi plugin, auto-read, & registrasi controller
├── RGBController_CoolmoonUSB.h             <-- Header controller virtual & deklarasi zone
├── RGBController_CoolmoonUSB.cpp           <-- Handler update LED, mode, & resize zone
├── CoolmoonUSBDriver.h                     <-- Driver HID USB, definisi struct port & RGB
├── CoolmoonUSBDriver.cpp                   <-- Implementasi pengiriman paket 0x88 & pembacaan config
├── openrgb_coolmoon_usb_plugin_blueprint.md<-- Dokumen spesifikasi teknis (SSOT)
└── third_party/
    └── OpenRGB/                            <-- Header OpenRGB SDK (CalcProgrammer1/OpenRGB)
```

---

## 4. Panduan Kompilasi & Deployment

### Persyaratan Lingkungan:
* **MSVC Hostx64 x64** (Visual Studio 2022 / Build Tools)
* **Windows SDK 10.0+**
* **Qt 6.8+ (msvc2022_64)**
* **CMake 3.16+**

### Perintah Build PowerShell:
```powershell
# 1. Konfigurasi Environment MSVC & Qt
$env:PATH = "C:\msvc\unpack\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64;C:\msvc\unpack\Windows Kits\10\bin\10.0.26100.0\x64;C:\Qt\6.8.3\msvc2022_64\bin;C:\Program Files\CMake\bin;" + $env:PATH
$env:INCLUDE = "C:\msvc\unpack\VC\Tools\MSVC\14.44.35207\include;C:\msvc\unpack\Windows Kits\10\Include\10.0.26100.0\ucrt;C:\msvc\unpack\Windows Kits\10\Include\10.0.26100.0\um;C:\msvc\unpack\Windows Kits\10\Include\10.0.26100.0\shared"
$env:LIB = "C:\msvc\unpack\VC\Tools\MSVC\14.44.35207\lib\x64;C:\msvc\unpack\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\msvc\unpack\Windows Kits\10\Lib\10.0.26100.0\um\x64;C:\Qt\6.8.3\msvc2022_64\lib"

# 2. Generate Build Files
cmake -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64" -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# 3. Kompilasi DLL
cmake --build build --config Release

# 4. Deploy ke OpenRGB
$dst = "$env:APPDATA\OpenRGB\plugins\OpenRGBCoolmoonPlugin.dll"
$src = "build\OpenRGBCoolmoonPlugin.dll"
if (Test-Path $dst) { Move-Item $dst "$dst.old" -Force }
Copy-Item $src $dst -Force
```

---

## 5. Implementasi Kode Lengkap

### 5.1. `OpenRGBCoolmoonPlugin.json`
```json
{
    "OpenRGBPluginAPIVersion": 5,
    "Id": "coolmoon_custom_fan_plugin"
}
```

### 5.2. `CoolmoonUSBDriver.h`
```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "hidapi.h"

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
```

### 5.3. `CoolmoonUSBDriver.cpp`
```cpp
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
    const size_t max_leds_per_pkt = 16;
    const size_t total_leds = leds.size();
    uint8_t packet_idx = 0;

    for (size_t offset = 0; offset < total_leds; offset += max_leds_per_pkt) {
        uint8_t pkt[64] = {0};
        size_t chunk = std::min(max_leds_per_pkt, total_leds - offset);

        pkt[0]  = 0x52;
        pkt[1]  = 0x42;
        pkt[2]  = 0x3C;
        pkt[3]  = packet_idx;
        pkt[4]  = 0x03;
        pkt[5]  = (channel == 0xFF) ? 0xFF : 0xFD;
        pkt[6]  = channel;
        pkt[7]  = 0x00;
        pkt[8]  = 0x03;
        pkt[9]  = 0x01;
        pkt[10] = 0x99;

        for (size_t i = 0; i < chunk; ++i) {
            const auto& c = leds[offset + i];
            pkt[11 + (i * 3) + 0] = c.r;
            pkt[11 + (i * 3) + 1] = c.g;
            pkt[11 + (i * 3) + 2] = c.b;
        }

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
        pkt[2] = i + 1;

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
}
```

### 5.4. `RGBController_CoolmoonUSB.h`
```cpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include "RGBControllerInterface.h"
#include "CoolmoonUSBDriver.h"

class RGBController_CoolmoonUSB {
public:
    RGBController_CoolmoonUSB(std::shared_ptr<CoolmoonUSBDriver> drv,
                               std::vector<CoolmoonPort> ports);
    ~RGBController_CoolmoonUSB() = default;

    RGBController_Setup CreateSetup();
    void SetControllerInterface(RGBControllerInterface* iface) { controller = iface; }
    RGBControllerInterface* GetControllerInterface() const { return controller; }

    void DeviceUpdateLEDs();
    void DeviceUpdateZoneLEDs(int zone);
    void DeviceUpdateSingleLED(int led);
    void DeviceUpdateMode();
    void DeviceConfigureZone(int zone);

private:
    std::shared_ptr<CoolmoonUSBDriver> driver;
    std::vector<CoolmoonPort> ports;
    RGBControllerInterface* controller = nullptr;
};
```

### 5.5. `RGBController_CoolmoonUSB.cpp`
```cpp
#include "RGBController_CoolmoonUSB.h"
#include <algorithm>

static void StaticUpdateLEDs(void* obj)              { if (obj) static_cast<RGBController_CoolmoonUSB*>(obj)->DeviceUpdateLEDs(); }
static void StaticUpdateZoneLEDs(void* obj, int z)   { if (obj) static_cast<RGBController_CoolmoonUSB*>(obj)->DeviceUpdateZoneLEDs(z); }
static void StaticUpdateSingleLED(void* obj, int l)  { if (obj) static_cast<RGBController_CoolmoonUSB*>(obj)->DeviceUpdateSingleLED(l); }
static void StaticUpdateMode(void* obj)              { if (obj) static_cast<RGBController_CoolmoonUSB*>(obj)->DeviceUpdateMode(); }
static void StaticConfigureZone(void* obj, int z)    { if (obj) static_cast<RGBController_CoolmoonUSB*>(obj)->DeviceConfigureZone(z); }

RGBController_CoolmoonUSB::RGBController_CoolmoonUSB(
    std::shared_ptr<CoolmoonUSBDriver> drv,
    std::vector<CoolmoonPort> p)
    : driver(std::move(drv)), ports(std::move(p)) {}

RGBController_Setup RGBController_CoolmoonUSB::CreateSetup() {
    RGBController_Setup setup;
    setup.name        = "Coolmoon USB ARGB Hub";
    setup.vendor      = "Coolmoon / USBFAN";
    setup.type        = DEVICE_TYPE_LEDSTRIP;
    setup.description = "Coolmoon USB ARGB Hub - per-port independent control";
    setup.location    = driver->GetLocation();
    setup.serial      = "";
    setup.version     = "1.2.0";
    setup.flags       = 0;
    setup.active_mode = 0;

    mode Direct;
    Direct.name       = "Direct";
    Direct.value      = 0x03;
    Direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR;
    Direct.color_mode = MODE_COLORS_PER_LED;
    setup.modes.push_back(Direct);

    for (size_t i = 0; i < ports.size(); ++i) {
        const auto& p = ports[i];

        zone z;
        z.name       = p.name;
        z.type       = ZONE_TYPE_LINEAR;
        z.flags      = ZONE_FLAG_MANUALLY_CONFIGURABLE_SIZE;
        z.leds_min   = 1;
        z.leds_max   = p.leds_max;
        z.leds_count = p.leds_default;
        setup.zones.push_back(z);

        for (unsigned j = 0; j < p.leds_default; ++j) {
            led l;
            l.name  = p.name + " LED " + std::to_string(j + 1);
            l.value = static_cast<unsigned>((i << 8) | j);
            setup.leds.push_back(l);
        }
    }

    setup.object_ptr            = this;
    setup.DeviceUpdateLEDs      = &StaticUpdateLEDs;
    setup.DeviceUpdateZoneLEDs  = &StaticUpdateZoneLEDs;
    setup.DeviceUpdateSingleLED = &StaticUpdateSingleLED;
    setup.DeviceUpdateMode      = &StaticUpdateMode;
    setup.DeviceConfigureZone   = &StaticConfigureZone;
    setup.DeviceSaveMode        = nullptr;
    setup.DeviceUpdateZoneMode                        = nullptr;
    setup.DeviceUpdateDeviceSpecificConfiguration     = nullptr;
    setup.DeviceUpdateDeviceSpecificZoneConfiguration = nullptr;

    return setup;
}

void RGBController_CoolmoonUSB::DeviceUpdateLEDs() {
    if (!controller) return;

    unsigned int total_leds = 0;
    for (size_t i = 0; i < ports.size(); ++i) {
        total_leds += controller->GetZoneLEDsCount(static_cast<unsigned int>(i));
    }

    std::vector<RGBColorData> buf(total_leds);
    for (unsigned int i = 0; i < total_leds; ++i) {
        RGBColor c = controller->GetColor(i);
        buf[i] = {
            static_cast<uint8_t>(RGBGetRValue(c)),
            static_cast<uint8_t>(RGBGetGValue(c)),
            static_cast<uint8_t>(RGBGetBValue(c))
        };
    }
    driver->SendBroadcastLEDs(buf);
}

void RGBController_CoolmoonUSB::DeviceUpdateZoneLEDs(int /*zone*/) {
    DeviceUpdateLEDs();
}

void RGBController_CoolmoonUSB::DeviceUpdateSingleLED(int /*led*/) {
    DeviceUpdateLEDs();
}

void RGBController_CoolmoonUSB::DeviceUpdateMode() {
    DeviceUpdateLEDs();
}

void RGBController_CoolmoonUSB::DeviceConfigureZone(int /*zone*/) {
    DeviceUpdateLEDs();
}
```

### 5.6. `OpenRGBCoolmoonPlugin.h`
```cpp
#pragma once

#include "OpenRGBPluginInterface.h"
#include "RGBController_CoolmoonUSB.h"
#include <vector>

class OpenRGBCoolmoonPlugin : public QObject, public OpenRGBPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID FILE "OpenRGBCoolmoonPlugin.json")
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    OpenRGBPluginInfo GetPluginInfo() override;
    unsigned int GetPluginAPIVersion() override;
    void Load(OpenRGBPluginAPIInterface* plugin_api_ptr) override;
    QWidget* GetWidget() override;
    void Unload() override;

private:
    OpenRGBPluginAPIInterface* api = nullptr;
    std::vector<RGBController_CoolmoonUSB*> controllers;
    QWidget* dummy_widget = nullptr;
};
```

### 5.7. `OpenRGBCoolmoonPlugin.cpp`
```cpp
#include "OpenRGBCoolmoonPlugin.h"
#include <QLabel>
#include <QVBoxLayout>

OpenRGBPluginInfo OpenRGBCoolmoonPlugin::GetPluginInfo() {
    OpenRGBPluginInfo info;
    info.Name            = "Coolmoon Custom Fan ARGB Plugin";
    info.Description     = "Native USB HID driver untuk kipas Coolmoon USB (Auto-Detect MCU & Per-LED)";
    info.Version         = "1.2.0";
    info.Commit          = "1.2.0";
    info.URL             = "https://openrgb.org";
    info.Location        = OPENRGB_PLUGIN_LOCATION_SETTINGS;
    info.Label           = "Coolmoon USB";
    info.ProtocolVersion = 0;
    return info;
}

unsigned int OpenRGBCoolmoonPlugin::GetPluginAPIVersion() {
    return OPENRGB_PLUGIN_API_VERSION;
}

void OpenRGBCoolmoonPlugin::Load(OpenRGBPluginAPIInterface* plugin_api_ptr) {
    api = plugin_api_ptr;
    hid_init();

    struct hid_device_info* devs = hid_enumerate(COOLMOON_VID, COOLMOON_PID);
    struct hid_device_info* cur  = devs;

    while (cur) {
        if (cur->usage_page >= 0xFF00 || cur->interface_number == 0) {
            hid_device* handle = hid_open_path(cur->path);
            if (handle) {
                auto drv = std::make_shared<CoolmoonUSBDriver>(handle, cur->path);

                // Auto-read port config dari hardware MCU
                std::vector<CoolmoonPort> port_map = drv->ReadFanConfig();
                if (port_map.empty()) {
                    // Fallback jika MCU belum/tidak merespon
                    port_map = {
                        {0, "Port 1 (Fan)",       200,  8},
                        {1, "Port 2 (Fan)",       200,  8},
                        {2, "Port 3 (Fan)",       200, 10},
                        {3, "Port 4 (Fan)",       200,  8},
                        {4, "Port 5 (Fan)",       200, 10},
                        {8, "Port 9 (Strip 72)",  200, 72},
                        {9, "Port 10 (Strip 18)", 200, 18},
                    };
                }
                auto* ctrl = new RGBController_CoolmoonUSB(drv, std::move(port_map));
                controllers.push_back(ctrl);

                RGBController_Setup setup = ctrl->CreateSetup();
                RGBControllerInterface* iface = api->CreateVirtualRGBController(&setup);
                ctrl->SetControllerInterface(iface);
                api->RegisterVirtualRGBController(iface);
                break;
            }
        }
        cur = cur->next;
    }
    hid_free_enumeration(devs);
}

QWidget* OpenRGBCoolmoonPlugin::GetWidget() {
    if (!dummy_widget) {
        dummy_widget = new QWidget();
        auto* layout = new QVBoxLayout(dummy_widget);
        auto* label  = new QLabel("Coolmoon Custom Fan ARGB Plugin is active.", dummy_widget);
        layout->addWidget(label);
    }
    return dummy_widget;
}

void OpenRGBCoolmoonPlugin::Unload() {
    for (auto* ctrl : controllers) {
        if (api && ctrl->GetControllerInterface()) {
            api->UnregisterVirtualRGBControllerInThread(ctrl->GetControllerInterface());
            api->DeleteVirtualRGBController(ctrl->GetControllerInterface());
        }
        delete ctrl;
    }
    controllers.clear();
    hid_exit();

    if (dummy_widget) {
        delete dummy_widget;
        dummy_widget = nullptr;
    }
}
```
