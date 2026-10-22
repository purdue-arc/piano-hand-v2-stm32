/**
 * @file stepper.h
 * @brief Lateral Stepper Motor Driver Interface via UART Command Packets
 * @author Embedded Systems Team
 * @license MIT
 *
 * NOTE: Completion detection is time-based only. Hardware acknowledgment
 * protocol is not yet verified. Do not rely on GetKeyPosition for absolute precision.
 * Actual position is not verified. Key position is accumulated from commands and
 * resets only on SET_HOME or ReturnToOrigin.
 */

#ifndef PH_STEPPER_H
#define PH_STEPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "protocol.h"

#ifdef UNIT_TEST
  #include "mocks/hal_mock.h"
#else
  #include "stm32f4xx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Stepper Motion State Machine
 * ============================================================================ */
typedef enum {
    STEPPER_IDLE,
    STEPPER_TRANSMITTING,
    STEPPER_WAITING,
    STEPPER_COMPLETE,
    STEPPER_ERROR,
} PH_StepperState_t;

/* ============================================================================
 * Public Driver API
 * ============================================================================ */

/**
 * @brief Initialize the stepper driver with an active UART handle.
 * @param[in] huart Initialized STM32 HAL UART handle.
 * @return PH_OK on success, PH_ERR_INVALID_ARG if huart is NULL.
 */
PH_Status_t       PH_Stepper_Init(UART_HandleTypeDef *huart);

/**
 * @brief Command a relative move by an integer number of piano key positions.
 * @param[in] key_steps Signed key delta (positive or negative).
 * @return PH_OK if command accepted and transmitted, PH_ERR_BUSY if already moving.
 */
PH_Status_t       PH_Stepper_MoveRelative(int32_t key_steps);

/**
 * @brief Command a move by an arbitrary angular displacement in degrees.
 * Used primarily for micro-stepping calibration (CAL:JOG).
 * @param[in] degrees Signed angular displacement in degrees.
 * @return PH_OK if accepted and transmitted, PH_ERR_BUSY if moving.
 */
PH_Status_t       PH_Stepper_MoveDegrees(float degrees);

/**
 * @brief Command the stepper to return to key position 0.
 * Pre-updates accumulated key_position to 0.
 * @return PH_OK if move initiated or already at 0.
 */
PH_Status_t       PH_Stepper_ReturnToOrigin(void);

/**
 * @brief Non-blocking state machine tick. Evaluates time-based move completion.
 * Must be called repeatedly from superloop.
 * @param[in] now_ms Current system timestamp in milliseconds.
 */
void              PH_Stepper_Update(uint32_t now_ms);

/**
 * @brief Check whether the stepper is currently executing a motion.
 * @return true if transmitting or waiting for move completion, false if idle.
 */
bool              PH_Stepper_IsMoving(void);

/**
 * @brief Query current state machine status.
 */
PH_StepperState_t PH_Stepper_GetState(void);

/**
 * @brief Get the accumulated key index relative to home.
 * @return Signed key position (0 = origin/home).
 */
int32_t           PH_Stepper_GetKeyPosition(void);

/**
 * @brief Reset accumulated key position to 0 (sets current physical location as home).
 */
void              PH_Stepper_SetHome(void);

/**
 * @brief Stop motion immediately and transition driver to STEPPER_IDLE.
 */
void              PH_Stepper_EmergencyStop(void);

/**
 * @brief Set calibration value for degrees per piano key.
 */
void              PH_Stepper_SetDegPerKey(float deg);

/**
 * @brief Get current calibration value for degrees per piano key.
 */
float             PH_Stepper_GetDegPerKey(void);

/**
 * @brief Compute 8-bit checksum for packet transmission.
 * Checksum is the sum of all preceding bytes masked to 8 bits.
 */
uint8_t           PH_Stepper_ComputeChecksum(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* PH_STEPPER_H */
