#include <Arduino.h>

#include "rtchelper.h"
#include "processSensorData.h"
#include "sdhelper.h"

#include "controlFan.h"
#include "zigbeeSwitchHelper.h"

#include "disphelper.h" // call after controlFan and after processSensorData
#include "Button.h"

#if RTC_FILENAMELENGTH != SD_FILENAMELENGTH
#error "Filenamelength in SD and RTC don't match"
#endif

ProcessSensorData processSensorData;

ControlFan controlFan;

RTCHelper rtcHelper;
SDHelper sdHelper(D2); // sd CS pin is on D2
DispHelper dispHelper;
ZigbeeSwitchHelper zigbeeSwitchHelper;

static uint8_t ledState = HIGH;

/// @brief Call back function for the external mode button click
/// @param button_handle
/// @param usr_data
static void onButtonSingleClickCb(void *button_handle, void *usr_data)
{
  Serial.println("Button single click");
  controlFan.incrementUserSetpoint();
  dispHelper.showSpecificDisplay(DISP_MODE); // switch the display to show the mode in next iteration
}

/// @brief Call back function for the internal "boot" button, directly on the esp32c6 module -> long press release to initiate a factory reset. Then, after the reset you have 180s to connect a new plug
/// @param button_handle
/// @param usr_data
static void onLongPressUpEventCb(void *button_handle, void *usr_data)
{
  Serial.println("Button long press up");
  dispHelper.showSpecificDisplay(DISP_ZIGBEERESET);
  zigbeeSwitchHelper.reset(); // blocks the systems and reboots
}

void setup()
{
  Serial.begin(115200);
  delay(4000); // Wait four seconds, to have enough time to start the serial monitor to see the setup
  rtcHelper.init();
  sdHelper.init();
  dispHelper.init();

  // initializing a button
  Button *btnD1 = new Button(GPIO_NUM_1, false);
  Button *btnBoot = new Button(GPIO_NUM_9, false);
  // GPIO_NUM_1 = D1 ButtonD1 on XIAO expansion
  btnD1->attachSingleClickEventCb(&onButtonSingleClickCb, NULL);
  btnBoot->attachLongPressUpEventCb(&onLongPressUpEventCb, NULL);

  controlFan.init();

  pinMode(LED_BUILTIN, OUTPUT); // builtin LED
  processSensorData.init();

  zigbeeSwitchHelper.init();
}

char tmpFileName[RTC_FILENAMELENGTH] = "/2024-12.csv";
char logStr[TEMPLOG_LENGTH];
char logCtrlStr[LOGCTRLSTR_LENGTH];
char timestamp[TIMESTAMP_LENGTH] = "2024-12-31 10:10:10";
char dateDispStr[DATE_LENGTH] = "31.12.2024";
char timeDispStr[TIME_LENGTH] = "10:10:10";

/* TODO List:
- Reset the whole controller once per day?
- showing in the display, if no plug is bound at all
*/
void loop()
{
  static unsigned long lastdebugTime = 0;
  unsigned long now = millis();

  // DHT Sensor loop
  // Get temperature event and print its value.
  processSensorData.loop();
  // processSensorData.printBuffer();

  // Control Fan loop
  boolean turnFanOn, isVentUseFul;

  isVentUseFul = processSensorData.isVentilationUsefullStatus();
  turnFanOn = controlFan.loop(isVentUseFul);
  zigbeeSwitchHelper.setLightSetpoint(turnFanOn);

  // RTC loop

  rtcHelper.loop();
  if (rtcHelper.createFileName())
  {
    Serial.println("newFileName detected");
    rtcHelper.getFileName(tmpFileName);
    sdHelper.setFileName(tmpFileName);
    sdHelper.writeCSVHeader();
    sdHelper.saveDataNow();
  }

  yield();

  // SD loop
  if (sdHelper.loop()) // check if it is time to write data to the sd card
  {
    // data should be updated and written!
    rtcHelper.getFileName(tmpFileName);
    sdHelper.setFileName(tmpFileName);

    rtcHelper.createTimeStampLogging(timestamp);
    processSensorData.createLogChar(logStr);
    controlFan.createLogChar(logCtrlStr);

    sdHelper.writeData(timestamp, logStr, logCtrlStr);
  }

  yield();

  // Check if a new screens needs to be drawn to the display
  switch (dispHelper.loop())
  {
  case DISP_TIME:
    rtcHelper.createTimeStampDisp(dateDispStr, timeDispStr);
    dispHelper.showTime(dateDispStr, timeDispStr);
    break;
  case DISP_VERSION:
    dispHelper.showVersion(sdHelper.isSDinserted(), zigbeeSwitchHelper.isReady());
    break;
  case DISP_TEMP:
    dispHelper.showTemp(processSensorData.getAverageMeasurements(true), processSensorData.getAverageMeasurements(false), processSensorData.getVentilationUsefullStatus());
    break;
  case DISP_MODE:
    dispHelper.showMode(controlFan.getUserSetpoint());
    break;
  case DISP_ZIGBEERESET:
    dispHelper.showZigBeeReset();
    break;
  default:
    // don't change display
    break;
  };

  yield();

  // Zigbee Loop
  zigbeeSwitchHelper.loop();

  yield();
  if (now - lastdebugTime >= 1000)
  {
    lastdebugTime = now;

    /*Serial.print(dateDispStr);
    Serial.print(" ");
    Serial.println(timeDispStr);
    processSensorData.printStatus();

    Serial.println(logCtrlStr);
*/

    // let the LED blink...
    ledState == HIGH ? ledState = LOW : ledState = HIGH;
    digitalWrite(LED_BUILTIN, ledState);
  }
}
