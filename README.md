# EmlaLock ESP32

A standalone ESP32 web dashboard for controlling and monitoring an EmlaLock session from a phone or computer on the local network.

## Features

- Wearer, Key Holder, and Alone modes.
- Live Time Passed and Time Left counters.
- Add or subtract session time where permitted.
- Mobile-friendly interface hosted directly by the ESP32.
- Wi-Fi scanning, saved-network recovery, and local setup access point.
- Credentials stored locally in ESP32 NVS, not in the repository.
- Role changes locked until credentials are cleared.
- OTA firmware updates from GitHub releases.

## Supported boards

- ESP32
- ESP32-C3
- ESP32-S2
- ESP32-S3

## Setup

1. Flash the firmware for your board.
2. Connect to the `EmlaLock-Setup` Wi-Fi network using password `EmlaLock-Setup`.
3. Open `192.168.4.1`.
4. Select Wearer, Key Holder, or Alone.
5. Enter Wi-Fi and EmlaLock credentials.
6. Save and connect.

If the saved Wi-Fi is unavailable, the setup access point is started again for recovery.

When editing existing settings, leaving a saved password or API-key field blank keeps the existing stored value.

## Building

The project uses PlatformIO, the Arduino framework, and ArduinoJson 7.

Build the environment matching your board, for example:

```bash
pio run -e esp32dev
```

## Releases

Release builds contain firmware binaries for all four supported board families. Releases are created manually from the GitHub Actions release workflow.

## Security

- EmlaLock credentials are stored locally in ESP32 NVS and are not committed to GitHub.
- The setup access point is protected with WPA2.
- HTTPS currently uses `WiFiClientSecure::setInsecure()` for broad ESP32 compatibility.
- Do not expose the ESP32 directly to the public internet without appropriate authentication and transport security.

## EmlaLock API

This project communicates with the EmlaLock API for session information and time controls.

API documentation: https://about.emlalock.com/docs/api/
