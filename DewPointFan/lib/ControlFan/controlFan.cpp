#include <Arduino.h>

#include "controlFan.h"

/// @brief Set everything to auto and zero seconds.
/// @return false -> dont't turn on the fan
boolean ControlFan::init()
{
    controlFanState = CF_OFF;
    cntOnSeconds = 0;
    cntOffSeconds = 0;
    return false; //don't start the fan
}

/// @brief ControlFan function that is called regularly in loop(). It decides wether to turn on the fan or turn it off. It depends on the usersetpoint mode, which can be iterated by incremenetUserSetpoint().
/// @param isVentilationUsefull decides in mode AUTO, if fan is turned on
/// @return true if fan shall actually be turned on.
boolean ControlFan::loop(boolean isVentilationUsefull)
{
    unsigned long now = millis();
    boolean turnFanOn = false;
    switch (controlFanState)
    {
    case CF_INIT:
        // usually, this init state is not entered, because init() is probably already called during setup() of main programm
#ifdef DEBUGFANHANDLING
        Serial.println("INIT");
#endif
        turnFanOn = init();
        break;
    case CF_OFF:
        turnFanOn = false;
        if (now - lastFanSMTime >= FANwaitMS)
        {
            cntOffSeconds = cntOffSeconds + (now - lastFanSMTime) / 1000;
            cntOffSeconds = min(cntOffSeconds, (uint16_t)65435); // keep value 100s below max
#ifdef DEBUGFANHANDLING
            Serial.println("OFF");
#endif
            if (now - lastFanRunTime >= FanOFF_MS)
            {
                // Fan was off long enough, maybe turn it on again
                // if userMode == auto and ventilationIsUsefull -> change to on
                if ((userSetpointState == CF_AUTO) && isVentilationUsefull)
                {
#ifdef DEBUGFANHANDLING
                    Serial.println("AUTO -> fan on");
#endif
                    controlFanState = CF_ON;
                    cntOffSeconds = 0;
                    lastFanRunTime = now;
                }

                // if userMode == on -> change to ON
                if (userSetpointState == CF_ON)
                {
#ifdef DEBUGFANHANDLING
                    Serial.println("ON -> fan on");
#endif
                    controlFanState = CF_ON;
                    cntOffSeconds = 0;
                    lastFanRunTime = now;
                }
            }
            lastFanSMTime = now;
        }
        break;
    case CF_ON:
        turnFanOn = true;
        if (now - lastFanSMTime >= FANwaitMS)
        {
#ifdef DEBUGFANHANDLING
            Serial.println("ON");
#endif
            cntOnSeconds = cntOnSeconds + (now - lastFanSMTime) / 1000;
            cntOnSeconds = min(cntOnSeconds, (uint16_t)65435); // keep value 100s below max
            // if userMode == OFF -> change to OFF
            if (userSetpointState == CF_OFF)
            {
#ifdef DEBUGFANHANDLING
                Serial.println("OFF -> fan off");
#endif
                controlFanState = CF_OFF;
                cntOnSeconds = 0; // reset to zero
                lastFanRunTime = now;
            }
            // Fan was on long enough, maybe turn it off
            else if (now - lastFanRunTime >= FanON_MS)
            {
#ifdef DEBUGFANHANDLING
                Serial.println("Fan ON long enough -> off");
#endif
                controlFanState = CF_OFF;
                cntOnSeconds = 0; // reset to zero
                lastFanRunTime = now;
            }
            lastFanSMTime = now;
        }
        break;
    default:
        turnFanOn = false;
        controlFanState = CF_INIT;
        break;
    }
    return turnFanOn;
} // end loop()

/// @brief get the setpoint chosen by the user Off, Auto or On
/// @return the set point
ControlFanStates ControlFan::getUserSetpoint()
{
    return userSetpointState;
}

/// @brief increment the setpoint chosen by the user Off -> Auto -> On -> Off
/// @return the new setpoint
ControlFanStates ControlFan::incrementUserSetpoint()
{
    switch (userSetpointState)
    {
    case CF_OFF:
        userSetpointState = CF_AUTO;
        break;
    case CF_AUTO:
        userSetpointState = CF_ON;
        break;
    case CF_ON:
        userSetpointState = CF_OFF;
        break;
    }
    resetFanRunTime(); // manuall change of mode -> set fan immediately
    return userSetpointState;
}

/// @brief Fill the logStr with infos: fan state, mode switch, onSec, offSec:
/// @param logCtrlStr char array length LOGCTRLSTR_LENGTH
void ControlFan::createLogChar(char *logCtrlStr)
{
    uint8_t fanState;
    switch (controlFanState)
    {
    case CF_ON:
        fanState = 1;
        break;
    default:
        fanState = 0;
        break;
    }

    snprintf(
        logCtrlStr, LOGCTRLSTR_LENGTH,
        "f%u;m%d;%u;%u", fanState, (uint8_t)userSetpointState, cntOnSeconds, cntOffSeconds);
}

/// @brief Reset the fan Run time. This is usefull, when the mode is switched manually to restart or stop the fan immediately.
void ControlFan::resetFanRunTime() {
        lastFanRunTime -= FanOFF_MS;
}