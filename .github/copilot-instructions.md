# Dew-Point-Ventilation-Zigbee AI Coding Instructions

## Project Overview
ESP32-C6 based ventilation controller that uses dew point calculations to determine when outdoor air is drier than indoor air. Controls a Zigbee socket to operate a fan with duty cycling (16 min on / 10 min off).

**Hardware**: Seeed XIAO ESP32-C6, 2x DHT22 sensors (indoor/outdoor), OLED display, RTC (PCF8563), SD card, Zigbee socket

## Architecture & Component Structure

The system follows a **modular helper class pattern** with 6 independent libraries in `DewPointFan/lib/`:

1. **ProcessSensorData** - Reads DHT22 sensors, uses 8-value circular buffers for averaging, calculates dew points, determines ventilation usefulness
2. **ControlFan** - State machine managing AUTO/ON/OFF modes with duty cycling logic
3. **ZigbeeSwitchHelper** - Zigbee coordinator using ESP32 Zigbee library (ZCZR mode)
4. **RTCHelper** - RTC management with automatic daylight saving time (CEST/CET) support
5. **SDHelper** - CSV data logging every 6 minutes to monthly files (`/YYYY-MM.csv`)
6. **DispHelper** - U8g2 display manager with auto-sleep after inactivity

**Main loop** (`DewPointFan/src/main.cpp`) orchestrates all helpers with `yield()` calls between major sections.

## Critical Build Configuration

**Platform**: Uses custom fork `pioarduino/platform-espressif32#53.03.11` (not standard Espressif platform)  
**Partition**: `zigbee_zczr.csv` (Zigbee Coordinator/Router mode)  
**Build flags**: `-DZIGBEE_MODE_ZCZR -DCORE_DEBUG_LEVEL=2`

Build/upload via PlatformIO: `pio run -t upload`

## Key Patterns & Conventions

### Sensor Power Reset Feature
Optional feature controlled by `#define SENSORPWRRESET` in `main.cpp`:
- When enabled, sensors connect to D3 pin instead of 3.3V
- Allows power cycling sensors if communication fails >30s
- Example: `digitalWrite(SensorPWR, LOW); delay(10000); digitalWrite(SensorPWR, HIGH);`

### Ventilation Decision Logic
Four conditions must ALL be true (see `processSensorData.h`):
- Indoor temp > 10°C (`TEMP_I_MIN`)
- Outdoor temp > -2°C (`TEMP_O_MIN`)
- Indoor dew point > 5°C (`DEWPOINT_I_MIN`)
- Outdoor dew point is 3°C lower than indoor (`DELTAP`)

### State Machine Timing
Each helper class has its own timing constant (not using Arduino timers):
- `FANwaitMS = 2000` - Fan state checks
- `SDwaitMS = 2100` - SD card operations
- `RTCwaitMS = 1100` - RTC updates
- `ZigbeeWAIT_MS = 1000` - Zigbee status checks

### Zigbee Factory Reset
Long press on BOOT button (GPIO 9) triggers `zigbeeSwitchHelper.reset()` which reboots the ESP32. New devices can pair within 180s after reset.

### Data Logging Format
CSV format with semicolon delimiters:
```
Date;Temperature T_i;Temperature T_o;Humidity H_i;Humidity H_o;Dew point DP_i;Dew point DP_o;validCnt_i;validCnt_o;Fan;Mode;On_s;Off_s
```
Files rotate monthly, named `/YYYY-MM.csv` by RTCHelper.

## Version Management
Manual semantic versioning in `main.cpp`:
```cpp
char versionStr[10] = "Ver 3.2.0";
```
Update per [preRelease.md](../preRelease.md): major.feature.fix

## Display Behavior
Display automatically sleeps after inactivity. Button press only wakes display (doesn't change mode) when sleeping. Second press increments mode.

## Debugging Features
Serial commands at 115200 baud:
- `Z` - Enter time adjustment mode (format: `dd.mm.yyyy hh:mm`)

Debug defines per file:
- `DEBUGSENSORHANDLING` in processSensorData.h
- `DEBUGFANHANDLING` in controlFan.h
- `DEBUGZIGBEEHANDLING` in zigbeeSwitchHelper.h

## Data Visualization
Two approaches in `Visualization/`:
1. **Jupyter Notebook** (`Dewpoint-Visualization.ipynb`) - Python/matplotlib with venv setup
2. **Browser-based** (`VisualizeData.html`) - Pure JavaScript, no dependencies, hosted on GitHub Pages

## Common Pitfalls
- DHT22 sensors: Not all models support negative temperatures despite specs
- Zigbee pairing: Only 180s window after factory reset
- SD card: Must use forward slash paths (`/2025-01.csv`)
- Sensor validation: `validCnt` field tracks successful readings in 8-value buffer
- Daylight saving: Controlled by `#define DAYLIGHTSAVING` in rtchelper.h (1=enabled)
