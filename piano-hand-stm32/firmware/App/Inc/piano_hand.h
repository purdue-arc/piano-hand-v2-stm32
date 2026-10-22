/**
 * @file piano_hand.h
 * @brief Piano Hand STM32 Main Application Controller Interface
 * @author Embedded Systems Team
 * @license MIT
 *
 * Coordinates servo finger actuation, lateral stepper movement, command
 * parsing, and state management in a cooperative non-blocking superloop.
 */

#ifndef PH_PIANO_HAND_H
#define PH_PIANO_HAND_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Application State
 * ============================================================================ */
typedef enum {
    PH_STATE_IDLE,
    PH_STATE_MOVING,
    PH_STATE_PLAYING,
    PH_STATE_HOMING,
    PH_STATE_ERROR,
} PH_State_t;

/* ============================================================================
 * Hardware Peripheral Handles
 * ============================================================================ */
typedef struct {
    void *i2c_handle;   // I2C_HandleTypeDef* for PCA9685
    void *uart_stepper; // UART_HandleTypeDef* for stepper motor driver
    void *uart_comms;   // UART_HandleTypeDef* for incoming text commands & telemetry
    void *tim_encoder;  // TIM_HandleTypeDef* (NULL if not used / ENCODER_ENABLED=0)
} PH_HardwareHandles_t;

/* ============================================================================
 * Application Controller API
 * ============================================================================ */

/**
 * @brief Initialize all BSP peripherals, queues, and reset hand to initial state.
 * @param[in] hw Hardware handle pointers initialized by STM32CubeMX.
 */
void         PH_Init(const PH_HardwareHandles_t *hw);

/**
 * @brief Primary superloop tick function. Must be called repeatedly from main().
 * @param[in] now_ms Current system timestamp in milliseconds (e.g., HAL_GetTick()).
 */
void         PH_Update(uint32_t now_ms);

/**
 * @brief Get the current high-level state of the application.
 */
PH_State_t   PH_GetState(void);

/**
 * @brief Trigger an immediate emergency stop across all actuators.
 * Releases all servo fingers, halts stepper motion, and flushes command queue.
 */
void         PH_EmergencyStop(void);

/**
 * @brief Convert application state enum to human-readable string.
 */
const char  *PH_StateStr(PH_State_t s);

#ifdef __cplusplus
}
#endif

#endif /* PH_PIANO_HAND_H */
