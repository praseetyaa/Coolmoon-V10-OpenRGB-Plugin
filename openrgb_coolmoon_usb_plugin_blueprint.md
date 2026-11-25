# Blueprint Pengembangan: Native OpenRGB C++ Plugin untuk Coolmoon / USBFAN (`1A86:FE05`)

Dokumen ini berfungsi sebagai **Single Source of Truth (SSOT)** dan spesifikasi teknis lengkap untuk membangun *Native OpenRGB C++ Plugin* (`.dll`) pada Windows x64 (**OpenRGB 1.0+ / Plugin API v5**).

---

## 1. Ringkasan Proyek & Tujuan

* **Target Aplikasi:** OpenRGB 1.0+ (Windows 64-bit, Plugin API v5).
* **Target Hardware Controller:** Coolmoon USB ARGB Controller (terdeteksi di Windows sebagai `USBFAN`).
* **Identitas USB HID:**
  * **Vendor ID (`VID`):** `0x1A86`
  * **Product ID (`PID`):** `0xFE05`
  * **HID Usage Page:** Vendor-Defined (`>= 0xFF00`)
* **Target Kipas & Topologi LED Custom:**
  * **Model Kipas:** Paradox Gaming F-120 PRO ARGB (`120 × 120 × 25 mm`) dengan modifikasi *3D-printed* bezel luar dan *center hub cap*.
  * **Konfigurasi Fisik per Kipas (`24 LED` total):**
    1. **Bezel Alis (`16 LED`):** 2 strip *side-view* WS2812B (`8 LED` kiri + `8 LED` kanan) terpasang seri pada indeks LED `0–15`.
    2. **Center Hub Ring (`8 LED`):** 1 ring WS2812B 8-bit di balik *hub cap* tengah pada indeks LED `16–23`.
  * **Pemisahan Zona di OpenRGB:** Setiap port kipas didaftarkan sebagai **2 Zone independen** (`Fan X - Bezel Alis` berisi `16 LED` dan `Fan X - Center Hub` berisi `8 LED`) agar *OpenRGB Effects Plugin* dapat menjalankan efek berbeda secara bersamaan pada alis dan bagian tengah kipas.

---

## 2. Spesifikasi Protokol USB HID (`USBFAN 1A86:FE05`)

Berdasarkan analisis paket *Interrupt OUT* dari perangkat asli (`USBPcap2`), controller menerima paket berukuran tetap **`64 byte`** (di Windows dikirim lewat `hid_write` dengan tambahan 1 byte `Report ID = 0x00` di depan, sehingga buffer pengiriman berjumlah **`65 byte`**).

### Struktur Frame Paket `64 Byte`

| Offset Byte | Panjang | Nilai Hex | Keterangan / Fungsi Protokol |
| :--- | :--- | :--- | :--- |
| `0 – 2` | 3 Byte | `52 42 3C` | Header ASCII **`"RB"`** + panjang frame aktif **`60` byte (`0x3C`)** |
| `3` | 1 Byte | `0x00..0x0F` | Index port / nomor urut fragmen paket (`0` untuk LED `0–15`, `1` untuk LED `16–23`) |
| `4` | 1 Byte | `0x03` | Command: **Direct Per-LED RGB Stream** |
| `5 – 6` | 2 Byte | `FF FF` | Port bitmask aktif |
| `7 – 10` | 4 Byte | `00 03 01 99` | Parameter kontrol stream warna bawaan firmware `USBFAN` |
| `11 – 58` | 48 Byte | `R0 G0 B0 ... R15 G15 B15` | Payload warna mentah untuk maksimal **`16 LED` (`16 × 3 byte`)** dengan urutan **`R-G-B`** |
| `59` | 1 Byte | `SUM & 0xFF` | **Checksum:** Penjumlahan byte `0` sampai byte `58`, diambil 8-bit terakhir |
| `60 – 63` | 4 Byte | `00 00 00 00` | Padding nol penggenap ukuran paket HID `64 byte` |

### Aturan Penting Komunikasi
1. **Urutan Warna (`Color Order`):** **`R-G-B`** (bukan `G-R-B`).
2. **Pembaruan Instan (*No Separate Latch*):** Controller langsung memperbarui nyala LED setiap kali paket `52 42 3C` diterima tanpa memerlukan paket *commit/latch* tambahan.
3. **Fragmentasi Paket (> 16 LED):** Satu paket menampung maksimal **`16 LED` (`48 byte`)**. Karena setiap kipas memiliki **`24 LED`**, driver memecah pengiriman per port menjadi **2 paket berurutan**:
   * **Paket 0:** Mengirim LED `0–15` (*Bezel Alis*).
   * **Paket 1:** Mengirim LED `16–23` (*Center Hub*) + 8 slot LED sisanya diisi `0x00`.

