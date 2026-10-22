/**
 * @file hal_mock.c
 * @brief STM32 HAL Mock Implementation for Host Unit Testing
 * @author Embedded Systems Team
 * @license MIT
 */

#include "hal_mock.h"

uint8_t  mock_uart_last_tx[64];
uint16_t mock_uart_last_tx_size = 0;

uint8_t  mock_i2c_last_dev_addr = 0;
uint16_t mock_i2c_last_mem_addr = 0;
uint8_t  mock_i2c_last_data[8];
uint16_t mock_i2c_last_data_size = 0;

uint32_t mock_tick_ms = 0;

void mock_reset(void)
{
    memset(mock_uart_last_tx, 0, sizeof(mock_uart_last_tx));
    mock_uart_last_tx_size = 0;

    mock_i2c_last_dev_addr = 0;
    mock_i2c_last_mem_addr = 0;
    memset(mock_i2c_last_data, 0, sizeof(mock_i2c_last_data));
    mock_i2c_last_data_size = 0;

    mock_tick_ms = 0;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c,
                                    uint16_t DevAddress,
                                    uint16_t MemAddress,
                                    uint16_t MemAddSize,
                                    uint8_t *pData,
                                    uint16_t Size,
                                    uint32_t Timeout)
{
    (void)hi2c;
    (void)MemAddSize;
    (void)Timeout;

    mock_i2c_last_dev_addr = (uint8_t)DevAddress;
    mock_i2c_last_mem_addr = MemAddress;
    mock_i2c_last_data_size = Size;

    uint16_t copy_size = (Size > sizeof(mock_i2c_last_data)) ? sizeof(mock_i2c_last_data) : Size;
    if (pData != NULL && copy_size > 0) {
        memcpy(mock_i2c_last_data, pData, copy_size);
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c,
                                   uint16_t DevAddress,
                                   uint16_t MemAddress,
                                   uint16_t MemAddSize,
                                   uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout)
{
    (void)hi2c;
    (void)DevAddress;
    (void)MemAddress;
    (void)MemAddSize;
    (void)pData;
    (void)Size;
    (void)Timeout;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart,
                                    uint8_t *pData,
                                    uint16_t Size,
                                    uint32_t Timeout)
{
    (void)huart;
    (void)Timeout;

    mock_uart_last_tx_size = Size;
    uint16_t copy_size = (Size > sizeof(mock_uart_last_tx)) ? sizeof(mock_uart_last_tx) : Size;
    if (pData != NULL && copy_size > 0) {
        memcpy(mock_uart_last_tx, pData, copy_size);
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart,
                                   uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout)
{
    (void)huart;
    (void)pData;
    (void)Size;
    (void)Timeout;
    /* Return timeout by default for empty rx buffer */
    return HAL_TIMEOUT;
}

uint32_t HAL_GetTick(void)
{
    return mock_tick_ms;
}

void HAL_Delay(uint32_t ms)
{
    mock_tick_ms += ms;
}
