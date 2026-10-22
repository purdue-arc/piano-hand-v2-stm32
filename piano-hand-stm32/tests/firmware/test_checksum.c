/**
 * @file test_checksum.c
 * @brief Unit Tests for Stepper Motor 8-bit Checksum Computation
 * @author Embedded Systems Team
 * @license MIT
 */

#include <stdio.h>
#include <stdint.h>
#include "stepper.h"

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

int run_checksum_tests(void)
{
    test_failures = 0;
    test_passes = 0;
    printf("--- Running Checksum Tests ---\n");

    /* 1. Known packet: [0xE0, 0xFD, 0x50, 0x00, 0x00, 0x01, 0x0B] */
    const uint8_t known_packet[7] = {0xE0, 0xFD, 0x50, 0x00, 0x00, 0x01, 0x0B};
    uint8_t csum1 = PH_Stepper_ComputeChecksum(known_packet, sizeof(known_packet));
    uint8_t expected_csum1 = (uint8_t)((0xE0 + 0xFD + 0x50 + 0x00 + 0x00 + 0x01 + 0x0B) & 0xFF);
    ASSERT(expected_csum1 == 57);
    ASSERT(csum1 == 57);

    /* 2. All-zero payload: checksum is 0 */
    const uint8_t zeros[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t csum2 = PH_Stepper_ComputeChecksum(zeros, sizeof(zeros));
    ASSERT(csum2 == 0);

    /* 3. Overflow wraps correctly */
    /* 0xFF + 0x01 = 0x100 -> wraps to 0x00 */
    const uint8_t overflow_packet[2] = {0xFF, 0x01};
    uint8_t csum3a = PH_Stepper_ComputeChecksum(overflow_packet, sizeof(overflow_packet));
    ASSERT(csum3a == 0x00);

    /* 3 x 0xFF = 765 -> 765 & 0xFF = 253 = 0xFD */
    const uint8_t multi_ff[3] = {0xFF, 0xFF, 0xFF};
    uint8_t csum3b = PH_Stepper_ComputeChecksum(multi_ff, sizeof(multi_ff));
    ASSERT(csum3b == 0xFD);

    /* 4. Single byte input checksum equals that byte & 0xFF */
    for (int b = 0; b <= 255; b++) {
        uint8_t single = (uint8_t)b;
        uint8_t csum4 = PH_Stepper_ComputeChecksum(&single, 1);
        ASSERT(csum4 == single);
    }

    /* Edge case: NULL or 0 length */
    ASSERT(PH_Stepper_ComputeChecksum(NULL, 10) == 0);
    ASSERT(PH_Stepper_ComputeChecksum(known_packet, 0) == 0);

    printf("  Checksum tests: %d passed, %d failed\n", test_passes, test_failures);
    return test_failures;
}
