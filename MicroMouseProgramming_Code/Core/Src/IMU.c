//********************************************************************
//*                          Micro Mouse                             *
//*                          IMU Library                             *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      18-10-2024                                           *
//*==================================================================*

#include "main.h"
#include "IMU.h"
#include <math.h>

//====================================================================
// EXTERNAL HANDLES
//====================================================================
extern I2C_HandleTypeDef hi2c2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

//====================================================================
// GLOBAL VARIABLES
//====================================================================
float IMU_Accel[3] = {0.0f, 0.0f, 0.0f};
float IMU_Gyro[3] = {0.0f, 0.0f, 0.0f};
float IMU_Gyro_DPS[3] = {0.0f, 0.0f, 0.0f};
float IMU_Temp = 0.0f;

// Current sensitivity values (updated by calibrateIMU)
static float accelSensitivity = ICM42605_ACCEL_SENS_2G;  // Default: ±2g
static float gyroSensitivity = ICM42605_GYRO_SENS_15_125DPS;  // Default: ±15.125°/s

// Current FSR configuration (for dynamic calibration)
static ICM42605_AccelFS_t currentAccelFS = ICM42605_ACCEL_FS_2G;
static ICM42605_GyroFS_t currentGyroFS = ICM42605_GYRO_FS_15_125DPS;

//====================================================================
// PRIVATE FUNCTION PROTOTYPES
//====================================================================
static void writeByte(uint8_t regAddr, uint8_t value);
static uint8_t readByte(uint8_t regAddr);
static int16_t readWord(uint8_t regAddr);
static void setAccelFS(ICM42605_AccelFS_t fs);
static void setGyroFS(ICM42605_GyroFS_t fs);
static float getAccelSensitivity(ICM42605_AccelFS_t fs);
static float getGyroSensitivity(ICM42605_GyroFS_t fs);

//====================================================================
// LOW-LEVEL I2C FUNCTIONS
//====================================================================

/**
 * @brief Write a single byte to ICM-42605 register
 */
static void writeByte(uint8_t regAddr, uint8_t value) {
    HAL_I2C_Mem_Write(&hi2c2, (ICM42605_I2C_ADDRESS << 1), regAddr, 1, &value, 1, I2C_TIMEOUT);
}

/**
 * @brief Read a single byte from ICM-42605 register
 */
static uint8_t readByte(uint8_t regAddr) {
    uint8_t value = 0;
    HAL_I2C_Mem_Read(&hi2c2, (ICM42605_I2C_ADDRESS << 1), regAddr, 1, &value, 1, I2C_TIMEOUT);
    
    // Check for I2C errors
    if (hi2c2.ErrorCode != HAL_I2C_ERROR_NONE) {
        restartI2C();
    }

    return value;
}

/**
 * @brief Read a 16-bit word (big-endian) from ICM-42605 register
 * @param regAddr Address of the high byte register
 * @return Signed 16-bit value
 */
static int16_t readWord(uint8_t regAddr) {
    uint8_t buffer[2];
    HAL_I2C_Mem_Read(&hi2c2, (ICM42605_I2C_ADDRESS << 1), regAddr, 1, buffer, 2, I2C_TIMEOUT);
    
    // Check for I2C errors
    if (hi2c2.ErrorCode != HAL_I2C_ERROR_NONE) {
        restartI2C();
    }
    
    // Combine high and low bytes (big-endian)
    return (int16_t)((buffer[0] << 8) | buffer[1]);
}

//====================================================================
// CONFIGURATION FUNCTIONS
//====================================================================

/**
 * @brief Set accelerometer full-scale range
 */
static void setAccelFS(ICM42605_AccelFS_t fs) {
    uint8_t config = readByte(ICM42605_REG_ACCEL_CONFIG0);
    config = (config & 0x1F) | (fs << 5);  // Clear bits 7:5, set new FS
    writeByte(ICM42605_REG_ACCEL_CONFIG0, config);
    currentAccelFS = fs;
    accelSensitivity = getAccelSensitivity(fs);
}

/**
 * @brief Set gyroscope full-scale range
 */
static void setGyroFS(ICM42605_GyroFS_t fs) {
    uint8_t config = readByte(ICM42605_REG_GYRO_CONFIG0);
    config = (config & 0x1F) | (fs << 5);  // Clear bits 7:5, set new FS
    writeByte(ICM42605_REG_GYRO_CONFIG0, config);
    currentGyroFS = fs;
    gyroSensitivity = getGyroSensitivity(fs);
}

/**
 * @brief Get accelerometer sensitivity for a given FSR
 */