---

## 3. Struktur Direktori Proyek

```text
OpenRGBCoolmoonPlugin/
├── OPENRGB_COOLMOON_PLUGIN_DOCS.md   <-- Dokumen blueprint ini
├── CMakeLists.txt                    <-- Build script otomatis (Qt + hidapi + OpenRGB SDK)
├── OpenRGBCoolmoonPlugin.h           <-- Header entry-point OpenRGB Plugin API v5
├── OpenRGBCoolmoonPlugin.cpp         <-- Implementasi deteksi HID & registrasi controller
├── RGBController_CoolmoonUSB.h       <-- Header definisi Zone (Bezel Alis + Center Hub)
├── RGBController_CoolmoonUSB.cpp     <-- Implementasi mapping warna per-zone & per-LED
├── CoolmoonUSBDriver.h               <-- Header driver USB HID & konstanta VID/PID
├── CoolmoonUSBDriver.cpp             <-- Implementasi pembentuk paket 64-byte & checksum
└── third_party/
    └── OpenRGB/                      <-- Shallow clone dari CalcProgrammer1/OpenRGB
```

---

## 4. Instruksi Setup & Build untuk AI Agent (Antigravity)

Jika proyek ini dikerjakan oleh AI Agent di Antigravity, ikuti urutan eksekusi berikut secara berurutan:

