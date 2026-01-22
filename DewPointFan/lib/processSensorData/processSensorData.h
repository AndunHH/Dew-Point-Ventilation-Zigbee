#define DHTPINI D0 // Digital pin connected to the DHT sensor
#define DHTPINO D7 // second DHT

#define DELTAP                                                                                     \
  3.0 // Der Taupunkt draußen muss um diese Gradzahl kleiner sein als drinnen, damit gelüftet wird
#define TEMP_I_MIN 10.0 // Minimale Innentemperatur, bei der die Lüftung nicht mehr aktiviert wird.
#define TEMP_O_MIN -2.0 // Minimale Außentemperatur, bei der die Lüftung nicht mehr aktiviert wird.
#define DEWPOINT_I_MIN 5.0 // Minimaler Taupunkt innen, nur oberhalb läuft der Lüfter

// Sensor power reset feature: enables power cycling sensors via GPIO pin
#define SENSORPWRRESET
#define SENSORPWRPIN D3 // GPIO pin that controls sensor power

// Timeout before triggering sensor reset (30 seconds)
#define SENSOR_RESET_TIMEOUT_MS 30000
// Duration to keep sensors powered off during reset (10 seconds)
#define SENSOR_POWER_OFF_DURATION_MS 10000

#define DEBUGSENSORHANDLING

#include "DHTesp.h"
#include <CircularBuffer.hpp>

#define TEMPLOG_LENGTH 40

/* init measurement handling */

typedef struct {
  float temperature;
  float humidity;
  float dewPoint;
  uint8_t validCnt;
} AvgMeasurement;

enum VentilationUseFull {
  USEFULL,
  NODATA,
  NODATAINDOOR,
  NODATAOUTDOOR,
  TOOCOLDINSIDE,
  TOOCOLDOUTSIDE,
  INSIDEDRYENOUGH,
  OUTSIDENOTDRYENOUGH
};

#define RING_BUFFER_SIZE 8 // Size of the ring buffer.

/// @brief ProcessSensorData class to read in two DHT sensors and calculate temperatur and
/// humidities with a circularbuffer.
class ProcessSensorData {
public:
  void loop();
  bool init();

  ProcessSensorData()
      : processSensorDataStates(INIT), condTempImin_degC(TEMP_I_MIN), condTempOmin_degC(TEMP_O_MIN),
        condDewPointImin_degC(DEWPOINT_I_MIN), condDewPointDiffmin_K(DELTAP),
        ventilationUseFull(NODATA), timeLastValidDataI_ms(0), timeLastValidDataO_ms(0),
        sensorResetInProgress(false), lastResetTime(0) {}

  void printBuffer();
  AvgMeasurement getAverageMeasurements(boolean inner);

  VentilationUseFull getVentilationUsefullStatus();
  boolean isVentilationUsefullStatus();
  void printStatus();
  void createLogChar(char *logStr);

  uint32_t timeSinceAllDataWhereValid();
  boolean areBothSensorAvgValuesValid();

  /// @brief Check if sensor reset/power cycle is currently in progress
  /// @return true if sensors are being reset (display should show reset screen)
  boolean isSensorResetInProgress();

private:
  VentilationUseFull ventilationUseFull;
  uint32_t delayMS;
  float condDewPointDiffmin_K, condTempImin_degC, condTempOmin_degC, condDewPointImin_degC;

  DHTesp dhtI;
  DHTesp dhtO;

  unsigned long lastReadI;
  unsigned long lastReadO;

  enum ProcessSensorDataStates {
    INIT,
    READI,
    READO,
    CALC,
    SENSOR_POWER_OFF_WAIT,
    SENSOR_REINIT
  } processSensorDataStates;

  boolean calcNewVentilationStartUseFull();

  CircularBuffer<TempAndHumidity, RING_BUFFER_SIZE> bufI;
  CircularBuffer<TempAndHumidity, RING_BUFFER_SIZE> bufO;

  boolean calculateAverage(CircularBuffer<TempAndHumidity, RING_BUFFER_SIZE> *buf,
                           AvgMeasurement *avg);

  /// @brief store the averaged measurements
  AvgMeasurement avgMeasurementI;
  AvgMeasurement avgMeasurementO;

  /// @brief store the time in ms since the last valid data arrived
  uint32_t timeLastValidDataI_ms;
  uint32_t timeLastValidDataO_ms;

  /// @brief Flag indicating if sensor reset is in progress
  boolean sensorResetInProgress;

  /// @brief Timestamp for non-blocking sensor reset timing
  unsigned long lastResetTime;
};