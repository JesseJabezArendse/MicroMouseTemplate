//********************************************************************
//*                          Micro Mouse                             *
//*                          IMU Library                             *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      18-10-2024                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This header file provides an interface for communicating with    *
//* the ICM-42605 6-axis IMU sensor. The library handles sensor      *
//* initialization, dynamic FSR calibration, and reading of          *
//* accelerometer and gyroscope data for motion tracking.            *
//*                                                                  *
//* Datasheet: DS-000292-ICM-42605-v1.5.pdf                          *
//* https://invensense.tdk.com/wp-content/uploads/2020/09/DS-000292-ICM-42605-v1.5.pdf
//********************************************************************

#ifndef IMU_H
#define IMU_H

#include "stm32l4xx.h"
#include "main.h"

//====================================================================
// CONFIGURATION
//====================================================================
#ifndef COMPILED_BY_SIMULINK
#define IMU_DYNAMIC_FSR  // Enable dynamic full-scale range adjustment
#endif

//====================================================================
// GLOBAL CONSTANTS
//====================================================================
#define IMU_GRAVITATIONAL_ACCELERATION 9.80665f  // Standard gravity in m/s²
#define IMU_DPS2RAD 0.01745329251994329576923690768489f  // π/180: degrees/s to rad/s
#define IMU_RAD2DPS 57.295779513082320876798154814105f   // 180/π: rad/s to degrees/s

//====================================================================
// ICM-42605 I2C CONFIGURATION
//====================================================================
#define ICM42605_I2C_ADDRESS    0x68  // 7-bit address (0xD0 write, 0xD1 read in 8-bit)
#define I2C_TIMEOUT             100   // I2C timeout in ms

//====================================================================
// ICM-42605 REGISTER MAP (BANK 0)
//====================================================================
typedef enum {
    ICM42605_REG_DEVICE_CONFIG      = 0x11,
    ICM42605_REG_DRIVE_CONFIG       = 0x13,
    ICM42605_REG_INT_CONFIG         = 0x14,
    ICM42605_REG_FIFO_CONFIG        = 0x16,
    ICM42605_REG_TEMP_DATA1         = 0x1D,
    ICM42605_REG_TEMP_DATA0         = 0x1E,
    ICM42605_REG_ACCEL_DATA_X1      = 0x1F,
    ICM42605_REG_ACCEL_DATA_X0      = 0x20,
    ICM42605_REG_ACCEL_DATA_Y1      = 0x21,
    ICM42605_REG_ACCEL_DATA_Y0      = 0x22,
    ICM42605_REG_ACCEL_DATA_Z1      = 0x23,
    ICM42605_REG_ACCEL_DATA_Z0      = 0x24,
    ICM42605_REG_GYRO_DATA_X1       = 0x25,
    ICM42605_REG_GYRO_DATA_X0       = 0x26,
    ICM42605_REG_GYRO_DATA_Y1       = 0x27,
    ICM42605_REG_GYRO_DATA_Y0       = 0x28,
    ICM42605_REG_GYRO_DATA_Z1       = 0x29,
    ICM42605_REG_GYRO_DATA_Z0       = 0x2A,
    ICM42605_REG_INT_STATUS         = 0x2D,
    ICM42605_REG_PWR_MGMT0          = 0x4E,
    ICM42605_REG_GYRO_CONFIG0       = 0x4F,
    ICM42605_REG_ACCEL_CONFIG0      = 0x50,
    ICM42605_REG_GYRO_CONFIG1       = 0x51,
    ICM42605_REG_ACCEL_CONFIG1      = 0x53,
    ICM42605_REG_WHO_AM_I           = 0x75,
    ICM42605_REG_BANK_SEL           = 0x76
} ICM42605_Register_t;

//====================================================================
// ACCELEROMETER FULL-SCALE RANGE (AFS_SEL)
//====================================================================
typedef enum {
    ICM42605_ACCEL_FS_16G = 0x00,  // ±16g (default)
    ICM42605_ACCEL_FS_8G  = 0x01,  // ±8g
    ICM42605_ACCEL_FS_4G  = 0x02,  // ±4g
    ICM42605_ACCEL_FS_2G  = 0x03   // ±2g
} ICM42605_AccelFS_t;

// Accelerometer sensitivity values (LSB/g) from datasheet
#define ICM42605_ACCEL_SENS_16G  2048.0f   // ±16g: 2048 LSB/g
#define ICM42605_ACCEL_SENS_8G   4096.0f   // ±8g:  4096 LSB/g
#define ICM42605_ACCEL_SENS_4G   8192.0f   // ±4g:  8192 LSB/g
#define ICM42605_ACCEL_SENS_2G   16384.0f  // ±2g:  16384 LSB/g

//====================================================================
// GYROSCOPE FULL-SCALE RANGE (GYRO_FS_SEL)
//====================================================================
typedef enum {
    ICM42605_GYRO_FS_2000DPS   = 0x00,  // ±2000°/s (default)
    ICM42605_GYRO_FS_1000DPS   = 0x01,  // ±1000°/s
    ICM42605_GYRO_FS_500DPS    = 0x02,  // ±500°/s
    ICM42605_GYRO_FS_250DPS    = 0x03,  // ±250°/s
    ICM42605_GYRO_FS_125DPS    = 0x04,  // ±125°/s
    ICM42605_GYRO_FS_62_5DPS   = 0x05,  // ±62.5°/s
    ICM42605_GYRO_FS_31_25DPS  = 0x06,  // ±31.25°/s
    ICM42605_GYRO_FS_15_125DPS = 0x07   // ±15.125°/s
} ICM42605_GyroFS_t;

