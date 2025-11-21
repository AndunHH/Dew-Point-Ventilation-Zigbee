// rtchelper.h

#pragma once

// how often shall rtc loop be handled
#define RTCwaitMS 1100

// "/2024-12.csv" + null
#define RTC_FILENAMELENGTH 13

#define TIMESTAMP_LENGTH 20
#define DATE_LENGTH 11
#define TIME_LENGTH 9

// print debug?
// define DEBUGRTCHANDLING
#define DEBUGRTCHANDLINGINIT

#include "pcf8563.h"

/// @brief RTCHelper class to handle the rtc.
class RTCHelper {
public:
  RTCHelper() : oldMonth(0), fileName("/YYYY-MM.csv") {};

  boolean init();

  boolean loop();

  void printCompilerTime();
  boolean getCompilerDate();
  boolean createFileName();
  void createTimeStampDisp(char *dateDispStr, char *timeDispStr);
  void createTimeStampDispShort(char *dateDispStr, char *timeDispStr);
  void createTimeStampLogging(char *logTimeStr);
  void getFileName(char *str);

  // lokale Zeit (inkl. Sommerzeit) in die RTC schreiben
  void setFromLocalDate(const RTC_Date &local);

  // Debug-Ausgabe von Basiszeit (RTC) + lokaler Zeit
  void debugPrintTimes();

  // Aktuelle lokale Zeit kurz ausgeben: "Aktuelle Zeit: tt.mm.yyyy hh:mm (Normalzeit/Sommerzeit)"
  void printCurrentLocalShortWithDST();

private:
  PCF8563_Class rtc;

  RTC_Date compilerDate; // time of compilation
  boolean isCompilerDateNewer();
  unsigned long lastRTCTime;         // used for loop()
  char fileName[RTC_FILENAMELENGTH]; // file name for the datalogger
  uint8_t oldMonth = 0; // used to remember which month is in filename, see createFileName()

  // lokale Zeit (mit Sommer-/Winterzeit) aus RTC holen
  RTC_Date getLocalDate();
};