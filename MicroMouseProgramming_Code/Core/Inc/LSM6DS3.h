//********************************************************************
//*                          Micro Mouse                             *
//*                       LSM6DS3 IMU Library                        *
//*==================================================================*
//* @author:    STMicroelectronics / Adapted by Jesse Jabez Arendse *
//* @date:      2017 / Adapted 2024                                  *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This header file provides a C interface for communicating with   *
//* the LSM6DS3 6-axis IMU sensor. The library handles sensor        *
//* initialization, and reading of accelerometer and gyroscope      *
//* data for motion tracking. Also supports advanced features like  *
//* tap detection, free fall detection, and 6D orientation sensing. *
//*                                                                  *
//* Datasheet: LSM6DS3 MEMS inertial measurement unit               *
//* Address: I2C 0xD4 (SAD=0) or 0xD6 (SAD=1)                       *
//* WHO_AM_I: 0x69                                                   *
//********************************************************************

#ifndef LSM6DS3_H
#define LSM6DS3_H

#include "stm32l4xx.h"
#include "main.h"
#include <stdint.h>

//====================================================================
// TYPE DEFINITIONS
//====================================================================
typedef unsigned char u8_t;
typedef unsigned short int u16_t;
typedef unsigned int u32_t;
typedef int i32_t;
typedef short int i16_t;
typedef signed char i8_t;

typedef enum {
    MEMS_SUCCESS = 0x01,
    MEMS_ERROR = 0x00
} mems_status_t;

typedef union {
    i16_t i16bit[3];
    u8_t u8bit[6];
} Type3Axis16bit_U;

//====================================================================
// I2C CONFIGURATION
//====================================================================
#define LSM6DS3_I2C_ADDRESS_LOW     0xD4  // SAD[0] = 0 (7-bit: 0x6A)
#define LSM6DS3_I2C_ADDRESS_HIGH    0xD6  // SAD[0] = 1 (7-bit: 0x6B)
#define LSM6DS3_I2C_ADDRESS         LSM6DS3_I2C_ADDRESS_LOW
#define I2C_TIMEOUT                 100   // I2C timeout in ms
#define LSM6DS3_ACC_GYRO_WHO_AM_I   0x69

//====================================================================
// REGISTER ADDRESSES
//====================================================================
#define LSM6DS3_ACC_GYRO_CTRL1_XL       0x10
#define LSM6DS3_ACC_GYRO_CTRL2_G        0x11
#define LSM6DS3_ACC_GYRO_CTRL3_C        0x12
#define LSM6DS3_ACC_GYRO_WHO_AM_I_REG   0x0F
#define LSM6DS3_ACC_GYRO_OUTX_L_XL      0x28
#define LSM6DS3_ACC_GYRO_OUTX_H_XL      0x29
#define LSM6DS3_ACC_GYRO_OUTY_L_XL      0x2A
#define LSM6DS3_ACC_GYRO_OUTY_H_XL      0x2B
#define LSM6DS3_ACC_GYRO_OUTZ_L_XL      0x2C
#define LSM6DS3_ACC_GYRO_OUTZ_H_XL      0x2D
#define LSM6DS3_ACC_GYRO_OUTX_L_G       0x22
#define LSM6DS3_ACC_GYRO_OUTX_H_G       0x23
#define LSM6DS3_ACC_GYRO_OUTY_L_G       0x24
#define LSM6DS3_ACC_GYRO_OUTY_H_G       0x25
#define LSM6DS3_ACC_GYRO_OUTZ_L_G       0x26
#define LSM6DS3_ACC_GYRO_OUTZ_H_G       0x27
#define LSM6DS3_ACC_GYRO_OUT_TEMP_L     0x20
#define LSM6DS3_ACC_GYRO_OUT_TEMP_H     0x21
#define LSM6DS3_ACC_GYRO_TAP_SRC        0x1C

//====================================================================
// REGISTER BIT MASKS AND ENUMS
//====================================================================

// Full-Scale Selection for Accelerometer
typedef enum {
    LSM6DS3_ACC_GYRO_FS_XL_2g   = 0x00,
    LSM6DS3_ACC_GYRO_FS_XL_16g  = 0x04,
    LSM6DS3_ACC_GYRO_FS_XL_4g   = 0x08,
    LSM6DS3_ACC_GYRO_FS_XL_8g   = 0x0C,
} LSM6DS3_ACC_GYRO_FS_XL_t;

#define LSM6DS3_ACC_GYRO_FS_XL_MASK  0x0C