static float getAccelSensitivity(ICM42605_AccelFS_t fs) {
    switch (fs) {
        case ICM42605_ACCEL_FS_2G:  return ICM42605_ACCEL_SENS_2G;
        case ICM42605_ACCEL_FS_4G:  return ICM42605_ACCEL_SENS_4G;
        case ICM42605_ACCEL_FS_8G:  return ICM42605_ACCEL_SENS_8G;
        case ICM42605_ACCEL_FS_16G: return ICM42605_ACCEL_SENS_16G;
        default: return ICM42605_ACCEL_SENS_2G;
    }
}

/**
 * @brief Get gyroscope sensitivity for a given FSR
 */
static float getGyroSensitivity(ICM42605_GyroFS_t fs) {
    switch (fs) {
        case ICM42605_GYRO_FS_15_125DPS: return ICM42605_GYRO_SENS_15_125DPS;
        case ICM42605_GYRO_FS_31_25DPS:  return ICM42605_GYRO_SENS_31_25DPS;
        case ICM42605_GYRO_FS_62_5DPS:   return ICM42605_GYRO_SENS_62_5DPS;
        case ICM42605_GYRO_FS_125DPS:    return ICM42605_GYRO_SENS_125DPS;
        case ICM42605_GYRO_FS_250DPS:    return ICM42605_GYRO_SENS_250DPS;
        case ICM42605_GYRO_FS_500DPS:    return ICM42605_GYRO_SENS_500DPS;
        case ICM42605_GYRO_FS_1000DPS:   return ICM42605_GYRO_SENS_1000DPS;
        case ICM42605_GYRO_FS_2000DPS:   return ICM42605_GYRO_SENS_2000DPS;
        default: return ICM42605_GYRO_SENS_15_125DPS;
    }
}

//====================================================================
// PUBLIC FUNCTIONS
//====================================================================

/**
 * @brief Check IMU communication by reading WHO_AM_I register
 */
uint8_t checkIMU(void) {
    return readByte(ICM42605_REG_WHO_AM_I);
}

/**
 * @brief Initialize the ICM-42605 IMU sensor
 */
void initIMU(void) {
    // Ensure we're in Bank 0
    writeByte(ICM42605_REG_BANK_SEL, 0x00);
    HAL_Delay(1);
    
    // Set power mode to low noise for both gyro and accel
    writeByte(ICM42605_REG_PWR_MGMT0, ICM42605_PWR_MODE_LOW_NOISE);
    HAL_Delay(50);  // Wait for sensors to power up
    
    // Configure accelerometer: ±2g, 1000 Hz ODR
    uint8_t accelConfig = (ICM42605_ACCEL_FS_2G << 5) | ICM42605_ACCEL_ODR_1000Hz;
    writeByte(ICM42605_REG_ACCEL_CONFIG0, accelConfig);
    
    // Configure gyroscope: ±15.125°/s, 1000 Hz ODR
    uint8_t gyroConfig = (ICM42605_GYRO_FS_15_125DPS << 5) | ICM42605_GYRO_ODR_1000Hz;
    writeByte(ICM42605_REG_GYRO_CONFIG0, gyroConfig);
    
    // Set temperature sensor low pass filter to 5Hz (GYRO_CONFIG1)
    uint8_t gyroConfig1 = readByte(ICM42605_REG_GYRO_CONFIG1);
    gyroConfig1 |= 0xD0;  // Set temp LPF
    writeByte(ICM42605_REG_GYRO_CONFIG1, gyroConfig1);
    
    // Initialize sensitivity values
    currentAccelFS = ICM42605_ACCEL_FS_2G;
    currentGyroFS = ICM42605_GYRO_FS_15_125DPS;
    accelSensitivity = ICM42605_ACCEL_SENS_2G;
    gyroSensitivity = ICM42605_GYRO_SENS_15_125DPS;
    
    HAL_Delay(10);  // Allow settings to stabilize
}

/**
 * @brief Read all sensor values from the IMU
 */
