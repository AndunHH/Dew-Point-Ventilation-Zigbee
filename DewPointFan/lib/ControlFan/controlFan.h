// how often shall fan states be checked?
// best be multiple of 1000 for more accurate counting in seconds
#define FANwaitMS 2000


//how long is the fan on and off?
#define FanON_MS 16*60*1000
#define FanOFF_MS 10*60*1000

// print debug?
//define DEBUGFANHANDLING

// LENGTH of string for control data logging
#define LOGCTRLSTR_LENGTH 22

enum ControlFanStates {CF_INIT, CF_OFF, CF_AUTO, CF_ON};

/// @brief ControlFan class to handle the control logic with the modes on, off and auto. The user is able to toggle through this modes by calling incrementuserSetpoint(). ControlFan takes care, that the cheap ventilator get a timeout after e.g. 16 min and restarts after 15 min. The function loop() shall be called regularly and will tell another actual output control function if the fan shall be enabled or disabled.
class ControlFan {
    public:
        boolean init();

        boolean loop(boolean isVentilationUsefull);

        ControlFanStates getUserSetpoint();

        ControlFanStates incrementUserSetpoint();

        ControlFan() : 
            controlFanState(CF_INIT),
            userSetpointState(CF_AUTO),
            cntOnSeconds(0),
            cntOffSeconds(0)
         {}

         void createLogChar(char *logStr);

         void resetFanRunTime();
         void getModeCharacter(char* modeChar);

     private:
         ControlFanStates controlFanState;
         ControlFanStates userSetpointState;
         unsigned long lastFanSMTime;  // time to check state machine
         unsigned long lastFanRunTime; // time to check actual fan runtim
         uint16_t cntOnSeconds;
         uint16_t cntOffSeconds;
 };