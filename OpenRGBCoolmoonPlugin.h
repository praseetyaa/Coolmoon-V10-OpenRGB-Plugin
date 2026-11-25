#pragma once

#include <QObject>
#include <QtPlugin>
#include <QWidget>
#include <vector>
#include "OpenRGBPluginInterface.h"
#include "RGBController_CoolmoonUSB.h"

class OpenRGBCoolmoonPlugin : public QObject, public OpenRGBPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID FILE "OpenRGBCoolmoonPlugin.json")
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    OpenRGBPluginInfo GetPluginInfo() override;
    unsigned int GetPluginAPIVersion() override;
    void Load(OpenRGBPluginAPIInterface* plugin_api_ptr) override;
    QWidget* GetWidget() override;
    QMenu* GetTrayMenu() override { return nullptr; }
    void Unload() override;

    void OnProfileAboutToLoad() override {}
    void OnProfileLoad(nlohmann::json /*profile_data*/) override {}
    nlohmann::json OnProfileSave() override { return nlohmann::json(); }
    unsigned char* OnSDKCommand(unsigned int /*pkt_id*/, unsigned char* /*pkt_data*/, unsigned int* /*pkt_size*/) override { return nullptr; }

    void ProfileManagerUpdated(unsigned int /*update_reason*/) override {}
    void ResourceManagerUpdated(unsigned int /*update_reason*/) override {}
    void SettingsManagerUpdated(unsigned int /*update_reason*/) override {}

private:
    OpenRGBPluginAPIInterface* api = nullptr;
    std::vector<RGBController_CoolmoonUSB*> controllers;
    QWidget* dummy_widget = nullptr;
};
