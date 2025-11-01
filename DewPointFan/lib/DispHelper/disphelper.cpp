#include <Arduino.h>
#include "controlFan.h" // call first
#include "processSensorData.h"

#include "disphelper.h"

/// @brief initialize the display helper
/// @param versionStr Version to display
/// @return
boolean DispHelper::init(char *versionStr) {
#ifdef DEBUGDISPHANDLING
  Serial.print("Initializing disp...");
#endif
  u8x8.begin();
  u8x8.setFlipMode(0); // set number from 1 to 3, the screen word will rotary

  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 0);
  u8x8.print("TAUPUNKT LUEFTER");
  u8x8.setCursor(0, 3);
  u8x8.println("init ... ");
  u8x8.setCursor(0, 6);
  u8x8.println(versionStr);
  lastDispTime = millis();
  dispState = DISP_TIME;
  return false;
}

/// @brief DispHelper function that is called regularly in loop()
/// @return the page to show
DispHelperState DispHelper::loop() {
  unsigned long now = millis();
  DispHelperState showPage = DISP_NOTHING;
  switch (dispState) {
  case DISP_INIT:
    // init is shown as a regular screen
    if (now - lastDispTime >= DispWaitMS) {
      // enough off showing time, show next screen
      showPage = DISP_TEMP;
      lastDispTime = now;
      dispState = DISP_TEMP;
    }
    break;
  case DISP_SPECIFIC:
    // show the specificly set display state
    showPage = specificDispState;
    // reset the wish for the display state
    specificDispState = DISP_NOTHING;
    // Specifics are shown for
    if (now - lastDispTime >= DispWaitSpecificMS) {
#ifdef DEBUGDISPHANDLING
      Serial.println("Disp SPEC");
#endif
      showPage = DISP_TIME;
      lastDispTime = now;
      dispState = DISP_TIME;
    }
    break;
  case DISP_TIME:
    if (now - lastDispTime >= DispWaitMS) {
      // enough off showing time, show next screen
      showPage = DISP_TEMP;
      lastDispTime = now;
      dispState = DISP_TEMP;
    }
    break;
  case DISP_TEMP:
    if (now - lastDispTime >= DispWaitTemperatureMS) {
      showPage = DISP_TIME;
      lastDispTime = now;
      dispState = DISP_TIME;
    }
    break;
  case DISP_VERSION: // not entered anymore
    if (now - lastDispTime >= DispWaitMS) {
      showPage = DISP_MODE;
      lastDispTime = now;
      dispState = DISP_MODE;
    }
    break;
  case DISP_MODE: // only entered, when button is pressed
    if (now - lastDispTime >= DispWaitMS) {
      showPage = DISP_TIME;
      lastDispTime = now;
      dispState = DISP_TIME;
    }
    break;
  case DISP_ZIGBEERESET:
    if (now - lastDispTime >= DispWaitMS) {
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
void DispHelper::showSpecificDisplay(DispHelperState targetState) {
  dispState = DISP_SPECIFIC;
  specificDispState = targetState;
  lastDispTime = millis();
}

void DispHelper::showMode(ControlFanStates controlFanState) {
  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r); //
  u8x8.setCursor(0, 0);
  u8x8.println("Modus:");
  u8x8.println(" ");
  u8x8.setFont(u8x8_font_courB18_2x3_f); //

  switch (controlFanState) {
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
/// @param versionStr version number to show
void DispHelper::showVersion(boolean isSDpresent, bool isZigbeeReady, char *versionStr) {
  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r); //
  u8x8.setCursor(0, 0);
  u8x8.println(versionStr);
  // TODO: find a better way to create, track and show a version number over the whole repo.
  u8x8.println(" ");
  if (isSDpresent) {
    u8x8.println("sd card ready");
  } else {
    u8x8.println("No SD-card!");
  }
  u8x8.println(" ");
  if (isZigbeeReady) {
    u8x8.println("Zigbee ready");
  } else {
    u8x8.println("No Zigbee!");
  }
}

/// @brief Show the measurement values
/// @param inner AvgMeasurement data for inner sensor
/// @param outer AvgMeasurement data for outer sensor
/// @param ventUseFull VentilationUseFull Enum to show info, if ventilation is usefull
/// @param modeChar single character to show mode
/// @param isFanOn indicate if fan should actually run at the moment
void DispHelper::showTemp(AvgMeasurement inner, AvgMeasurement outer,
                          VentilationUseFull ventUseFull, char *modeChar, boolean isFanOn) {
  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 0);
  u8x8.print(modeChar); // print the character showing the mode
  u8x8.print("   MESSWERTE");
  printFanOnSymbol(isFanOn);
  u8x8.print(" Drin  | Aussen ");
  if (inner.validCnt != 8) {
    u8x8.setCursor(6, 1);
    u8x8.print(inner.validCnt);
  }
  if (outer.validCnt != 8) {
    u8x8.setCursor(15, 1);
    u8x8.print(outer.validCnt);
  }

#define LENGTHNUMBER 6
  char fixedPoint[LENGTHNUMBER] = "+12.2"; // placeholder to fill with characters
#define PRINTFORMAT "%4.1f"

  u8x8.setCursor(0, 2);
  u8x8.print("T:");
  snprintf(fixedPoint, LENGTHNUMBER, PRINTFORMAT, inner.temperature);
  u8x8.print(fixedPoint);
  u8x8.setCursor(7, 2);
  u8x8.print("| ");
  snprintf(fixedPoint, LENGTHNUMBER, PRINTFORMAT, outer.temperature);
  u8x8.print(fixedPoint);
  u8x8.setCursor(15, 2);
  u8x8.print("C");

  u8x8.setCursor(0, 3);
  u8x8.print("H:");
  snprintf(fixedPoint, LENGTHNUMBER, PRINTFORMAT, inner.humidity);
  u8x8.print(fixedPoint);
  u8x8.setCursor(7, 3);
  u8x8.print("| ");
  snprintf(fixedPoint, LENGTHNUMBER, PRINTFORMAT, outer.humidity);
  u8x8.print(fixedPoint);
  u8x8.setCursor(15, 3);
  u8x8.print("%");

  u8x8.setCursor(0, 4);
  u8x8.print("D:");
  snprintf(fixedPoint, LENGTHNUMBER, PRINTFORMAT, inner.dewPoint);
  u8x8.print(fixedPoint);
  u8x8.setCursor(7, 4);
  u8x8.print("| ");
  snprintf(fixedPoint, LENGTHNUMBER, PRINTFORMAT, outer.dewPoint);
  u8x8.print(fixedPoint);
  u8x8.setCursor(15, 4);
  u8x8.print("C");

  switch (ventUseFull) {
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
  case NODATAINDOOR:
    u8x8.setCursor(0, 6);
    u8x8.print("Innen-S. fehlt");
    break;
  case NODATAOUTDOOR:
    u8x8.setCursor(0, 6);
    u8x8.print("Aussen-S.fehlt");
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

/// @brief print a symbol, if the fan is actually running
/// @param isFanOn true, if fan should actually run
void DispHelper::printFanOnSymbol(boolean isFanOn) {
  u8x8.setCursor(15, 0);
  if (isFanOn) {
    u8x8.print("+");
  } else {
    u8x8.print("_");
  }
  u8x8.setCursor(0, 1);
}

void DispHelper::showTime(char *dateDispStr, char *timeDispStr) {
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
/// @param modeChar mode character to show
/// @param isFanOn indicate if fan should actually run at the moment
void DispHelper::showTimeAndStatus(char *dateDispStr, char *timeDispStr, boolean isSDpresent,
                                   bool isZigbeeReady, char *versionStr, char *modeChar,
                                   boolean isFanOn) {
  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 0);
  u8x8.print(modeChar); // print the character showing the mode
  u8x8.print("  ");
  u8x8.print(dateDispStr);
  printFanOnSymbol(isFanOn);
  u8x8.print("   ");
  u8x8.setFont(u8x8_font_inr21_2x4_f);
  u8x8.print(timeDispStr);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 5);
  if (isSDpresent) {
    u8x8.println("    SD: present");
  } else {
    u8x8.println("    SD: missing");
  }
  if (isZigbeeReady) {
    u8x8.println("Zigbee: ready");
  } else {
    u8x8.println("Zigbee: n/a");
  }
  u8x8.println(versionStr);
}

void DispHelper::showZigBeeReset() {
  u8x8.clear();
  u8x8.setFont(u8x8_font_courB18_2x3_f); //
  u8x8.setCursor(0, 0);
  u8x8.println("Zigbee");
  u8x8.println("Reset");
}