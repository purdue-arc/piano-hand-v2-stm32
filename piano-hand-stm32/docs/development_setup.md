# Development Setup & Workflow Guide

This guide details environment setup, firmware generation, automated testing, and team collaboration protocols for the Piano Hand STM32 project.

---

## 1. Prerequisites

Before starting development, ensure the following software tools are installed on your machine:
- **Operating System**: Windows 10/11, macOS, or Ubuntu Linux (20.04+).
- **STM32CubeIDE**: Version 1.14.0 or newer (includes GCC toolchain and GDB server for ARM Cortex-M4).
- **CMake**: Version 3.20 or newer (for host-based off-target C test compilation).
- **Python**: Version 3.10 or higher.
- **Git**: Version 2.30 or higher.
- **Hardware Tools (for physical testing)**:
  - ST-Link V2/V3 (embedded on Nucleo board).
  - Micro-USB / Mini-USB cable for Nucleo connection.
  - Logic analyzer / Oscilloscope (optional, for I²C and UART debugging).

---

## 2. Repository Clone & Structure

Clone the repository to your local development environment:

```bash
git clone https://github.com/purdue-arc/piano-hand-stm32.git
cd piano-hand-stm32
```

---

## 3. Host Python Environment Setup

Set up a virtual environment and install the required dependencies:

```bash
# Create virtual environment
python -m venv .venv

# Activate virtual environment
# Windows (PowerShell):
.venv\Scripts\Activate.ps1
# Windows (CMD):
.venv\Scripts\activate.bat
# Linux / macOS:
source .venv/bin/activate

# Install dependencies
pip install --upgrade pip
pip install -r host/requirements.txt
```

---

## 4. STM32CubeMX Setup Instructions

When opening or regenerating the STM32CubeMX `.ioc` file for the Nucleo-F446RE:

1. **Clock Configuration**:
   - High-Speed External (HSE) oscillator enabled with ST-Link MCO or on-board crystal.
   - Configure PLL to run SYSCLK at maximum rated frequency (**180 MHz**).
   - APB1 peripheral clock at 45 MHz, APB2 at 90 MHz.
2. **Connectivity Peripherals**:
   - **`USART2`**: Asynchronous mode, 115200 baud, 8N1, connected to ST-Link virtual COM port (PA2 TX / PA3 RX).
   - **`I2C1` or `I2C2`**: Fast Mode (400 kHz) for PCA9685 communication.
   - **`USART1` or `USART3`**: Asynchronous mode, 115200 baud, 8N1 for Stepper Motor Driver. Enable RX DMA / Interrupt.
   - **`UART4` or `USART6`**: Asynchronous mode, 115200 baud, 8N1 for BLE bridge. Enable RX DMA.
3. **Timers & GPIO**:
   - Optional: `TIM2` / `TIM3` in Combined Channels -> Encoder Mode.
   - GPIO Inputs with pull-ups for limit switches and emergency stop interrupts.
4. **Project Settings**:
   - Toolchain / IDE: **STM32CubeIDE**.
   - Code Generator: Check *"Generate peripheral initialization as a pair of '.c/.h' files per peripheral"*.
   - Check *"Keep User Code when re-generating"*.

---

## 5. Running Host-Side C Firmware Unit Tests

Host tests run on your local machine using CMake and standard C compiler without needing an attached Nucleo board:

```bash
cd tests/firmware
mkdir build && cd build
cmake ..
cmake --build .
./piano_hand_tests
```

---

## 6. Running Python Unit Tests

Validate command builders, protocol string validation, timestamp conversion, and song parser:

```bash
# From the repository root:
python -m pytest tests/host -v
```

---

## 7. Running the Host Controller

### 7.1 Song Playback Mode
To stream a song file to the robot over BLE:

```bash
python host/controller.py --hand left --file examples/hot_cross_buns.txt
```

### 7.2 Manual Interactive CLI
To jog the carriage, actuate individual fingers, or query status interactively:

```bash
python host/controller.py --hand left --interactive
```

---

## 8. Git Workflow & Branching Guidelines

- **Protected Branch**: `main` is protected. All changes must arrive via reviewed pull requests.
- **Branch Naming Conventions**:
  - `feature/<module>-<description>` (e.g. `feature/pca9685-i2c-driver`)
  - `bugfix/<issue-id>-<description>` (e.g. `bugfix/servo-timing-drift`)
  - `docs/<topic>` (e.g. `docs/pin-mapping-updates`)
  - `test/<test-suite>` (e.g. `test/song-parser-fixtures`)
- **Pull Request Rules**:
  - All automated host tests must pass (`pytest tests/host` and CMake C tests).
  - Clean git history with clear, descriptive commit messages.
  - At least one peer review approval before merging.

---

## 9. Five-Person Team Task Allocation

| Team Member | Role | Core Responsibility |
|---|---|---|
| **Member 1 (Lead & Host Tools)** | Host Systems Engineer | Maintain `host/controller.py`, `song_parser.py`, and BLE communications pipeline. |
| **Member 2 (Firmware Core)** | Embedded Software Lead | Implement STM32 state machine, command ring buffers, and protocol decoders. |
| **Member 3 (Actuator Drivers)** | Hardware Abstraction Dev | Implement PCA9685 I²C driver, servo angle calibration, and PWM interrupt timing. |
| **Member 4 (Motion Control)** | Stepper & Kinematics Dev | Stepper driver UART/pulse interface, acceleration ramping, homing routines, and encoder integration. |
| **Member 5 (QA & Hardware)** | Test & Electrical Integrator | PCB/breadboard wiring, power distribution, oscilloscope validation, and end-to-end integration testing. |
