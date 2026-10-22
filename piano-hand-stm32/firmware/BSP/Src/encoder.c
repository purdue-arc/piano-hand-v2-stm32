/**
 * @file encoder.c
 * @brief Optical / Magnetic Quadrature Encoder Driver Implementation
 * @author Embedded Systems Team
 * @license MIT
 *
 * Provides hardware timer quadrature decoding, count accumulation, and direction tracking.
 * Entire file is conditionally compiled based on ENCODER_ENABLED.
 */

#include "encoder.h"

#if ENCODER_ENABLED

static TIM_HandleTypeDef *s_htim = NULL;
static uint32_t           s_last_hw_cnt = 0;
static int32_t            s_accumulated_ticks = 0;
static int8_t             s_direction = 0;

PH_Status_t PH_Encoder_Init(TIM_HandleTypeDef *htim)
{
    if (htim == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    s_htim = htim;
    s_last_hw_cnt = 0;
    s_accumulated_ticks = 0;
    s_direction = 0;

    HAL_TIM_Encoder_Start(s_htim, TIM_CHANNEL_ALL);
    return PH_OK;
}

int32_t PH_Encoder_Read(void)
{
    if (s_htim == NULL) {
        return 0;
    }

    uint32_t current_cnt = __HAL_TIM_GET_COUNTER(s_htim);
    int16_t delta = (int16_t)(current_cnt - s_last_hw_cnt);

    s_accumulated_ticks += delta;
    if (delta > 0) {
        s_direction = 1;
    } else if (delta < 0) {
        s_direction = -1;
    } else {
        s_direction = 0;
    }

    s_last_hw_cnt = current_cnt;
    return s_accumulated_ticks;
}

void PH_Encoder_Reset(void)
{
    if (s_htim != NULL) {
        __HAL_TIM_SET_COUNTER(s_htim, 0);
    }
    s_last_hw_cnt = 0;
    s_accumulated_ticks = 0;
    s_direction = 0;
}

int8_t PH_Encoder_GetDirection(void)
{
    return s_direction;
}

#endif /* ENCODER_ENABLED */
