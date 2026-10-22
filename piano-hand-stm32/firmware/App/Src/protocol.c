/**
 * @file protocol.c
 * @brief Piano Hand Text Command Protocol & Bounded Queue Implementation
 * @author Embedded Systems Team
 * @license MIT
 *
 * Implements line-based command parsing and a fixed-size ring buffer for
 * command queuing. No dynamic memory allocation is used.
 */

#include "protocol.h"
#include "config.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * Internal Static Storage for Command Queue
 * ============================================================================ */
static PH_Command_t s_cmd_queue[CMD_QUEUE_DEPTH];
static int          s_head = 0;
static int          s_tail = 0;
static int          s_count = 0;

/* ============================================================================
 * Case-Insensitive String Utilities
 * ============================================================================ */
static int ph_strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static int ph_strncasecmp(const char *s1, const char *s2, size_t n)
{
    while (n && *s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/* ============================================================================
 * Protocol Parsing Implementation
 * ============================================================================ */

PH_ParseResult_t PH_Protocol_Parse(const char *raw, PH_Command_t *out)
{
    if (raw == NULL || out == NULL) {
        return PARSE_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(PH_Command_t));
    out->type = CMD_UNKNOWN;

    /* 1. Strip leading whitespace */
    while (*raw && isspace((unsigned char)*raw)) {
        raw++;
    }

    if (*raw == '\0') {
        return PARSE_ERR_EMPTY;
    }

    /* 2. Copy and strip trailing whitespace into a local stack buffer */
    const char *end = raw + strlen(raw) - 1;
    while (end > raw && isspace((unsigned char)*end)) {
        end--;
    }

    size_t len = (size_t)(end - raw + 1);
    if (len >= CMD_MAX_LEN) {
        return PARSE_ERR_INVALID_ARG;
    }

    char buf[CMD_MAX_LEN];
    memcpy(buf, raw, len);
    buf[len] = '\0';

    /* 3. Strip optional timestamp prefix: [0-9]+: */
    const char *cmd_start = buf;
    const char *first_colon = strchr(buf, ':');
    if (first_colon != NULL && first_colon > buf) {
        bool is_timestamp = true;
        for (const char *p = buf; p < first_colon; p++) {
            if (!isdigit((unsigned char)*p)) {
                is_timestamp = false;
                break;
            }
        }
        if (is_timestamp) {
            cmd_start = first_colon + 1;
            while (*cmd_start && isspace((unsigned char)*cmd_start)) {
                cmd_start++;
            }
            if (*cmd_start == '\0') {
                return PARSE_ERR_EMPTY;
            }
        }
    }

    /* 4. Match single-word commands (case-insensitive) */
    if (ph_strcasecmp(cmd_start, "RESET") == 0) {
        out->type = CMD_RESET;
        return PARSE_OK;
    }
    if (ph_strcasecmp(cmd_start, "STOP") == 0) {
        out->type = CMD_STOP;
        return PARSE_OK;
    }
    if (ph_strcasecmp(cmd_start, "STATUS") == 0) {
        out->type = CMD_STATUS;
        return PARSE_OK;
    }
    if (ph_strcasecmp(cmd_start, "CAL:SET_HOME") == 0) {
        out->type = CMD_CAL_SET_HOME;
        return PARSE_OK;
    }

    /* 5. Match parameterized commands: command:value */
    if (ph_strncasecmp(cmd_start, "step:", 5) == 0) {
        const char *val_str = cmd_start + 5;
        char *endptr = NULL;
        long val = strtol(val_str, &endptr, 10);
        while (endptr && *endptr && isspace((unsigned char)*endptr)) {
            endptr++;
        }
        if (endptr == val_str || *endptr != '\0') {
            return PARSE_ERR_INVALID_ARG;
        }
        out->type = CMD_STEP;
        out->step_value = (int32_t)val;
        return PARSE_OK;
    }

    if (ph_strncasecmp(cmd_start, "servo_on:", 9) == 0) {
        const char *val_str = cmd_start + 9;
        char *endptr = NULL;
        long val = strtol(val_str, &endptr, 10);
        while (endptr && *endptr && isspace((unsigned char)*endptr)) {
            endptr++;
        }
        if (endptr == val_str || *endptr != '\0') {
            return PARSE_ERR_INVALID_ARG;
        }
        if (val < 1 || val > SERVO_COUNT) {
            return PARSE_ERR_OUT_OF_RANGE;
        }
        out->type = CMD_SERVO_ON;
        out->finger_index = (uint8_t)val;
        return PARSE_OK;
    }

    if (ph_strncasecmp(cmd_start, "servo_off:", 10) == 0) {
        const char *val_str = cmd_start + 10;
        char *endptr = NULL;
        long val = strtol(val_str, &endptr, 10);
        while (endptr && *endptr && isspace((unsigned char)*endptr)) {
            endptr++;
        }
        if (endptr == val_str || *endptr != '\0') {
            return PARSE_ERR_INVALID_ARG;
        }
        if (val < 1 || val > SERVO_COUNT) {
            return PARSE_ERR_OUT_OF_RANGE;
        }
        out->type = CMD_SERVO_OFF;
        out->finger_index = (uint8_t)val;
        return PARSE_OK;
    }

    if (ph_strncasecmp(cmd_start, "CAL:JOG:", 8) == 0) {
        const char *val_str = cmd_start + 8;
        char *endptr = NULL;
        float val = strtof(val_str, &endptr);
        while (endptr && *endptr && isspace((unsigned char)*endptr)) {
            endptr++;
        }
        if (endptr == val_str || *endptr != '\0') {
            return PARSE_ERR_INVALID_ARG;
        }
        out->type = CMD_CAL_JOG;
        out->float_value = val;
        out->step_value = (int32_t)(val * 1000.0f);
        return PARSE_OK;
    }

    if (ph_strncasecmp(cmd_start, "CAL:SET_STEP:", 13) == 0) {
        const char *val_str = cmd_start + 13;
        char *endptr = NULL;
        float val = strtof(val_str, &endptr);
        while (endptr && *endptr && isspace((unsigned char)*endptr)) {
            endptr++;
        }
        if (endptr == val_str || *endptr != '\0') {
            return PARSE_ERR_INVALID_ARG;
        }
        out->type = CMD_CAL_SET_STEP;
        out->float_value = val;
        return PARSE_OK;
    }

    if (ph_strncasecmp(cmd_start, "CAL:GOTO:", 9) == 0) {
        const char *val_str = cmd_start + 9;
        char *endptr = NULL;
        long val = strtol(val_str, &endptr, 10);
        while (endptr && *endptr && isspace((unsigned char)*endptr)) {
            endptr++;
        }
        if (endptr == val_str || *endptr != '\0') {
            return PARSE_ERR_INVALID_ARG;
        }
        out->type = CMD_CAL_GOTO;
        out->step_value = (int32_t)val;
        return PARSE_OK;
    }

    /* Check for commands provided without their required colon/argument */
    if (ph_strcasecmp(cmd_start, "step") == 0 ||
        ph_strcasecmp(cmd_start, "servo_on") == 0 ||
        ph_strcasecmp(cmd_start, "servo_off") == 0 ||
        ph_strcasecmp(cmd_start, "CAL:JOG") == 0 ||
        ph_strcasecmp(cmd_start, "CAL:SET_STEP") == 0 ||
        ph_strcasecmp(cmd_start, "CAL:GOTO") == 0) {
        return PARSE_ERR_INVALID_ARG;
    }

    return PARSE_ERR_UNKNOWN_CMD;
}

/* ============================================================================
 * Command Queue Implementation
 * ============================================================================ */

PH_Status_t PH_Protocol_Enqueue(const PH_Command_t *cmd)
{
    if (cmd == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    if (s_count >= CMD_QUEUE_DEPTH) {
        return PH_ERR_QUEUE_FULL;
    }
    s_cmd_queue[s_head] = *cmd;
    s_head = (s_head + 1) % CMD_QUEUE_DEPTH;
    s_count++;
    return PH_OK;
}

PH_Status_t PH_Protocol_Dequeue(PH_Command_t *out)
{
    if (out == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    if (s_count <= 0) {
        return PH_ERR_QUEUE_EMPTY;
    }
    *out = s_cmd_queue[s_tail];
    s_tail = (s_tail + 1) % CMD_QUEUE_DEPTH;
    s_count--;
    return PH_OK;
}

PH_Status_t PH_Protocol_Peek(PH_Command_t *out)
{
    if (out == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    if (s_count <= 0) {
        return PH_ERR_QUEUE_EMPTY;
    }
    *out = s_cmd_queue[s_tail];
    return PH_OK;
}

int PH_Protocol_QueueDepth(void)
{
    return s_count;
}

void PH_Protocol_QueueReset(void)
{
    s_head = 0;
    s_tail = 0;
    s_count = 0;
}

const char *PH_Protocol_ParseResultStr(PH_ParseResult_t r)
{
    switch (r) {
        case PARSE_OK:              return "OK";
        case PARSE_ERR_EMPTY:        return "ERR_EMPTY";
        case PARSE_ERR_UNKNOWN_CMD:  return "ERR_UNKNOWN_CMD";
        case PARSE_ERR_INVALID_ARG:  return "ERR_INVALID_ARG";
        case PARSE_ERR_OUT_OF_RANGE: return "ERR_OUT_OF_RANGE";
        default:                     return "UNKNOWN";
    }
}

const char *PH_Protocol_CmdTypeStr(PH_CmdType_t t)
{
    switch (t) {
        case CMD_STEP:         return "STEP";
        case CMD_SERVO_ON:     return "SERVO_ON";
        case CMD_SERVO_OFF:    return "SERVO_OFF";
        case CMD_RESET:        return "RESET";
        case CMD_STOP:         return "STOP";
        case CMD_STATUS:       return "STATUS";
        case CMD_CAL_JOG:      return "CAL_JOG";
        case CMD_CAL_SET_HOME: return "CAL_SET_HOME";
        case CMD_CAL_SET_STEP: return "CAL_SET_STEP";
        case CMD_CAL_GOTO:     return "CAL_GOTO";
        case CMD_UNKNOWN:
        default:               return "UNKNOWN";
    }
}
