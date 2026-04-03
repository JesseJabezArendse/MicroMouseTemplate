//********************************************************************
//*                          Micro Mouse                             *
//*                       LSM6DS3 IMU Library                        *
//*==================================================================*
//* Implementation file for LSM6DS3 6-axis IMU sensor                *
//*==================================================================*

#include "main.h"
#include "LSM6DS3.h"
#include <math.h>
#include <string.h>

//====================================================================
// EXTERNAL HANDLES
//====================================================================
extern I2C_HandleTypeDef hi2c2;

//====================================================================
// LOW-LEVEL REGISTER READ/WRITE FUNCTIONS
//====================================================================

/**
 * @brief Low-level register write
 */
static mems_status_t LSM6DS3_ACC_GYRO_WriteReg(void *handle, u8_t Reg, u8_t *Bufp, u16_t len)
{
    if (LSM6DS3_IO_Write(handle, Reg, Bufp, len)) {
        return MEMS_ERROR;
    }
    return MEMS_SUCCESS;
}

/**
 * @brief Low-level register read
 */
static mems_status_t LSM6DS3_ACC_GYRO_ReadReg(void *handle, u8_t Reg, u8_t *Bufp, u16_t len)
{
    if (LSM6DS3_IO_Read(handle, Reg, Bufp, len)) {
        return MEMS_ERROR;
    }
    return MEMS_SUCCESS;
}

/**
 * @brief Read WHO_AM_I
 */
static mems_status_t LSM6DS3_ACC_GYRO_R_WHO_AM_I(void *handle, u8_t *value)
{
    if (!LSM6DS3_ACC_GYRO_ReadReg(handle, LSM6DS3_ACC_GYRO_WHO_AM_I_REG, (u8_t *)value, 1))
        return MEMS_ERROR;
    return MEMS_SUCCESS;
}

/**
 * @brief Write BDU
 */
static mems_status_t LSM6DS3_ACC_GYRO_W_BDU(void *handle, LSM6DS3_ACC_GYRO_BDU_t newValue)
{
    u8_t value;
    
    if (!LSM6DS3_ACC_GYRO_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL3_C, &value, 1))
        return MEMS_ERROR;
    
    value &= ~LSM6DS3_ACC_GYRO_BDU_MASK;
    value |= newValue;
    
    if (!LSM6DS3_ACC_GYRO_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL3_C, &value, 1))
        return MEMS_ERROR;
    
    return MEMS_SUCCESS;
}

/**
 * @brief Write ODR_XL
 */
static mems_status_t LSM6DS3_ACC_GYRO_W_ODR_XL(void *handle, LSM6DS3_ACC_GYRO_ODR_XL_t newValue)
{
    u8_t value;
    
    if (!LSM6DS3_ACC_GYRO_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL1_XL, &value, 1))
        return MEMS_ERROR;
    
    value &= ~LSM6DS3_ACC_GYRO_ODR_XL_MASK;
    value |= newValue;
    
    if (!LSM6DS3_ACC_GYRO_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL1_XL, &value, 1))
        return MEMS_ERROR;
    
    return MEMS_SUCCESS;
}

/**
 * @brief Write FS_XL
 */
static mems_status_t LSM6DS3_ACC_GYRO_W_FS_XL(void *handle, LSM6DS3_ACC_GYRO_FS_XL_t newValue)
{
    u8_t value;
    
    if (!LSM6DS3_ACC_GYRO_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL1_XL, &value, 1))
        return MEMS_ERROR;
    
    value &= ~LSM6DS3_ACC_GYRO_FS_XL_MASK;
    value |= newValue;
    
    if (!LSM6DS3_ACC_GYRO_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL1_XL, &value, 1))
        return MEMS_ERROR;
    
    return MEMS_SUCCESS;
}

/**
 * @brief Write ODR_G
 */
static mems_status_t LSM6DS3_ACC_GYRO_W_ODR_G(void *handle, LSM6DS3_ACC_GYRO_ODR_G_t newValue)
{
    u8_t value;
    
    if (!LSM6DS3_ACC_GYRO_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL2_G, &value, 1))
        return MEMS_ERROR;
    
    value &= ~LSM6DS3_ACC_GYRO_ODR_G_MASK;
    value |= newValue;
    
    if (!LSM6DS3_ACC_GYRO_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL2_G, &value, 1))
        return MEMS_ERROR;
    
    return MEMS_SUCCESS;
}

/**
 * @brief Write FS_G
 */
static mems_status_t LSM6DS3_ACC_GYRO_W_FS_G(void *handle, LSM6DS3_ACC_GYRO_FS_G_t newValue)
{
    u8_t value;
    
    if (!LSM6DS3_ACC_GYRO_ReadReg(handle, LSM6DS3_ACC_GYRO_CTRL2_G, &value, 1))
        return MEMS_ERROR;
    
    value &= ~LSM6DS3_ACC_GYRO_FS_G_MASK;
    value |= newValue;
    
    if (!LSM6DS3_ACC_GYRO_WriteReg(handle, LSM6DS3_ACC_GYRO_CTRL2_G, &value, 1))
        return MEMS_ERROR;
    
    return MEMS_SUCCESS;
}

