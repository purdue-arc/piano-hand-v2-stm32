/**
 * @file hal_mock.h
 * @brief STM32 HAL Mock Definitions for Host Unit Testing
 * @author Embedded Systems Team
 * @license MIT
 */

#ifndef HAL_MOCK_H
#define HAL_MOCK_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#ifndef UNIT_TEST
#define UNIT_TEST 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* HAL status definition */
typedef enum {
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

/* Peripheral handle stubs */
typedef struct {
    uint32_t dummy;
} I2C_HandleTypeDef;

typedef struct {
    uint32_t dummy;
} UART_HandleTypeDef;

typedef struct {
    struct {
        uint32_t CNT;
    } Instance[1];
} TIM_HandleTypeDef;

/* HAL function stubs */
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c,
                                    uint16_t DevAddress,
                                    uint16_t MemAddress,
                                    uint16_t MemAddSize,
                                    uint8_t *pData,
                                    uint16_t Size,
                                    uint32_t Timeout);

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c,
                                   uint16_t DevAddress,
                                   uint16_t MemAddress,
                                   uint16_t MemAddSize,
                                   uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout);

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart,
                                    uint8_t *pData,
                                    uint16_t Size,
                                    uint32_t Timeout);

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart,
                                   uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout);

uint32_t HAL_GetTick(void);
void     HAL_Delay(uint32_t ms);

/* Mock inspection data */
extern uint8_t  mock_uart_last_tx[64];
extern uint16_t mock_uart_last_tx_size;
extern uint8_t  mock_i2c_last_dev_addr;
extern uint16_t mock_i2c_last_mem_addr;
extern uint8_t  mock_i2c_last_data[8];
extern uint16_t mock_i2c_last_data_size;
extern uint32_t mock_tick_ms;

void mock_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_MOCK_H */
