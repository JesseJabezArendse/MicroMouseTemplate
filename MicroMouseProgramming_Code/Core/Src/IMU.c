//********************************************************************
//*                          Micro Mouse                             *
//*                          IMU Library (Multi-IC)                  *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      18-10-2024                                           *
//* @modified:  Dual IC support (ICM-42605 / LSM6DS3)                *
//*==================================================================*

#include "main.h"
#include "IMU.h"
#include <math.h>
#include <string.h>

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

//====================================================================
#ifdef IMU_USE_ICM42605
#pragma region ICM42605_IMPLEMENTATION
// ================== ICM-42605 Implementation ========================
//====================================================================

// Current sensitivity values
static float accelSensitivity = ICM42605_ACCEL_SENS_2G;
static float gyroSensitivity = ICM42605_GYRO_SENS_15_125DPS;

// Current FSR configuration
static ICM42605_AccelFS_t currentAccelFS = ICM42605_ACCEL_FS_2G;
static ICM42605_GyroFS_t currentGyroFS = ICM42605_GYRO_FS_15_125DPS;

//====================================================================
// PRIVATE I2C FUNCTIONS
//====================================================================

static void writeByte(uint8_t regAddr, uint8_t value) {
    HAL_I2C_Mem_Write(&hi2c2, (ICM42605_I2C_ADDRESS << 1), regAddr, 1, &value, 1, I2C_TIMEOUT);
}

static uint8_t readByte(uint8_t regAddr) {
    uint8_t value = 0;
    HAL_I2C_Mem_Read(&hi2c2, (ICM42605_I2C_ADDRESS << 1), regAddr, 1, &value, 1, I2C_TIMEOUT);
    
    if (hi2c2.ErrorCode != HAL_I2C_ERROR_NONE) {
        restartI2C();
    }
    return value;
}

static int16_t readWord(uint8_t regAddr) {
    uint8_t buffer[2];
    HAL_I2C_Mem_Read(&hi2c2, (ICM42605_I2C_ADDRESS << 1), regAddr, 1, buffer, 2, I2C_TIMEOUT);
    
    if (hi2c2.ErrorCode != HAL_I2C_ERROR_NONE) {
        restartI2C();
    }
    
    return (int16_t)((buffer[0] << 8) | buffer[1]);
}

//====================================================================
// FSR CONFIGURATION
//====================================================================

static void setAccelFS(ICM42605_AccelFS_t fs) {
    uint8_t config = readByte(ICM42605_REG_ACCEL_CONFIG0);
    config = (config & 0x1F) | (fs << 5);
    writeByte(ICM42605_REG_ACCEL_CONFIG0, config);
    currentAccelFS = fs;
    
    switch (fs) {
        case ICM42605_ACCEL_FS_2G:  accelSensitivity = ICM42605_ACCEL_SENS_2G; break;
        case ICM42605_ACCEL_FS_4G:  accelSensitivity = ICM42605_ACCEL_SENS_4G; break;
        case ICM42605_ACCEL_FS_8G:  accelSensitivity = ICM42605_ACCEL_SENS_8G; break;
        case ICM42605_ACCEL_FS_16G: accelSensitivity = ICM42605_ACCEL_SENS_16G; break;
        default: break;
    }
}

static void setGyroFS(ICM42605_GyroFS_t fs) {
    uint8_t config = readByte(ICM42605_REG_GYRO_CONFIG0);
    config = (config & 0x1F) | (fs << 5);
    writeByte(ICM42605_REG_GYRO_CONFIG0, config);
    currentGyroFS = fs;
    
    switch (fs) {
        case ICM42605_GYRO_FS_15_125DPS: gyroSensitivity = ICM42605_GYRO_SENS_15_125DPS; break;
        case ICM42605_GYRO_FS_31_25DPS:  gyroSensitivity = ICM42605_GYRO_SENS_31_25DPS; break;
        case ICM42605_GYRO_FS_62_5DPS:   gyroSensitivity = ICM42605_GYRO_SENS_62_5DPS; break;
        case ICM42605_GYRO_FS_125DPS:    gyroSensitivity = ICM42605_GYRO_SENS_125DPS; break;
        case ICM42605_GYRO_FS_250DPS:    gyroSensitivity = ICM42605_GYRO_SENS_250DPS; break;
        case ICM42605_GYRO_FS_500DPS:    gyroSensitivity = ICM42605_GYRO_SENS_500DPS; break;
        case ICM42605_GYRO_FS_1000DPS:   gyroSensitivity = ICM42605_GYRO_SENS_1000DPS; break;
        case ICM42605_GYRO_FS_2000DPS:   gyroSensitivity = ICM42605_GYRO_SENS_2000DPS; break;
        default: break;
    }
}

