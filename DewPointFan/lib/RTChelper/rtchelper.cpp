#include <Arduino.h>

#include "rtchelper.h"
#include <Wire.h>
#include "buildTime.h"

/// @brief Init the RTChelper. Checks RTC and compiler date and uses the newest, valid time and
/// syncs the local esp time to it.
/// @return true if local esp time has been successfully synced
boolean RTCHelper::init() {
  Wire.begin();
  rtc.begin();

  // parse compiler date
  boolean compilerDateValid = getCompilerDate();
  boolean rtcValid = rtc.isValid();
  boolean someValidTime = false;
#ifdef DEBUGRTCHANDLINGINIT
  Serial.print("Compiler Rawdate: ");
  Serial.print(__TIME__);
  Serial.print(" ");
  Serial.println(__DATE__);
  printCompilerTime();
  Serial.print("RTC: ");
  Serial.println(rtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
  // printLocalTime();
  if (rtcValid) {
    Serial.println("RTC valid.");
  } else {
    Serial.println("RTC invalid!!!");
  }
#endif

  if (rtcValid && compilerDateValid) {
    if (isCompilerDateNewer()) {
#ifdef DEBUGRTCHANDLINGINIT
      Serial.println("Updating RTC to compiler date");
#endif
      rtc.setDateTime(compilerDate);
      someValidTime = true;
    } else {
#ifdef DEBUGRTCHANDLINGINIT
      Serial.println("RTC is up to date...");
#endif
      // the compilerDate is not newer, the rtc don't need updating.
      someValidTime = true;
    }
  } else if (rtcValid && !compilerDateValid) {
#ifdef DEBUGRTCHANDLINGINIT
    Serial.println("Compiler date invalid, using RTC");
#endif
    someValidTime = true;
  } else if (!rtcValid && compilerDateValid) {
#ifdef DEBUGRTCHANDLINGINIT
    Serial.println("RTC date invalid, using compiler");
#endif
    rtc.setDateTime(compilerDate);
    someValidTime = true;
  } else {
// nothing valid!?
#ifdef DEBUGRTCHANDLINGINIT
    Serial.println("No valid time found");
#endif
    someValidTime = false;
  }
  if (someValidTime) {
    // try to set the local esp32 time to this value
    if (rtc.syncToSystem()) {
      return true;
    }
  }
  return false;
}

/// @brief If debugging is enabled (DEBUGRTCHANDLING) rtc time and valid infos are printed
/// @return always true
boolean RTCHelper::loop() {
  unsigned long now = millis();
  if (now - lastRTCTime >= RTCwaitMS) {
#ifdef DEBUGRTCHANDLING
    Serial.print("RTC: ");
    Serial.println(rtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
    // printLocalTime();
    if (rtc.isValid()) {
      Serial.println("RTC valid.");
    } else {
      Serial.println("RTC invalid!!!");
    }
    Serial.println(fileName);
#endif
    lastRTCTime = now;
  }
  return true;
}

/*/// @brief prints the localTime to the serial console
void RTCHelper::printLocalTime()
{
  Serial.print("Local: ");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}*/

/// @brief prints the compiler time to the serial console
void RTCHelper::printCompilerTime() {
  char serialSend[18]; // declaration without initialization
  sprintf(serialSend, "%d-%d-%d %d:%d:%d", compilerDate.year, compilerDate.month, compilerDate.day,
          compilerDate.hour, compilerDate.minute, compilerDate.second);
  Serial.print("Compile: ");
  Serial.println(serialSend);
}

/// @brief get time information from compile date and time, that looks like   "Dec 24 2024" and
/// "13:09:03" and store it in compilerDate
/// @return true if decoding was successfull
boolean RTCHelper::getCompilerDate() {
  char s_month[5] = "xxx ";
  char s_date[12] = __DATE__;
  char s_time[9] = __TIME__;
  uint8_t day;
  uint16_t year;

  if (sscanf(s_date, "%s %d %d", s_month, &day, &year) != 3) {
    return false;
  }

  // month is not taken from this string but from buildTime.h preprocess makro

  compilerDate.day = day;
  compilerDate.month = BUILD_MONTH;
  compilerDate.year = year;

  uint8_t hour, min, sec;
  if (sscanf(s_time, "%d:%d:%d", &hour, &min, &sec) != 3)
    return false;
  compilerDate.hour = hour;
  compilerDate.minute = min;
  compilerDate.second = sec;
  return true;
}

/// @brief compare compilerDate to rtc.getDateTime(). Ignore minutes and seconds. Call rtc.isValid()
/// before!
/// @return true, if compilation date is newer than rtc date
boolean RTCHelper::isCompilerDateNewer() {
  RTC_Date now = rtc.getDateTime();
  if (compilerDate.year > now.year) {
    return true; // compiler year is the newer!
  } else if (compilerDate.year < now.year) {
    // compiler year is smaller i.e. older ->
    return false;
  } else { // year is eqal
    if (compilerDate.month > now.month) {
      return true;
    } else if (compilerDate.month < now.month) {
      return false;
    } else { // month is eqal
      if (compilerDate.day > now.day) {
        return true;
      } else if (compilerDate.day < now.day) {
        return false;
      } else { // Day is eqal
        if (compilerDate.hour > now.hour) {
          return true;
        } else if (compilerDate.hour < now.hour) {
          return false;
        } else { // hour is eqal
          if (compilerDate.minute > now.minute) {
            return true;
          } else if (compilerDate.minute < now.minute) {
            return false;
          } else {
            // minute is equal: regard as not newer
            return false;
          }
        }
      }
    }
  }
}

/// @brief Create filename
/// @return Returns true if filename has changed
boolean RTCHelper::createFileName() {
  RTC_Date now = rtc.getDateTime();
  sprintf(fileName, "/%04d-%02d.csv", now.year, now.month);
  if (now.month != oldMonth) {
    oldMonth = now.month;
    return true; // new fileName created
  } else {
    // month (and therefore whole filename) is the same
    return false;
  }
}

/// @brief Create timestamp strings for display
/// @param dateDispStr [DATE_LENGTH] DD.MM.YYYY
/// @param timeDispStr [TIME_LENGTH] hh:mm:ss
void RTCHelper::createTimeStampDisp(char *dateDispStr, char *timeDispStr) {
  RTC_Date now = rtc.getDateTime();
  // dateDispStr DD.MM.YYYY
  snprintf(dateDispStr, DATE_LENGTH, "%02d.%02d.%04d", now.day, now.month, now.year);
  // timeDispStr hh:mm:ss
  snprintf(timeDispStr, TIME_LENGTH, "%02d:%02d:%02d", now.hour, now.minute, now.second);
}

/// @brief Create timestamp strings for display. Short: without seconds
/// @param dateDispStr [DATE_LENGTH] DD.MM.YYYY
/// @param timeDispStr [TIME_LENGTH] hh:mm
void RTCHelper::createTimeStampDispShort(char *dateDispStr, char *timeDispStr) {
  RTC_Date now = rtc.getDateTime();
  // dateDispStr DD.MM.YYYY
  snprintf(dateDispStr, DATE_LENGTH, "%02d.%02d.%04d", now.day, now.month, now.year);
  // timeDispStr hh:mm:ss
  snprintf(timeDispStr, TIME_LENGTH, "%02d:%02d", now.hour, now.minute);
}

/// @brief Create timestamp strings for sd logging
/// @param logTimeStr  [TIMESTAMP_LENGTH] "YYYY-MM-DD hh:mm:ss" for sd data logging
void RTCHelper::createTimeStampLogging(char *logTimeStr) {
  RTC_Date now = rtc.getDateTime();
  //"YYYY-MM-DD hh:mm:ss"
  snprintf(logTimeStr, TIMESTAMP_LENGTH, "%04d-%02d-%02d %02d:%02d:%02d", now.year, now.month,
           now.day, now.hour, now.minute, now.second);
}

/// @brief get filename
void RTCHelper::getFileName(char *str) {
  memcpy(str, fileName, sizeof(fileName));
}