//********************************************************************
//*                          Micro Mouse                             *
//*                          IMU Library (Multi-IC Support)          *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      18-10-2024                                           *
//* @modified:  Dual IC support with conditional compilation         *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This IMU library supports multiple sensor options:               *
//*   - LSM6DS3 (STMicroelectronics) - Default                       *
//*   - ICM-42605 (TDK InvenSense) - Define IMU_USE_ICM42605         *
//*                                                                  *
//* Configure via compiler define: IMU_USE_ICM42605 or default LSM6DS3
//********************************************************************

#ifndef IMU_H
#define IMU_H

#include "stm32l4xx.h"
#include "main.h"
#include <stdint.h>

//====================================================================
// IMU IC SELECTION
//====================================================================
// Define IMU_USE_ICM42605 to use ICM-42605, otherwise defaults to LSM6DS3
// #define IMU_USE_ICM42605

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
#define IMU_DPS2RAD 0.01745329251994329576923690768489f  // π/180
#define IMU_RAD2DPS 57.295779513082320876798154814105f   // 180/π

//====================================================================
// I2C CONFIGURATION (Common)
//====================================================================
// I2C timeout is defined in main.h (20ms)

//====================================================================
// IC-SPECIFIC DEFINITIONS
//====================================================================

#ifdef IMU_USE_ICM42605
// ==================== ICM-42605 Configuration ====================

#define ICM42605_I2C_ADDRESS    0x68  // 7-bit address
#define ICM42605_REG_WHO_AM_I   0x75
#define ICM42605_WHO_AM_I_VAL   0x42

// Register Map (BANK 0)
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
    ICM42605_REG_BANK_SEL           = 0x76
} ICM42605_Register_t;

// Accelerometer FSR
typedef enum {
    ICM42605_ACCEL_FS_16G = 0x00,
    ICM42605_ACCEL_FS_8G  = 0x01,
    ICM42605_ACCEL_FS_4G  = 0x02,
    ICM42605_ACCEL_FS_2G  = 0x03
} ICM42605_AccelFS_t;

#define ICM42605_ACCEL_SENS_16G  2048.0f
#define ICM42605_ACCEL_SENS_8G   4096.0f
#define ICM42605_ACCEL_SENS_4G   8192.0f
#define ICM42605_ACCEL_SENS_2G   16384.0f

// Gyroscope FSR
typedef enum {
    ICM42605_GYRO_FS_2000DPS   = 0x00,
    ICM42605_GYRO_FS_1000DPS   = 0x01,
    ICM42605_GYRO_FS_500DPS    = 0x02,
    ICM42605_GYRO_FS_250DPS    = 0x03,
    ICM42605_GYRO_FS_125DPS    = 0x04,
    ICM42605_GYRO_FS_62_5DPS   = 0x05,
    ICM42605_GYRO_FS_31_25DPS  = 0x06,
    ICM42605_GYRO_FS_15_125DPS = 0x07
} ICM42605_GyroFS_t;

#define ICM42605_GYRO_SENS_2000DPS    16.4f
#define ICM42605_GYRO_SENS_1000DPS    32.8f
#define ICM42605_GYRO_SENS_500DPS     65.5f
#define ICM42605_GYRO_SENS_250DPS     131.0f
#define ICM42605_GYRO_SENS_125DPS     262.0f
#define ICM42605_GYRO_SENS_62_5DPS    524.3f
#define ICM42605_GYRO_SENS_31_25DPS   1048.6f
#define ICM42605_GYRO_SENS_15_125DPS  2097.2f