//====================================================================
// PUBLIC API
//====================================================================

uint8_t checkIMU(void) {
    return readByte(ICM42605_REG_WHO_AM_I);
}

void initIMU(void) {
    writeByte(ICM42605_REG_BANK_SEL, 0x00);
    HAL_Delay(1);
    
    writeByte(ICM42605_REG_PWR_MGMT0, ICM42605_PWR_MODE_LOW_NOISE);
    HAL_Delay(50);
    
    uint8_t accelConfig = (ICM42605_ACCEL_FS_2G << 5) | ICM42605_ACCEL_ODR_1000Hz;
    writeByte(ICM42605_REG_ACCEL_CONFIG0, accelConfig);
    
    uint8_t gyroConfig = (ICM42605_GYRO_FS_15_125DPS << 5) | ICM42605_GYRO_ODR_1000Hz;
    writeByte(ICM42605_REG_GYRO_CONFIG0, gyroConfig);
    
    uint8_t gyroConfig1 = readByte(ICM42605_REG_GYRO_CONFIG1);
    gyroConfig1 |= 0xD0;
    writeByte(ICM42605_REG_GYRO_CONFIG1, gyroConfig1);
    
    currentAccelFS = ICM42605_ACCEL_FS_2G;
    currentGyroFS = ICM42605_GYRO_FS_15_125DPS;
    accelSensitivity = ICM42605_ACCEL_SENS_2G;
    gyroSensitivity = ICM42605_GYRO_SENS_15_125DPS;
    
    HAL_Delay(10);
}

void refreshIMUValues(void) {
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);

    int16_t accel_x_raw = readWord(ICM42605_REG_ACCEL_DATA_X1);
    int16_t accel_y_raw = readWord(ICM42605_REG_ACCEL_DATA_Y1);
    int16_t accel_z_raw = readWord(ICM42605_REG_ACCEL_DATA_Z1);
    
    IMU_Accel[0] = ((float)accel_x_raw / accelSensitivity) * IMU_GRAVITATIONAL_ACCELERATION;
    IMU_Accel[1] = ((float)accel_y_raw / accelSensitivity) * IMU_GRAVITATIONAL_ACCELERATION;
    IMU_Accel[2] = ((float)accel_z_raw / accelSensitivity) * IMU_GRAVITATIONAL_ACCELERATION;
    
    int16_t gyro_x_raw = readWord(ICM42605_REG_GYRO_DATA_X1);
    int16_t gyro_y_raw = readWord(ICM42605_REG_GYRO_DATA_Y1);
    int16_t gyro_z_raw = readWord(ICM42605_REG_GYRO_DATA_Z1);
    
    IMU_Gyro_DPS[0] = (float)gyro_x_raw / gyroSensitivity;
    IMU_Gyro_DPS[1] = (float)gyro_y_raw / gyroSensitivity;
    IMU_Gyro_DPS[2] = (float)gyro_z_raw / gyroSensitivity;
    
    IMU_Gyro[0] = IMU_Gyro_DPS[0] * IMU_DPS2RAD;
    IMU_Gyro[1] = IMU_Gyro_DPS[1] * IMU_DPS2RAD;
    IMU_Gyro[2] = IMU_Gyro_DPS[2] * IMU_DPS2RAD;
    
    int16_t temp_raw = readWord(ICM42605_REG_TEMP_DATA1);
    IMU_Temp = ((float)temp_raw / 132.48f) + 25.0f;
    
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    
    #ifdef IMU_DYNAMIC_FSR
    calibrateIMU();
    #endif
}

