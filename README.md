# ESP32S3 Toslink Scream Sender


# Note: This code is not yet tested, I am pending TOSLINK receiver modules.


## Overview

The ESP32S3 Toslink Scream Sender is a tool to configure an ESP32S3 microcontroller as a digital audio receiver using Toslink (optical) input. This implementation allows you to forward audio from various devices that support Toslink output (such as TVs, gaming consoles, and audio equipment) to ScreamRouter.

## Key Features

- Implements Toslink (S/PDIF) digital audio reception
- Compatible with a wide range of devices with optical audio output
- Forwards audio to ScreamRouter
- Utilizes the ESP32S3's digital I/O capabilities

## Hardware Requirements

- ESP32S3 microcontroller
- Toslink receiver module (e.g., TORX1353)
- Appropriate optical cable

## Software Dependencies

- ESP-IDF (Espressif IoT Development Framework)

## Building and Flashing

To build and flash the project onto your ESP32S3, follow these steps:

1. Clone the repository to your local machine.
2. Navigate to the project directory.
3. Copy `main/secrets_example.h` to `main/secrets.h`.
4. Open `main/secrets.h` and fill out the required information.
5. Connect your ESP32S3 to your computer via USB.
6. Open a terminal in the project directory within the ESP IDF framework.
7. Run the following command, replacing `<esp32s3 com port>` with the appropriate COM port for your device:

```
idf.py -p <esp32s3 com port> flash monitor
```