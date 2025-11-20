#pragma once

#include <Arduino.h>
#include "rtchelper.h"

/// @brief Hilfsklasse für serielle Kommandos zur Zeiteinstellung.
/// Kommando:
///   Z  -> Zeit setzen (dd.mm.yyyy hh:mm), inkl. Sommer-/Winterzeit-Test
class SerialTimeHelper {
public:
  SerialTimeHelper(RTCHelper &rtc) : rtcHelper(rtc), waitForTimeInput(false), buffer("") {}

  /// @brief In loop() aufrufen, um serielle Kommandos zu verarbeiten.
  void handleSerial();

  /// @brief true, wenn gerade auf eine Datum/Zeit-Eingabe gewartet wird.
  bool isWaitingForTimeInput() const {
    return waitForTimeInput;
  }

private:
  RTCHelper &rtcHelper;
  bool waitForTimeInput;
  String buffer;

  bool parseDateTimeLine(const String &line, uint16_t &year, uint8_t &month, uint8_t &day,
                         uint8_t &hour, uint8_t &minute);
};