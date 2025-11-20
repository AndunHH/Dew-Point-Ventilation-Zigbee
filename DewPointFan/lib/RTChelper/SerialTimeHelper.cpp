#include "SerialTimeHelper.h"

bool SerialTimeHelper::parseDateTimeLine(const String &line, uint16_t &year, uint8_t &month,
                                         uint8_t &day, uint8_t &hour, uint8_t &minute) {
  int d, m, y, h, mi;
  if (sscanf(line.c_str(), "%d.%d.%d %d:%d", &d, &m, &y, &h, &mi) == 5) {
    day = d;
    month = m;
    year = y;
    hour = h;
    minute = mi;
    return true;
  }
  return false;
}

void SerialTimeHelper::handleSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      buffer.trim();
      if (buffer.length() > 0) {
        if (!waitForTimeInput) {
          // Kommandomodus
          if (buffer.equalsIgnoreCase("Z")) {
            waitForTimeInput = true;
            Serial.println("ZEIT-EINGABE: Bitte 'dd.mm.yyyy hh:mm' eingeben und Enter druecken.");
          } else {
            Serial.print("Unbekanntes Kommando: ");
            Serial.println(buffer);
            Serial.println("Verfuegbare Kommandos:");
            Serial.println("  Z  -> Zeit setzen (Test Sommer/Winterzeit)");
          }
        } else {
          // Wir erwarten jetzt eine Zeitzeile
          uint16_t y;
          uint8_t m, d, h, mi;
          if (parseDateTimeLine(buffer, y, m, d, h, mi)) {
            RTC_Date local;
            local.year = y;
            local.month = m;
            local.day = d;
            local.hour = h;
            local.minute = mi;
            local.second = 0;

            rtcHelper.setFromLocalDate(local);
            rtcHelper.debugPrintTimes();
            waitForTimeInput = false;
          } else {
            Serial.println("Formatfehler. Erwartet wird: dd.mm.yyyy hh:mm");
            Serial.println("Beispiel: 30.03.2025 02:00");
          }
        }
      }
      buffer = "";
    } else {
      buffer += c;
    }
  }
}