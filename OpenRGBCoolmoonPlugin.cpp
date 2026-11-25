#include "OpenRGBCoolmoonPlugin.h"
#include "CoolmoonIcon.h"
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

OpenRGBPluginInfo OpenRGBCoolmoonPlugin::GetPluginInfo() {
    OpenRGBPluginInfo info;
    info.Name            = "Coolmoon Custom Fan ARGB Plugin";
    info.Description     = "Native USB HID driver untuk kipas Coolmoon USB (Auto-Detect MCU & Per-LED & Fan Control)";
    info.Version         = "1.3.0";
    info.Commit          = "1.3.0";
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
        auto* main_layout = new QVBoxLayout(dummy_widget);
        main_layout->setAlignment(Qt::AlignTop);

        // Header
        auto* title_lbl = new QLabel("<h2>Coolmoon USB Fan & ARGB Controller</h2>", dummy_widget);
        main_layout->addWidget(title_lbl);

        auto* desc_lbl = new QLabel("Native hardware driver untuk OpenRGB. Kontrol pencahayaan ARGB dan kecepatan kipas.", dummy_widget);
        desc_lbl->setStyleSheet("color: #888; margin-bottom: 12px;");
        main_layout->addWidget(desc_lbl);

        // Group: Fan Speed Control
        auto* fan_group = new QGroupBox("Fan Speed Control (PWM)", dummy_widget);
        auto* fan_layout = new QVBoxLayout(fan_group);

        // Slider Row
        auto* slider_row = new QHBoxLayout();
        auto* slider_label = new QLabel("Kecepatan Kipas:", fan_group);
        auto* speed_val_lbl = new QLabel("50%", fan_group);
        speed_val_lbl->setStyleSheet("font-weight: bold; min-width: 45px;");

        auto* slider = new QSlider(Qt::Horizontal, fan_group);
        slider->setRange(0, 100);
        slider->setValue(50);
        slider->setTickInterval(10);
        slider->setTickPosition(QSlider::TicksBelow);

        slider_row->addWidget(slider_label);
        slider_row->addWidget(slider, 1);
        slider_row->addWidget(speed_val_lbl);
        fan_layout->addLayout(slider_row);

        // Preset Buttons Row
        auto* btn_row = new QHBoxLayout();
        auto* btn_silent = new QPushButton("Silent (30%)", fan_group);
        auto* btn_normal = new QPushButton("Normal (60%)", fan_group);
        auto* btn_turbo  = new QPushButton("Turbo (85%)", fan_group);
        auto* btn_max    = new QPushButton("Full Speed (100%)", fan_group);

        btn_row->addWidget(btn_silent);
        btn_row->addWidget(btn_normal);
        btn_row->addWidget(btn_turbo);
        btn_row->addWidget(btn_max);
        fan_layout->addLayout(btn_row);

        // Motherboard PWM Sync Checkbox
        auto* chk_sync = new QCheckBox("Motherboard PWM Sync (Gunakan sinyal PWM kabel header Motherboard)", fan_group);
        chk_sync->setStyleSheet("margin-top: 10px;");
        fan_layout->addWidget(chk_sync);

        main_layout->addWidget(fan_group);

        // Wire Slider & Buttons to Hardware Driver
        QObject::connect(slider, &QSlider::valueChanged, [this, speed_val_lbl](int val) {
            speed_val_lbl->setText(QString::number(val) + "%");
            for (auto* ctrl : controllers) {
                if (ctrl && ctrl->GetDriver()) {
                    ctrl->GetDriver()->SetFanSpeed(static_cast<uint8_t>(val));
                }
            }
        });

        auto set_preset = [slider](int val) {
            slider->setValue(val);
        };
        QObject::connect(btn_silent, &QPushButton::clicked, [set_preset]() { set_preset(30); });
        QObject::connect(btn_normal, &QPushButton::clicked, [set_preset]() { set_preset(60); });
        QObject::connect(btn_turbo,  &QPushButton::clicked, [set_preset]() { set_preset(85); });
        QObject::connect(btn_max,    &QPushButton::clicked, [set_preset]() { set_preset(100); });

        QObject::connect(chk_sync, &QCheckBox::toggled, [this, slider, btn_silent, btn_normal, btn_turbo, btn_max](bool checked) {
            uint8_t mode = checked ? 1 : 0;
            slider->setEnabled(!checked);
            btn_silent->setEnabled(!checked);
            btn_normal->setEnabled(!checked);
            btn_turbo->setEnabled(!checked);
            btn_max->setEnabled(!checked);

            for (auto* ctrl : controllers) {
                if (ctrl && ctrl->GetDriver()) {
                    ctrl->GetDriver()->SetFanControlMode(mode);
                }
            }
        });

        main_layout->addStretch();
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
