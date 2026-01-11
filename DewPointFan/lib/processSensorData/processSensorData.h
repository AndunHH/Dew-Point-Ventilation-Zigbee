#define DHTPINI D0 // Digital pin connected to the DHT sensor
#define DHTPINO D7 // second DHT

#define DELTAP                                                                                     \
  3.0 // Der Taupunkt draußen muss um diese Gradzahl kleiner sein als drinnen, damit gelüftet wird
#define TEMP_I_MIN 10.0 // Minimale Innentemperatur, bei der die Lüftung nicht mehr aktiviert wird.
#define TEMP_O_MIN -2.0 // Minimale Außentemperatur, bei der die Lüftung nicht mehr aktiviert wird.
#define DEWPOINT_I_MIN 5.0 // Minimaler Taupunkt innen, nur oberhalb läuft der Lüfter

// define DEBUGSENSORHANDLING

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
        ventilationUseFull(NODATA), timeLastValidDataI_ms(0), timeLastValidDataO_ms(0) {}

  void printBuffer();
  AvgMeasurement getAverageMeasurements(boolean inner);

  VentilationUseFull getVentilationUsefullStatus();
  boolean isVentilationUsefullStatus();
  void printStatus();
  void createLogChar(char *logStr);

  uint32_t timeSinceAllDataWhereValid();
  boolean areBothSensorAvgValuesValid();

private:
  VentilationUseFull ventilationUseFull;
  uint32_t delayMS;
  float condDewPointDiffmin_K, condTempImin_degC, condTempOmin_degC, condDewPointImin_degC;

  DHTesp dhtI;
  DHTesp dhtO;

  unsigned long lastReadI;
  unsigned long lastReadO;

  enum ProcessSensorDataStates { INIT, READI, READO, CALC } processSensorDataStates;

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
};