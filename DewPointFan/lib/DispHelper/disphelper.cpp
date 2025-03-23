#include <Arduino.h>
#include "controlFan.h" // call first
#include "processSensorData.h"

#include "disphelper.h"

/// @brief initialize the display helper
/// @param versionStr Version to display
/// @return
boolean DispHelper::init(char *versionStr)
{
#ifdef DEBUGDISPHANDLING
    Serial.print("Initializing disp...");
#endif
    u8x8.begin();
    u8x8.setFlipMode(0); // set number from 1 to 3, the screen word will rotary

    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.println("Disp Init!");
    u8x8.println(" ");
    u8x8.println(versionStr);

    dispState = DISP_TIME;
    return false;
}

/// @brief DispHelper function that is called regularly in loop()
/// @return the page to show
DispHelperState DispHelper::loop()
{
    unsigned long now = millis();
    DispHelperState showPage = DISP_NOTHING;
    switch (dispState)
    {
    case DISP_INIT:
#ifdef DEBUGDISPHANDLING
        Serial.println("Disp INIT");
#endif
        init("error"); // init should be already called outside!
        lastDispTime = now;
        break;
    case DISP_SPECIFIC:
        // show the specificly set display state
        showPage = specificDispState;
        // reset the wish for the display state
        specificDispState = DISP_NOTHING;
        // Specifics are shown for
        if (now - lastDispTime >= DispWaitSpecificMS)
        {
#ifdef DEBUGDISPHANDLING
            Serial.println("Disp SPEC");
#endif
            showPage = DISP_TIME;
            lastDispTime = now;
            dispState = DISP_TIME;
        }
        break;
    case DISP_TIME:
        if (now - lastDispTime >= DispWaitMS)
        {
            // enough off showing time, show next screen
            showPage = DISP_TEMP;
            lastDispTime = now;
            dispState = DISP_TEMP;
        }
        break;
    case DISP_TEMP:
        if (now - lastDispTime >= DispWaitTemperatureMS)
        {
            showPage = DISP_TIME;
            lastDispTime = now;
            dispState = DISP_TIME;
        }
        break;
    case DISP_VERSION: // not entered anymore
        if (now - lastDispTime >= DispWaitMS)
        {
            showPage = DISP_MODE;
            lastDispTime = now;
            dispState = DISP_MODE;
        }
        break;
    case DISP_MODE: // only entered, when button is pressed
        if (now - lastDispTime >= DispWaitMS)
        {
            showPage = DISP_TIME;
            lastDispTime = now;
            dispState = DISP_TIME;
        }
        break;
    case DISP_ZIGBEERESET:
        if (now - lastDispTime >= DispWaitMS)
        {
            showPage = DISP_TIME;
            lastDispTime = now;
            dispState = DISP_TIME;
        }
        break;
    default:
        dispState = DISP_INIT;
        break;
    }
    return showPage;
} // end loop()

/// @brief Switch the display to the specified state and stay there
/// @param targetState state to switch to
void DispHelper::showSpecificDisplay(DispHelperState targetState)
{
    dispState = DISP_SPECIFIC;
    specificDispState = targetState;
    lastDispTime = millis();
}

void DispHelper::showMode(ControlFanStates controlFanState)
{
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r); //
    u8x8.setCursor(0, 0);
    u8x8.println("Modus:");
    u8x8.println(" ");
    u8x8.setFont(u8x8_font_courB18_2x3_f); //

    switch (controlFanState)
    {
    case CF_OFF:
        u8x8.println("OFF");
        break;
    case CF_AUTO:
        u8x8.println("AUTO");
        break;
    case CF_ON:
        u8x8.println("ON");
        break;
    }
}

/// @brief Print additional infos on the display
/// @param isSDpresent
/// @param isZigbeeReady
void DispHelper::showVersion(boolean isSDpresent, bool isZigbeeReady)
{
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r); //
    u8x8.setCursor(0, 0);
    u8x8.println("Version 1");
    // TODO: find a better way to create, track and show a version number over the whole repo.
    u8x8.println(" ");
    if (isSDpresent)
    {
        u8x8.println("sd card ready");
    }
    else
    {
        u8x8.println("No SD-card!");
    }
    u8x8.println(" ");
    if (isZigbeeReady)
    {
        u8x8.println("Zigbee ready");
    }
    else
    {
        u8x8.println("No Zigbee!");
    }
}