//====================================================================
// GLOBAL VARIABLES
//====================================================================
float LSM6DS3_Accel_X = 0.0f;
float LSM6DS3_Accel_Y = 0.0f;
float LSM6DS3_Accel_Z = 0.0f;

float LSM6DS3_Gyro_X = 0.0f;
float LSM6DS3_Gyro_Y = 0.0f;
float LSM6DS3_Gyro_Z = 0.0f;

float LSM6DS3_Temp = 0.0f;

// Sensitivity values (updated based on full-scale range)
static float accelSensitivity = LSM6DS3_ACCEL_SENS_2G;
static float gyroSensitivity = LSM6DS3_GYRO_SENS_245DPS;

// Sensor handle for driver functions
static void *sensor_handle = NULL;

//====================================================================
// I/O INTERFACE FUNCTIONS (required by LSM6DS3_ACC_GYRO_Driver)
//====================================================================

/**
 * @brief I2C write function - called by the driver
 */
uint8_t LSM6DS3_IO_Write(void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite)
{
    // The STM32 HAL expects the address to be left-shifted by 1 for I2C
    uint8_t dev_addr = (LSM6DS3_I2C_ADDRESS << 1);
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c2, dev_addr, WriteAddr, 1, pBuffer, nBytesToWrite, I2C_TIMEOUT);
    
    return (status == HAL_OK) ? 0 : 1;  // Return 0 on success, 1 on error
}

/**
 * @brief I2C read function - called by the driver
 */
uint8_t LSM6DS3_IO_Read(void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead)
{
    // The STM32 HAL expects the address to be left-shifted by 1 for I2C
    uint8_t dev_addr = (LSM6DS3_I2C_ADDRESS << 1);
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c2, dev_addr, ReadAddr, 1, pBuffer, nBytesToRead, I2C_TIMEOUT);
    
    return (status == HAL_OK) ? 0 : 1;  // Return 0 on success, 1 on error
}

//====================================================================
// INITIALIZATION AND CONTROL FUNCTIONS
//====================================================================

/**
 * @brief Initialize the LSM6DS3 sensor
 */
