/**
 * @file servo.h
 * @brief PCA9685 16-Channel 12-Bit PWM Driver for Piano Hand Finger Servos
 * @author Embedded Systems Team
 * @license MIT
 *
 * Provides control for 5 finger servos via I2C interface to PCA9685.
 * Angles map to pulse lengths (500us–2500us) at 50Hz (20ms, 4096 counts).
 */

#ifndef PH_SERVO_H
#define PH_SERVO_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

#ifdef UNIT_TEST
  #include "mocks/hal_mock.h"
#else
  #include "stm32f4xx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize PCA9685 with target I2C handle and set default 50Hz PWM frequency.
 * @param[in] hi2c Initialized STM32 HAL I2C handle.
 * @return PH_OK on success, PH_ERR_INVALID_ARG or PH_ERR_I2C on failure.
 */
PH_Status_t PH_Servo_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Set the output PWM frequency of the PCA9685 driver.
 * @param[in] freq_hz Frequency in Hertz (typically 50.0 Hz).
 * @return PH_OK on success, or PH_ERR_I2C.
 */
PH_Status_t PH_Servo_SetPWMFreq(float freq_hz);

/**
 * @brief Directly write ON and OFF 12-bit tick counts to a specific PCA9685 channel.
 * @param[in] channel Channel index (0-15).
 * @param[in] on_count Tick count (0-4095) for output HIGH transition.
 * @param[in] off_count Tick count (0-4095) for output LOW transition.
 * @return PH_OK on success, or PH_ERR_I2C.
 */
PH_Status_t PH_Servo_SetPWM(uint8_t channel, uint16_t on_count, uint16_t off_count);

/**
 * @brief Command a specific PCA9685 channel to an angular position in degrees.
 * Clamped between 0.0 deg (500 us) and 180.0 deg (2500 us).
 * @param[in] channel Channel index (0-15).
 * @param[in] angle_deg Angular position (0.0 to 180.0 degrees).
 * @return PH_OK on success, or error code.
 */
PH_Status_t PH_Servo_SetAngle(uint8_t channel, float angle_deg);

/**
 * @brief Actuate fingers based on a 5-bit mask (bits 0-4 correspond to fingers 1-5).
 * Set bit = press; cleared bit = release.
 * @param[in] finger_mask Bitmask of desired finger press states.
 * @return PH_OK on success, or error code.
 */
PH_Status_t PH_Servo_PressAll(uint8_t finger_mask);

/**
 * @brief Press a specific finger (1-based: 1 to SERVO_COUNT).
 * @param[in] finger_index Finger index (1 to 5).
 * @return PH_OK on success, PH_ERR_INVALID_ARG if out of range, or PH_ERR_I2C.
 */
PH_Status_t PH_Servo_Press(uint8_t finger_index);

/**
 * @brief Release a specific finger (1-based: 1 to SERVO_COUNT).
 * @param[in] finger_index Finger index (1 to 5).
 * @return PH_OK on success, PH_ERR_INVALID_ARG if out of range, or PH_ERR_I2C.
 */
PH_Status_t PH_Servo_Release(uint8_t finger_index);

/**
 * @brief Release all fingers to default rest position (SERVO_RELEASE_ANGLE_DEG).
 * @return PH_OK on success, or error code.
 */
PH_Status_t PH_Servo_ReleaseAll(void);

#ifdef __cplusplus
}
#endif

#endif /* PH_SERVO_H */
