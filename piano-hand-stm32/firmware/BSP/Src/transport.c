/**
 * @file transport.c
 * @brief Communication Transport Layer Implementation
 * @author Embedded Systems Team
 * @license MIT
 *
 * Implements non-blocking line-based UART polling and formatted telemetry transmission.
 */

#include "transport.h"
#include <string.h>
#include <stdio.h>

#define TRANSPORT_RX_BUF_SIZE  128
#define TRANSPORT_TX_TIMEOUT_MS 100

static UART_HandleTypeDef *s_huart_comms = NULL;
static char                s_rx_line_buf[TRANSPORT_RX_BUF_SIZE];
static uint16_t            s_rx_line_len = 0;

PH_Status_t PH_Transport_Init(UART_HandleTypeDef *huart)
{
    if (huart == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    s_huart_comms = huart;
    s_rx_line_len = 0;
    return PH_OK;
}

void PH_Transport_Poll(void)
{
    if (s_huart_comms == NULL) {
        return;
    }

    uint8_t rx_byte = 0;
    /* Non-blocking single-byte reception poll */
    while (HAL_UART_Receive(s_huart_comms, &rx_byte, 1, 0) == HAL_OK) {
        if (rx_byte == '\n' || rx_byte == '\r') {
            if (s_rx_line_len > 0) {
                s_rx_line_buf[s_rx_line_len] = '\0';
                PH_Command_t cmd;
                if (PH_Protocol_Parse(s_rx_line_buf, &cmd) == PARSE_OK) {
                    PH_Protocol_Enqueue(&cmd);
                }
                s_rx_line_len = 0;
            }
        } else {
            if (s_rx_line_len < (TRANSPORT_RX_BUF_SIZE - 1)) {
                s_rx_line_buf[s_rx_line_len++] = (char)rx_byte;
            } else {
                /* Line exceeded buffer capacity; reset to prevent corrupted parse */
                s_rx_line_len = 0;
            }
        }
    }
}

PH_Status_t PH_Transport_Send(const char *str)
{
    if (s_huart_comms == NULL || str == NULL) {
        return PH_ERR_INVALID_ARG;
    }

    uint16_t len = (uint16_t)strlen(str);
    if (len == 0) {
        return PH_OK;
    }

    HAL_StatusTypeDef st = HAL_UART_Transmit(s_huart_comms, (uint8_t *)str, len, TRANSPORT_TX_TIMEOUT_MS);
    if (st != HAL_OK) {
        return PH_ERR_UART;
    }

    /* Append CRLF if string does not already end in a newline */
    if (str[len - 1] != '\n') {
        static const uint8_t crlf[2] = {'\r', '\n'};
        st = HAL_UART_Transmit(s_huart_comms, (uint8_t *)crlf, 2, 50);
        if (st != HAL_OK) {
            return PH_ERR_UART;
        }
    }

    return PH_OK;
}

PH_Status_t PH_Transport_Sendf(const char *fmt, ...)
{
    if (fmt == NULL) {
        return PH_ERR_INVALID_ARG;
    }

    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    return PH_Transport_Send(buf);
}

PH_Status_t PH_Transport_BLE_Init(UART_HandleTypeDef *huart_ble)
{
    (void)huart_ble;
    return PH_ERR_NOT_SUPPORTED;
}

PH_Status_t PH_Transport_BLE_Send(const char *str)
{
    (void)str;
    return PH_ERR_NOT_SUPPORTED;
}