### Langkah 1: Cek Versi Qt dari Instalasi OpenRGB Lokal
Periksa folder instalasi OpenRGB di Windows pengguna (biasanya di `C:\Program Files\OpenRGB\` atau folder portable OpenRGB):
* Jika terdapat `Qt6Core.dll`, gunakan **Qt 6** (`find_package(Qt6 REQUIRED COMPONENTS Core Widgets)`).
* Jika terdapat `Qt5Core.dll`, gunakan **Qt 5** (`find_package(Qt5 REQUIRED COMPONENTS Core Widgets)`).
* *Penting:* Versi mayor Qt (Qt5 vs Qt6) dan arsitektur (`x64`) pada `.dll` plugin **wajib sama** dengan executable `OpenRGB.exe` agar tidak gagal dimuat (`LoadLibrary` error).

### Langkah 2: Ambil Header & Base Class OpenRGB SDK
Jalankan perintah berikut di root folder proyek agar header resmi OpenRGB dan `RGBController.cpp` tersedia untuk proses kompilasi:
```powershell
git clone --depth 1 https://github.com/CalcProgrammer1/OpenRGB.git third_party/OpenRGB
```
*Verifikasi kompatibilitas API:* Sebelum mengompilasi, periksa `third_party/OpenRGB/OpenRGBPluginInterface.h` dan `third_party/OpenRGB/OpenRGBPluginAPIInterface.h` untuk memastikan nama metode registrasi controller (`RegisterVirtualRGBController` / `UnregisterVirtualRGBControllerInThread` atau `RegisterRGBController` / `UnregisterRGBController`) sesuai dengan versi header tersebut.

### Langkah 3: Kompilasi `.dll` via CMake (MSVC x64)
`CMakeLists.txt` di bawah sudah menggunakan `FetchContent` untuk mengunduh dan membangun pustaka `hidapi` secara statis secara otomatis sehingga tidak perlu mencari file `.lib` eksternal:
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Output akhir akan dihasilkan pada: `build/Release/OpenRGBCoolmoonPlugin.dll`.

---

## 5. Kode Sumber Lengkap (Production-Ready)

### 5.1. `CMakeLists.txt`
```cmake
cmake_minimum_required(VERSION 3.16)
project(OpenRGBCoolmoonPlugin LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

include(FetchContent)

# 1. Unduh & build hidapi secara otomatis untuk Windows HID
FetchContent_Declare(
    hidapi
    GIT_REPOSITORY https://github.com/libusb/hidapi.git
    GIT_TAG        hidapi-0.14.0
)
set(HIDAPI_BUILD_HIDTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(hidapi)

# 2. Deteksi Qt6 atau Qt5 (sesuaikan dengan versi OpenRGB yang terpasang)
find_package(Qt6 COMPONENTS Core Widgets QUIET)
if(Qt6_FOUND)
    set(QT_LIBS Qt6::Core Qt6::Widgets)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
    set(QT_LIBS Qt5::Core Qt5::Widgets)
endif()

set(OPENRGB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OpenRGB")

# 3. Target Plugin DLL
add_library(OpenRGBCoolmoonPlugin SHARED
    OpenRGBCoolmoonPlugin.h
    OpenRGBCoolmoonPlugin.cpp
    RGBController_CoolmoonUSB.h
    RGBController_CoolmoonUSB.cpp
    CoolmoonUSBDriver.h
    CoolmoonUSBDriver.cpp
    ${OPENRGB_DIR}/RGBController/RGBController.cpp
)

target_include_directories(OpenRGBCoolmoonPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENRGB_DIR}
    ${OPENRGB_DIR}/RGBController
    ${OPENRGB_DIR}/dependencies/json
)

target_link_libraries(OpenRGBCoolmoonPlugin PRIVATE
    ${QT_LIBS}
    hidapi::hidapi
    setupapi
)
```

### 5.2. `CoolmoonUSBDriver.h`
```cpp
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

class CoolmoonUSBDriver {
public:
    CoolmoonUSBDriver(hid_device* handle, std::string path);
    ~CoolmoonUSBDriver();

    std::string GetLocation() const { return device_path; }
    void SendChannelLEDs(uint8_t channel, const std::vector<RGBColorData>& leds);
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

    // Windows HID membutuhkan 1 byte Report ID (0x00) di depan paket 64 byte
    uint8_t report[65];
    report[0] = 0x00;
    std::memcpy(&report[1], payload, 64);
    hid_write(dev, report, sizeof(report));
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
        pkt[3]  = static_cast<uint8_t>(channel + packet_idx);
        pkt[4]  = 0x03; // Mode Direct Per-LED
        pkt[5]  = 0xFF; // Port bitmask high
        pkt[6]  = 0xFF; // Port bitmask low
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

void CoolmoonUSBDriver::CommitFrame() {
    // USBFAN menerapkan perubahan warna langsung saat paket 52 42 3C diterima
}
```

### 5.4. `RGBController_CoolmoonUSB.h`
```cpp
#pragma once

#include <memory>
#include "RGBController.h"
#include "CoolmoonUSBDriver.h"

class RGBController_CoolmoonUSB : public RGBController {
public:
    RGBController_CoolmoonUSB(std::shared_ptr<CoolmoonUSBDriver> drv, uint8_t fan_ports = 4);
    ~RGBController_CoolmoonUSB() override = default;

    void SetupZones() override;
    void ResizeZone(int zone, int new_size) override;
    void DeviceUpdateLEDs() override;
    void UpdateZoneLEDs(int zone) override;
    void UpdateSingleLED(int led) override;
    void DeviceUpdateMode() override;

private:
    std::shared_ptr<CoolmoonUSBDriver> driver;
    uint8_t active_ports;
};
```

### 5.5. `RGBController_CoolmoonUSB.cpp`
```cpp
#include "RGBController_CoolmoonUSB.h"

RGBController_CoolmoonUSB::RGBController_CoolmoonUSB(std::shared_ptr<CoolmoonUSBDriver> drv, uint8_t fan_ports)
    : driver(std::move(drv)), active_ports(fan_ports) {
    name        = "Coolmoon USB ARGB Controller (Custom Bezel + Hub)";
    vendor      = "Coolmoon / USBFAN";
    type        = DEVICE_TYPE_LEDSTRIP;
    description = "Native HID Plugin untuk Kipas Custom 16-LED Bezel Alis + 8-LED Center Ring";
    location    = driver->GetLocation();

    mode Direct;
    Direct.name       = "Direct";
    Direct.value      = 0x03;
    Direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR;
    Direct.color_mode = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    mode Static;
    Static.name       = "Static";
    Static.value      = 0x01;
    Static.flags      = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
    Static.color_mode = MODE_COLORS_MODE_SPECIFIC;
    Static.colors_min = 1;
    Static.colors_max = 1;
    Static.colors.resize(1);
    modes.push_back(Static);

    SetupZones();
}

void RGBController_CoolmoonUSB::SetupZones() {
    zones.clear();
    leds.clear();

    // Setiap port kipas (24 LED) dipecah menjadi 2 Zone independen:
    // Zone (2*p)     -> Bezel Alis (16 LED: 8 kiri + 8 kanan)
    // Zone (2*p + 1) -> Center Hub Ring (8 LED)
    for (uint8_t p = 0; p < active_ports; ++p) {
        zone bezel_zone;
        bezel_zone.name       = "Fan " + std::to_string(p + 1) + " - Bezel Alis";
        bezel_zone.type       = ZONE_TYPE_LINEAR;
        bezel_zone.leds_min   = 16;
        bezel_zone.leds_max   = 16;
        bezel_zone.leds_count = 16;
        bezel_zone.matrix_map = nullptr;
        zones.push_back(bezel_zone);

        for (int i = 0; i < 16; ++i) {
            led l;
            l.name  = "Fan " + std::to_string(p + 1) + " Alis LED " + std::to_string(i + 1);
            l.value = (p << 8) | i;
            leds.push_back(l);
        }

        zone hub_zone;
        hub_zone.name       = "Fan " + std::to_string(p + 1) + " - Center Hub";
        hub_zone.type       = ZONE_TYPE_LINEAR;
        hub_zone.leds_min   = 8;
        hub_zone.leds_max   = 8;
        hub_zone.leds_count = 8;
        hub_zone.matrix_map = nullptr;
        zones.push_back(hub_zone);

        for (int i = 0; i < 8; ++i) {
            led l;
            l.name  = "Fan " + std::to_string(p + 1) + " Hub LED " + std::to_string(i + 1);
            l.value = (p << 8) | (16 + i);
            leds.push_back(l);
        }
    }

    SetupColors();
}

void RGBController_CoolmoonUSB::ResizeZone(int /*zone*/, int /*new_size*/) {
    // Dikunci pada topologi fisik 16 LED Bezel + 8 LED Center Hub per kipas
}

void RGBController_CoolmoonUSB::DeviceUpdateLEDs() {
    for (uint8_t p = 0; p < active_ports; ++p) {
        std::vector<RGBColorData> port_buffer(24);
        const size_t base_idx = p * 24;

        for (size_t i = 0; i < 24; ++i) {
            RGBColor c = (modes[active_mode].color_mode == MODE_COLORS_MODE_SPECIFIC)
                             ? modes[active_mode].colors[0]
                             : colors[base_idx + i];
            port_buffer[i] = {
                static_cast<uint8_t>(RGBGetRValue(c)),
                static_cast<uint8_t>(RGBGetGValue(c)),
                static_cast<uint8_t>(RGBGetBValue(c))
            };
        }
        driver->SendChannelLEDs(p, port_buffer);
    }
    driver->CommitFrame();
}

void RGBController_CoolmoonUSB::UpdateZoneLEDs(int zone) {
    const uint8_t port = static_cast<uint8_t>(zone / 2);
    std::vector<RGBColorData> port_buffer(24);
    const size_t base_idx = port * 24;

    for (size_t i = 0; i < 24; ++i) {
        RGBColor c = colors[base_idx + i];
        port_buffer[i] = {
            static_cast<uint8_t>(RGBGetRValue(c)),
            static_cast<uint8_t>(RGBGetGValue(c)),
            static_cast<uint8_t>(RGBGetBValue(c))
        };
    }
    driver->SendChannelLEDs(port, port_buffer);
    driver->CommitFrame();
}

void RGBController_CoolmoonUSB::UpdateSingleLED(int led) {
    UpdateZoneLEDs((led / 24) * 2);
}

void RGBController_CoolmoonUSB::DeviceUpdateMode() {
    DeviceUpdateLEDs();
}
```

### 5.6. `OpenRGBCoolmoonPlugin.h`
```cpp
#pragma once

#include <QObject>
#include <QtPlugin>
#include <vector>
#include "OpenRGBPluginInterface.h"
#include "RGBController_CoolmoonUSB.h"

class OpenRGBCoolmoonPlugin : public QObject, public OpenRGBPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID)
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    OpenRGBPluginInfo GetPluginInfo() override;
    unsigned int GetPluginAPIVersion() override;
    void Load(OpenRGBPluginAPIInterface* plugin_api_ptr) override;
    QWidget* GetWidget() override { return nullptr; }
    QMenu* GetTrayMenu() override { return nullptr; }
    void Unload() override;

    void OnProfileLoad(nlohmann::json /*profile_data*/) override {}
    nlohmann::json OnProfileSave() override { return nlohmann::json(); }
    unsigned char* OnSDKCommand(unsigned int, unsigned char*, unsigned int*) override { return nullptr; }

private:
    OpenRGBPluginAPIInterface* api = nullptr;
    std::vector<RGBController_CoolmoonUSB*> controllers;
};
```

### 5.7. `OpenRGBCoolmoonPlugin.cpp`
```cpp
#include "OpenRGBCoolmoonPlugin.h"

OpenRGBPluginInfo OpenRGBCoolmoonPlugin::GetPluginInfo() {
    OpenRGBPluginInfo info;
    info.Name        = "Coolmoon Custom Fan ARGB Plugin";
    info.Description = "Native USB HID driver untuk kipas custom 16-LED Bezel + 8-LED Hub Ring";
    info.Version     = "1.0.0";
    info.Author      = "Custom Build";
    info.Location    = OPENRGB_PLUGIN_LOCATION_NONE;
    info.Label       = "Coolmoon USB";
    return info;
}

unsigned int OpenRGBCoolmoonPlugin::GetPluginAPIVersion() {
    return OPENRGB_PLUGIN_API_VERSION; // API v5 (OpenRGB 1.0+)
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
                auto drv   = std::make_shared<CoolmoonUSBDriver>(handle, cur->path);
                auto* ctrl = new RGBController_CoolmoonUSB(drv, 4); // Ubah angka 4 sesuai jumlah kipas terpasang
                controllers.push_back(ctrl);
                api->RegisterVirtualRGBController(ctrl);
                break;
            }
        }
        cur = cur->next;
    }
    hid_free_enumeration(devs);
}

