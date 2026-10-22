# Piano Hand STM32

[![Firmware CI](https://img.shields.io/badge/firmware-passing-brightgreen.svg)](#)
[![Python Host CI](https://img.shields.io/badge/host--tests-passing-brightgreen.svg)](#)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: STM32F446RE](https://img.shields.io/badge/MCU-STM32F446RE-orange.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f446re.html)

> Robotic piano-playing hand firmware and non-blocking BLE host controller, migrated from ESP32 MicroPython to STM32 C.

---

## 1. Quick-Start (3 Steps)

Get up and running with the host controller in under two minutes:

1. **Install dependencies**:
   ```bash
   python -m venv .venv
   .venv\Scripts\activate      # On Windows (or source .venv/bin/activate on Linux/macOS)
   pip install -r host/requirements.txt
   ```

2. **Run Python unit tests**:
   ```bash
   python -m pytest tests/host -v
   ```

3. **Stream a song to the robot**:
   ```bash
   python host/controller.py --hand left --file examples/hot_cross_buns.txt
   ```

*(Alternatively, run in interactive mode without a file: `python host/controller.py --hand left --interactive`)*

---

## 2. Repository Structure

```text
piano-hand-stm32/
├── .gitignore                      # Git ignore rules for STM32, Python, & CMake
├── README.md                       # Project overview, quickstart, and status
├── docs/                           # System design and developer documentation
│   ├── architecture.md             # Complete system architecture and state machines
│   ├── communication_protocol.md   # ASCII packet format, BLE UUIDs, and commands
│   ├── development_setup.md        # Environment setup, workflow, and CubeMX guide
│   └── hardware_requirements.md    # Electrical specs, BOM, wiring, and safety homing
├── examples/                       # Canonical song files
│   └── hot_cross_buns.txt          # Annotated reference canonical song file
├── firmware/                       # Embedded C firmware for STM32 Nucleo-F446RE
│   ├── Core/                       # Application logic, state machine, decoders
│   └── Drivers/                    # Hardware abstraction (PCA9685, Stepper, BLE)
├── hardware/                       # Hardware pin mappings and electronics
│   ├── pin_mapping.md              # Nucleo-F446RE pin allocation table
│   ├── pcb/                        # Printed circuit board layouts
│   └── schematics/                 # Electrical schematics
├── host/                           # Host-side Python tools & BLE controller
│   ├── __init__.py                 # Host package definition
│   ├── controller.py               # Asyncio Bleak BLE controller & CLI
│   ├── note_map.py                 # Note-to-stepper translation stub
│   ├── protocol.py                 # Command building and validation logic
│   ├── requirements.txt            # Python dependencies (bleak, pytest)
│   └── song_parser.py              # Canonical & legacy song file parser
└── tests/                          # Automated test suites
    ├── firmware/                   # Off-target C unit tests (CMake)
    └── host/                       # Host Python unit tests (pytest)
        ├── __init__.py             # Test package marker
        ├── test_note_map.py        # Note transition tests
        ├── test_protocol_host.py   # Protocol builder & validation tests
        └── test_song_parser.py     # Parser & unit auto-detection tests
```

---

## 3. Hardware Overview

- **Controller**: STMicroelectronics Nucleo-F446RE (ARM Cortex-M4 @ 180 MHz, 512 KB Flash, 128 KB SRAM).
- **Finger Actuation**: 5 independent micro servos actuated via an Adafruit PCA9685 16-channel 12-bit PWM controller (I²C bus, 50 Hz PWM).
- **Carriage Translation**: NEMA 17 stepper motor driving a lead screw / timing belt lateral rail, driven by a quiet UART-enabled driver (TMC2208/TMC2209).
- **Host Connectivity**: Low-power BLE UART module communicating with the host PC.
- **Power Delivery**: Dual-rail architecture (3.3V logic + isolated 5V/5A servo rail and 12V/24V stepper rail).

---

## 4. Current Status

### ✅ Complete & Working
- **Host Protocol Builder & Validator** (`host/protocol.py`): Zero-dependency command encoding, bounds checking for fingers (1–5) and int32 key displacements.
- **Canonical Song Parser** (`host/song_parser.py`): Ingests canonical and legacy formats, normalizes ms/seconds timestamps, skips comments, validates monotonic timing.
- **Host BLE Controller** (`host/controller.py`): Non-blocking asynchronous event loop, scheduled servo release without timing drift, configurable BLE UUIDs, automatic reset and retry loops.
- **Unit Test Suite** (`tests/host/`): Comprehensive pytest coverage for protocol, parser, and note mapping stubs.
- **Complete Architecture & Protocol Documentation** (`docs/`): Detailed sequence diagrams, packet framing, hardware specs, and safety homing definitions.

### ⚠️ Provisional (Requires Project Sign-off)
- **Pin Allocations** (`hardware/pin_mapping.md`): Peripheral pin assignments (I²C, UART instances) marked provisional pending schematic review.
- **Note-to-Step Calibration** (`host/note_map.py`): Functional translation stub awaiting physical key distance measurements.

### ❌ Requires Physical Hardware Verification
- **PCA9685 Physical Bus Acknowledgment**: Hardware I²C signal integrity and rise times under 50 Hz multi-servo loading.
- **Stepper Physical Homing**: Installation and endstop interrupt integration with physical limit switches.
- **BLE Module Pairing & Throughput**: On-target BLE latency benchmarking during rapid chord sequences.

---

## 5. Team Roles

| Member | Role | Focus Area |
|---|---|---|
| **Member 1** | Host Systems Engineer | BLE controller, song parser, command streaming pipeline |
| **Member 2** | Embedded Software Lead | STM32 state machine, command queues, ring buffers |
| **Member 3** | Actuator Drivers Dev | PCA9685 I²C driver, PWM pulse calibration |
| **Member 4** | Motion & Kinematics Dev | Stepper driver UART/pulses, acceleration profiles, homing |
| **Member 5** | QA & Electrical Integrator | PCB/breadboard wiring, power distribution, test fixtures |

---

## 6. Documentation Index

- [System Architecture](file:///docs/architecture.md)
- [Communication Protocol](file:///docs/communication_protocol.md)
- [Hardware Requirements & Safety](file:///docs/hardware_requirements.md)
- [Development Setup & Workflow](file:///docs/development_setup.md)
- [STM32 Nucleo Pin Mapping](file:///hardware/pin_mapping.md)