// Accel ODR
typedef enum {
    ICM42605_ACCEL_ODR_8000Hz   = 0x03,
    ICM42605_ACCEL_ODR_4000Hz   = 0x04,
    ICM42605_ACCEL_ODR_2000Hz   = 0x05,
    ICM42605_ACCEL_ODR_1000Hz   = 0x06,
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

// Gyro ODR
typedef enum {
    ICM42605_GYRO_ODR_8000Hz  = 0x03,
    ICM42605_GYRO_ODR_4000Hz  = 0x04,
    ICM42605_GYRO_ODR_2000Hz  = 0x05,
    ICM42605_GYRO_ODR_1000Hz  = 0x06,
    ICM42605_GYRO_ODR_200Hz   = 0x07,
    ICM42605_GYRO_ODR_100Hz   = 0x08,
    ICM42605_GYRO_ODR_50Hz    = 0x09,
    ICM42605_GYRO_ODR_25Hz    = 0x0A,
    ICM42605_GYRO_ODR_12_5Hz  = 0x0B,
    ICM42605_GYRO_ODR_500Hz   = 0x0F
} ICM42605_GyroODR_t;

// Power Mode
typedef enum {
    ICM42605_PWR_MODE_SLEEP     = 0x00,
    ICM42605_PWR_MODE_STANDBY   = 0x04,
    ICM42605_PWR_MODE_LOW_NOISE = 0x0F
} ICM42605_PowerMode_t;

#else  // Default: LSM6DS3
// ==================== LSM6DS3 Configuration ====================

#define LSM6DS3_I2C_ADDRESS         0x6A  // 7-bit address
#define LSM6DS3_ACC_GYRO_WHO_AM_I   0x69

// Register Addresses
#define LSM6DS3_ACC_GYRO_CTRL1_XL        0x10
#define LSM6DS3_ACC_GYRO_CTRL2_G         0x11
#define LSM6DS3_ACC_GYRO_CTRL3_C         0x12
#define LSM6DS3_ACC_GYRO_WHO_AM_I_REG    0x0F
#define LSM6DS3_ACC_GYRO_OUTX_L_XL       0x28
#define LSM6DS3_ACC_GYRO_OUTY_L_XL       0x2A
#define LSM6DS3_ACC_GYRO_OUTZ_L_XL       0x2C
#define LSM6DS3_ACC_GYRO_OUTX_L_G        0x22
#define LSM6DS3_ACC_GYRO_OUTY_L_G        0x24
#define LSM6DS3_ACC_GYRO_OUTZ_L_G        0x26
#define LSM6DS3_ACC_GYRO_OUT_TEMP_L      0x20
#define LSM6DS3_ACC_GYRO_TAP_SRC         0x1C

// Accelerometer FSR
typedef enum {
    LSM6DS3_ACC_GYRO_FS_XL_2g   = 0x00,
    LSM6DS3_ACC_GYRO_FS_XL_4g   = 0x08,
    LSM6DS3_ACC_GYRO_FS_XL_8g   = 0x0C,
    LSM6DS3_ACC_GYRO_FS_XL_16g  = 0x04,
} LSM6DS3_ACC_GYRO_FS_XL_t;

#define LSM6DS3_ACC_GYRO_FS_XL_MASK  0x0C

// Gyroscope FSR
typedef enum {
    LSM6DS3_ACC_GYRO_FS_G_125dps   = 0x02,
    LSM6DS3_ACC_GYRO_FS_G_245dps   = 0x00,
    LSM6DS3_ACC_GYRO_FS_G_500dps   = 0x04,
    LSM6DS3_ACC_GYRO_FS_G_1000dps  = 0x08,
    LSM6DS3_ACC_GYRO_FS_G_2000dps  = 0x0C,
} LSM6DS3_ACC_GYRO_FS_G_t;

#define LSM6DS3_ACC_GYRO_FS_G_MASK  0x0C

// Accelerometer ODR
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

// Gyroscope ODR
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
    LSM6DS3_ACC_GYRO_BDU_CONTINUOS = 0x00,
    LSM6DS3_ACC_GYRO_BDU_ENABLED   = 0x40,
} LSM6DS3_ACC_GYRO_BDU_t;

#define LSM6DS3_ACC_GYRO_BDU_MASK  0x40

// Sensitivity Constants
#define LSM6DS3_ACCEL_SENS_2G       0.061f
#define LSM6DS3_ACCEL_SENS_4G       0.122f
#define LSM6DS3_ACCEL_SENS_8G       0.244f
#define LSM6DS3_ACCEL_SENS_16G      0.488f

#define LSM6DS3_GYRO_SENS_125DPS    4.375f
#define LSM6DS3_GYRO_SENS_245DPS    8.750f
#define LSM6DS3_GYRO_SENS_500DPS    17.500f
#define LSM6DS3_GYRO_SENS_1000DPS   35.000f
#define LSM6DS3_GYRO_SENS_2000DPS   70.000f

// Data Structures
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

#endif  // IMU_USE_ICM42605

//====================================================================
// EXTERNAL VARIABLES (Common to all ICs)
//====================================================================
extern float IMU_Accel[3];      // Accelerometer data in m/s² [X, Y, Z]
extern float IMU_Gyro[3];       // Gyroscope data in rad/s [X, Y, Z]
extern float IMU_Gyro_DPS[3];   // Gyroscope data in °/s [X, Y, Z]
extern float IMU_Temp;          // Temperature in °C

//====================================================================
// FUNCTION DECLARATIONS (Common API)
//====================================================================

/**
 * @brief Initialize the IMU sensor
 */
void initIMU(void);

/**
 * @brief Read all sensor values from the IMU
 */
void refreshIMUValues(void);

/**
 * @brief Dynamically calibrate IMU full-scale ranges
 */
void calibrateIMU(void);

/**
 * @brief Check IMU communication
 * @return WHO_AM_I value
 */
uint8_t checkIMU(void);

//====================================================================

#endif // IMU_H

//********************************************************************
// END OF PROGRAM
//********************************************************************
