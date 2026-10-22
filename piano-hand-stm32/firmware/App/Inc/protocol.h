/**
 * @file protocol.h
 * @brief Piano Hand Text Command Protocol & Bounded Queue Definitions
 * @author Embedded Systems Team
 * @license MIT
 *
 * NOTE: Pure C module with zero HAL dependencies. Designed for testability
 * on host environments as well as on-target STM32 execution.
 */

#ifndef PH_PROTOCOL_H
#define PH_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Return and Status Codes
 * ============================================================================ */
typedef enum {
    PH_OK = 0,
    PH_ERR_INVALID_ARG,
    PH_ERR_I2C,
    PH_ERR_UART,
    PH_ERR_BUSY,
    PH_ERR_TIMEOUT,
    PH_ERR_NOT_SUPPORTED,
    PH_ERR_QUEUE_FULL,
    PH_ERR_QUEUE_EMPTY,
    PH_ERR_UNKNOWN_CMD,
    PH_ERR_PARSE,
} PH_Status_t;

/* ============================================================================
 * Protocol Command Types
 * ============================================================================ */
typedef enum {
    CMD_STEP,
    CMD_SERVO_ON,
    CMD_SERVO_OFF,
    CMD_RESET,
    CMD_STOP,
    CMD_STATUS,
    CMD_CAL_JOG,
    CMD_CAL_SET_HOME,
    CMD_CAL_SET_STEP,
    CMD_CAL_GOTO,
    CMD_UNKNOWN
} PH_CmdType_t;

/* ============================================================================
 * Parsed Command Representation
 * ============================================================================ */
typedef struct {
    PH_CmdType_t type;
    int32_t      step_value;   // signed key steps for STEP/CAL_GOTO; degrees*1000 for CAL_JOG stored as int
    float        float_value;  // for CAL_SET_STEP and CAL_JOG degrees
    uint8_t      finger_index; // 1-5 for SERVO_ON/OFF
} PH_Command_t;

/* ============================================================================
 * Parse Result Codes
 * ============================================================================ */
typedef enum {
    PARSE_OK,
    PARSE_ERR_EMPTY,
    PARSE_ERR_UNKNOWN_CMD,
    PARSE_ERR_INVALID_ARG,
    PARSE_ERR_OUT_OF_RANGE,
} PH_ParseResult_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/**
 * @brief Parse a raw protocol string into a PH_Command_t struct.
 * @param[in]  raw Null-terminated raw command string.
 * @param[out] out Destination pointer for the parsed command.
 * @return PARSE_OK on success, or appropriate error code.
 */
PH_ParseResult_t PH_Protocol_Parse(const char *raw, PH_Command_t *out);

/**
 * @brief Enqueue a command into the bounded ring buffer.
 * @param[in] cmd Command to enqueue.
 * @return PH_OK if queued, PH_ERR_QUEUE_FULL if queue is at capacity.
 */
PH_Status_t      PH_Protocol_Enqueue(const PH_Command_t *cmd);

/**
 * @brief Dequeue the oldest command from the bounded ring buffer (FIFO).
 * @param[out] out Destination buffer for dequeued command.
 * @return PH_OK if dequeued, PH_ERR_QUEUE_EMPTY if queue has no entries.
 */
PH_Status_t      PH_Protocol_Dequeue(PH_Command_t *out);

/**
 * @brief Peek at the oldest command in the queue without removing it.
 * @param[out] out Destination buffer for peeked command.
 * @return PH_OK if peeked, PH_ERR_QUEUE_EMPTY if queue has no entries.
 */
PH_Status_t      PH_Protocol_Peek(PH_Command_t *out);

/**
 * @brief Get the current number of elements in the command queue.
 * @return Number of queued commands (0 to CMD_QUEUE_DEPTH).
 */
int              PH_Protocol_QueueDepth(void);

/**
 * @brief Clear and reset the command queue ring buffer pointers.
 */
void             PH_Protocol_QueueReset(void);

/**
 * @brief Return human-readable string representation of a parse result.
 */
const char      *PH_Protocol_ParseResultStr(PH_ParseResult_t r);

/**
 * @brief Return human-readable string representation of a command type.
 */
const char      *PH_Protocol_CmdTypeStr(PH_CmdType_t t);

#ifdef __cplusplus
}
#endif

#endif /* PH_PROTOCOL_H */
