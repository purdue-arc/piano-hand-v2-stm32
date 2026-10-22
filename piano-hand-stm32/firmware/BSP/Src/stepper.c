/**
 * @file stepper.c
 * @brief Lateral Stepper Motor Driver Implementation
 * @author Embedded Systems Team
 * @license MIT
 *
 * Move completion is time-based. Actual physical position is not verified.
 * Key position is accumulated from commands and resets only on SET_HOME or ReturnToOrigin.
 */

#include "stepper.h"
#include "config.h"

#include <math.h>
#include <string.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */
static UART_HandleTypeDef *s_huart = NULL;
static PH_StepperState_t   s_state = STEPPER_IDLE;
static int32_t             s_key_position = 0;
static uint32_t            s_move_start_ms = 0;
static uint32_t            s_move_timeout_ms = 0;
static float               s_deg_per_key = STEPPER_DEG_PER_KEY;

/* ============================================================================
 * Checksum Utility
 * ============================================================================ */

uint8_t PH_Stepper_ComputeChecksum(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return 0;
    }
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum = (uint8_t)(sum + data[i]);
    }
    return sum;
}

/* ============================================================================
 * Internal Move Helper
 * ============================================================================ */

static PH_Status_t stepper_transmit_packet(float degrees)
{
    if (s_huart == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    if (s_state != STEPPER_IDLE) {
        return PH_ERR_BUSY;
    }

    float abs_deg = fabsf(degrees);
    uint32_t pulses = (uint32_t)roundf((abs_deg / 360.0f) * (float)STEPPER_PPR);

    if (pulses == 0) {
        return PH_OK; // No-op for 0 displacement
    }

    /* Speed byte: MSB 0x80 encodes negative (CCW) direction */
    uint8_t speed_byte = (uint8_t)STEPPER_SPEED_GEAR;
    if (degrees < 0.0f) {
        speed_byte |= 0x80;
    }

    uint8_t packet[8];
    packet[0] = STEPPER_ADDR;
    packet[1] = STEPPER_CMD;
    packet[2] = speed_byte;
    packet[3] = (uint8_t)((pulses >> 24) & 0xFF);
    packet[4] = (uint8_t)((pulses >> 16) & 0xFF);
    packet[5] = (uint8_t)((pulses >> 8) & 0xFF);
    packet[6] = (uint8_t)(pulses & 0xFF);
    packet[7] = PH_Stepper_ComputeChecksum(packet, 7);

    s_state = STEPPER_TRANSMITTING;
    HAL_StatusTypeDef hal_st = HAL_UART_Transmit(s_huart, packet, 8, 50);
    if (hal_st != HAL_OK) {
        s_state = STEPPER_ERROR;
        return PH_ERR_UART;
    }

    /* Calculate time-based move timeout:
     * rpm = STEPPER_SPEED_GEAR * 30000.0f / (16.0f * 200.0f) (MSTEP=16)
     * rotations = abs_deg / 360.0f
     * theoretical_ms = (rotations / rpm) * 60000.0f
     * timeout_ms = (theoretical_ms * STEPPER_TIMEOUT_FACTOR) + STEPPER_TIMEOUT_MARGIN_MS
     */
    float rpm = (float)STEPPER_SPEED_GEAR * 30000.0f / (16.0f * 200.0f);
    float rotations = abs_deg / 360.0f;
    uint32_t theoretical_ms = (uint32_t)((rotations / rpm) * 60000.0f);
    uint32_t timeout_ms = (uint32_t)((float)theoretical_ms * STEPPER_TIMEOUT_FACTOR) +
                          STEPPER_TIMEOUT_MARGIN_MS;

    s_move_start_ms = HAL_GetTick();
    s_move_timeout_ms = timeout_ms;
    s_state = STEPPER_WAITING;

    return PH_OK;
}

/* ============================================================================
 * Public Driver API
 * ============================================================================ */

PH_Status_t PH_Stepper_Init(UART_HandleTypeDef *huart)
{
    if (huart == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    s_huart = huart;
    s_state = STEPPER_IDLE;
    s_key_position = 0;
    s_move_start_ms = 0;
    s_move_timeout_ms = 0;
    s_deg_per_key = STEPPER_DEG_PER_KEY;
    return PH_OK;
}

PH_Status_t PH_Stepper_MoveRelative(int32_t key_steps)
{
    if (s_state != STEPPER_IDLE) {
        return PH_ERR_BUSY;
    }

    float degrees = (float)key_steps * s_deg_per_key;
    PH_Status_t st = stepper_transmit_packet(degrees);
    if (st == PH_OK && key_steps != 0) {
        s_key_position += key_steps;
    }
    return st;
}

PH_Status_t PH_Stepper_MoveDegrees(float degrees)
{
    /* Used for micro-stepping calibration jog; does not alter accumulated integer key_position */
    return stepper_transmit_packet(degrees);
}

PH_Status_t PH_Stepper_ReturnToOrigin(void)
{
    if (s_key_position == 0) {
        return PH_OK;
    }
    int32_t delta = -s_key_position;
    PH_Status_t st = PH_Stepper_MoveRelative(delta);
    /* Pre-update key_position immediately to 0 per specification */
    s_key_position = 0;
    return st;
}

void PH_Stepper_Update(uint32_t now_ms)
{
    if (s_state == STEPPER_WAITING) {
        if ((now_ms - s_move_start_ms) >= s_move_timeout_ms) {
            s_state = STEPPER_COMPLETE;
            s_state = STEPPER_IDLE;
        }
    }
}

bool PH_Stepper_IsMoving(void)
{
    return (s_state == STEPPER_TRANSMITTING || s_state == STEPPER_WAITING);
}

PH_StepperState_t PH_Stepper_GetState(void)
{
    return s_state;
}

int32_t PH_Stepper_GetKeyPosition(void)
{
    return s_key_position;
}

void PH_Stepper_SetHome(void)
{
    s_key_position = 0;
}

void PH_Stepper_EmergencyStop(void)
{
    s_state = STEPPER_IDLE;
}

void PH_Stepper_SetDegPerKey(float deg)
{
    s_deg_per_key = deg;
}

float PH_Stepper_GetDegPerKey(void)
{
    return s_deg_per_key;
}