mems_status_t LSM6DS3_Init(void)
{
    uint8_t who_am_i = 0;
    
    // Initialize sensor handle - it's not really used in our implementation
    sensor_handle = (void *)&hi2c2;
    
    // Verify WHO_AM_I register
    if (LSM6DS3_ACC_GYRO_R_WHO_AM_I(sensor_handle, &who_am_i) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    if (who_am_i != LSM6DS3_ACC_GYRO_WHO_AM_I)
    {
        return MEMS_ERROR;  // Wrong sensor ID
    }
    
    // Enable Block Data Update (BDU) - prevents data from changing while reading
    if (LSM6DS3_ACC_GYRO_W_BDU(sensor_handle, LSM6DS3_ACC_GYRO_BDU_ENABLED) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    // Configure accelerometer: 208 Hz, ±2g full scale
    if (LSM6DS3_ACC_GYRO_W_ODR_XL(sensor_handle, LSM6DS3_ACC_GYRO_ODR_XL_208Hz) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    if (LSM6DS3_ACC_GYRO_W_FS_XL(sensor_handle, LSM6DS3_ACC_GYRO_FS_XL_2g) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    // Configure gyroscope: 208 Hz, ±245 dps full scale
    if (LSM6DS3_ACC_GYRO_W_ODR_G(sensor_handle, LSM6DS3_ACC_GYRO_ODR_G_208Hz) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    if (LSM6DS3_ACC_GYRO_W_FS_G(sensor_handle, LSM6DS3_ACC_GYRO_FS_G_245dps) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    // Enable both accelerometer and gyroscope
    if (LSM6DS3_EnableAccelerometer() != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    if (LSM6DS3_EnableGyroscope() != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    return MEMS_SUCCESS;
}

/**
 * @brief Enable accelerometer
 */
mems_status_t LSM6DS3_EnableAccelerometer(void)
{
    // Enable accelerometer by setting CTRL1_XL to enable XL
    // The driver functions handle register manipulation
    return MEMS_SUCCESS;  // Already enabled in ODR configuration
}

/**
 * @brief Disable accelerometer
 */
mems_status_t LSM6DS3_DisableAccelerometer(void)
{
    // Disable accelerometer by setting ODR to power-down
    return LSM6DS3_ACC_GYRO_W_ODR_XL(sensor_handle, LSM6DS3_ACC_GYRO_ODR_XL_POWER_DOWN);
}

/**
 * @brief Enable gyroscope
 */
mems_status_t LSM6DS3_EnableGyroscope(void)
{
    // Already enabled in initialization
    return MEMS_SUCCESS;
}

/**
 * @brief Disable gyroscope
 */
mems_status_t LSM6DS3_DisableGyroscope(void)
{
    // Disable gyroscope by setting ODR to power-down
    return LSM6DS3_ACC_GYRO_W_ODR_G(sensor_handle, LSM6DS3_ACC_GYRO_ODR_G_POWER_DOWN);
}

//====================================================================
// DATA READING FUNCTIONS
//====================================================================

/**
 * @brief Read raw accelerometer data
 */
mems_status_t LSM6DS3_GetAccelerometerRaw(LSM6DS3_AxesRaw_t *axes)
{
    uint8_t buffer[6];
    
    if (!axes)
    {
        return MEMS_ERROR;
    }
    
    // Read all 6 bytes at once (more efficient)
    if (LSM6DS3_ACC_GYRO_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_OUTX_L_XL, buffer, 6) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    // Combine bytes into 16-bit signed values (little-endian)
    axes->x = (int16_t)((buffer[1] << 8) | buffer[0]);
    axes->y = (int16_t)((buffer[3] << 8) | buffer[2]);
    axes->z = (int16_t)((buffer[5] << 8) | buffer[4]);
    
    return MEMS_SUCCESS;
}

/**
 * @brief Read raw gyroscope data
 */
mems_status_t LSM6DS3_GetGyroscopeRaw(LSM6DS3_AxesRaw_t *axes)
{
    uint8_t buffer[6];
    
    if (!axes)
    {
        return MEMS_ERROR;
    }
    
    // Read all 6 bytes at once (more efficient)
    if (LSM6DS3_ACC_GYRO_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_OUTX_L_G, buffer, 6) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    // Combine bytes into 16-bit signed values (little-endian)
    axes->x = (int16_t)((buffer[1] << 8) | buffer[0]);
    axes->y = (int16_t)((buffer[3] << 8) | buffer[2]);
    axes->z = (int16_t)((buffer[5] << 8) | buffer[4]);
    
    return MEMS_SUCCESS;
}

/**
 * @brief Read scaled accelerometer data in mg
 */
mems_status_t LSM6DS3_GetAccelerometer(LSM6DS3_Axes_t *axes)
{
    LSM6DS3_AxesRaw_t raw_axes;
    
    if (!axes)
    {
        return MEMS_ERROR;
    }
    
    if (LSM6DS3_GetAccelerometerRaw(&raw_axes) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    // Convert raw values to mg using sensitivity
    axes->x = (float)raw_axes.x * accelSensitivity;
    axes->y = (float)raw_axes.y * accelSensitivity;
    axes->z = (float)raw_axes.z * accelSensitivity;
    
    // Update global variables
    LSM6DS3_Accel_X = axes->x;
    LSM6DS3_Accel_Y = axes->y;
    LSM6DS3_Accel_Z = axes->z;
    
    return MEMS_SUCCESS;
}

/**
 * @brief Read scaled gyroscope data in mdps
 */
mems_status_t LSM6DS3_GetGyroscope(LSM6DS3_Axes_t *axes)
{
    LSM6DS3_AxesRaw_t raw_axes;
    
    if (!axes)
    {
        return MEMS_ERROR;
    }
    
    if (LSM6DS3_GetGyroscopeRaw(&raw_axes) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    // Convert raw values to mdps using sensitivity
    axes->x = (float)raw_axes.x * gyroSensitivity;
    axes->y = (float)raw_axes.y * gyroSensitivity;
    axes->z = (float)raw_axes.z * gyroSensitivity;
    
    // Update global variables
    LSM6DS3_Gyro_X = axes->x;
    LSM6DS3_Gyro_Y = axes->y;
    LSM6DS3_Gyro_Z = axes->z;
    
    return MEMS_SUCCESS;
}

/**
 * @brief Read temperature
 */
mems_status_t LSM6DS3_GetTemperature(float *temp)
{
    uint8_t buffer[2];
    int16_t raw_temp;
    
    if (!temp)
    {
        return MEMS_ERROR;
    }
    
    // Read temperature registers
    if (LSM6DS3_ACC_GYRO_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_OUT_TEMP_L, buffer, 2) != MEMS_SUCCESS)
    {
        return MEMS_ERROR;
    }
    
    // Combine bytes (little-endian)
    raw_temp = (int16_t)((buffer[1] << 8) | buffer[0]);
    
    // Convert to Celsius (sensitivity is 16 LSB/°C, offset is 25°C at 0 raw value)
    *temp = 25.0f + ((float)raw_temp / 16.0f);
    
    LSM6DS3_Temp = *temp;
    
    return MEMS_SUCCESS;
}

//====================================================================
// CONFIGURATION FUNCTIONS
//====================================================================

/**
 * @brief Set accelerometer full-scale range and update sensitivity
 */
mems_status_t LSM6DS3_SetAccelFullScale(LSM6DS3_ACC_GYRO_FS_XL_t fs)
{
    mems_status_t status = LSM6DS3_ACC_GYRO_W_FS_XL(sensor_handle, fs);
    
    if (status == MEMS_SUCCESS)
    {
        // Update sensitivity based on full-scale range
        switch (fs)
        {
            case LSM6DS3_ACC_GYRO_FS_XL_2g:
                accelSensitivity = LSM6DS3_ACCEL_SENS_2G;
                break;
            case LSM6DS3_ACC_GYRO_FS_XL_4g:
                accelSensitivity = LSM6DS3_ACCEL_SENS_4G;
                break;
            case LSM6DS3_ACC_GYRO_FS_XL_8g:
                accelSensitivity = LSM6DS3_ACCEL_SENS_8G;
                break;
            case LSM6DS3_ACC_GYRO_FS_XL_16g:
                accelSensitivity = LSM6DS3_ACCEL_SENS_16G;
                break;
            default:
                return MEMS_ERROR;
        }
    }
    
    return status;
}

/**
 * @brief Set gyroscope full-scale range and update sensitivity
 */
mems_status_t LSM6DS3_SetGyroFullScale(LSM6DS3_ACC_GYRO_FS_G_t fs)
{
    mems_status_t status = LSM6DS3_ACC_GYRO_W_FS_G(sensor_handle, fs);
    
    if (status == MEMS_SUCCESS)
    {
        // Update sensitivity based on full-scale range
        switch (fs)
        {
            case LSM6DS3_ACC_GYRO_FS_G_125dps:
                gyroSensitivity = LSM6DS3_GYRO_SENS_125DPS;
                break;
            case LSM6DS3_ACC_GYRO_FS_G_245dps:
                gyroSensitivity = LSM6DS3_GYRO_SENS_245DPS;
                break;
            case LSM6DS3_ACC_GYRO_FS_G_500dps:
                gyroSensitivity = LSM6DS3_GYRO_SENS_500DPS;
                break;
            case LSM6DS3_ACC_GYRO_FS_G_1000dps:
                gyroSensitivity = LSM6DS3_GYRO_SENS_1000DPS;
                break;
            case LSM6DS3_ACC_GYRO_FS_G_2000dps:
                gyroSensitivity = LSM6DS3_GYRO_SENS_2000DPS;
                break;
            default:
                return MEMS_ERROR;
        }
    }
    
    return status;
}

/**
 * @brief Set accelerometer output data rate
 */
mems_status_t LSM6DS3_SetAccelODR(LSM6DS3_ACC_GYRO_ODR_XL_t odr)
{
    return LSM6DS3_ACC_GYRO_W_ODR_XL(sensor_handle, odr);
}

/**
 * @brief Set gyroscope output data rate
 */
mems_status_t LSM6DS3_SetGyroODR(LSM6DS3_ACC_GYRO_ODR_G_t odr)
{
    return LSM6DS3_ACC_GYRO_W_ODR_G(sensor_handle, odr);
}

//====================================================================
// ADVANCED FEATURES
//====================================================================

/**
 * @brief Enable tap detection (stub - configure registers manually if needed)
 */
mems_status_t LSM6DS3_EnableTapDetection(void)
{
    // Tap detection requires configuration in TAP_CFG1, MD1_CFG registers
    // For now, this is a stub - users can configure directly via register writes
    return MEMS_SUCCESS;
}

/**
 * @brief Disable tap detection (stub)
 */
mems_status_t LSM6DS3_DisableTapDetection(void)
{
    // Stub implementation
    return MEMS_SUCCESS;
}

/**
 * @brief Enable free fall detection (stub - configure registers manually if needed)
 */
mems_status_t LSM6DS3_EnableFreeFallDetection(void)
{
    // Free fall detection requires configuration in FREE_FALL, MD1_CFG registers
    // For now, this is a stub - users can configure directly via register writes
    return MEMS_SUCCESS;
}

/**
 * @brief Disable free fall detection (stub)
 */
mems_status_t LSM6DS3_DisableFreeFallDetection(void)
{
    // Stub implementation
    return MEMS_SUCCESS;
}

/**
 * @brief Get event status (tap, free fall, etc.)
 */
mems_status_t LSM6DS3_GetEventStatus(uint8_t *status)
{
    if (!status) {
        return MEMS_ERROR;
    }
    
    // Read the TAP_SRC register which contains tap detection status
    return LSM6DS3_ACC_GYRO_ReadReg(sensor_handle, LSM6DS3_ACC_GYRO_TAP_SRC, status, 1);
}