// Full-Scale Selection for Gyroscope
typedef enum {
    LSM6DS3_ACC_GYRO_FS_G_245dps   = 0x00,
    LSM6DS3_ACC_GYRO_FS_G_500dps   = 0x04,
    LSM6DS3_ACC_GYRO_FS_G_1000dps  = 0x08,
    LSM6DS3_ACC_GYRO_FS_G_2000dps  = 0x0C,
} LSM6DS3_ACC_GYRO_FS_G_t;

#define LSM6DS3_ACC_GYRO_FS_G_MASK  0x0C

// Output Data Rate for Accelerometer
typedef enum {
    LSM6DS3_ACC_GYRO_ODR_XL_POWER_DOWN  = 0x00,
    LSM6DS3_ACC_GYRO_ODR_XL_13Hz   = 0x10,
    LSM6DS3_ACC_GYRO_ODR_XL_26Hz   = 0x20,
    LSM6DS3_ACC_GYRO_ODR_XL_52Hz   = 0x30,
    LSM6DS3_ACC_GYRO_ODR_XL_104Hz  = 0x40,
    LSM6DS3_ACC_GYRO_ODR_XL_208Hz  = 0x50,
    LSM6DS3_ACC_GYRO_ODR_XL_416Hz  = 0x60,
    LSM6DS3_ACC_GYRO_ODR_XL_833Hz  = 0x70,
    LSM6DS3_ACC_GYRO_ODR_XL_1660Hz = 0x80,
} LSM6DS3_ACC_GYRO_ODR_XL_t;

#define LSM6DS3_ACC_GYRO_ODR_XL_MASK  0xF0

// Output Data Rate for Gyroscope
typedef enum {
    LSM6DS3_ACC_GYRO_ODR_G_POWER_DOWN  = 0x00,
    LSM6DS3_ACC_GYRO_ODR_G_13Hz = 0x10,
    LSM6DS3_ACC_GYRO_ODR_G_26Hz = 0x20,
    LSM6DS3_ACC_GYRO_ODR_G_52Hz = 0x30,
    LSM6DS3_ACC_GYRO_ODR_G_104Hz = 0x40,
    LSM6DS3_ACC_GYRO_ODR_G_208Hz = 0x50,
    LSM6DS3_ACC_GYRO_ODR_G_416Hz = 0x60,
    LSM6DS3_ACC_GYRO_ODR_G_833Hz = 0x70,
    LSM6DS3_ACC_GYRO_ODR_G_1660Hz = 0x80,
} LSM6DS3_ACC_GYRO_ODR_G_t;

#define LSM6DS3_ACC_GYRO_ODR_G_MASK  0xF0

// Block Data Update
typedef enum {
    LSM6DS3_ACC_GYRO_BDU_CONTINUOS      = 0x00,
    LSM6DS3_ACC_GYRO_BDU_ENABLED        = 0x40,
} LSM6DS3_ACC_GYRO_BDU_t;

#define LSM6DS3_ACC_GYRO_BDU_MASK  0x40

// Tap Events
typedef enum {
    LSM6DS3_ACC_GYRO_SINGLE_TAP_EV_DISABLED = 0x00,
    LSM6DS3_ACC_GYRO_SINGLE_TAP_EV_ENABLED  = 0x20,
} LSM6DS3_ACC_GYRO_SINGLE_TAP_EV_t;

typedef enum {
    LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_DISABLED = 0x00,
    LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_ENABLED  = 0x40,
} LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_t;

// Free Fall Events
typedef enum {
    LSM6DS3_ACC_GYRO_FREE_FALL_EV_DISABLED = 0x00,
    LSM6DS3_ACC_GYRO_FREE_FALL_EV_ENABLED  = 0x20,
} LSM6DS3_ACC_GYRO_FREE_FALL_EV_t;

typedef enum {
    LSM6DS3_ACC_GYRO_INT1_FREE_FALL_DISABLED = 0x00,
    LSM6DS3_ACC_GYRO_INT1_FREE_FALL_ENABLED  = 0x10,
} LSM6DS3_ACC_GYRO_INT1_FREE_FALL_t;

//====================================================================
// CONFIGURATION
//====================================================================

//====================================================================
// SENSITIVITY CONSTANTS (mg/LSB for accel, mdps/LSB for gyro)
//====================================================================
#define LSM6DS3_ACCEL_SENS_2G       0.061f    // ±2g
#define LSM6DS3_ACCEL_SENS_4G       0.122f    // ±4g
#define LSM6DS3_ACCEL_SENS_8G       0.244f    // ±8g
#define LSM6DS3_ACCEL_SENS_16G      0.488f    // ±16g

