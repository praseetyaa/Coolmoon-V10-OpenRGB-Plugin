#pragma once

#include <memory>
#include <vector>
#include <string>
#include "RGBControllerInterface.h"
#include "CoolmoonUSBDriver.h"

class RGBController_CoolmoonUSB {
public:
    // ports: list of active ports with their channel mapping
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
