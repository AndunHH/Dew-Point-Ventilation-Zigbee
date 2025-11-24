// SerielTimeHelper.cpp

#include "SerialTimeHelper.h"

/// @brief Parse a date given in dd.mm.yy hh:mm format (e.g. "24.12.2025 20:56")
/// @param &line: String to be parsed
/// @param &year: resulting year
/// @param &month: resulting month
/// @param &day: resulting day
/// @param &hour: resulting hour
/// @param &minute: resulting minute
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

        // =======================
        //   KOMMANDO-MODUS
        // =======================
        if (!waitForTimeInput) {

          // Immer zuerst IST-Zeit ausgeben
          rtcHelper.printCurrentLocalShortWithDST();

          // Kommando auswerten
          if (buffer.equalsIgnoreCase("Z")) {
            waitForTimeInput = true;
            Serial.println("ZEIT-EINGABE: Bitte 'dd.mm.yyyy hh:mm' eingeben und Enter druecken.");
            Serial.println("Zum Abbrechen: X eingeben und Enter druecken.");
            Serial.println("Hinweis: Bitte immer die lokale Uhrzeit eingeben - Sommer-/Winterzeit "
                           "wird automatisch erkannt.");
          } else {
            Serial.print("Unbekanntes Kommando: ");
            Serial.println(buffer);
            Serial.println("Verfuegbare Kommandos:");
            Serial.println("  Z  -> Zeit setzen (Test Sommer/Winterzeit)");
          }

          // =======================
          //   ZEIT-EINGABE-MODUS
          // =======================
        } else {

          // Abbruch mit X
          if (buffer.equalsIgnoreCase("X")) {
            Serial.println("ZEIT-EINGABE abgebrochen.");
            waitForTimeInput = false;
            buffer = "";
            return;
          }

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

            // Nach dem Setzen nochmal die aktuelle Zeit kurz anzeigen
            rtcHelper.printCurrentLocalShortWithDST();

            waitForTimeInput = false;
          } else {
            Serial.println("Formatfehler. Erwartet wird: dd.mm.yyyy hh:mm");
            Serial.println("Beispiel: 30.03.2025 02:00");
            Serial.println("Oder: X zum Abbrechen");
          }
        }
      }

      // Zeilenpuffer löschen
      buffer = "";
    } else {
      buffer += c;
    }
  }
}