/// @brief Show the measurement values
/// @param inner AvgMeasurement data for inner sensor
/// @param outer AvgMeasurement data for outer sensor
/// @param ventUseFull VentilationUseFull Enum to show info, if ventilation is usefull
/// @param modeChar single character to show mode
void DispHelper::showTemp(AvgMeasurement inner, AvgMeasurement outer, VentilationUseFull ventUseFull, char* modeChar)
{
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print(modeChar); // print the character showing the mode
    u8x8.println("| MESSWERTE   ");
    u8x8.print(" Drin  | Aussen ");
    if (inner.validCnt != 8)
    {
        u8x8.setCursor(6, 1);
        u8x8.print(inner.validCnt);
    }
    if (outer.validCnt != 8)
    {
        u8x8.setCursor(15, 1);
        u8x8.print(outer.validCnt);
    }

    u8x8.setCursor(0, 2);
    u8x8.print("T:");
    u8x8.print(inner.temperature, 1);
    u8x8.setCursor(6, 2);
    u8x8.print(" | ");
    u8x8.print(outer.temperature, 1);
    u8x8.setCursor(15, 2);
    u8x8.print("C");

    u8x8.setCursor(0, 3);
    u8x8.print("H:");
    u8x8.print(inner.humidity, 1);
    u8x8.setCursor(6, 3);
    u8x8.print(" | ");
    u8x8.print(outer.humidity, 1);
    u8x8.setCursor(15, 3);
    u8x8.print("%");

    u8x8.setCursor(0, 4);
    u8x8.print("D:");
    u8x8.print(inner.dewPoint, 1);
    u8x8.setCursor(6, 4);
    u8x8.print(" | ");
    u8x8.print(outer.dewPoint, 1);
    u8x8.setCursor(15, 4);
    u8x8.print("C");

    switch (ventUseFull)
    {
    case USEFULL:
        u8x8.setCursor(0, 6);
        u8x8.print("Lueftung");
        u8x8.setCursor(0, 7);
        u8x8.print("  sinnvoll.");
        break;
    case NODATA:
        u8x8.setCursor(0, 6);
        u8x8.print("Keine Sensoren");
        break;
    case TOOCOLDINSIDE:
        u8x8.setCursor(0, 6);
        u8x8.print("Drin zu kalt");
        break;
    case TOOCOLDOUTSIDE:
        u8x8.setCursor(0, 6);
        u8x8.print("Draussen zu kalt");
        break;
    case INSIDEDRYENOUGH:
        u8x8.setCursor(0, 6);
        u8x8.print("Drinnen trocken");
        break;
    case OUTSIDENOTDRYENOUGH:
        u8x8.setCursor(0, 6);
        u8x8.print(" Draussen nicht");
        u8x8.setCursor(0, 7);
        u8x8.print("   trockener.");
        break;
    default:
        u8x8.setCursor(0, 6);
        u8x8.print("??");
        break;
    }
}

void DispHelper::showTime(char *dateDispStr, char *timeDispStr)
{
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.println("TIME");
    u8x8.println(dateDispStr);
    u8x8.println(" ");
    u8x8.setFont(u8x8_font_inr21_2x4_f); //
    u8x8.print(timeDispStr);
}

/// @brief Show Time and Status on the display
/// @param dateDispStr actual date
/// @param timeDispStr actual time
/// @param isSDpresent is the sd card present?
/// @param isZigbeeReady is zigbee ready?
/// @param versionStr version number to show
/// @param versionStr mode character to show
void DispHelper::showTimeAndStatus(char *dateDispStr, char *timeDispStr, boolean isSDpresent, bool isZigbeeReady, char *versionStr, char *modeChar)
{
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
    u8x8.print(modeChar);
    u8x8.print("| ");
    u8x8.println(dateDispStr);
    u8x8.setFont(u8x8_font_inr21_2x4_f); //
    u8x8.print("  ");
    u8x8.print(timeDispStr);
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 5);
    if (isSDpresent)
    {
        u8x8.println("    SD: present");
    }
    else
    {
        u8x8.println("    SD: missing");
    }
    if (isZigbeeReady)
    {
        u8x8.println("Zigbee: ready");
    }
    else
    {
        u8x8.println("Zigbee: n/a");
    }
    u8x8.println(versionStr);
}

void DispHelper::showZigBeeReset()
{
    u8x8.clear();
    u8x8.setFont(u8x8_font_courB18_2x3_f); //
    u8x8.setCursor(0, 0);
    u8x8.println("Zigbee");
    u8x8.println("Reset");
}