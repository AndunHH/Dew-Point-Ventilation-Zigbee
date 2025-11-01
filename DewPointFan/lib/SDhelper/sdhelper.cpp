#include <Arduino.h>

#include <SPI.h>
#include <SD.h>
#include "FS.h"

#include "sdhelper.h"

/// @brief return wifi credentials if found.
/// @param ssid pointer to array for ssid
/// @param pw pointer to array for pw
/// @return true if valid data found
boolean SDHelper::getWifiCredentials(char *ssid, char *pw) {
  if (credentialsValid) {
    memcpy(ssid, _ssid, WIFICREDENTIALLENGTH);
    memcpy(pw, _pw, WIFICREDENTIALLENGTH);
    return true;
  } else {
    return false;
  }
}

/// @brief get WiFi credentials from SD card file WIFIFILENAME. SD must be ready to read.
/// @param ssid pointer to array for ssid
/// @param pw pointer to array for pw
/// @return true if valid data found
boolean SDHelper::getWifiCredentialsFromSD() {
  boolean returnVal = false;
  File myFile;
  if (SD.begin(csPin)) {
    myFile = SD.open(WIFIFILENAME);
    if (myFile) {
      Serial.println("opened wifi file");

      // read from the file until there's nothing else in it:
      while (myFile.available()) {
        int l = myFile.readBytesUntil('\n', _ssid, WIFICREDENTIALLENGTH);
        if (l > 0 && _ssid[l - 1] == '\r') {
          l--;
        }
        _ssid[l] = 0;

        l = myFile.readBytesUntil('\n', _pw, WIFICREDENTIALLENGTH);
        if (l > 0 && _pw[l - 1] == '\r') {
          l--;
        }
        _pw[l] = 0;
        Serial.println(_ssid);
        Serial.println(_pw);
        credentialsValid = true;
        returnVal = true;
      }
      // close the file:
      myFile.close();
      SD.end();
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening wifi.txt");
      credentialsValid = false;
    }
  } else {
    // SD begin failed ... go to no sd?
  }

  return returnVal;
}

/// @brief initialize the SD helper, i.e. open the SD card and follow to NOSD or GETCREDENTIALS
/// @return
boolean SDHelper::init() {
#ifdef DEBUGSDHANDLING
  Serial.print("Initializing SD card...");
#endif
  pinMode(csPin, OUTPUT); // Modify the pins here to fit the CS pins of the SD card you are using.
  if (checkSDPresence())  // checkSDPresence sets sdState to NOSD
  {
    sdState = GETCREDENTIALS;
    return true;
  }
  return false;
}

/// @brief SDhelper function that is called regularly in loop()
/// @return true if data shall be written to SD card with writeData()
boolean SDHelper::loop() {
  unsigned long now = millis();
  boolean writeDataNow = false;
  switch (sdState) {
  case SDINIT:
#ifdef DEBUGSDHANDLING
    Serial.println("SD INIT");
#endif
    init();
    lastSDTime = now;
    lastSDSaveTime = now;
    break;
  case GETCREDENTIALS:
#ifdef DEBUGSDHANDLING
    Serial.println("SD GETCREDENTIALS");
#endif
    getWifiCredentialsFromSD();
    sdState = SDREADY;
    break;

  case SDREADY:
    if (now - lastSDTime >= SDwaitMS) {
      if (!checkSDPresence()) {
#ifdef DEBUGSDHANDLING
        Serial.print("leaving SD READY: ");
#endif
        sdState = NOSD;
        lastSDSaveTime = now;
        break; // don't evaluate READY anymore
      }
#ifdef DEBUGSDHANDLING
      Serial.print("SD READY: ");
      // Serial.print("now: "); Serial.println(now);
      Serial.println(fileName);
#endif
      // check lastSDSaveTime for 10 min to save
      if (now - lastSDSaveTime >= SD_SAVE_INTERVALL_MS) {
#ifdef DEBUGSDHANDLING
        Serial.println("save data!");
        // Serial.print("last: "); Serial.println(lastSDSaveTime);
#endif
        writeDataNow = true;
        lastSDSaveTime = now;
      }

      lastSDTime = now;
      sdState = SDREADY;
    }
    break;

  case NOSD:
    // wait for NOSDwaitMS and go to init again
    if (now - lastSDTime >= NOSDwaitMS) {
#ifdef DEBUGSDHANDLING
      Serial.println("NOSD");
#endif
      if (checkSDPresence()) {
        // sd card found!
        sdState = SDINIT;
      }
      lastSDTime = now;
    }
    break;
  default:
    sdState = SDINIT;
    break;
  }
  return writeDataNow;
} // end loop()

/// @brief resets the save data counter, so the next loop() will save data
void SDHelper::saveDataNow() {
  lastSDSaveTime = (millis() - SD_SAVE_INTERVALL_MS) - 1;
}

/// @brief set the fileName, which is used by the data logger
/// @param fn "YYYY-MM.csv"
void SDHelper::setFileName(char *fn) {
  memcpy(fileName, fn, 12);
}

/// @brief Open fileName and write the header into it
/// @return
boolean SDHelper::writeCSVHeader() {
  if (SD.begin(csPin)) {
    File logFile = SD.open(fileName, FILE_APPEND);
    logFile.println(CSV_HEADER);
    logFile.close();
#ifdef DEBUGSDHANDLING
    Serial.println("Wrote header sd.");
#endif
    SD.end();
    return true;
  } else {
    return false;
  }
  return true;
}

/// @brief This functions writes the three strings to the sd card, seperated by ";".
/// @param dateStr date info
/// @param tempStr temperature infos
/// @param controlStr control infors
/// @return true if written successfull
boolean SDHelper::writeData(char *dateStr, char *tempStr, char *controlStr) {
#ifdef DEBUGSDHANDLING
  Serial.println("writing data!");
#endif
  if (SD.begin(csPin)) {
    File logFile = SD.open(fileName, FILE_APPEND);
    if (logFile) {
      logFile.print(dateStr);
      logFile.print(";");
      logFile.print(tempStr);
      logFile.print(";");
      logFile.println(controlStr);
      logFile.close();

#ifdef DEBUGSDHANDLING
      Serial.println("wrote:: ");
      Serial.print(dateStr);
      Serial.print(";");
      Serial.print(tempStr);
      Serial.print(";");
      Serial.println(controlStr);
#endif
      return true;
    }
  } else {
    // if the file didn't open, print an error:
    Serial.println("error writing to file");
    sdState = NOSD;
    return false;
  }
  Serial.println("error connecting to sd");
  sdState = NOSD;
  return false;
}

/// @brief Try to open init the sd card and check wether an sd card is present. Sets sdState to NOSD
/// if not successfull.
/// @return sdPresent indicates wether the sd card is present
boolean SDHelper::checkSDPresence() {
  if (!SD.begin(csPin)) {
#ifdef DEBUGSDHANDLING
    Serial.println("no sd found");
#endif
    sdPresent = false;
    sdState = NOSD;
    return false;
  }
  SD.end();
  sdPresent = true;
  return sdPresent;
}

/// @brief is SD present?
/// @return true if sd is present
boolean SDHelper::isSDinserted() {
  return sdPresent;
}