void OpenRGBCoolmoonPlugin::Unload() {
    for (auto* ctrl : controllers) {
        if (api) {
            api->UnregisterVirtualRGBControllerInThread(ctrl);
        }
        delete ctrl;
    }
    controllers.clear();
    hid_exit();
}
```

---

## 6. Instalasi, Pengujian & Kalibrasi Lanjutan

1. **Tutup Aplikasi Bawaan:** Pastikan *MoonCenter* tertutup penuh (termasuk ikon *system tray*) sebelum membuka OpenRGB agar tidak terjadi konflik eksklusif pada endpoint USB HID.
2. **Pasang Plugin ke OpenRGB:**
   * Buka **OpenRGB $\rightarrow$ Settings $\rightarrow$ Plugins $\rightarrow$ Install Plugin**.
   * Pilih file `OpenRGBCoolmoonPlugin.dll` dari folder `build/Release/` (atau salin langsung ke `%APPDATA%\OpenRGB\plugins\`).
   * Centang **Enabled** pada *Coolmoon Custom Fan ARGB Plugin*, lalu restart OpenRGB.
3. **Verifikasi Zona:**
   * Pada tab **Devices**, pastikan muncul perangkat **Coolmoon USB ARGB Controller (Custom Bezel + Hub)**.
   * Uji nyala warna terpisah pada **`Fan 1 - Bezel Alis`** (`16 LED`) dan **`Fan 1 - Center Hub`** (`8 LED`).
4. **Catatan Kalibrasi Hardware (Jika Diperlukan Nanti):**
   * **Jumlah Kipas (`active_ports`):** Saat ini diset default `4` kipas (`new RGBController_CoolmoonUSB(drv, 4)` di `OpenRGBCoolmoonPlugin.cpp`). Ubah angka `4` jika jumlah kipas di casing berbeda.
   * **Per-Port Addressing vs Broadcast:** Pada tangkapan awal, `pkt[5..6]` bernilai `0xFF 0xFF` (*port bitmask*). Jika saat diuji semua port menyala seragam secara bersamaan, lakukan 1 kali perekaman Wireshark saat mengubah warna **Port 2** secara terpisah di MoonCenter untuk memperbarui logika *bitmask* `pkt[5..6]` pada `CoolmoonUSBDriver.cpp`.

---

## 7. Prompt Siap Pakai untuk Antigravity

Salin dan kirimkan instruksi berikut ke chat Agent Antigravity setelah menyimpan dokumen ini di folder kerja proyekmu:

> *"Baca file `OPENRGB_COOLMOON_PLUGIN_DOCS.md` di workspace ini. Buat seluruh file sumber proyek C++ sesuai struktur pada Bagian 3 dan Bagian 5, lalu jalankan langkah pada Bagian 4: cek versi Qt dari instalasi OpenRGB yang terpasang di PC ini, clone repository `CalcProgrammer1/OpenRGB` ke `third_party/OpenRGB`, sesuaikan API registrasi controller jika diperlukan, dan compile proyek ini hingga menghasilkan file `OpenRGBCoolmoonPlugin.dll` versi Release x64."*