void refreshIMUValues(void) {
    // Temporarily stop PWM to reduce I2C noise interference
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);

    // Read accelerometer data (3 axes, 16-bit each)
    int16_t accel_x_raw = readWord(ICM42605_REG_ACCEL_DATA_X1);
    int16_t accel_y_raw = readWord(ICM42605_REG_ACCEL_DATA_Y1);
    int16_t accel_z_raw = readWord(ICM42605_REG_ACCEL_DATA_Z1);
    
    // Convert to m/s² (raw / sensitivity = g, then multiply by gravity constant)
    IMU_Accel[0] = ((float)accel_x_raw / accelSensitivity) * IMU_GRAVITATIONAL_ACCELERATION;
    IMU_Accel[1] = ((float)accel_y_raw / accelSensitivity) * IMU_GRAVITATIONAL_ACCELERATION;
    IMU_Accel[2] = ((float)accel_z_raw / accelSensitivity) * IMU_GRAVITATIONAL_ACCELERATION;
    
    // Read gyroscope data (3 axes, 16-bit each)
    int16_t gyro_x_raw = readWord(ICM42605_REG_GYRO_DATA_X1);
    int16_t gyro_y_raw = readWord(ICM42605_REG_GYRO_DATA_Y1);
    int16_t gyro_z_raw = readWord(ICM42605_REG_GYRO_DATA_Z1);
    
    // Convert to °/s
    IMU_Gyro_DPS[0] = (float)gyro_x_raw / gyroSensitivity;
    IMU_Gyro_DPS[1] = (float)gyro_y_raw / gyroSensitivity;
    IMU_Gyro_DPS[2] = (float)gyro_z_raw / gyroSensitivity;
    
    // Convert to rad/s
    IMU_Gyro[0] = IMU_Gyro_DPS[0] * IMU_DPS2RAD;
    IMU_Gyro[1] = IMU_Gyro_DPS[1] * IMU_DPS2RAD;
    IMU_Gyro[2] = IMU_Gyro_DPS[2] * IMU_DPS2RAD;
    
    // Read temperature data
    int16_t temp_raw = readWord(ICM42605_REG_TEMP_DATA1);
    IMU_Temp = ((float)temp_raw / 132.48f) + 25.0f;  // Convert to °C per datasheet
    
    // Restart PWM
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    
    #ifdef IMU_DYNAMIC_FSR
    calibrateIMU();
    #endif
}

/**
 * @brief Dynamically calibrate IMU full-scale ranges based on current readings
 * @note Uses 80% of max range as threshold to switch FSR before saturation
 */
void calibrateIMU(void) {
    // Find maximum accelerometer magnitude across all axes
    float accelMaxMag = 0.0f;
    for (int i = 0; i < 3; i++) {
        float absAccel = fabsf(IMU_Accel[i] / IMU_GRAVITATIONAL_ACCELERATION);  // Convert m/s² to g
        if (absAccel > accelMaxMag) {
            accelMaxMag = absAccel;
        }
    }
    
    // Determine optimal accelerometer FSR (using 80% threshold)
    ICM42605_AccelFS_t newAccelFS = currentAccelFS;
    
    if (accelMaxMag < 1.6f) {  // 80% of ±2g
        newAccelFS = ICM42605_ACCEL_FS_2G;
    } else if (accelMaxMag < 3.2f) {  // 80% of ±4g
        newAccelFS = ICM42605_ACCEL_FS_4G;
    } else if (accelMaxMag < 6.4f) {  // 80% of ±8g
        newAccelFS = ICM42605_ACCEL_FS_8G;
    } else {  // Above 6.4g
        newAccelFS = ICM42605_ACCEL_FS_16G;
    }
    
    // Find maximum gyroscope magnitude across all axes
    float gyroMaxMag = 0.0f;
    for (int i = 0; i < 3; i++) {
        float absGyro = fabsf(IMU_Gyro[i]);  // Already in rad/s
        if (absGyro > gyroMaxMag) {
            gyroMaxMag = absGyro;
        }
    }
    
    // Determine optimal gyroscope FSR (using 80% threshold)
    // Thresholds are 80% of max range in rad/s
    ICM42605_GyroFS_t newGyroFS = currentGyroFS;
    
    if (gyroMaxMag < 0.211f) {  // 80% of ±15.125°/s (0.264 rad/s)
        newGyroFS = ICM42605_GYRO_FS_15_125DPS;
    } else if (gyroMaxMag < 0.436f) {  // 80% of ±31.25°/s (0.545 rad/s)
        newGyroFS = ICM42605_GYRO_FS_31_25DPS;
    } else if (gyroMaxMag < 0.873f) {  // 80% of ±62.5°/s (1.091 rad/s)
        newGyroFS = ICM42605_GYRO_FS_62_5DPS;
    } else if (gyroMaxMag < 1.746f) {  // 80% of ±125°/s (2.182 rad/s)
        newGyroFS = ICM42605_GYRO_FS_125DPS;
    } else if (gyroMaxMag < 3.490f) {  // 80% of ±250°/s (4.363 rad/s)
        newGyroFS = ICM42605_GYRO_FS_250DPS;
    } else if (gyroMaxMag < 6.981f) {  // 80% of ±500°/s (8.727 rad/s)
        newGyroFS = ICM42605_GYRO_FS_500DPS;
    } else if (gyroMaxMag < 13.963f) {  // 80% of ±1000°/s (17.453 rad/s)
        newGyroFS = ICM42605_GYRO_FS_1000DPS;
    } else {  // Above 13.963 rad/s
        newGyroFS = ICM42605_GYRO_FS_2000DPS;
    }
    
    // Apply new configurations only if they changed
    if (newAccelFS != currentAccelFS) {
        setAccelFS(newAccelFS);
    }
    
    if (newGyroFS != currentGyroFS) {
        setGyroFS(newGyroFS);
    }
}

//********************************************************************
// END OF PROGRAM
//********************************************************************
