/* This helper class in mainly based on the Zigbee Switch example taken from: https://github.com/espressif/arduino-esp32/tree/master/libraries/Zigbee/examples/Zigbee_On_Off_Switch

Therefore the apache license from the example applies*/

// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <Arduino.h>

#include "zigbeeSwitchHelper.h"

#ifndef ZIGBEE_MODE_ZCZR
#error "Zigbee coordinator mode is not selected in Tools->Zigbee mode"
#endif

/// @brief initialize the zigbee helper
/// @return
boolean ZigbeeSwitchHelper::init()
{
#ifdef DEBUGZIGBEEHANDLING
    Serial.print("Initializing zigbee ...");
#endif
    // Optional: set Zigbee device name and model
    zbSwitch.setManufacturerAndModel("Espressif", "ZigbeeSwitch");

    // Optional to allow multiple light to bind to the switch
    zbSwitch.allowMultipleBinding(true);

// Add endpoint to Zigbee Core
#ifdef DEBUGZIGBEEHANDLING
    Serial.println("Adding ZigbeeSwitch endpoint to Zigbee Core");
#endif
    Zigbee.addEndpoint(&zbSwitch);

    // Open network for 180 seconds after boot
    Zigbee.setRebootOpenNetwork(180);

    // When all EPs are registered, start Zigbee with ZIGBEE_COORDINATOR mode
    if (!Zigbee.begin(ZIGBEE_COORDINATOR))
    {
        Serial.println("Zigbee failed to start!");
        Serial.println("Rebooting...");
        ESP.restart();
    }

    zigbeeSwitchHelperState = ZB_WAIT;
    return true;
}

/// @brief zigbeeSwitchHelper function that is called regularly in loop()
/// @return true if something is bound
boolean ZigbeeSwitchHelper::loop()
{
    unsigned long now = millis();

    switch (zigbeeSwitchHelperState)
    {
    case ZB_WAIT:
        if (now - lastZigbeeTime >= ZigbeeWAIT_MS)
        {
#ifdef DEBUGZIGBEEHANDLING
            Serial.println("ZB WAIT");
#endif
            if (zbSwitch.bound()) // if something is bound
            {
                // TODO: check if it is a light or plug?
                zigbeeSwitchHelperState = ZB_READY;
            }
            else
            {
                zigbeeSwitchHelperState = ZB_WAIT;
            }
            lastZigbeeTime = now;
        }
        break;

    case ZB_READY:
        if (now - lastZigbeeTime >= ZigbeeREADY_MS)
        {
            if (zbSwitch.bound())
            {
#ifdef DEBUGZIGBEEHANDLING
                Serial.println("READY");
                // printBoundDevicesLong();
                //  readManufacturer leads to reboot https://github.com/espressif/arduino-esp32/issues/10777
                Serial.print("Plug: ");
                Serial.println(lightSetpoint ? "on" : "off");
#endif
                if (lightSetpoint)
                {
                    zbSwitch.lightOn();
                }
                else
                {
                    zbSwitch.lightOff();
                }
                lastZigbeeTime = now;
                zigbeeSwitchHelperState = ZB_READY;
            }
            else // nothing bound anymore
            {
                lastZigbeeTime = now;
                zigbeeSwitchHelperState = ZB_WAIT;
            }
        }
        break;
    default:
        zigbeeSwitchHelperState = ZB_WAIT;
        break;
    }

    // Return if something is bound
    return zbSwitch.bound();
} // end loop()

/// @brief print the long version of the bound device list with name and manufacturer
void ZigbeeSwitchHelper::printBoundDevicesLong()
{
    // zbSwitch.printBoundDevices(Serial) does the ieee adress, but without naming the manufacturer and light model. therefore hard coded long version

    std::list<zb_device_params_t *> boundLights = zbSwitch.getBoundDevices();
    for (const auto &device : boundLights)
    {
        /*Serial.printf("Device on endpoint %d, short address: 0x%x\r\n", device->endpoint, device->short_addr);
        Serial.printf(
            "IEEE Address: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n", device->ieee_addr[7], device->ieee_addr[6], device->ieee_addr[5], device->ieee_addr[4],
            device->ieee_addr[3], device->ieee_addr[2], device->ieee_addr[1], device->ieee_addr[0]);
        */
        Serial.printf("Light manufacturer: %s\r\n", zbSwitch.readManufacturer(device->endpoint, device->short_addr, device->ieee_addr));
        Serial.printf("Light model: %s\r\n", zbSwitch.readModel(device->endpoint, device->short_addr, device->ieee_addr));
    }
}

/// @brief toggle the setpoint for light
void ZigbeeSwitchHelper::toggleLightSetpoint()
{
    lightSetpoint = !lightSetpoint;
    // don't wait until the next regular update of the lift
    lastZigbeeTime -= ZigbeeREADY_MS;
}

/// @brief The setpoint, if light shall be on or off, is sent regularly with this parameter
/// @param on
void ZigbeeSwitchHelper::setLightSetpoint(boolean on)
{
    if (lightSetpoint != on)
    {
        // if something changed
        // don't wait until the next regular update of the light
        lastZigbeeTime -= ZigbeeREADY_MS;
        lightSetpoint = on;
    }
}

/// @brief Reset the zigbee device and reboot the esp to allow new binding
void ZigbeeSwitchHelper::reset()
{
    Serial.println("Zigbee factory reset!");
    delay(500); // yes, a real delay, just to flush the serial buffer before resetting the controller
    Zigbee.factoryReset();
}

/// @brief Check if zigbeeSwitch Helper is in state ready i.e. commands can be sent
/// @return true if ready
boolean ZigbeeSwitchHelper::isReady() {
    return (zigbeeSwitchHelperState == ZB_READY);
}