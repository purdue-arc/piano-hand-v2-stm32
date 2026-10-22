/**
 * @file transport.h
 * @brief Communication Transport Layer (UART / BLE Stub) Interface
 * @author Embedded Systems Team
 * @license MIT
 *
 * Receives newline-delimited command strings over UART, parses them,
 * and pushes validated commands to the protocol queue. Sends status
 * and telemetry responses back over UART.
 */

#ifndef PH_TRANSPORT_H
#define PH_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "protocol.h"

#ifdef UNIT_TEST
  #include "mocks/hal_mock.h"
#else
  #include "stm32f4xx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRANSPORT_UART_COMMS,
    TRANSPORT_BLE,       // Not yet implemented
} PH_TransportType_t;

/**
 * @brief Initialize communication transport with UART handle.
 * @param[in] huart Initialized STM32 HAL UART handle.
 * @return PH_OK on success, or PH_ERR_INVALID_ARG.
 */
PH_Status_t PH_Transport_Init(UART_HandleTypeDef *huart);

/**
 * @brief Non-blocking poll for incoming UART data. Must be called every superloop tick.
 * Accumulates characters until newline, parses commands, and enqueues to protocol queue.
 */
void        PH_Transport_Poll(void);

/**
 * @brief Transmit a null-terminated string over the active transport UART.
 * Automatically appends CRLF if not already present.
 * @param[in] str Null-terminated string to transmit.
 * @return PH_OK on success, or error code.
 */
PH_Status_t PH_Transport_Send(const char *str);

/**
 * @brief Transmit formatted string (printf style) over active transport UART.
 * @param[in] fmt Format string.
 * @return PH_OK on success, or error code.
 */
PH_Status_t PH_Transport_Sendf(const char *fmt, ...);

/**
 * @brief BLE placeholder init function.
 * @return PH_ERR_NOT_SUPPORTED until BLE module is integrated.
 */
PH_Status_t PH_Transport_BLE_Init(UART_HandleTypeDef *huart_ble);

/**
 * @brief BLE placeholder send function.
 * @return PH_ERR_NOT_SUPPORTED until BLE module is integrated.
 */
PH_Status_t PH_Transport_BLE_Send(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* PH_TRANSPORT_H */
