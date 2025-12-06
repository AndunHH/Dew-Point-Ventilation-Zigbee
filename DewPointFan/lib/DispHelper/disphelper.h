// how often shall display states be changed?
#define DispWaitMS 4000

// How long shall the temperature be shown?
#define DispWaitTemperatureMS 12000

// if a specific display is shown -> how long?
#define DispWaitSpecificMS 10000

// inactivity timeout for display (ms). Adjust as needed.
// Default: 10 minutes
#define DISPLAY_INACTIVITY_TIMEOUT_MS (10UL * 60UL * 1000UL)

// print debug?
#define DEBUGDISPHANDLING

#include <U8x8lib.h>
#include <Wire.h>

enum DispHelperState {
  DISP_NOTHING,
  DISP_SPECIFIC,
  DISP_INIT,
  DISP_TIME,
  DISP_TEMP,
  DISP_VERSION,
  DISP_MODE,
  DISP_ZIGBEERESET
};

/// @brief DispHelper class to handle the display. The regularly called loop() returns which screen
/// shall be drawn. The external main routine shall than call the corresponding showVersion(),
/// showMode(), showTemp(), showTime() etc. with data which is supplied by other functions.
class DispHelper {
public:
  boolean init(char *versionStr);

  DispHelperState loop();

  /// @brief Enable or disable the OLED display using the U8x8 power save feature.
  /// @param on true: display on, false: power save (display off)
  void setDisplayPower(bool on);

  /// @brief Reset the inactivity timer (user activity)
  void resetActivityTimer();

  /// @brief Returns the last set on/off state of the display.
  bool isDisplayOn() const {
    return displayOn;
  }

  void showVersion(boolean isSDpresent, bool isZigbeeReady, char *versionStr);
  void showMode(ControlFanStates controlFanState);

  void showTemp(AvgMeasurement inner, AvgMeasurement outer, VentilationUseFull ventUseFull,
                char *modeChar, boolean isFanOn);

  void printFanOnSymbol(boolean isFanOn);

  void showTime(char *dateDispStr, char *timeDispStr);

  void showTimeAndStatus(char *dateDispStr, char *timeDispStr, boolean isSDpresent,
                         bool isZigbeeReady, char *versionStr, char *modeChar, boolean isFanOn);

  void showZigBeeReset();

  void showSpecificDisplay(DispHelperState targetState);

  DispHelper()
      : dispState(DISP_INIT), specificDispState(DISP_NOTHING), lastDispTime(0),
        u8x8(/* clock=*/SCL, /* data=*/SDA,
             /* reset=*/U8X8_PIN_NONE), // OLEDs without Reset of the Display
        displayOn(true),                // Display startet eingeschaltet
        lastActivityTime(0)             // initialize activity timer
  {}

private:
  DispHelperState dispState;
  DispHelperState specificDispState;
  unsigned long lastDispTime;
  U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8;

  bool displayOn; // aktueller An/Aus-Status des Displays

  unsigned long lastActivityTime; // milliseconds of last user activity
};