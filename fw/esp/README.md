# ESP32-C3 UART to BLE (NimBLE)

Bridges UART data to a BLE GATT characteristic and notifies connected devices.
Use a BLE scanner app (e.g., nRF Connect) to view notifications.

## Features
- Reads text from UART and sends it as BLE notifications
- BLE GATT characteristic is readable and notifiable

## Requirements
- ESP-IDF 6.0.1
- ESP32-C3 target
- Phone app: nRF Connect (or similar)

## Build and Flash
```bash
idf.py set-target esp32c3
idf.py build
idf.py -p <PORT> flash monitor
```

## Pin Wiring
- UART TX (ESP32-C3 GPIO4) -> UART RX of external device
- UART RX (ESP32-C3 GPIO5) -> UART TX of external device
- GND -> GND
- Hardware flow control is disabled (RTS/CTS not used)

## Usage
1. Wire UART as shown above.
2. Build/flash/monitor the firmware.
3. Open nRF Connect (or similar) and scan for the device name.
4. Connect and enable notifications on the UART characteristic.
5. Incoming UART text appears as BLE notifications.

## Notes
- Device name: ESP32C3-NimBLE
- UART baud rate: 115200
- BLE characteristic supports read and notify
- Max BLE payload per notify: 128 bytes
