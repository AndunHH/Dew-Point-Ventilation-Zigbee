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

    // Handle backspace (BS 0x08) and DEL (0x7F)
    if (c == '\b' || c == 127) {
      if (buffer.length() > 0) {
        buffer.remove(buffer.length() - 1);
        // erase last character on terminal: backspace, space, backspace
        Serial.print("\b \b");
      }
      continue;
    }

    // If newline or carriage return -> process the accumulated line
    if (c == '\r' || c == '\n') {
      // Echo newline for user feedback (keeps prompt tidy)
      Serial.println();

      String line = buffer;
      line.trim();

      if (line.length() > 0) {

        // =======================
        //   COMMAND MODE
        // =======================
        if (!waitForTimeInput) {

          // Print current local time first
          rtcHelper.printCurrentLocalShortWithDST();

          // Evaluate command
          if (line.equalsIgnoreCase("Z")) {
            waitForTimeInput = true;
            Serial.println("ZEIT-EINGABE: Bitte 'dd.mm.yyyy hh:mm' eingeben und Enter druecken.");
            Serial.println("Zum Abbrechen: X eingeben und Enter druecken.");
            Serial.println("Hinweis: Bitte immer die lokale Uhrzeit eingeben - Sommer-/Winterzeit "
                           "wird automatisch erkannt.");
          } else {
            Serial.print("Unbekanntes Kommando: ");
            Serial.println(line);
            Serial.println("Verfuegbare Kommandos:");
            Serial.println("  Z  -> Zeit setzen (Test Sommer/Winterzeit)");
          }

          // =======================
          //   TIME-INPUT MODE
          // =======================
        } else { // waitForTimeInput == true

          // Cancel with X
          if (line.equalsIgnoreCase("X")) {
            Serial.println("ZEIT-EINGABE abgebrochen.");
            waitForTimeInput = false;
            buffer = "";
            continue;
          }

          // We expect a datetime line now
          uint16_t y;
          uint8_t m, d, h, mi;
          if (parseDateTimeLine(line, y, m, d, h, mi)) {
            RTC_Date local;
            local.year = y;
            local.month = m;
            local.day = d;
            local.hour = h;
            local.minute = mi;
            local.second = 0;

            rtcHelper.setFromLocalDate(local);

            // Show current time briefly after setting
            rtcHelper.printCurrentLocalShortWithDST();

            waitForTimeInput = false;
          } else {
            Serial.println("Formatfehler. Erwartet wird: dd.mm.yyyy hh:mm");
            Serial.println("Beispiel: 30.03.2025 02:00");
            Serial.println("Oder: X zum Abbrechen");
          }
        }
      }

      // Clear line buffer after processing
      buffer = "";
      continue;
    }

    // Printable characters: append to buffer and echo to serial so it behaves like a CLI
    if (c >= 32 && c <= 126) {
      buffer += c;
      Serial.print(c); // echo input character
    }
    // ignore other control characters
  }
}