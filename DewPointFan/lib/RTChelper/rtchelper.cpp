#include <Arduino.h>

#include "rtchelper.h"
#include <Wire.h>
#include "buildTime.h"

// ===== Hilfsfunktionen für Sommer-/Winterzeit (EU, Europe/Berlin) =====

static bool isLeapYear(uint16_t year) {
  return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

static uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t dim[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year))
    return 29;
  return dim[month - 1];
}

// 0 = Sonntag, 1 = Montag, ... 6 = Samstag
static uint8_t dayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  int y = year;
  int m = month;
  if (m < 3) {
    m += 12;
    y -= 1;
  }
  int K = y % 100;
  int J = y / 100;
  int h = (day + (13 * (m + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
  int dow = (h + 6) % 7; // 0 = Sonntag
  return dow;
}

// letzte Sonntag im bestimmten Monat (3=März, 10=Oktober)
static uint8_t lastSunday(uint16_t year, uint8_t month) {
  uint8_t d = daysInMonth(year, month);
  uint8_t dow = dayOfWeek(year, month, d); // 0=So ... 6=Sa
  return d - dow;                          // letzter Sonntag
}

// Gilt Sommerzeit (CEST) für diese lokale *Normalzeit* (Basiszeit CET)?
static bool isDST_Europe_CET(uint16_t year, uint8_t month, uint8_t day, uint8_t hour) {
  // Jan, Feb, Nov, Dez -> immer Normalzeit
  if (month < 3 || month > 10)
    return false;

  // Apr–Sep -> immer Sommerzeit
  if (month > 3 && month < 10)
    return true;

  // März / Oktober -> rund um die Umstellung prüfen
  uint8_t ls = lastSunday(year, month);

  if (month == 3) {
    // Umstellung: letzter Sonntag im März, 2:00 (von 2:00 auf 3:00)
    if (day > ls)
      return true;
    if (day < ls)
      return false;
    // Tag der Umstellung:
    return (hour >= 2);
  } else { // month == 10
    // Umstellung: letzter Sonntag im Oktober, 3:00 (von 3:00 auf 2:00 zurück)
    if (day < ls)
      return true;
    if (day > ls)
      return false;
    // Tag der Umstellung:
    return (hour < 3);
  }
}

// Hilfsfunktionen: eine Stunde zu einem RTC_Date addieren / abziehen
static void addOneHour(RTC_Date &dt) {
  dt.hour++;
  if (dt.hour >= 24) {
    dt.hour = 0;
    dt.day++;
    if (dt.day > daysInMonth(dt.year, dt.month)) {
      dt.day = 1;
      dt.month++;
      if (dt.month > 12) {
        dt.month = 1;
        dt.year++;
      }
    }
  }
}

static void subOneHour(RTC_Date &dt) {
  if (dt.hour > 0) {
    dt.hour--;
  } else {
    dt.hour = 23;
    if (dt.day > 1) {
      dt.day--;
    } else {
      if (dt.month > 1) {
        dt.month--;
      } else {
        dt.month = 12;
        dt.year--;
      }
      dt.day = daysInMonth(dt.year, dt.month);
    }
  }
}

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
      // Compilerzeit ist lokale PC-Zeit (inkl. Sommerzeit) -> als Basiszeit in RTC schreiben
      setFromLocalDate(compilerDate);
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
    // Compilerzeit ist lokale PC-Zeit (inkl. evtl. Sommerzeit)
    setFromLocalDate(compilerDate);
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
  char serialSend[22]; // etwas größer, um Überlauf zu vermeiden
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
  RTC_Date now = getLocalDate(); // lokale Zeit verwenden
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
  RTC_Date now = getLocalDate(); // lokale Zeit
  // dateDispStr DD.MM.YYYY
  snprintf(dateDispStr, DATE_LENGTH, "%02d.%02d.%04d", now.day, now.month, now.year);
  // timeDispStr hh:mm:ss
  snprintf(timeDispStr, TIME_LENGTH, "%02d:%02d:%02d", now.hour, now.minute, now.second);
}

/// @brief Create timestamp strings for display. Short: without seconds
/// @param dateDispStr [DATE_LENGTH] DD.MM.YYYY
/// @param timeDispStr [TIME_LENGTH] hh:mm
void RTCHelper::createTimeStampDispShort(char *dateDispStr, char *timeDispStr) {
  RTC_Date now = getLocalDate(); // lokale Zeit
  // dateDispStr DD.MM.YYYY
  snprintf(dateDispStr, DATE_LENGTH, "%02d.%02d.%04d", now.day, now.month, now.year);
  // timeDispStr hh:mm:ss
  snprintf(timeDispStr, TIME_LENGTH, "%02d:%02d", now.hour, now.minute);
}

/// @brief Create timestamp strings for sd logging
/// @param logTimeStr  [TIMESTAMP_LENGTH] "YYYY-MM-DD hh:mm:ss" for sd data logging
void RTCHelper::createTimeStampLogging(char *logTimeStr) {
  RTC_Date now = getLocalDate(); // lokale Zeit
  //"YYYY-MM-DD hh:mm:ss"
  snprintf(logTimeStr, TIMESTAMP_LENGTH, "%04d-%02d-%02d %02d:%02d:%02d", now.year, now.month,
           now.day, now.hour, now.minute, now.second);
}

/// @brief get filename
void RTCHelper::getFileName(char *str) {
  memcpy(str, fileName, sizeof(fileName));
}

/// RTC aus lokaler Zeit (inkl. Sommerzeit) setzen:
/// lokale Zeit -> ggf. 1h abziehen -> Basiszeit (CET) in RTC
void RTCHelper::setFromLocalDate(const RTC_Date &local) {
  RTC_Date base = local; // Kopie

  if (isDST_Europe_CET(base.year, base.month, base.day, base.hour)) {
    subOneHour(base);
  }

  rtc.setDateTime(base);
}

/// lokale Zeit (mit Sommer-/Winterzeit) aus RTC holen
RTC_Date RTCHelper::getLocalDate() {
  RTC_Date base = rtc.getDateTime(); // Basiszeit
  if (isDST_Europe_CET(base.year, base.month, base.day, base.hour)) {
    addOneHour(base);
  }
  return base;
}

/// Debug-Ausgabe: Basiszeit vs. lokale Zeit
void RTCHelper::debugPrintTimes() {
  RTC_Date base = rtc.getDateTime();
  RTC_Date local = getLocalDate();
  bool dst = isDST_Europe_CET(base.year, base.month, base.day, base.hour);

  Serial.println("==== RTC Debug ====");
  Serial.print("RAW (Basis/CET): ");
  char buf[32];
  snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d:%02d", base.day, base.month, base.year,
           base.hour, base.minute, base.second);
  Serial.println(buf);

  Serial.print("Local (mit DST): ");
  snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d:%02d", local.day, local.month, local.year,
           local.hour, local.minute, local.second);
  Serial.println(buf);

  Serial.print("DST flag        : ");
  Serial.println(dst ? "1 (Sommerzeit)" : "0 (Normalzeit)");
  Serial.println("===================");
}