void calibrateIMU(void) {
    float accelMaxMag = 0.0f;
    for (int i = 0; i < 3; i++) {
        float absAccel = fabsf(IMU_Accel[i] / IMU_GRAVITATIONAL_ACCELERATION);
        if (absAccel > accelMaxMag) {
            accelMaxMag = absAccel;
        }
    }
    
    ICM42605_AccelFS_t newAccelFS = currentAccelFS;
    if (accelMaxMag < 1.6f) {
        newAccelFS = ICM42605_ACCEL_FS_2G;
    } else if (accelMaxMag < 3.2f) {
        newAccelFS = ICM42605_ACCEL_FS_4G;
    } else if (accelMaxMag < 6.4f) {
        newAccelFS = ICM42605_ACCEL_FS_8G;
    } else {
        newAccelFS = ICM42605_ACCEL_FS_16G;
    }
    
    float gyroMaxMag = 0.0f;
    for (int i = 0; i < 3; i++) {
        float absGyro = fabsf(IMU_Gyro[i]);
        if (absGyro > gyroMaxMag) {
            gyroMaxMag = absGyro;
        }
    }
    
    ICM42605_GyroFS_t newGyroFS = currentGyroFS;
    if (gyroMaxMag < 0.211f) {
        newGyroFS = ICM42605_GYRO_FS_15_125DPS;
    } else if (gyroMaxMag < 0.436f) {
        newGyroFS = ICM42605_GYRO_FS_31_25DPS;
    } else if (gyroMaxMag < 0.873f) {
        newGyroFS = ICM42605_GYRO_FS_62_5DPS;
    } else if (gyroMaxMag < 1.746f) {
        newGyroFS = ICM42605_GYRO_FS_125DPS;
    } else if (gyroMaxMag < 3.490f) {
        newGyroFS = ICM42605_GYRO_FS_250DPS;
    } else if (gyroMaxMag < 6.981f) {
        newGyroFS = ICM42605_GYRO_FS_500DPS;
    } else if (gyroMaxMag < 13.963f) {
        newGyroFS = ICM42605_GYRO_FS_1000DPS;
    } else {
        newGyroFS = ICM42605_GYRO_FS_2000DPS;
    }
    
    if (newAccelFS != currentAccelFS) {
        setAccelFS(newAccelFS);
    }
    if (newGyroFS != currentGyroFS) {
        setGyroFS(newGyroFS);
    }
}

#pragma endregion ICM42605_IMPLEMENTATION

#else
#pragma region LSM6DS3_IMPLEMENTATION
// ================== LSM6DS3 Implementation ===========================
//====================================================================

// Current sensitivity values
static float accelSensitivity = LSM6DS3_ACCEL_SENS_2G;
static float gyroSensitivity = LSM6DS3_GYRO_SENS_245DPS;

// Current FSR configuration
static LSM6DS3_ACC_GYRO_FS_XL_t currentAccelFS = LSM6DS3_ACC_GYRO_FS_XL_2g;
static LSM6DS3_ACC_GYRO_FS_G_t currentGyroFS = LSM6DS3_ACC_GYRO_FS_G_245dps;

// Sensor handle
static void *sensor_handle = NULL;

//====================================================================
// LOW-LEVEL I2C FUNCTIONS
//====================================================================

static uint8_t LSM6DS3_IO_Write(void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite) {
    uint8_t dev_addr = (LSM6DS3_I2C_ADDRESS << 1);
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c2, dev_addr, WriteAddr, 1, pBuffer, nBytesToWrite, I2C_TIMEOUT);
    return (status == HAL_OK) ? 0 : 1;
}

static uint8_t LSM6DS3_IO_Read(void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead) {
    uint8_t dev_addr = (LSM6DS3_I2C_ADDRESS << 1);
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c2, dev_addr, ReadAddr, 1, pBuffer, nBytesToRead, I2C_TIMEOUT);
    return (status == HAL_OK) ? 0 : 1;
}

//====================================================================
// REGISTER READ/WRITE
//====================================================================

