//********************************************************************
//*                          Micro Mouse                             *
//*                          Motors Library                          *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      04-04-2026                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This source file implements functions for initializing and       *
//* refreshing motor control values for the Micro Mouse robot.       *
//* It includes functions for starting PWM and updating motor        *
//* speed and direction.                                             *
//*                                                                  *
//********************************************************************

#include "Motors.h"
#include "main.h"
#include "math.h"
#include <stdlib.h>

Motor_t MOTOR_L = {0, 0};
Motor_t MOTOR_R = {0, 0};

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

// ─────────────────────────────────────────────────────────────────
//  Quadrature encoder — TIM4 input-capture, interrupt-driven (1 MHz)
//  CH1 = MOTORR_A_ENC (PD12) — right A-phase  (IT → speed in callback)
//  CH2 = MOTORR_B_ENC (PD13) — right B-phase  (IC only → direction via GPIO)
//  CH3 = MOTORL_A_ENC (PD14) — left  A-phase  (IT → speed in callback)
//  CH4 = MOTORL_B_ENC (PD15) — left  B-phase  (IC only → direction via GPIO)
// ─────────────────────────────────────────────────────────────────

// Last captured A-phase timestamps — edge-to-edge period (1 µs per tick)
static uint16_t prev_r_cap = 0;
static uint16_t prev_l_cap = 0;

// Timestamp (ms) of the last observed encoder edge — for stall detection
static uint32_t last_r_edge_ms = 0;
static uint32_t last_l_edge_ms = 0;

// If no edge seen for this many ms, assume motor stopped
#define ENC_STALL_TIMEOUT_MS  250

// TIM3 ARR value — set in MX_TIM3_Init (1000 - 1 = 999)
#define TIM3_ARR  999

// Encoder resolution: 4680 quadrature counts per revolution (4 edges × 1170 physical slots).
// The IC callback fires only on A-phase rising edges → 1170 events per revolution.
// RPM = (1,000,000 µs/s * 60 s/min) / (edges_per_rev * delta_µs)
//     = 60,000,000 / (1170 * delta_us)
#define ENC_TICKS_PER_REV  1170   // 4680 quadrature counts / 4 (single-edge capture)

// Fixed-point scaling for encoderRate: store RPM with 2 decimal places.
// Example: 123.45 RPM is stored as 12345.
#define RPM_SCALE 100

static int16_t clamp_to_i16(int32_t value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (int16_t)value;
}


void initMotors(void)
{
    // ── Enable motor-driver output FET ──────────────────────────
    HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_SET);

    // ── Start TIM3 PWM on all 4 H-bridge channels (duty = 0) ────
    // CH1 = MOTORR_A_EN (PC6)  — right forward
    // CH2 = MOTORR_B_EN (PC7)  — right backward
    // CH3 = MOTORL_A_EN (PC8)  — left  forward
    // CH4 = MOTORL_B_EN (PC9)  — left  backward
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

    // ── Start TIM4 input-capture ─────────────────────────────────────────
    // CH1 = right A-phase: IT fires on every rising edge, callback computes speed
    HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_1);
    // CH2 = right B-phase: plain IC, no IT — direction read via GPIO in callback
    HAL_TIM_IC_Start(&htim4, TIM_CHANNEL_2);
    // CH3 = left A-phase: IT fires on every rising edge, callback computes speed
    HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_3);
    // CH4 = left B-phase: plain IC, no IT — direction read via GPIO in callback
    HAL_TIM_IC_Start(&htim4, TIM_CHANNEL_4);

    last_r_edge_ms = HAL_GetTick();
    last_l_edge_ms = HAL_GetTick();
}


void refreshMotors(void)
{
    uint32_t arr = TIM3_ARR;

    // ── PWM output: map magnitude [-100..+100] → duty [0..ARR] ─
    // Positive magnitude  → forward channel active, backward = 0
    // Negative magnitude  → backward channel active, forward = 0
    // Zero                → both channels = 0  (coast)

    // Right motor
    if (MOTOR_R.magnitude > 0) {
        uint32_t duty = ((uint32_t)MOTOR_R.magnitude * (arr + 1)) / 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    } else if (MOTOR_R.magnitude < 0) {
        uint32_t duty = ((uint32_t)(-MOTOR_R.magnitude) * (arr + 1)) / 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty);
    } else {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    }

    // Left motor
    if (MOTOR_L.magnitude > 0) {
        uint32_t duty = ((uint32_t)MOTOR_L.magnitude * (arr + 1)) / 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, duty);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
    } else if (MOTOR_L.magnitude < 0) {
        uint32_t duty = ((uint32_t)(-MOTOR_L.magnitude) * (arr + 1)) / 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, duty);
    } else {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
    }

    // ── Stall detection ───────────────────────────────────────────────────
    // encoderRate is written by HAL_TIM_IC_CaptureCallback; zero it here
    // if no A-phase edge has been seen for longer than the stall timeout.
    uint32_t now_ms = HAL_GetTick();
    if ((now_ms - last_r_edge_ms) > ENC_STALL_TIMEOUT_MS) MOTOR_R.encoderRate = 0;
    if ((now_ms - last_l_edge_ms) > ENC_STALL_TIMEOUT_MS) MOTOR_L.encoderRate = 0;
}


// ── TIM4 input-capture interrupt callback ────────────────────────────────────
// Called by HAL_TIM_IRQHandler (in stm32l4xx_it.c) on every captured edge.
// Only CH1 (right A) and CH3 (left A) are started with _IT, so only those fire.
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM4) return;

    uint32_t now_ms = HAL_GetTick();

    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        // Right motor A-phase rising edge — measure inter-edge period
        uint16_t cap      = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        uint16_t delta_us = cap - prev_r_cap;   // 16-bit subtraction wraps correctly
        prev_r_cap        = cap;
        last_r_edge_ms    = now_ms;

        if (delta_us > 0 && delta_us < 62000U) {
            int32_t rate_x100 = (int32_t)(((60000000LL * RPM_SCALE)) / ((int64_t)ENC_TICKS_PER_REV * (int64_t)delta_us));
            int8_t  dir  = HAL_GPIO_ReadPin(MOTORR_B_ENC_GPIO_Port, MOTORR_B_ENC_Pin) ? -1 : 1;  // inverted: motor mounted opposing
            MOTOR_R.encoderRate = clamp_to_i16((int32_t)dir * rate_x100);
        }
    }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
    {
        // Left motor A-phase rising edge — measure inter-edge period
        uint16_t cap      = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
        uint16_t delta_us = cap - prev_l_cap;
        prev_l_cap        = cap;
        last_l_edge_ms    = now_ms;

        if (delta_us > 0 && delta_us < 62000U) {
            int32_t rate_x100 = (int32_t)(((60000000LL * RPM_SCALE)) / ((int64_t)ENC_TICKS_PER_REV * (int64_t)delta_us));
            int8_t  dir  = HAL_GPIO_ReadPin(MOTORL_B_ENC_GPIO_Port, MOTORL_B_ENC_Pin) ? 1 : -1;
            MOTOR_L.encoderRate = clamp_to_i16((int32_t)dir * rate_x100);
        }
    }
}
