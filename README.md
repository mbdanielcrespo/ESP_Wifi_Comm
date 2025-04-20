# WiFi-Enabled Gate Controller

A networked ESP32-based solution for remote gate control over WiFi using ESP-IDF.

## Overview

This project implements a WiFi-enabled gate controller using the ESP32 microcontroller. It enables remote triggering of a gate/door mechanism over a local network. The system establishes a WiFi connection with a static IP, scans for a specific server on the network, and listens for commands to activate a relay that controls the gate.

Designed for applications like automated entrances, garage doors, or security gates, this controller provides a reliable and configurable solution that can be integrated into existing building automation systems.

## Features

- 🌐 Automatic WiFi connection with static IP configuration
- 🔍 Network scanning to find control server
- 🔌 TCP socket communication for reliable command exchange
- ⚡ Relay control with configurable timing parameters
- 🔄 Automatic reconnection on connection loss
- ⚙️ Centralized configuration through a single header file

## Hardware Requirements

- ESP32 development board
- Relay module (connected to GPIO 4 by default)
- Power supply for ESP32
- Connections to gate/door mechanism

## Software Architecture

### ESP-IDF Framework

This project is built using Espressif's IoT Development Framework (ESP-IDF), which is the official development framework for the ESP32 series of microcontrollers. ESP-IDF:

- Provides a robust, event-driven programming environment
- Implements FreeRTOS for task management
- Includes comprehensive drivers for ESP32 peripherals
- Supports WiFi, Bluetooth, and other communication protocols
- Features a component-based architecture for modular design

### Task-based Architecture

The ESP32 runs FreeRTOS, enabling multitasking capabilities:

1. **Main Task**: Initializes system components and starts other tasks
2. **TCP Connection Task**: Handles communication with the server
3. **WiFi Event Handlers**: Process WiFi connection events asynchronously

Tasks communicate via event groups and global flags, ensuring proper synchronization without complex locking mechanisms. This multitasking approach allows the system to:

- Monitor network connectivity while controlling the gate
- Handle multiple potential connections in parallel
- Respond to commands without blocking other operations

### Network Communication

The system implements a client-server model where:

1. The ESP32 acts as a client with a static IP on the local network
2. It scans a configurable range of IP addresses to find the server
3. Upon finding a server, it establishes a TCP connection
4. The ESP32 listens for specific commands from the server

This approach eliminates the need for port forwarding or complex networking configurations, making deployment simpler in typical residential or commercial settings.

## Workflow

1. **Initialization**: The system initializes NVS (Non-Volatile Storage), GPIO, and WiFi components
2. **WiFi Connection**: Connects to the configured WiFi network with static IP
3. **Server Discovery**: Scans the network for the control server
4. **Command Handling**: Listens for the gate activation command ("GATE" by default)
5. **Gate Control**: When commanded, activates the relay for a configurable duration

## Configuration

All system parameters are centralized in `wifi_config.h`, making it easy to adjust:

- WiFi credentials and network settings
- Server identification parameters
- IP scanning range
- Gate control timing
- Hardware pin assignments

## Building and Flashing

### Prerequisites

- ESP-IDF v4.4 or later (recommended v5.0+)
- CMake and Ninja build system
- Python 3.7 or later

### Building

```bash
# Clone the repository
git clone https://github.com/yourusername/wifi-gate-controller.git
cd wifi-gate-controller

# Set up ESP-IDF environment (if not already done)
. $IDF_PATH/export.sh

# Configure and build
idf.py set-target esp32
idf.py menuconfig  # Optional: for additional configuration
idf.py build
```

### Flashing

```bash
idf.py -p /dev/ttyUSB0 flash  # Replace with your serial port
```

### Monitoring

```bash
idf.py -p /dev/ttyUSB0 monitor
```

## Usage in Real-World Settings

This controller has been successfully implemented in automatic door systems where:

- Building security systems need to control access remotely
- Door/gate mechanisms need to be triggered by a central management system
- Manual control buttons need to be supplemented with network control

The system provides a reliable way to remotely control physical access points without requiring complex infrastructure changes or dedicated control lines.

## Security Considerations

This implementation focuses on functionality in a trusted local network. For production deployments, consider:

- Implementing authentication for the TCP communication
- Enabling TLS/SSL for encrypted communications
- Adding input validation to prevent command injection
- Implementing rate limiting to prevent relay wear from rapid triggering

## License

This project is released under the MIT License - see the LICENSE file for details.
