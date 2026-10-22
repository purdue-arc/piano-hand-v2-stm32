/**
 * @file servo.c
 * @brief PCA9685 16-Channel 12-Bit PWM Driver Implementation
 * @author Embedded Systems Team
 * @license MIT
 *
 * Implements I2C register configuration, prescale math, and angular pulse
 * mapping for 5 finger servos.
 */

#include "servo.h"
#include "config.h"

/* ============================================================================
 * PCA9685 Register Map & Bit Definitions
 * ============================================================================ */
#define PCA9685_REG_MODE1       0x00
#define PCA9685_REG_MODE2       0x01
#define PCA9685_REG_LED0_ON_L   0x06
#define PCA9685_REG_PRESCALE    0xFE

#define PCA9685_MODE1_RESTART   0x80
#define PCA9685_MODE1_EXTCLK    0x40
#define PCA9685_MODE1_AI        0x20  /* Auto-Increment enable */
#define PCA9685_MODE1_SLEEP     0x10  /* Low power sleep / osc off */
#define PCA9685_MODE1_ALLCALL   0x01  /* Respond to LED All Call */

#define PCA9685_I2C_ADDR_HAL    ((uint16_t)(PCA9685_I2C_ADDR << 1))
#define PCA9685_MEMADD_SIZE_8B  1
#define PCA9685_I2C_TIMEOUT_MS  100

/* ============================================================================
 * File-Static State
 * ============================================================================ */
static I2C_HandleTypeDef *s_hi2c = NULL;
static const uint8_t s_channels[SERVO_COUNT] = SERVO_CHANNELS;

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */
static PH_Status_t pca9685_write_reg(uint8_t reg, uint8_t val)
{
    if (s_hi2c == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(s_hi2c,
                                            PCA9685_I2C_ADDR_HAL,
                                            reg,
                                            PCA9685_MEMADD_SIZE_8B,
                                            &val,
                                            1,
                                            PCA9685_I2C_TIMEOUT_MS);
    return (st == HAL_OK) ? PH_OK : PH_ERR_I2C;
}

/* ============================================================================
 * Public Driver API
 * ============================================================================ */

PH_Status_t PH_Servo_Init(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL) {
        return PH_ERR_INVALID_ARG;
    }
    s_hi2c = hi2c;

    /* Write 0x00 to MODE1 to wake up PCA9685 */
    PH_Status_t st = pca9685_write_reg(PCA9685_REG_MODE1, 0x00);
    if (st != PH_OK) {
        return st;
    }

    HAL_Delay(5);

    /* Set default frequency (50 Hz) */
    st = PH_Servo_SetPWMFreq(PCA9685_PWM_FREQ_HZ);
    if (st != PH_OK) {
        return st;
    }

    return PH_OK;
}

PH_Status_t PH_Servo_SetPWMFreq(float freq_hz)
{
    if (s_hi2c == NULL || freq_hz < 1.0f) {
        return PH_ERR_INVALID_ARG;
    }

    /* Legacy formula: prescale = int(25000000.0 / (4096 * freq) - 1) */
    float prescale_val = (25000000.0f / (4096.0f * freq_hz)) - 1.0f;
    uint8_t prescale = (uint8_t)(prescale_val + 0.5f);

    /* Oscillator must be put to sleep before updating PRESCALE register */
    PH_Status_t st = pca9685_write_reg(PCA9685_REG_MODE1, PCA9685_MODE1_SLEEP);
    if (st != PH_OK) {
        return st;
    }

    HAL_Delay(5);

    /* Write prescale */
    st = pca9685_write_reg(PCA9685_REG_PRESCALE, prescale);
    if (st != PH_OK) {
        return st;
    }

    /* Wake up oscillator */
    st = pca9685_write_reg(PCA9685_REG_MODE1, 0x00);
    if (st != PH_OK) {
        return st;
    }

    HAL_Delay(5);

    /* Restore with Auto-Increment and All-Call enabled */
    st = pca9685_write_reg(PCA9685_REG_MODE1, PCA9685_MODE1_AI | PCA9685_MODE1_ALLCALL);
    if (st != PH_OK) {
        return st;
    }

    return PH_OK;
}

PH_Status_t PH_Servo_SetPWM(uint8_t channel, uint16_t on_count, uint16_t off_count)
{
    if (s_hi2c == NULL || channel >= 16) {
        return PH_ERR_INVALID_ARG;
    }

    uint8_t data[4];
    data[0] = (uint8_t)(on_count & 0xFF);
    data[1] = (uint8_t)((on_count >> 8) & 0x0F);
    data[2] = (uint8_t)(off_count & 0xFF);
    data[3] = (uint8_t)((off_count >> 8) & 0x0F);

    uint8_t reg = (uint8_t)(PCA9685_REG_LED0_ON_L + (4 * channel));
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(s_hi2c,
                                            PCA9685_I2C_ADDR_HAL,
                                            reg,
                                            PCA9685_MEMADD_SIZE_8B,
                                            data,
                                            4,
                                            PCA9685_I2C_TIMEOUT_MS);
    return (st == HAL_OK) ? PH_OK : PH_ERR_I2C;
}

PH_Status_t PH_Servo_SetAngle(uint8_t channel, float angle_deg)
{
    if (channel >= 16) {
        return PH_ERR_INVALID_ARG;
    }

    /* Clamp angle to valid physical range */
    if (angle_deg < 0.0f) {
        angle_deg = 0.0f;
    } else if (angle_deg > 180.0f) {
        angle_deg = 180.0f;
    }

    /* Angle to pulse duration in microseconds */
    float pulse_us = (float)SERVO_MIN_PULSE_US +
                     (angle_deg / 180.0f) * (float)(SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);

    /* Pulse duration to 12-bit tick count (50Hz => 20,000 us period => 4096 counts) */
    uint16_t off_count = (uint16_t)((pulse_us / 20000.0f) * 4096.0f);

    return PH_Servo_SetPWM(channel, 0, off_count);
}

PH_Status_t PH_Servo_Press(uint8_t finger_index)
{
    if (finger_index < 1 || finger_index > SERVO_COUNT) {
        return PH_ERR_INVALID_ARG;
    }
    uint8_t channel = s_channels[finger_index - 1];
    return PH_Servo_SetAngle(channel, SERVO_PRESS_ANGLE_DEG);
}

PH_Status_t PH_Servo_Release(uint8_t finger_index)
{
    if (finger_index < 1 || finger_index > SERVO_COUNT) {
        return PH_ERR_INVALID_ARG;
    }
    uint8_t channel = s_channels[finger_index - 1];
    return PH_Servo_SetAngle(channel, SERVO_RELEASE_ANGLE_DEG);
}

PH_Status_t PH_Servo_PressAll(uint8_t finger_mask)
{
    PH_Status_t last_status = PH_OK;
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        uint8_t finger_idx = (uint8_t)(i + 1);
        PH_Status_t st;
        if (finger_mask & (1 << i)) {
            st = PH_Servo_Press(finger_idx);
        } else {
            st = PH_Servo_Release(finger_idx);
        }
        if (st != PH_OK) {
            last_status = st;
        }
    }
    return last_status;
}

PH_Status_t PH_Servo_ReleaseAll(void)
{
    PH_Status_t last_status = PH_OK;
    for (uint8_t i = 1; i <= SERVO_COUNT; i++) {
        PH_Status_t st = PH_Servo_Release(i);
        if (st != PH_OK) {
            last_status = st;
        }
    }
    return last_status;
}
