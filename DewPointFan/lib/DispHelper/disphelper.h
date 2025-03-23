// how often shall display states be changed?
#define DispWaitMS 4000

// How long shall the temperature be shown?
#define DispWaitTemperatureMS 10000

// if a specific display is shown -> how long?
#define DispWaitSpecificMS 5000

// print debug?
#define DEBUGDISPHANDLING

#include <U8x8lib.h>
#include <Wire.h>

enum DispHelperState
{
    DISP_NOTHING,
    DISP_SPECIFIC,
    DISP_INIT,
    DISP_TIME,
    DISP_TEMP,
    DISP_VERSION,
    DISP_MODE,
    DISP_ZIGBEERESET
};

/// @brief DispHelper class to handle the display. The regularly called loop() returns which screen shall be drawn. The external main routine shall than call the corresponding showVersion(), showMode(), showTemp(), showTime() etc. with data which is supplied by other functions.
class DispHelper
{
public:
    boolean init(char *versionStr);

    DispHelperState loop();

    void showVersion(boolean isSDpresent, bool isZigbeeReady);
    void showMode(ControlFanStates controlFanState);

    void showTemp(AvgMeasurement inner, AvgMeasurement outer, VentilationUseFull ventUseFull, char* modeChar);

    void showTime(char *dateDispStr, char *timeDispStr);

    void showTimeAndStatus(char *dateDispStr, char *timeDispStr, boolean isSDpresent, bool isZigbeeReady, char *versionStr, char *modeChar);

    void showZigBeeReset();

    void showSpecificDisplay(DispHelperState targetState);

    DispHelper() : dispState(DISP_INIT),
                   lastDispTime(0),
                   u8x8(/* clock=*/SCL, /* data=*/SDA, /* reset=*/U8X8_PIN_NONE) // OLEDs without Reset of the Display
    {
    }

private:
    DispHelperState dispState;
    DispHelperState specificDispState;
    unsigned long lastDispTime;
    U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8;
};