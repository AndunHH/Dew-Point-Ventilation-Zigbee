#include <Arduino.h>

#include "processSensorData.h"

/// @brief initialize process sensor data with two DHT sensors
/// @return true after initialization
bool ProcessSensorData::init() {
  // Initialize temperature sensor 1
  dhtI.setup(DHTPINI, DHTesp::DHT22);
  // Initialize temperature sensor 2
  dhtO.setup(DHTPINO, DHTesp::DHT22);
  // allow the system to gather valid data and therefore assume that initally valid data may be
  // given
  timeLastValidDataI_ms = millis();
  timeLastValidDataO_ms = timeLastValidDataI_ms;
  delayMS = 2000;
  return true;
}

/// @brief This is the loop function to read the temperature and humidity sensors and calculate
/// wether ventilation is usefull or not
void ProcessSensorData::loop() {
  unsigned long now = millis();
  TempAndHumidity sensorData;

  switch (processSensorDataStates) {
  case INIT:
    processSensorDataStates = READO;
    break;
  case READO:
    if (now - lastReadO >= delayMS) {
#ifdef DEBUGSENSORHANDLING
      Serial.println(now);
      Serial.println("out");
#endif
      sensorData = dhtO.getTempAndHumidity(); // Read values from sensor 1
      bufO.push(sensorData);
      lastReadO = now;
      processSensorDataStates = READI;
    }
    break;
  case READI:
    if (now - lastReadI >= delayMS) {
#ifdef DEBUGSENSORHANDLING
      Serial.println(now);
      Serial.println("in");
#endif
      sensorData = dhtI.getTempAndHumidity(); // Read values from sensor 1
      bufI.push(sensorData);
      lastReadI = now;
      processSensorDataStates = CALC;
    }
    break;
  case CALC:
    if (calculateAverage(&bufI, &avgMeasurementI)) {
      // if at least one valid data package is valid in the buffer, the timer to check for valid
      // data at all is reset.
      timeLastValidDataI_ms = now;
    }
    if (calculateAverage(&bufO, &avgMeasurementO)) {
      timeLastValidDataO_ms = now;
    }
    calcNewVentilationStartUseFull();
    processSensorDataStates = READO;
    break;
  }
}

/// @brief prints the buffer for internal sensor bufI for debugging
void ProcessSensorData::printBuffer() {
  if (bufI.isEmpty()) {
    Serial.println("empty");
  } else {
    Serial.print("hum: [");
    for (decltype(bufI)::index_t i = 0; i < bufI.size(); i++) {
      Serial.print(bufI[i].humidity);
      Serial.print(",");
    }
    Serial.println("]");
    Serial.print("temp: [");
    for (decltype(bufI)::index_t i = 0; i < bufI.size(); i++) {
      Serial.print(bufI[i].temperature);
      Serial.print(",");
    }
    Serial.print("] (");

    Serial.print(bufI.size());
    Serial.print("/");
    Serial.print(bufI.size() + bufI.available());
    if (bufI.isFull()) {
      Serial.print(" full");
    }

    Serial.println(")");
  }
}

/// @brief Calculate the average temperature and humidity
/// @return true, if valid data were found
boolean ProcessSensorData::calculateAverage(CircularBuffer<TempAndHumidity, RING_BUFFER_SIZE> *buf,
                                            AvgMeasurement *avg) {
  avg->validCnt = 0; // no valid data package jet
  avg->temperature = 0;
  avg->humidity = 0;
  avg->dewPoint = NAN;

  for (CircularBuffer<TempAndHumidity, RING_BUFFER_SIZE>::index_t i = 0; i < buf->size(); i++) {
    if ((*buf)[i].temperature > 500 || (*buf)[i].humidity > 500 || isnan((*buf)[i].temperature) ||
        isnan((*buf)[i].humidity)) {
      // no valid data
      continue; // try next element in counter
    }
    // this "else" is taken automatically, because the if is left with continue
    avg->temperature += (*buf)[i].temperature;
    avg->humidity += (*buf)[i].humidity;
    avg->validCnt++;
  }

  if (avg->validCnt == 0)
    return false; // no valid data found in the whole counter

  avg->temperature = avg->temperature / avg->validCnt;
  avg->humidity = avg->humidity / avg->validCnt;
  // calculate the dew point
  // the following method is called from within the dhtI object,
  // even though calculateAverage is also used for the outer sensor.
  // This is okay, because computeDewPoint relies only on the values provided
  // as attributes and not on the values in the object itself. The computeDewPoint could be
  // implemented as static, but the lib is as it is.
  avg->dewPoint = dhtI.computeDewPoint(avg->temperature, avg->humidity, false);

  return true;
}

/// @brief get averaged and dewPoint calculated data
/// @param inner true for inner sensor, false for outer sensor
/// @return AvgMeasurement
AvgMeasurement ProcessSensorData::getAverageMeasurements(boolean inner) {
  if (inner)
    return avgMeasurementI;
  else
    return avgMeasurementO;
}

