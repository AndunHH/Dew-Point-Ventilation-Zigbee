#include <Arduino.h>

#include "rtchelper.h"
#include "processSensorData.h"
#include "sdhelper.h"

#include "controlFan.h"
#include "zigbeeSwitchHelper.h"

#include "disphelper.h" // call after controlFan and after processSensorData
#include "Button.h"
#include "SerialTimeHelper.h"

#if RTC_FILENAMELENGTH != SD_FILENAMELENGTH
#error "Filenamelength in SD and RTC don't match"
#endif

// define SENSORPWRRESET
#ifdef SENSORPWRRESET
const int SensorPWR = D3; // Sensors are powered from this pin
#endif

ProcessSensorData processSensorData;

ControlFan controlFan;

RTCHelper rtcHelper;
SDHelper sdHelper(D2); // sd CS pin is on D2
DispHelper dispHelper;
ZigbeeSwitchHelper zigbeeSwitchHelper;

// NEU: Helfer für serielle Zeit-Kommandos (Z-Eingabe)
SerialTimeHelper serialTimeHelper(rtcHelper);

static uint8_t ledState = HIGH;

// Display-Timeout in Millisekunden (10 Minuten)
static const unsigned long DISPLAY_TIMEOUT_MS = 10UL * 60UL * 1000UL;

// Letzte "Nutzeraktivität" fürs Display (Start: setup, später: Buttondruck)
static unsigned long lastDisplayActivity = 0;

static void onButtonSingleClickCb(void *button_handle, void *usr_data) {
  Serial.println("Button single click");

  // Wenn das Display aus ist:
  //  -> nur Display einschalten und Timer zurücksetzen
  //  -> KEINE Änderung des Modus / Setpoints
  if (!dispHelper.isDisplayOn()) {
    dispHelper.setDisplayPower(true);
    lastDisplayActivity = millis();
    return;
  }

  // Wenn das Display an ist:
  //  -> Verhalten wie bisher (Setpoint erhöhen, Modus anzeigen)
  //  -> zusätzlich Timer zurücksetzen
  controlFan.incrementUserSetpoint();
  dispHelper.showSpecificDisplay(DISP_MODE);
  lastDisplayActivity = millis();
}

static void onLongPressUpEventCb(void *button_handle, void *usr_data) {
  Serial.println("Button long press up");
  dispHelper.showSpecificDisplay(DISP_ZIGBEERESET);
  zigbeeSwitchHelper.reset(); // blocks the systems and reboots
}

char versionStr[10] = "Ver 3.1.0";
char tmpFileName[RTC_FILENAMELENGTH] = "/2025-06.csv";
char logStr[TEMPLOG_LENGTH];
char logCtrlStr[LOGCTRLSTR_LENGTH];
char timestamp[TIMESTAMP_LENGTH] = "2025-06-25 20:01:10";
char dateDispStr[DATE_LENGTH] = "25.06.2025";
char timeDispStr[TIME_LENGTH] = "20:01:10";
char modeChar[2] = "m"; // active mode "0", "1", or "A" for auto

void setup() {
#ifdef SENSORPWRRESET
  pinMode(SensorPWR, OUTPUT);
  digitalWrite(SensorPWR, HIGH);
#endif
  Serial.begin(115200);
  delay(4000); // genügend Zeit, um den Serial Monitor zu öffnen

  rtcHelper.init();
  sdHelper.init();
  dispHelper.init(versionStr);

  // Display-Timer initialisieren und sicherstellen, dass das Display an ist
  dispHelper.setDisplayPower(true);
  lastDisplayActivity = millis();

  Button *btnD1 = new Button(GPIO_NUM_1, false);
  Button *btnBoot = new Button(GPIO_NUM_9, false);
  btnD1->attachSingleClickEventCb(&onButtonSingleClickCb, NULL);
  btnBoot->attachLongPressUpEventCb(&onLongPressUpEventCb, NULL);

  controlFan.init();

  pinMode(LED_BUILTIN, OUTPUT);
#ifdef SENSORPWRRESET
  digitalWrite(D3, LOW); // sensor power not enabled yet
#endif
  processSensorData.init();

  zigbeeSwitchHelper.init();
}

