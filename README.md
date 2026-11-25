# OpenRGB Coolmoon USB ARGB Plugin

Native C++ Plugin untuk **OpenRGB 1.0+ (Windows x64 / Plugin API v5)** untuk mengontrol **Coolmoon USB ARGB Controller (`USBFAN` / `ROBOBLOQ`)**.

---

## ⚡ Fitur Utama

- **Auto-Read Hardware Config**: Otomatis mendeteksi port aktif dan jumlah LED langsung dari memori MCU controller saat startup atau rescan devices.
- **Full Per-LED Control**: Menggunakan protokol streaming beruntun `0x88` (`mubu_chai`), memungkinkan kontrol mandiri per-titik LED tanpa batasan 16-LED dan tanpa bentrok broadcast.
- **Fan Speed Control (PWM)**: Kontrol kecepatan kipas bawaan (0–100%) dengan preset (Silent, Normal, Turbo, Full Speed) dan dukungan sinkronisasi kabel PWM motherboard di tab **Settings -> Coolmoon USB**.
- **Manual Zone Resize**: Mendukung penyesuaian jumlah LED manual (1–200 LED per zone) lewat dialog bawaan **"Edit Zone"** di OpenRGB.
- **High LED Count Support**: Teruji stabil pada konfigurasi multi-kipas dan strip panjang (Port 72 LED, Port 18 LED, dsb.).

---

## 🛠️ Spesifikasi Hardware

| Properti | Nilai |
| :--- | :--- |
| **Vendor ID (VID)** | `0x1A86` |
| **Product ID (PID)** | `0xFE05` |
| **Usage Page** | `0xFF00` (Vendor-Defined) |
| **Usage** | `0x0001` |
| **Interface** | `0` (USB HID) |

> [!IMPORTANT]
> **Tutup software resmi MoonCenter** sebelum menjalankan OpenRGB agar tidak terjadi konflik akses eksklusif perangkat USB HID.

---

## 📥 Instalasi (Siap Pakai)

1. Pastikan OpenRGB versi **1.0+ (Windows 64-bit)** sudah terpasang.
2. Salin file `OpenRGBCoolmoonPlugin.dll` ke direktori plugin OpenRGB:
   ```text
   %APPDATA%\OpenRGB\plugins\OpenRGBCoolmoonPlugin.dll
   ```
3. Buka **OpenRGB** -> buka tab **Settings** -> **Plugins**.
4. Aktifkan centang **Coolmoon Custom Fan ARGB Plugin**.
5. Di tab **Devices**, pilih **Coolmoon USB ARGB Hub** dan atur warna sesuai selera.

---

## 🔨 Kompilasi Mandiri (Build from Source)

### Kebutuhan:
- Visual Studio 2022 / MSVC x64
- Windows SDK 10.0+
- Qt 6.8+ (MSVC 2022 64-bit)
- CMake 3.16+

### Perintah Build PowerShell:
```powershell
# 1. Konfigurasi Environment MSVC & Qt
$env:PATH = "C:\msvc\unpack\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64;C:\msvc\unpack\Windows Kits\10\bin\10.0.26100.0\x64;C:\Qt\6.8.3\msvc2022_64\bin;C:\Program Files\CMake\bin;" + $env:PATH
$env:INCLUDE = "C:\msvc\unpack\VC\Tools\MSVC\14.44.35207\include;C:\msvc\unpack\Windows Kits\10\Include\10.0.26100.0\ucrt;C:\msvc\unpack\Windows Kits\10\Include\10.0.26100.0\um;C:\msvc\unpack\Windows Kits\10\Include\10.0.26100.0\shared"
$env:LIB = "C:\msvc\unpack\VC\Tools\MSVC\14.44.35207\lib\x64;C:\msvc\unpack\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\msvc\unpack\Windows Kits\10\Lib\10.0.26100.0\um\x64;C:\Qt\6.8.3\msvc2022_64\lib"

# 2. Generate Build
cmake -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64" -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# 3. Build Plugin DLL
cmake --build build --config Release
```
Output DLL berada di `build/OpenRGBCoolmoonPlugin.dll`.

---

## 📖 Dokumentasi Teknis Lengkap

Spesifikasi protokol USB HID, analisis paket mentah, dan breakdown reverse engineering tersedia di [openrgb_coolmoon_usb_plugin_blueprint.md](file:///c:/laragon/www/project%20windows/coolmoon%20plugin%20openrgb/openrgb_coolmoon_usb_plugin_blueprint.md).