static mems_status_t LSM6DS3_WriteReg(void *handle, uint8_t Reg, uint8_t *Bufp, uint16_t len) {
    return (LSM6DS3_IO_Write(handle, Reg, Bufp, len)) ? MEMS_ERROR : MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_ReadReg(void *handle, uint8_t Reg, uint8_t *Bufp, uint16_t len) {
    return (LSM6DS3_IO_Read(handle, Reg, Bufp, len)) ? MEMS_ERROR : MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_W_BDU(void *handle, LSM6DS3_ACC_GYRO_BDU_t newValue) {
    uint8_t value;
    if (!LSM6DS3_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL3_C, &value, 1)) return MEMS_ERROR;
    value = (value & ~LSM6DS3_ACC_GYRO_BDU_MASK) | newValue;
    if (!LSM6DS3_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL3_C, &value, 1)) return MEMS_ERROR;
    return MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_W_ODR_XL(void *handle, LSM6DS3_ACC_GYRO_ODR_XL_t newValue) {
    uint8_t value;
    if (!LSM6DS3_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL1_XL, &value, 1)) return MEMS_ERROR;
    value = (value & ~LSM6DS3_ACC_GYRO_ODR_XL_MASK) | newValue;
    if (!LSM6DS3_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL1_XL, &value, 1)) return MEMS_ERROR;
    return MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_W_FS_XL(void *handle, LSM6DS3_ACC_GYRO_FS_XL_t newValue) {
    uint8_t value;
    if (!LSM6DS3_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL1_XL, &value, 1)) return MEMS_ERROR;
    value = (value & ~LSM6DS3_ACC_GYRO_FS_XL_MASK) | newValue;
    if (!LSM6DS3_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL1_XL, &value, 1)) return MEMS_ERROR;
    return MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_W_ODR_G(void *handle, LSM6DS3_ACC_GYRO_ODR_G_t newValue) {
    uint8_t value;
    if (!LSM6DS3_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL2_G, &value, 1)) return MEMS_ERROR;
    value = (value & ~LSM6DS3_ACC_GYRO_ODR_G_MASK) | newValue;
    if (!LSM6DS3_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL2_G, &value, 1)) return MEMS_ERROR;
    return MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_W_FS_G(void *handle, LSM6DS3_ACC_GYRO_FS_G_t newValue) {
    uint8_t value;
    if (!LSM6DS3_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL2_G, &value, 1)) return MEMS_ERROR;
    value = (value & ~LSM6DS3_ACC_GYRO_FS_G_MASK) | newValue;
    if (!LSM6DS3_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL2_G, &value, 1)) return MEMS_ERROR;
    return MEMS_SUCCESS;
}

//====================================================================
// DATA READING
//====================================================================

static mems_status_t LSM6DS3_GetAccelerometerRaw(LSM6DS3_AxesRaw_t *axes) {
    uint8_t buffer[6];
    if (!axes) return MEMS_ERROR;
    if (LSM6DS3_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_OUTX_L_XL, buffer, 6) != MEMS_SUCCESS) return MEMS_ERROR;
    axes->x = (int16_t)((buffer[1] << 8) | buffer[0]);
    axes->y = (int16_t)((buffer[3] << 8) | buffer[2]);
    axes->z = (int16_t)((buffer[5] << 8) | buffer[4]);
    return MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_GetGyroscopeRaw(LSM6DS3_AxesRaw_t *axes) {
    uint8_t buffer[6];
    if (!axes) return MEMS_ERROR;
    if (LSM6DS3_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_OUTX_L_G, buffer, 6) != MEMS_SUCCESS) return MEMS_ERROR;
    axes->x = (int16_t)((buffer[1] << 8) | buffer[0]);
    axes->y = (int16_t)((buffer[3] << 8) | buffer[2]);
    axes->z = (int16_t)((buffer[5] << 8) | buffer[4]);
    return MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_GetAccelerometer(LSM6DS3_Axes_t *axes) {
    LSM6DS3_AxesRaw_t raw_axes;
    if (!axes) return MEMS_ERROR;
    if (LSM6DS3_GetAccelerometerRaw(&raw_axes) != MEMS_SUCCESS) return MEMS_ERROR;
    axes->x = (float)raw_axes.x * accelSensitivity;
    axes->y = (float)raw_axes.y * accelSensitivity;
    axes->z = (float)raw_axes.z * accelSensitivity;
    return MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_GetGyroscope(LSM6DS3_Axes_t *axes) {
    LSM6DS3_AxesRaw_t raw_axes;
    if (!axes) return MEMS_ERROR;
    if (LSM6DS3_GetGyroscopeRaw(&raw_axes) != MEMS_SUCCESS) return MEMS_ERROR;
    axes->x = (float)raw_axes.x * gyroSensitivity;
    axes->y = (float)raw_axes.y * gyroSensitivity;
    axes->z = (float)raw_axes.z * gyroSensitivity;
    return MEMS_SUCCESS;
}

static mems_status_t LSM6DS3_GetTemperature(float *temp) {
    uint8_t buffer[2];
    int16_t raw_temp;
    if (!temp) return MEMS_ERROR;
    if (LSM6DS3_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_OUT_TEMP_L, buffer, 2) != MEMS_SUCCESS) return MEMS_ERROR;
    raw_temp = (int16_t)((buffer[1] << 8) | buffer[0]);
    *temp = 25.0f + ((float)raw_temp / 16.0f);
    return MEMS_SUCCESS;
}

