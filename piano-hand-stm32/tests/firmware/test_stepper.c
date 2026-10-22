/**
 * @file test_stepper.c
 * @brief Unit Tests for Stepper Motor Packet Generation and State Machine
 * @author Embedded Systems Team
 * @license MIT
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "stepper.h"
#include "config.h"
#include "mocks/hal_mock.h"

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

int run_stepper_tests(void)
{
    test_failures = 0;
    test_passes = 0;
    printf("--- Running Stepper Tests ---\n");

    UART_HandleTypeDef dummy_uart;
    mock_reset();
    PH_Status_t init_st = PH_Stepper_Init(&dummy_uart);
    ASSERT(init_st == PH_OK);
    ASSERT(PH_Stepper_GetState() == STEPPER_IDLE);

    /* 1. Move +1 key: verify UART packet [0xE0, 0xFD, 80, 0, 0, 1, 11, checksum] */
    mock_reset();
    PH_Status_t st = PH_Stepper_MoveRelative(1);
    ASSERT(st == PH_OK);
    ASSERT(mock_uart_last_tx_size == 8);
    ASSERT(mock_uart_last_tx[0] == 0xE0);
    ASSERT(mock_uart_last_tx[1] == 0xFD);
    ASSERT(mock_uart_last_tx[2] == 80);
    ASSERT(mock_uart_last_tx[3] == 0);
    ASSERT(mock_uart_last_tx[4] == 0);
    ASSERT(mock_uart_last_tx[5] == 1);
    ASSERT(mock_uart_last_tx[6] == 11);
    uint8_t expected_csum = (uint8_t)(0xE0 + 0xFD + 80 + 0 + 0 + 1 + 11);
    ASSERT(mock_uart_last_tx[7] == expected_csum);
    ASSERT(mock_uart_last_tx[7] == 57);

    /* Complete move 1 */
    mock_tick_ms += 10000;
    PH_Stepper_Update(mock_tick_ms);
    ASSERT(PH_Stepper_GetState() == STEPPER_IDLE);

    /* 2. Move -1 key: verify speed byte has 0x80 set */
    mock_reset();
    st = PH_Stepper_MoveRelative(-1);
    ASSERT(st == PH_OK);
    ASSERT(mock_uart_last_tx_size == 8);
    ASSERT((mock_uart_last_tx[2] & 0x80) == 0x80);
    ASSERT(mock_uart_last_tx[2] == (80 | 0x80));

    /* Complete move 2 */
    mock_tick_ms += 10000;
    PH_Stepper_Update(mock_tick_ms);
    ASSERT(PH_Stepper_GetState() == STEPPER_IDLE);

    /* 3. Move 0 keys: verify no UART transmission occurs */
    mock_reset();
    st = PH_Stepper_MoveRelative(0);
    ASSERT(st == PH_OK);
    ASSERT(mock_uart_last_tx_size == 0);
    ASSERT(PH_Stepper_GetState() == STEPPER_IDLE);

    /* 4. ReturnToOrigin after +3 keys: verify movement of -3 keys */
    PH_Stepper_SetHome();
    ASSERT(PH_Stepper_GetKeyPosition() == 0);
    st = PH_Stepper_MoveRelative(3);
    ASSERT(st == PH_OK);
    ASSERT(PH_Stepper_GetKeyPosition() == 3);

    /* Wait for move to complete */
    mock_tick_ms += 10000;
    PH_Stepper_Update(mock_tick_ms);
    ASSERT(PH_Stepper_GetState() == STEPPER_IDLE);

    mock_reset();
    st = PH_Stepper_ReturnToOrigin();
    ASSERT(st == PH_OK);
    ASSERT(mock_uart_last_tx_size == 8);
    /* Speed byte has negative direction bit set */
    ASSERT((mock_uart_last_tx[2] & 0x80) == 0x80);
    /* 3 keys * 30 deg = 90 deg => 800 pulses = 0x00000320 */
    ASSERT(mock_uart_last_tx[5] == 3);
    ASSERT(mock_uart_last_tx[6] == 32);

    /* 5. After ReturnToOrigin, GetKeyPosition == 0 */
    ASSERT(PH_Stepper_GetKeyPosition() == 0);

    /* Complete return to origin move */
    mock_tick_ms += 10000;
    PH_Stepper_Update(mock_tick_ms);
    ASSERT(PH_Stepper_GetState() == STEPPER_IDLE);

    /* 6. IsMoving returns true while in WAITING state, false after timeout elapses */
    st = PH_Stepper_MoveRelative(1);
    ASSERT(st == PH_OK);
    ASSERT(PH_Stepper_IsMoving() == true);
    ASSERT(PH_Stepper_GetState() == STEPPER_WAITING);

    /* Tick with only 10ms elapsed (timeout is > 100ms) */
    PH_Stepper_Update(mock_tick_ms + 10);
    ASSERT(PH_Stepper_IsMoving() == true);

    /* Advance time beyond timeout */
    mock_tick_ms += 5000;
    PH_Stepper_Update(mock_tick_ms);
    ASSERT(PH_Stepper_IsMoving() == false);
    ASSERT(PH_Stepper_GetState() == STEPPER_IDLE);

    /* 7. EmergencyStop transitions to IDLE */
    st = PH_Stepper_MoveRelative(1);
    ASSERT(st == PH_OK);
    ASSERT(PH_Stepper_IsMoving() == true);
    PH_Stepper_EmergencyStop();
    ASSERT(PH_Stepper_GetState() == STEPPER_IDLE);
    ASSERT(PH_Stepper_IsMoving() == false);

    printf("  Stepper tests: %d passed, %d failed\n", test_passes, test_failures);
    return test_failures;
}
