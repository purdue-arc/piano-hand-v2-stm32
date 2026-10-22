/**
 * @file test_protocol.c
 * @brief Unit Tests for Text Command Protocol Parser and Queue
 * @author Embedded Systems Team
 * @license MIT
 */

#include <stdio.h>
#include <math.h>
#include "protocol.h"
#include "config.h"

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        test_failures++; \
    } else { \
        test_passes++; \
    } \
} while(0)

static int test_failures = 0;
static int test_passes = 0;

int run_protocol_tests(void)
{
    test_failures = 0;
    test_passes = 0;
    printf("--- Running Protocol Tests ---\n");

    PH_Command_t cmd;
    PH_ParseResult_t res;

    /* 1. Parse "RESET" -> CMD_RESET */
    res = PH_Protocol_Parse("RESET", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_RESET);

    /* 2. Parse "STOP" -> CMD_STOP */
    res = PH_Protocol_Parse("STOP", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_STOP);

    /* 3. Parse "STATUS" -> CMD_STATUS */
    res = PH_Protocol_Parse("STATUS", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_STATUS);

    /* 4. Parse "0:step:5" -> CMD_STEP, step_value=5 */
    res = PH_Protocol_Parse("0:step:5", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_STEP);
    ASSERT(cmd.step_value == 5);

    /* 5. Parse "0:step:-3" -> CMD_STEP, step_value=-3 */
    res = PH_Protocol_Parse("0:step:-3", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_STEP);
    ASSERT(cmd.step_value == -3);

    /* 6. Parse "0:servo_on:3" -> CMD_SERVO_ON, finger_index=3 */
    res = PH_Protocol_Parse("0:servo_on:3", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_SERVO_ON);
    ASSERT(cmd.finger_index == 3);

    /* 7. Parse "0:servo_off:1" -> CMD_SERVO_OFF, finger_index=1 */
    res = PH_Protocol_Parse("0:servo_off:1", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_SERVO_OFF);
    ASSERT(cmd.finger_index == 1);

    /* 8. Parse "0:servo_on:0" -> PARSE_ERR_OUT_OF_RANGE (finger 0 invalid) */
    res = PH_Protocol_Parse("0:servo_on:0", &cmd);
    ASSERT(res == PARSE_ERR_OUT_OF_RANGE);

    /* 9. Parse "0:servo_on:6" -> PARSE_ERR_OUT_OF_RANGE (finger 6 invalid) */
    res = PH_Protocol_Parse("0:servo_on:6", &cmd);
    ASSERT(res == PARSE_ERR_OUT_OF_RANGE);

    /* 10. Parse "" -> PARSE_ERR_EMPTY */
    res = PH_Protocol_Parse("", &cmd);
    ASSERT(res == PARSE_ERR_EMPTY);

    /* 11. Parse "garbage" -> PARSE_ERR_UNKNOWN_CMD */
    res = PH_Protocol_Parse("garbage", &cmd);
    ASSERT(res == PARSE_ERR_UNKNOWN_CMD);

    /* 12. Parse "CAL:SET_HOME" -> CMD_CAL_SET_HOME */
    res = PH_Protocol_Parse("CAL:SET_HOME", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_CAL_SET_HOME);

    /* 13. Parse "CAL:SET_STEP:30.5" -> CMD_CAL_SET_STEP, float_value=30.5 */
    res = PH_Protocol_Parse("CAL:SET_STEP:30.5", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_CAL_SET_STEP);
    ASSERT(fabsf(cmd.float_value - 30.5f) < 0.001f);

    /* 14. Parse "CAL:JOG:90.0" -> CMD_CAL_JOG, float_value=90.0 */
    res = PH_Protocol_Parse("CAL:JOG:90.0", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_CAL_JOG);
    ASSERT(fabsf(cmd.float_value - 90.0f) < 0.001f);

    /* 15. Parse "CAL:GOTO:7" -> CMD_CAL_GOTO, step_value=7 */
    res = PH_Protocol_Parse("CAL:GOTO:7", &cmd);
    ASSERT(res == PARSE_OK);
    ASSERT(cmd.type == CMD_CAL_GOTO);
    ASSERT(cmd.step_value == 7);

    /* 16. Queue: enqueue 32 commands successfully, 33rd returns PH_ERR_QUEUE_FULL */
    PH_Protocol_QueueReset();
    ASSERT(PH_Protocol_QueueDepth() == 0);

    PH_Command_t fill_cmd;
    fill_cmd.type = CMD_STEP;
    fill_cmd.step_value = 1;
    fill_cmd.float_value = 0.0f;
    fill_cmd.finger_index = 0;

    for (int i = 0; i < CMD_QUEUE_DEPTH; i++) {
        PH_Status_t st = PH_Protocol_Enqueue(&fill_cmd);
        ASSERT(st == PH_OK);
    }
    ASSERT(PH_Protocol_QueueDepth() == CMD_QUEUE_DEPTH);

    PH_Status_t st_full = PH_Protocol_Enqueue(&fill_cmd);
    ASSERT(st_full == PH_ERR_QUEUE_FULL);

    /* 17. Dequeue from empty returns PH_ERR_QUEUE_EMPTY */
    PH_Protocol_QueueReset();
    PH_Command_t out_cmd;
    PH_Status_t st_empty = PH_Protocol_Dequeue(&out_cmd);
    ASSERT(st_empty == PH_ERR_QUEUE_EMPTY);

    /* 18. Enqueue then dequeue preserves command type (FIFO order) */
    PH_Protocol_QueueReset();
    PH_Command_t c1, c2, c3;
    c1.type = CMD_STEP;
    c1.step_value = 10;
    c2.type = CMD_SERVO_ON;
    c2.finger_index = 4;
    c3.type = CMD_STOP;

    ASSERT(PH_Protocol_Enqueue(&c1) == PH_OK);
    ASSERT(PH_Protocol_Enqueue(&c2) == PH_OK);
    ASSERT(PH_Protocol_Enqueue(&c3) == PH_OK);
    ASSERT(PH_Protocol_QueueDepth() == 3);

    PH_Command_t r1, r2, r3;
    ASSERT(PH_Protocol_Dequeue(&r1) == PH_OK);
    ASSERT(r1.type == CMD_STEP);
    ASSERT(r1.step_value == 10);

    ASSERT(PH_Protocol_Dequeue(&r2) == PH_OK);
    ASSERT(r2.type == CMD_SERVO_ON);
    ASSERT(r2.finger_index == 4);

    ASSERT(PH_Protocol_Dequeue(&r3) == PH_OK);
    ASSERT(r3.type == CMD_STOP);

    ASSERT(PH_Protocol_QueueDepth() == 0);

    printf("  Protocol tests: %d passed, %d failed\n", test_passes, test_failures);
    return test_failures;
}
