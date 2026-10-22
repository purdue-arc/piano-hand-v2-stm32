/**
 * @file encoder.h
 * @brief Optical / Magnetic Quadrature Encoder Driver Interface
 * @author Embedded Systems Team
 * @license MIT
 *
 * NOTE: Encoder hardware not yet verified. Timer pin mapping TBD.
 * When ENCODER_ENABLED=0, all functions are compiled out.
 */

#ifndef PH_ENCODER_H
#define PH_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"
#include "config.h"

#ifdef UNIT_TEST
  #include "mocks/hal_mock.h"
#else
  #include "stm32f4xx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if ENCODER_ENABLED

/**
 * @brief Initialize timer in encoder mode.
 * @param[in] htim Hardware timer configured in encoder mode.
 * @return PH_OK on success, or PH_ERR_INVALID_ARG.
 */
PH_Status_t PH_Encoder_Init(TIM_HandleTypeDef *htim);

/**
 * @brief Read the accumulated quadrature encoder count.
 * @return Signed 32-bit count.
 */
int32_t     PH_Encoder_Read(void);

/**
 * @brief Reset accumulated encoder count and timer hardware counter to 0.
 */
void        PH_Encoder_Reset(void);

/**
 * @brief Get the last detected movement direction.
 * @return +1 for forward, -1 for reverse, 0 for stationary.
 */
int8_t      PH_Encoder_GetDirection(void);

#endif /* ENCODER_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* PH_ENCODER_H */