// Gyroscope sensitivity values (LSB/(°/s)) from datasheet Table 2
#define ICM42605_GYRO_SENS_2000DPS    16.4f    // ±2000°/s:   16.4 LSB/(°/s)
#define ICM42605_GYRO_SENS_1000DPS    32.8f    // ±1000°/s:   32.8 LSB/(°/s)
#define ICM42605_GYRO_SENS_500DPS     65.5f    // ±500°/s:    65.5 LSB/(°/s)
#define ICM42605_GYRO_SENS_250DPS     131.0f   // ±250°/s:    131.0 LSB/(°/s)
#define ICM42605_GYRO_SENS_125DPS     262.0f   // ±125°/s:    262.0 LSB/(°/s)
#define ICM42605_GYRO_SENS_62_5DPS    524.3f   // ±62.5°/s:   524.3 LSB/(°/s)
#define ICM42605_GYRO_SENS_31_25DPS   1048.6f  // ±31.25°/s:  1048.6 LSB/(°/s)
#define ICM42605_GYRO_SENS_15_125DPS  2097.2f  // ±15.125°/s: 2097.2 LSB/(°/s)

//====================================================================
// ACCELEROMETER OUTPUT DATA RATE (ACCEL_ODR)
//====================================================================
typedef enum {
    ICM42605_ACCEL_ODR_8000Hz   = 0x03,
    ICM42605_ACCEL_ODR_4000Hz   = 0x04,
    ICM42605_ACCEL_ODR_2000Hz   = 0x05,
    ICM42605_ACCEL_ODR_1000Hz   = 0x06,  // Default
    ICM42605_ACCEL_ODR_200Hz    = 0x07,
    ICM42605_ACCEL_ODR_100Hz    = 0x08,
    ICM42605_ACCEL_ODR_50Hz     = 0x09,
    ICM42605_ACCEL_ODR_25Hz     = 0x0A,
    ICM42605_ACCEL_ODR_12_5Hz   = 0x0B,
    ICM42605_ACCEL_ODR_6_25Hz   = 0x0C,
    ICM42605_ACCEL_ODR_3_125Hz  = 0x0D,
    ICM42605_ACCEL_ODR_1_5625Hz = 0x0E,
    ICM42605_ACCEL_ODR_500Hz    = 0x0F
} ICM42605_AccelODR_t;

//====================================================================
// GYROSCOPE OUTPUT DATA RATE (GYRO_ODR)
//====================================================================
typedef enum {
    ICM42605_GYRO_ODR_8000Hz  = 0x03,
    ICM42605_GYRO_ODR_4000Hz  = 0x04,
    ICM42605_GYRO_ODR_2000Hz  = 0x05,
    ICM42605_GYRO_ODR_1000Hz  = 0x06,  // Default
    ICM42605_GYRO_ODR_200Hz   = 0x07,
    ICM42605_GYRO_ODR_100Hz   = 0x08,
    ICM42605_GYRO_ODR_50Hz    = 0x09,
    ICM42605_GYRO_ODR_25Hz    = 0x0A,
    ICM42605_GYRO_ODR_12_5Hz  = 0x0B,
    ICM42605_GYRO_ODR_500Hz   = 0x0F
} ICM42605_GyroODR_t;

//====================================================================
// POWER MANAGEMENT MODES
//====================================================================
typedef enum {
    ICM42605_PWR_MODE_SLEEP     = 0x00,
    ICM42605_PWR_MODE_STANDBY   = 0x04,
    ICM42605_PWR_MODE_LOW_NOISE = 0x0F  // Both gyro and accel in low noise mode
} ICM42605_PowerMode_t;

//====================================================================
// EXTERNAL VARIABLES
//====================================================================
extern float IMU_Accel[3];      // Accelerometer data in m/s² [X, Y, Z]
extern float IMU_Gyro[3];       // Gyroscope data in rad/s [X, Y, Z]
extern float IMU_Gyro_DPS[3];   // Gyroscope data in °/s [X, Y, Z] (for debugging)
extern float IMU_Temp;          // Temperature in °C

//====================================================================
// FUNCTION DECLARATIONS
//====================================================================

/**
 * @brief Initialize the ICM-42605 IMU sensor
 * @note Sets up I2C communication, configures power mode, FSR, and ODR
 */
void initIMU(void);

/**
 * @brief Read all sensor values from the IMU
 * @note Updates IMU_Accel[], IMU_Gyro[], IMU_Gyro_DPS[], and IMU_Temp
 */
void refreshIMUValues(void);

/**
 * @brief Dynamically calibrate IMU full-scale ranges based on current readings
 * @note Only active if IMU_DYNAMIC_FSR is defined
 */
void calibrateIMU(void);

/**
 * @brief Check IMU communication by reading WHO_AM_I register
 * @return WHO_AM_I register value (should be 0x42 for ICM-42605)
 */
uint8_t checkIMU(void);

//====================================================================

#endif // IMU_H

//********************************************************************
// END OF PROGRAM
//********************************************************************
