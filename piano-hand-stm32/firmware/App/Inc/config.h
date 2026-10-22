/**
 * @file config.h
 * @brief Piano Hand STM32 Firmware Central Configuration
 * @author Embedded Systems Team
 * @license MIT
 *
 * IMPORTANT STATUS NOTES:
 * - Confirmed parameters (from legacy ESP32 MicroPython implementation):
 *     - PCA9685 I2C address: 0x40
 *     - PWM frequency: 50.0 Hz (20ms period, 4096 counts)
 *     - Servo channels: [0, 1, 2, 3, 4] for fingers [1, 2, 3, 4, 5]
 *     - Servo pulse width range: 500 us (min) to 2500 us (max)
 *     - Stepper address byte: 0xE0
 *     - Stepper command byte: 0xFD
 *     - Stepper speed gear: 80
 *     - Stepper pulses per revolution (PPR): 3200 (at 16 microsteps)
 *     - Stepper UART baud rate: 115200 bps
 * - Provisional parameters (subject to mechanical piano testing):
 *     - Servo press angle: 125.0 deg (needs individual acoustic/touch tuning)
 *     - Servo release angle: 0.0 deg
 *     - Stepper degrees per key: 30.0 deg (signed float; sign encodes direction)
 *     - Stepper timeout factor: 1.4x theoretical + 100ms margin
 *     - Encoder: disabled (0) pending hardware verification and pinout
 */

#ifndef PH_CONFIG_H
#define PH_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Hand Identity Configuration
 * ============================================================================ */
#define HAND_IDENTITY_LEFT      0
#define HAND_IDENTITY_RIGHT     1
#define HAND_IDENTITY           HAND_IDENTITY_LEFT

/* ============================================================================
 * PCA9685 PWM Driver Configuration
 * ============================================================================ */
#define PCA9685_I2C_ADDR        0x40
#define PCA9685_PWM_FREQ_HZ     50.0f
#define SERVO_COUNT             5
#define SERVO_MIN_PULSE_US      500
#define SERVO_MAX_PULSE_US      2500
#define SERVO_PRESS_ANGLE_DEG   125.0f
#define SERVO_RELEASE_ANGLE_DEG 0.0f

/**
 * Per-finger PCA9685 channel mapping [finger 1 .. finger 5]
 */
#define SERVO_CHANNELS          {0, 1, 2, 3, 4}

/* ============================================================================
 * Lateral Stepper Motor Driver Configuration
 * ============================================================================ */
#define STEPPER_ADDR            0xE0
#define STEPPER_CMD             0xFD
#define STEPPER_SPEED_GEAR      80
#define STEPPER_PPR             3200
#define STEPPER_DEG_PER_KEY     30.0f
#define STEPPER_UART_BAUD       115200

/**
 * Estimated move timeout calculation factor and margin.
 * Timeout = (theoretical_ms * STEPPER_TIMEOUT_FACTOR) + STEPPER_TIMEOUT_MARGIN_MS
 */
#define STEPPER_TIMEOUT_FACTOR    1.4f
#define STEPPER_TIMEOUT_MARGIN_MS 100

/* ============================================================================
 * Command Protocol & Ring Buffer Queue Configuration
 * ============================================================================ */
#define CMD_QUEUE_DEPTH         32
#define CMD_MAX_LEN             64

/* ============================================================================
 * Optional Optical/Magnetic Quadrature Encoder Configuration
 * ============================================================================ */
#define ENCODER_ENABLED         0

#ifdef __cplusplus
}
#endif

#endif /* PH_CONFIG_H */