void loop() {
  static unsigned long lastdebugTime = 0;
  unsigned long now = millis();

  // NEU: serielle Kommandos (z.B. "Z" zum Zeit-Verbiegungs-Test)
  serialTimeHelper.handleSerial();

  // Wenn eine Zeit-Eingabe aktiv ist, überspringen wir den Rest der loop(),
  // damit die serielle Anzeige nicht von anderen Ausgaben zugemüllt wird.
  if (serialTimeHelper.isWaitingForTimeInput()) {
    // Optional: ein kleines yield(), damit WiFi/RTOS zufrieden sind
    yield();
    return;
  }

  // DHT Sensor loop
  processSensorData.loop();

  // Control Fan loop
  boolean turnFanOn, isVentUseFul;
  isVentUseFul = processSensorData.isVentilationUsefullStatus();
  turnFanOn = controlFan.loop(isVentUseFul);
  zigbeeSwitchHelper.setLightSetpoint(turnFanOn);

  // RTC loop
  rtcHelper.loop();
  if (rtcHelper.createFileName()) {
    Serial.println("newFileName detected");
    rtcHelper.getFileName(tmpFileName);
    sdHelper.setFileName(tmpFileName);
    sdHelper.writeCSVHeader();
    sdHelper.saveDataNow();
  }

  yield();

  // SD loop
  if (sdHelper.loop()) {
    rtcHelper.getFileName(tmpFileName);
    sdHelper.setFileName(tmpFileName);

    rtcHelper.createTimeStampLogging(timestamp);
    processSensorData.createLogChar(logStr);
    controlFan.createLogChar(logCtrlStr);

    sdHelper.writeData(timestamp, logStr, logCtrlStr);
  }

  yield();

  // Display
  switch (dispHelper.loop()) {
  case DISP_TIME:
    controlFan.getModeCharacter(modeChar);
    rtcHelper.createTimeStampDispShort(dateDispStr, timeDispStr);
    dispHelper.showTimeAndStatus(dateDispStr, timeDispStr, sdHelper.isSDinserted(),
                                 zigbeeSwitchHelper.isReady(), versionStr, modeChar, turnFanOn);
    break;
  case DISP_VERSION:
    dispHelper.showVersion(sdHelper.isSDinserted(), zigbeeSwitchHelper.isReady(), versionStr);
    break;
  case DISP_TEMP:
    controlFan.getModeCharacter(modeChar);
    dispHelper.showTemp(processSensorData.getAverageMeasurements(true),
                        processSensorData.getAverageMeasurements(false),
                        processSensorData.getVentilationUsefullStatus(), modeChar, turnFanOn);
    break;
  case DISP_MODE:
    dispHelper.showMode(controlFan.getUserSetpoint());
    break;
  case DISP_ZIGBEERESET:
    dispHelper.showZigBeeReset();
    break;
  default:
    break;
  };

  yield();

  // Zigbee Loop
  zigbeeSwitchHelper.loop();

  yield();

  // Debug-/Hilfsfunktionen alle 2s
  if (now - lastdebugTime >= 2000) {
    lastdebugTime = now;

#ifdef SENSORPWRRESET
    if (processSensorData.timeSinceAllDataWhereValid() > 30000) {
      Serial.println("Restarting sensors!");
      digitalWrite(SensorPWR, LOW);
      delay(10000);
    }
#endif
  }

  // NEU: Display-Timeout prüfen
  if (dispHelper.isDisplayOn() && (now - lastDisplayActivity >= DISPLAY_TIMEOUT_MS)) {
    dispHelper.setDisplayPower(false);
  }
}