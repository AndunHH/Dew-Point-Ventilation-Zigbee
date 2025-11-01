// how often shall ZigbeeHelper state machine be checked?
#define ZigbeeWAIT_MS 1000

// how often shall subtask in READY be repeated (send cmd and print debug)
#define ZigbeeREADY_MS 20000

// how often shall zigbee states be checked, if no switch present
#define ZigbeeNOTHINGBOUND_MS 1600

// print debug?
#define DEBUGZIGBEEHANDLING

enum ZigbeeSwitchHelperStates { ZB_INIT, ZB_WAIT, ZB_READY };

#include "Zigbee.h"

/* Zigbee switch configuration */
#define SWITCH_ENDPOINT_NUMBER 5

/// @brief ZigbeeSwitchHelper class to help with all zigbee related stuff. Call loop() regularly to
/// check the status etc. Use setLightSetpoint(true/false) to turn on the zigbee light/socket.
class ZigbeeSwitchHelper {
public:
  boolean init();

  void setLightSetpoint(boolean on);

  boolean loop();

  void printBoundDevicesLong();

  void toggleLightSetpoint();

  ZigbeeSwitchHelper()
      : zbSwitch(ZigbeeSwitch(SWITCH_ENDPOINT_NUMBER)), zigbeeSwitchHelperState(ZB_INIT),
        lightSetpoint(false) {}

  void reset();

  boolean isReady();

private:
  ZigbeeSwitch zbSwitch;
  boolean lightSetpoint;
  ZigbeeSwitchHelperStates zigbeeSwitchHelperState;
  unsigned long lastZigbeeTime;
};