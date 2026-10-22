/**
 * @file piano_hand.c
 * @brief Piano Hand STM32 Main Application Controller Implementation
 * @author Embedded Systems Team
 * @license MIT
 *
 * Implements non-blocking dispatch and supervisory control for the Piano Hand.
 */

#include "piano_hand.h"
#include "config.h"
#include "servo.h"
#include "stepper.h"
#include "transport.h"
#include "encoder.h"

#include <stdio.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */
static PH_State_t s_app_state = PH_STATE_IDLE;
static uint8_t    s_active_fingers_mask = 0; // bit (finger-1) is 1 when pressed

/* ============================================================================
 * Public Application Controller Functions
 * ============================================================================ */

void PH_Init(const PH_HardwareHandles_t *hw)
{
    s_app_state = PH_STATE_IDLE;
    s_active_fingers_mask = 0;

    PH_Protocol_QueueReset();

    if (hw != NULL) {
        if (hw->i2c_handle != NULL) {
            PH_Servo_Init((I2C_HandleTypeDef *)hw->i2c_handle);
        }
        if (hw->uart_stepper != NULL) {
            PH_Stepper_Init((UART_HandleTypeDef *)hw->uart_stepper);
        }
        if (hw->uart_comms != NULL) {
            PH_Transport_Init((UART_HandleTypeDef *)hw->uart_comms);
        }
#if ENCODER_ENABLED
        if (hw->tim_encoder != NULL) {
            PH_Encoder_Init((TIM_HandleTypeDef *)hw->tim_encoder);
        }
#endif
    }

    PH_Servo_ReleaseAll();
    PH_Transport_Send("READY");
}

void PH_Update(uint32_t now_ms)
{
    /* 1. Poll transport for incoming UART characters */
    PH_Transport_Poll();

    /* 2. Tick the stepper motor timing state machine */
    PH_Stepper_Update(now_ms);

    /* 3. Check for emergency stop or command dispatch */
    if (PH_Protocol_QueueDepth() > 0) {
        PH_Command_t next_cmd;
        if (PH_Protocol_Peek(&next_cmd) == PH_OK) {
            /* Priority 1: STOP command always interrupts immediately */
            if (next_cmd.type == CMD_STOP) {
                PH_Protocol_Dequeue(&next_cmd);
                PH_EmergencyStop();
                PH_Transport_Send("OK STOP");
            }
            /* Priority 2: RESET can clear ERROR state or reset from IDLE/MOVING */
            else if (next_cmd.type == CMD_RESET) {
                PH_Protocol_Dequeue(&next_cmd);
                PH_Servo_ReleaseAll();
                s_active_fingers_mask = 0;
                PH_Stepper_ReturnToOrigin();
                s_app_state = PH_STATE_HOMING;
                PH_Transport_Send("OK RESET");
            }
            /* In ERROR state, drop other commands until RESET is sent */
            else if (s_app_state == PH_STATE_ERROR) {
                PH_Protocol_Dequeue(&next_cmd);
                PH_Transport_Send("ERR IN_ERROR_STATE");
            }
            /* Process commands when stepper is not busy moving */
            else if (!PH_Stepper_IsMoving()) {
                PH_Command_t cmd;
                if (PH_Protocol_Dequeue(&cmd) == PH_OK) {
                    switch (cmd.type) {
                        case CMD_STEP:
                            if (PH_Stepper_MoveRelative(cmd.step_value) == PH_OK) {
                                s_app_state = PH_STATE_MOVING;
                                PH_Transport_Sendf("OK STEP %ld", (long)cmd.step_value);
                            } else {
                                PH_Transport_Send("ERR STEP_BUSY");
                            }
                            break;

                        case CMD_SERVO_ON:
                            if (PH_Servo_Press(cmd.finger_index) == PH_OK) {
                                s_active_fingers_mask |= (1 << (cmd.finger_index - 1));
                                PH_Transport_Sendf("OK SERVO_ON %u", cmd.finger_index);
                            } else {
                                PH_Transport_Send("ERR SERVO_I2C");
                            }
                            break;

                        case CMD_SERVO_OFF:
                            if (PH_Servo_Release(cmd.finger_index) == PH_OK) {
                                s_active_fingers_mask &= ~(1 << (cmd.finger_index - 1));
                                PH_Transport_Sendf("OK SERVO_OFF %u", cmd.finger_index);
                            } else {
                                PH_Transport_Send("ERR SERVO_I2C");
                            }
                            break;

                        case CMD_STATUS:
                            PH_Transport_Sendf("STATUS state=%s key_pos=%ld moving=%d active_fingers=0x%02X",
                                               PH_StateStr(s_app_state),
                                               (long)PH_Stepper_GetKeyPosition(),
                                               PH_Stepper_IsMoving() ? 1 : 0,
                                               s_active_fingers_mask);
                            break;

                        case CMD_CAL_JOG:
                            if (PH_Stepper_MoveDegrees(cmd.float_value) == PH_OK) {
                                s_app_state = PH_STATE_MOVING;
                                PH_Transport_Sendf("OK CAL_JOG %.2f", cmd.float_value);
                            } else {
                                PH_Transport_Send("ERR JOG_BUSY");
                            }
                            break;

                        case CMD_CAL_SET_HOME:
                            PH_Stepper_SetHome();
                            PH_Transport_Send("OK CAL_SET_HOME");
                            break;

                        case CMD_CAL_SET_STEP:
                            PH_Stepper_SetDegPerKey(cmd.float_value);
                            PH_Transport_Sendf("OK CAL_SET_STEP %.2f", cmd.float_value);
                            break;

                        case CMD_CAL_GOTO: {
                            int32_t current_pos = PH_Stepper_GetKeyPosition();
                            int32_t delta = cmd.step_value - current_pos;
                            if (PH_Stepper_MoveRelative(delta) == PH_OK) {
                                s_app_state = PH_STATE_MOVING;
                                PH_Transport_Sendf("OK CAL_GOTO %ld", (long)cmd.step_value);
                            } else {
                                PH_Transport_Send("ERR GOTO_BUSY");
                            }
                            break;
                        }

                        default:
                            PH_Transport_Send("ERR UNKNOWN");
                            break;
                    }
                }
            }
        }
    }

    /* 4. Update overall application state */
    if (s_app_state != PH_STATE_ERROR) {
        if (PH_Stepper_IsMoving()) {
            if (s_app_state != PH_STATE_HOMING) {
                s_app_state = PH_STATE_MOVING;
            }
        } else {
            if (s_active_fingers_mask != 0) {
                s_app_state = PH_STATE_PLAYING;
            } else {
                s_app_state = PH_STATE_IDLE;
            }
        }
    }
}

PH_State_t PH_GetState(void)
{
    return s_app_state;
}

void PH_EmergencyStop(void)
{
    s_app_state = PH_STATE_ERROR;
    s_active_fingers_mask = 0;
    PH_Stepper_EmergencyStop();
    PH_Servo_ReleaseAll();
    PH_Protocol_QueueReset();
}

const char *PH_StateStr(PH_State_t s)
{
    switch (s) {
        case PH_STATE_IDLE:    return "IDLE";
        case PH_STATE_MOVING:  return "MOVING";
        case PH_STATE_PLAYING: return "PLAYING";
        case PH_STATE_HOMING:  return "HOMING";
        case PH_STATE_ERROR:   return "ERROR";
        default:               return "UNKNOWN";
    }
}
