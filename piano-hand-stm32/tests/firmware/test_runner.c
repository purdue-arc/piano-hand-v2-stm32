/**
 * @file test_runner.c
 * @brief Main Test Runner for Piano Hand Firmware Unit Tests
 * @author Embedded Systems Team
 * @license MIT
 */

#include <stdio.h>
#include <stdlib.h>

extern int run_protocol_tests(void);
extern int run_stepper_tests(void);
extern int run_checksum_tests(void);

int main(void)
{
    printf("========================================\n");
    printf("PIANO HAND STM32 FIRMWARE TEST RUNNER\n");
    printf("========================================\n\n");

    int total_failures = 0;

    total_failures += run_protocol_tests();
    printf("\n");

    total_failures += run_stepper_tests();
    printf("\n");

    total_failures += run_checksum_tests();
    printf("\n");

    printf("========================================\n");
    if (total_failures == 0) {
        printf("OVERALL RESULT: ALL TESTS PASSED (0 failures)\n");
    } else {
        printf("OVERALL RESULT: %d TEST FAILURE(S) DETECTED\n", total_failures);
    }
    printf("========================================\n");

    return (total_failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
