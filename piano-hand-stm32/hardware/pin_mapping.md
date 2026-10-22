# STM32 Nucleo-F446RE Pin Mapping

> [!WARNING]
> All pin assignments are PROVISIONAL. Finalize after schematic review.

## I²C — PCA9685 Servo Driver
| Signal | STM32 Pin | Notes |
|--------|-----------|-------|
| SCL    | TBD       | I2C1 or I2C2 |
| SDA    | TBD       | |
| VCC    | 3.3V or 5V | Check PCA9685 logic level |
| GND    | GND       | |

## UART — Stepper Motor Driver
| Signal | STM32 Pin | Notes |
|--------|-----------|-------|
| TX     | TBD       | UART1 or UART2 |
| RX     | TBD       | Driver may not send responses |

## UART — BLE Module
| Signal | STM32 Pin | Notes |
|--------|-----------|-------|
| TX     | TBD       | BLE module model TBD |
| RX     | TBD       | |

## Timer — Encoder (Optional)
| Signal | STM32 Pin | Notes |
|--------|-----------|-------|
| A      | TBD       | Timer encoder mode. ENCODER_ENABLED=0 until wired. |
| B      | TBD       | |

## STM32 Default Debug UART
| Signal | STM32 Pin | Notes |
|--------|-----------|-------|
| TX     | PA2       | USART2, connected to ST-Link on Nucleo |
| RX     | PA3       | Default Nucleo debug serial |
