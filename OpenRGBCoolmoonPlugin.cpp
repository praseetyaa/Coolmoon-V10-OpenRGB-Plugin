#include "OpenRGBCoolmoonPlugin.h"
#include "CoolmoonIcon.h"
#include <QLabel>
#include <QVBoxLayout>

OpenRGBPluginInfo OpenRGBCoolmoonPlugin::GetPluginInfo() {
    OpenRGBPluginInfo info;
    info.Name            = "Coolmoon Custom Fan ARGB Plugin";
    info.Description     = "Native USB HID driver untuk kipas Coolmoon USB (Auto-Detect MCU & Per-LED)";
    info.Version         = "1.2.0";
    info.Commit          = "1.2.0";
    info.URL             = "https://github.com/praseetyaa/Coolmoon-V10-OpenRGB-Plugin";
    info.Location        = OPENRGB_PLUGIN_LOCATION_SETTINGS;
    info.Label           = "Coolmoon USB";
    info.ProtocolVersion = 0;

    QImage icon          = GetCoolmoonIcon();
    info.Icon            = icon;
    info.TabIcon         = icon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation);

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
