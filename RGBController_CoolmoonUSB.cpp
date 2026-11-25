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
    setup.version     = "1.1.0";
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
        // matrix_map not needed for LINEAR
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