#define LSM6DS3_GYRO_SENS_125DPS    4.375f    // ±125 dps
#define LSM6DS3_GYRO_SENS_245DPS    8.750f    // ±245 dps
#define LSM6DS3_GYRO_SENS_500DPS    17.500f   // ±500 dps
#define LSM6DS3_GYRO_SENS_1000DPS   35.000f   // ±1000 dps
#define LSM6DS3_GYRO_SENS_2000DPS   70.000f   // ±2000 dps

//====================================================================
// ACCELEROMETER AND GYROSCOPE DATA STRUCTURES
//====================================================================
typedef struct {
    float x;
    float y;
    float z;
} LSM6DS3_Axes_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} LSM6DS3_AxesRaw_t;

//====================================================================
// GLOBAL DATA EXPORTS
//====================================================================
// Accelerometer data (in mg)
extern float LSM6DS3_Accel_X;
extern float LSM6DS3_Accel_Y;
extern float LSM6DS3_Accel_Z;

// Gyroscope data (in mdps)
extern float LSM6DS3_Gyro_X;
extern float LSM6DS3_Gyro_Y;
extern float LSM6DS3_Gyro_Z;

// Temperature (in °C)
extern float LSM6DS3_Temp;

//====================================================================
// INITIALIZATION AND CONTROL
//====================================================================

/**
 * @brief Initialize the LSM6DS3 sensor
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_Init(void);

/**
 * @brief Enable accelerometer
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_EnableAccelerometer(void);

/**
 * @brief Disable accelerometer
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_DisableAccelerometer(void);

/**
 * @brief Enable gyroscope
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_EnableGyroscope(void);

/**
 * @brief Disable gyroscope
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_DisableGyroscope(void);

//====================================================================
// DATA READING FUNCTIONS
//====================================================================

/**
 * @brief Read raw accelerometer data
 * @param axes Pointer to LSM6DS3_AxesRaw_t structure
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_GetAccelerometerRaw(LSM6DS3_AxesRaw_t *axes);

/**
 * @brief Read raw gyroscope data
 * @param axes Pointer to LSM6DS3_AxesRaw_t structure
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_GetGyroscopeRaw(LSM6DS3_AxesRaw_t *axes);

/**
 * @brief Read scaled accelerometer data in mg
 * @param axes Pointer to LSM6DS3_Axes_t structure
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_GetAccelerometer(LSM6DS3_Axes_t *axes);

/**
 * @brief Read scaled gyroscope data in mdps
 * @param axes Pointer to LSM6DS3_Axes_t structure
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_GetGyroscope(LSM6DS3_Axes_t *axes);

/**
 * @brief Read temperature
 * @param temp Pointer to float for temperature in °C
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_GetTemperature(float *temp);

//====================================================================
// CONFIGURATION FUNCTIONS
//====================================================================

/**
 * @brief Set accelerometer full-scale range
 * @param fs Full-scale range (LSM6DS3_ACC_GYRO_FS_XL_t)
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_SetAccelFullScale(LSM6DS3_ACC_GYRO_FS_XL_t fs);

/**
 * @brief Set gyroscope full-scale range
 * @param fs Full-scale range (LSM6DS3_ACC_GYRO_FS_G_t)
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_SetGyroFullScale(LSM6DS3_ACC_GYRO_FS_G_t fs);

/**
 * @brief Set accelerometer output data rate
 * @param odr Output data rate (LSM6DS3_ACC_GYRO_ODR_XL_t)
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_SetAccelODR(LSM6DS3_ACC_GYRO_ODR_XL_t odr);

/**
 * @brief Set gyroscope output data rate
 * @param odr Output data rate (LSM6DS3_ACC_GYRO_ODR_G_t)
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_SetGyroODR(LSM6DS3_ACC_GYRO_ODR_G_t odr);

//====================================================================
// ADVANCED FEATURES
//====================================================================

/**
 * @brief Enable tap detection
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_EnableTapDetection(void);

/**
 * @brief Disable tap detection
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_DisableTapDetection(void);

/**
 * @brief Enable free fall detection
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_EnableFreeFallDetection(void);

/**
 * @brief Disable free fall detection
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_DisableFreeFallDetection(void);

/**
 * @brief Get event status (tap, free fall, etc.)
 * @param status Pointer to uint8_t for status
 * @return MEMS_SUCCESS if successful, MEMS_ERROR otherwise
 */
mems_status_t LSM6DS3_GetEventStatus(uint8_t *status);

#endif // LSM6DS3_H