//====================================================================
// FSR CONFIGURATION
//====================================================================

static mems_status_t LSM6DS3_SetAccelFullScale(LSM6DS3_ACC_GYRO_FS_XL_t fs) {
    mems_status_t status = LSM6DS3_W_FS_XL(sensor_handle, fs);
    if (status == MEMS_SUCCESS) {
        currentAccelFS = fs;
        switch (fs) {
            case LSM6DS3_ACC_GYRO_FS_XL_2g:   accelSensitivity = LSM6DS3_ACCEL_SENS_2G; break;
            case LSM6DS3_ACC_GYRO_FS_XL_4g:   accelSensitivity = LSM6DS3_ACCEL_SENS_4G; break;
            case LSM6DS3_ACC_GYRO_FS_XL_8g:   accelSensitivity = LSM6DS3_ACCEL_SENS_8G; break;
            case LSM6DS3_ACC_GYRO_FS_XL_16g:  accelSensitivity = LSM6DS3_ACCEL_SENS_16G; break;
            default: return MEMS_ERROR;
        }
    }
    return status;
}

static mems_status_t LSM6DS3_SetGyroFullScale(LSM6DS3_ACC_GYRO_FS_G_t fs) {
    mems_status_t status = LSM6DS3_W_FS_G(sensor_handle, fs);
    if (status == MEMS_SUCCESS) {
        currentGyroFS = fs;
        switch (fs) {
            case LSM6DS3_ACC_GYRO_FS_G_125dps:   gyroSensitivity = LSM6DS3_GYRO_SENS_125DPS; break;
            case LSM6DS3_ACC_GYRO_FS_G_245dps:   gyroSensitivity = LSM6DS3_GYRO_SENS_245DPS; break;
            case LSM6DS3_ACC_GYRO_FS_G_500dps:   gyroSensitivity = LSM6DS3_GYRO_SENS_500DPS; break;
            case LSM6DS3_ACC_GYRO_FS_G_1000dps:  gyroSensitivity = LSM6DS3_GYRO_SENS_1000DPS; break;
            case LSM6DS3_ACC_GYRO_FS_G_2000dps:  gyroSensitivity = LSM6DS3_GYRO_SENS_2000DPS; break;
            default: return MEMS_ERROR;
        }
    }
    return status;
}

//====================================================================
// PUBLIC API
//====================================================================

uint8_t checkIMU(void) {
    uint8_t who_am_i = 0;
    sensor_handle = (void *)&hi2c2;
    if (LSM6DS3_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_WHO_AM_I_REG, &who_am_i, 1) == MEMS_SUCCESS) {
        return who_am_i;
    }
    return 0x00;
}

void initIMU(void) {
    sensor_handle = (void *)&hi2c2;
    
    uint8_t who_am_i = 0;
    LSM6DS3_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_WHO_AM_I_REG, &who_am_i, 1);
    
    LSM6DS3_W_BDU(sensor_handle, LSM6DS3_ACC_GYRO_BDU_ENABLED);
    
    LSM6DS3_W_ODR_XL(sensor_handle, LSM6DS3_ACC_GYRO_ODR_XL_208Hz);
    LSM6DS3_W_FS_XL(sensor_handle, LSM6DS3_ACC_GYRO_FS_XL_2g);
    
    LSM6DS3_W_ODR_G(sensor_handle, LSM6DS3_ACC_GYRO_ODR_G_208Hz);
    LSM6DS3_W_FS_G(sensor_handle, LSM6DS3_ACC_GYRO_FS_G_245dps);
    
    currentAccelFS = LSM6DS3_ACC_GYRO_FS_XL_2g;
    currentGyroFS = LSM6DS3_ACC_GYRO_FS_G_245dps;
    accelSensitivity = LSM6DS3_ACCEL_SENS_2G;
    gyroSensitivity = LSM6DS3_GYRO_SENS_245DPS;
    
    HAL_Delay(10);
}

