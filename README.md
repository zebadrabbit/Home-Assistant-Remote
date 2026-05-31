# Home Assistant Remote

Embedded touchscreen remote for Home Assistant, built on ESP32 with LVGL and PlatformIO.

## Overview

This project runs on an ESP32-based 2.8 inch touch display and provides:

- WiFi onboarding and credential storage
- Home Assistant URL/token setup
- Dashboard tile control for lights and switches
- Long-press light controls with brightness, color, and white temperature
- Idle dim/screensaver behavior

## Hardware and Stack

- MCU: ESP32 (esp32dev environment)
- Display: ILI9341 with XPT2046 touch
- Framework: Arduino
- UI: LVGL 8.x
- Build system: PlatformIO

## Quick Start

### 1. Install dependencies

Open this folder in VS Code with PlatformIO installed. PlatformIO resolves dependencies from platformio.ini automatically.

### 2. Build

```bash
platformio run --environment esp32dev
```

### 3. Upload

```bash
platformio run --target upload --environment esp32dev
```

### 4. Serial Monitor

```bash
platformio device monitor --environment esp32dev
```

## Configuration Notes

Primary project settings live in:

- include/config.h
- include/lv_conf.h
- platformio.ini

Important runtime configuration (WiFi, HA URL/token, filter mode, refresh interval) is persisted in NVS through StorageManager.

## Repository Layout

- src/: application code
- src/managers/: storage, WiFi, Home Assistant client logic
- src/screens/: LVGL screen implementations
- include/: compile-time config headers
- test/: test placeholder and future test code

## Documentation

- CHANGELOG.md: release and change history
- CONTRIBUTING.md: development and contribution workflow
- docs/PROCESS.md: end-to-end development process
- LICENSE: project license

## Current Status

The project is active and tuned for iterative firmware development on-device. Build and upload workflows are validated through PlatformIO.
