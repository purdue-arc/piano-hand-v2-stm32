# Hardware Requirements & Peripherals Specification

## 1. Bill of Materials (BOM)

| Subsystem | Item / Component | Status | Purpose / Specifications |
|---|---|---|---|
| **MCU Board** | STM32 Nucleo-F446RE | **Confirmed** | Main ARM Cortex-M4 @ 180MHz controller board. |
| **Finger Actuators** | 5x Micro Servos (e.g. SG90 / MG90S) | **Confirmed** | Individual downward actuation for fingers 1 through 5. |
| **PWM Driver** | Adafruit / Generic PCA9685 16-Ch 12-bit PWM | **Confirmed** | I²C servo driver with separate 5V V+ servo power rail. |
| **Lateral Motor** | NEMA 17 Stepper Motor | **Confirmed** | Drives lead screw / belt for carriage lateral movement. |
| **Stepper Driver** | TMC2208 / TMC2209 / UART-compatible Stepper Driver | **Provisional** | Driven via UART (115200, 8N1) or Step/Dir pulses. |
| **BLE Module** | AT-09 / HM-10 / JDY-23 BLE UART Module | **Provisional (TBD)** | Serial-to-BLE peripheral bridging commands from host. |
| **Carriage Encoder** | Optical / Magnetic Quadrature Rotary Encoder | **Optional (TBD)** | Closed-loop carriage positioning feedback. |
| **Limit Switches** | Optical or mechanical microswitches | **Required (TBD)** | Physical carriage endstops and zero-reference homing. |
| **Power Supply** | 5V / 5A DC Regulated Supply + 12V/24V Supply | **Confirmed** | Dedicated power rails for servos and stepper motor. |

---

## 2. STM32 Nucleo-F446RE Peripheral Allocation Table

> [!WARNING]
> All microcontroller pin assignments are provisional. Pin assignments must be validated against board schematics, header conflicts, and CubeMX pinout configuration before PCB/breadboard assembly.

| Peripheral | Function | STM32 Instance | Pins | Status |
|---|---|---|---|---|
| **I²C** | PCA9685 Servo Controller | `I2C1` or `I2C2` | **TBD** (e.g. PB8/PB9 or PB10/PB11) | **PROVISIONAL** |
| **UART (Stepper)** | Stepper Motor Driver Comms | `USART1` or `USART3` | **TBD** | **PROVISIONAL** |
| **UART (BLE)** | Bluetooth Host Communications | `UART4` or `USART6` | **TBD** | **PROVISIONAL** |
| **Timer (Encoder)** | Lateral Encoder Quadrature Mode | `TIM2`, `TIM3`, or `TIM4` | **TBD** | **OPTIONAL (TBD)** |
| **UART (Debug)** | ST-Link Virtual COM Port Console | `USART2` | `PA2` (TX), `PA3` (RX) | **CONFIRMED** |

---

## 3. PCA9685 Servo Driver Wiring Requirements

The PCA9685 handles PWM generation for all 5 finger servos at 50 Hz update rate (20ms period).

- **Logic Level Power (`VCC`)**: Connect to Nucleo **3.3V** (or 5V if PCA9685 board supports 3.3V logic translation).
- **Servo Motor Power (`V+`)**: Connect strictly to external **5.0V regulated power supply** capable of supplying at least 3A continuous (5A peak for multi-finger chords). **NEVER power servo V+ from the Nucleo 5V / 3.3V pin.**
- **Ground (`GND`)**: Common ground between Nucleo, external 5V supply, and PCA9685.
- **I²C Bus Pull-ups**: Verify onboard 4.7kΩ pull-up resistors on SDA and SCL lines.
- **Output Enable (`OE`)**: Pulled low to enable outputs; can be tied to GND or connected to an STM32 GPIO for emergency actuator cut-off.

---

## 4. Stepper Driver Interface Requirements

- **Protocol**: Serial UART, 115,200 baud, 8 data bits, no parity, 1 stop bit (8N1).
- **Control Mode**: Native UART register configuration (e.g. TMC UART) or Step/Direction pulse generator synchronized with hardware timer interrupt.
- **Acceleration Profiling**: Trapezoidal or S-curve acceleration profile managed by the firmware to prevent carriage jerking, belt slippage, and step loss during fast lateral transitions.
- **Logic Level**: 3.3V UART signals from STM32. Ensure driver UART pin accepts 3.3V logic or incorporate level translation.

---

## 5. Power Supply Requirements & Grounding

1. **Isolation**:
   - High current inductive loads (stepper coils and servo motors) produce substantial inductive spikes and electrical noise.
   - Separate voltage regulators must be used for logic (Nucleo/STM32) and actuators (servos & stepper).
2. **Capacitance**:
   - Provide a large electrolytic capacitor (≥ 1000 µF, 16V) across the 5V servo V+ power terminals on the PCA9685 board.
   - Provide a 100 µF capacitor across the motor driver 12V/24V VMOT terminals.
3. **Common Star Ground**:
   - All DC grounds (Nucleo GND, 5V Servo PSU GND, 12V Stepper PSU GND) must join at a single star point to eliminate ground loops.

---

## 6. Safety & Physical Homing Specification

> [!CAUTION]
> **Physical Homing Requirement**:
> Software return-to-origin based on accumulated relative steps is **NOT** equivalent to physical homing. Limit switches or another physical reference mechanism are required for reliable physical homing.
>
> Open-loop step tracking accumulates mechanical errors, step losses during rapid acceleration, and manual perturbations while powered down. Attempting to home solely via accumulated step counts risks over-driving the carriage into mechanical endstops, causing belt stretching, gear stripping, or motor stall overcurrent.

### Safety Protocol:
1. **Power-On Initial State**: On reset, the carriage position is marked `UNCALIBRATED`.
2. **Homing Procedure**: The firmware drives the carriage at low velocity toward the home limit switch.
3. **Limit Trigger**: Upon limit switch assertion (falling edge on GPIO interrupt), carriage motion halts immediately, position counter resets to `0`, and the carriage backs off by a fixed calibration offset (e.g. 5mm).
4. **Emergency Stop (`STOP`)**: Immediately disables motor enable lines and centers finger servos.