void refreshIMUValues(void) {
    LSM6DS3_Axes_t accelData, gyroData;
    float temp;
    
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);

    if (LSM6DS3_GetAccelerometer(&accelData) == MEMS_SUCCESS) {
        IMU_Accel[0] = (accelData.x / 1000.0f) * IMU_GRAVITATIONAL_ACCELERATION;
        IMU_Accel[1] = (accelData.y / 1000.0f) * IMU_GRAVITATIONAL_ACCELERATION;
        IMU_Accel[2] = (accelData.z / 1000.0f) * IMU_GRAVITATIONAL_ACCELERATION;
    }
    
    if (LSM6DS3_GetGyroscope(&gyroData) == MEMS_SUCCESS) {
        IMU_Gyro_DPS[0] = gyroData.x / 1000.0f;
        IMU_Gyro_DPS[1] = gyroData.y / 1000.0f;
        IMU_Gyro_DPS[2] = gyroData.z / 1000.0f;
        
        IMU_Gyro[0] = IMU_Gyro_DPS[0] * IMU_DPS2RAD;
        IMU_Gyro[1] = IMU_Gyro_DPS[1] * IMU_DPS2RAD;
        IMU_Gyro[2] = IMU_Gyro_DPS[2] * IMU_DPS2RAD;
    }
    
    if (LSM6DS3_GetTemperature(&temp) == MEMS_SUCCESS) {
        IMU_Temp = temp;
    }
    
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    
    #ifdef IMU_DYNAMIC_FSR
    calibrateIMU();
    #endif
}

void calibrateIMU(void) {
    float accelMaxMag = 0.0f;
    for (int i = 0; i < 3; i++) {
        float absAccel = fabsf(IMU_Accel[i] / IMU_GRAVITATIONAL_ACCELERATION);
        if (absAccel > accelMaxMag) {
            accelMaxMag = absAccel;
        }
    }
    
    LSM6DS3_ACC_GYRO_FS_XL_t newAccelFS = currentAccelFS;
    if (accelMaxMag < 1.6f) {
        newAccelFS = LSM6DS3_ACC_GYRO_FS_XL_2g;
    } else if (accelMaxMag < 3.2f) {
        newAccelFS = LSM6DS3_ACC_GYRO_FS_XL_4g;
    } else if (accelMaxMag < 6.4f) {
        newAccelFS = LSM6DS3_ACC_GYRO_FS_XL_8g;
    } else {
        newAccelFS = LSM6DS3_ACC_GYRO_FS_XL_16g;
    }
    
    float gyroMaxMag = 0.0f;
    for (int i = 0; i < 3; i++) {
        float absGyro = fabsf(IMU_Gyro[i]);
        if (absGyro > gyroMaxMag) {
            gyroMaxMag = absGyro;
        }
    }
    
    LSM6DS3_ACC_GYRO_FS_G_t newGyroFS = currentGyroFS;
    if (gyroMaxMag < 0.211f) {
        newGyroFS = LSM6DS3_ACC_GYRO_FS_G_125dps;
    } else if (gyroMaxMag < 0.436f) {
        newGyroFS = LSM6DS3_ACC_GYRO_FS_G_245dps;
    } else if (gyroMaxMag < 0.873f) {
        newGyroFS = LSM6DS3_ACC_GYRO_FS_G_500dps;
    } else if (gyroMaxMag < 1.746f) {
        newGyroFS = LSM6DS3_ACC_GYRO_FS_G_1000dps;
    } else {
        newGyroFS = LSM6DS3_ACC_GYRO_FS_G_2000dps;
    }
    
    if (newAccelFS != currentAccelFS) {
        LSM6DS3_SetAccelFullScale(newAccelFS);
    }
    if (newGyroFS != currentGyroFS) {
        LSM6DS3_SetGyroFullScale(newGyroFS);
    }
}

#pragma endregion LSM6DS3_IMPLEMENTATION

#endif  // IMU_USE_ICM42605

//********************************************************************
// END OF PROGRAM
//********************************************************************
