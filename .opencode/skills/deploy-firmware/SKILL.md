# Deploy Firmware — fitbit-display

## Overview

This skill describes how to compile and deploy the fitbit-display firmware to the ESP32 device (ID: `CjvJ39w8`).

The firmware communicates via WebSocket to `offbeat-iot.com`, receives Fitbit metrics, and displays goal progress on 4 WS2812 LED strips (sleep/green, water/blue, calories/red, steps/orange).

## Device Info

| Field          | Value                               |
|----------------|-------------------------------------|
| Device ID      | `CjvJ39w8`                          |
| WebSocket host | `offbeat-iot.com`                   |
| WebSocket path | `/ws?device=CjvJ39w8`              |
| OTA host       | `192.168.1.55`                      |
| Serial port    | `COM5`                              |
| Board          | ESP32 DOIT DEVKIT V1                |
| Framework      | Arduino + PlatformIO                |

## Prerequisites

- [PlatformIO](https://platformio.org/) (or `arduino-cli`)
- Python 3 (for OTA upload via `espota.py`)
- Git (for version string in build)

## Quick Deploy (Serial)

```powershell
cd fitbit-display
git rev-parse --short HEAD > version.tmp
platformio run -e esp32dev_serial --upload-port COM5 -t upload
```

## Quick Deploy (OTA)

```powershell
cd fitbit-display
git rev-parse --short HEAD > version.tmp
platformio run -e esp32dev_ota -t upload --upload-port 192.168.1.55
```

## Deployment Methods

### 1. Serial — `deployUsingSerial.sh`

Compiles with `arduino-cli` and uploads via serial to `COM5`.

```bash
./deployUsingSerial.sh
```

This generates version from `git rev-parse --short HEAD`, writes it to `version.h`, compiles for `esp32:esp32:esp32doit-devkit-v1`, and flashes via `upload.py` at 115200 baud.

### 2. OTA — `deployUsingOta.sh`

Compiles and uploads wirelessly to `192.168.1.55`.

```bash
./deployUsingOta.sh
```

Uses `espota.py` to push the firmware image to the device over WiFi.

### 3. API — `deployUsingApi.sh`

Compiles and uploads the binary to the offbeat-iot server via HTTP API.

```bash
./deployUsingApi.sh
```

The server distributes the firmware to devices over the air. The binary is POSTed to `https://soeren.herokuapp.com/api/devices/CjvJ39w8/images`.

### 4. PlatformIO (recommended for dev)

Use PlatformIO environment `esp32dev_serial` (serial) or `esp32dev_ota` (WiFi OTA).

**Serial upload:**
```powershell
platformio run -e esp32dev_serial -t upload --upload-port COM5
```

**OTA upload:**
```powershell
platformio run -e esp32dev_ota -t upload --upload-port 192.168.1.55
```

**Monitor serial output:**
```powershell
platformio device monitor -p COM5 -b 115200
```

### 5. Jenkins CI/CD

The `Jenkinsfile` uses the shared library `shared-jenkins-pipelines` and calls `platformioPipeline`:

```groovy
@Library("shared-jenkins-pipelines") _
platformioPipeline('offbeatCredentialsId': "offbeatCredentialsId"
    ,'offbeatDeviceId':"CjvJ39w8"
    ,'publishOffbeat':'true')
```

The pipeline builds the firmware and publishes the binary to the offbeat-iot server.

## Versioning

The firmware embeds the current Git SHA as a version string.

**Before building**, ensure `version.h` contains the current SHA:
```powershell
$sha = git rev-parse --short HEAD
(Get-Content version.tmp) -replace 'newVersion', $sha | Set-Content include/version.h
```

Or let the deploy scripts (`deployUsing*.sh`) handle this automatically via `sed`.

## Monitoring / Debugging

Once deployed, connect via telnet to `192.168.1.55` to see live debug output:

```bash
telnet 192.168.1.55
```

Telnet commands:
- `r` — restart the device
- `o` — print firmware version
- `1` — turn on pump
- `2` — turn off pump
- `3` — fetch fitbit weight
- `4` — fetch fitbit weight goal
- `5` — fetch fitbit daily activities
- `6` — update display mode
- `7` — calculate temp/humidity
- `8` — fetch all fitbit values
- `t` — turn on pump for 3 minutes

## Architecture

The firmware uses a Chain-of-Responsibility pattern:

```
webSocketEvent
  -> handleCommandDocument (powerstate dispatch)
    -> WaterGoalHandler -> CaloriesGoalHandler -> SleepGoalHandler
    -> StepGoalsHandler -> WeightGoalHandler -> LedModeHandler
```

Each handler checks for specific keys in the JSON/CBOR response and updates `Context` fields with goal progress percentages. The `LedModeHandler` reads these percentages in its `run()` method and lights up WS2812 LEDs accordingly.