/// @brief Checks wethere a new ventilation start is usefull
/// @return true if starting ventilation is usefull
boolean ProcessSensorData::calcNewVentilationStartUseFull() {
  // both sensors invalid?
  if (avgMeasurementI.validCnt < 1 && avgMeasurementO.validCnt < 1) {
    ventilationUseFull = NODATA;
    return false;
  }
  // inner sensor invalid?
  if ((avgMeasurementI.validCnt < 1) && (avgMeasurementO.validCnt >= 1)) {
    ventilationUseFull = NODATAINDOOR;
    return false;
  }
  // outer sensor invalid?
  if ((avgMeasurementI.validCnt >= 1) && (avgMeasurementO.validCnt < 1)) {
    ventilationUseFull = NODATAOUTDOOR;
    return false;
  }

  if (avgMeasurementI.temperature < condTempImin_degC) {
    // to cold inside!
    ventilationUseFull = TOOCOLDINSIDE;
    return false;
  }
  if (avgMeasurementO.temperature < condTempOmin_degC) {
    // to cold outside!
    ventilationUseFull = TOOCOLDOUTSIDE;
    return false;
  }
  // compare dewpoint and other conditions to decide if ventilation is usefull
  if (avgMeasurementI.dewPoint < condDewPointImin_degC) {
    // it's dry enough inside, turn fan off
    ventilationUseFull = INSIDEDRYENOUGH;
    return false;
  }
  if ((avgMeasurementI.dewPoint - avgMeasurementO.dewPoint) > condDewPointDiffmin_K) {
    // if dew point inside is higher than dew point outside
    ventilationUseFull = USEFULL;
    return true;
  } else {
    // dewpoint is inside nearly outside
    ventilationUseFull = OUTSIDENOTDRYENOUGH;
    return false;
  }
}

VentilationUseFull ProcessSensorData::getVentilationUsefullStatus() {
  return ventilationUseFull;
}

/// @brief Reports if a ventilation start is usefull
/// @return true if start is usefull
boolean ProcessSensorData::isVentilationUsefullStatus() {
  if (ventilationUseFull == USEFULL)
    return true;
  else
    return false;
}

/// @brief Print the status
void ProcessSensorData::printStatus() {
  Serial.print("Inner Sensor:");
  Serial.print("Temp: ");
  Serial.print(avgMeasurementI.temperature);
  Serial.print("°C - Humidty: ");
  Serial.print(avgMeasurementI.humidity);
  Serial.print("%% - Dewpoint: ");
  Serial.print(avgMeasurementI.dewPoint);
  Serial.print("°C - ValidCnt: ");
  Serial.println(avgMeasurementI.validCnt);
  Serial.print("Outer Sensor:");
  Serial.print("Temp: ");
  Serial.print(avgMeasurementO.temperature);
  Serial.print("°C - Humidty: ");
  Serial.print(avgMeasurementO.humidity);
  Serial.print("%% - Dewpoint: ");
  Serial.print(avgMeasurementO.dewPoint);
  Serial.print("°C - ValidCnt: ");
  Serial.println(avgMeasurementO.validCnt);
  Serial.print("Ventilation usefull? ");
  switch (ventilationUseFull) {
  case USEFULL:
    Serial.println("Ventilation is usefull");
    break;
  case NODATA:
    Serial.println("No valid data");
    break;
  case NODATAINDOOR:
    Serial.println("No valid data from Indoor sensor");
    break;
  case NODATAOUTDOOR:
    Serial.println("No valid data from Outdoor sensor");
    break;
  case TOOCOLDINSIDE:
    Serial.println("Too cold inside");
    break;
  case TOOCOLDOUTSIDE:
    Serial.println("Too cold outside");
    break;
  case INSIDEDRYENOUGH:
    Serial.println("Inside is dry enough");
    break;
  case OUTSIDENOTDRYENOUGH:
    Serial.println("Outside is not drier");
    break;
  }
}

/// @brief Fill the string with actual averaged sensor data
/// @param logStr
void ProcessSensorData::createLogChar(char *logStr) {
  // check/update the header CSV_HEADER in sdhelper.h

  // temp i, temp o, hum i, hum o, dew i, dew o, valid
  // temperatures with sign and one fraction
  // humidity three numbers
  //+23.4;+22.7;+83.8;+58.1;+20.5;+14.1;8;8
  snprintf(logStr, TEMPLOG_LENGTH, "%+3.1f;%+3.1f;%+3.1f;%+3.1f;%+3.1f;%+3.1f;%u;%u",
           avgMeasurementI.temperature, avgMeasurementO.temperature, avgMeasurementI.humidity,
           avgMeasurementO.humidity, avgMeasurementI.dewPoint, avgMeasurementO.dewPoint,
           avgMeasurementI.validCnt, avgMeasurementO.validCnt);
}

/// @brief Returns the duration when the last valid data packages for both sensors where in the
/// buffer
/// @return duration in ms
uint32_t ProcessSensorData::timeSinceAllDataWhereValid() {
  uint32_t now = millis();
  /*  Serial.print("Duration i: ");
    Serial.print(now - timeLastValidDataI_ms);
    Serial.println("");
    Serial.print("Duration o: ");
    Serial.print(now - timeLastValidDataO_ms);
    Serial.println("");
  */
  return max(now - timeLastValidDataO_ms, now - timeLastValidDataI_ms);
}