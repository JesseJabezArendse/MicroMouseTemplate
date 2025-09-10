//********************************************************************
//*                          Micro Mouse                             *
//*                          IMU Library                             *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      24-10-2023                                           *
//*==================================================================*

#include "main.h"
#include "IMU.h"
#include <math.h>

extern I2C_HandleTypeDef hi2c2;

uint8_t checking = 0;

float accel_x = 0;
float accel_y = 0;
float accel_z = 0;
float gyro_x  = 0;
float gyro_y  = 0;
float gyro_z  = 0;

uint16_t accelMem[3];
uint16_t gyroMem[3];

short IMUCounter = 0;

float IMU_Accel[3];
float IMU_Gyro[3];
float IMU_Temp;
uint8_t checkIMU;
float accelDivisor = 16384.0f;
float gyroDivisor = 131.0f;

int16_t signNumber16(uint16_t unsignedValue){
  int16_t signedValue;

  if (unsignedValue <= INT16_MAX) {
        signedValue = (int16_t)unsignedValue; // No change needed, it fits in the signed range.
    } else {
        signedValue = -((int16_t)(UINT16_MAX - unsignedValue + 1));
    }

  return signedValue;
}

static void WriteMem(uint8_t devAddress, uint8_t RegisterAddress, uint16_t Value)
{
  uint8_t addr[2];
  addr[0] = (Value >> 8) & 0xff;  // upper byte
  addr[1] = (Value >> 0) & 0xff; // lower byte
  HAL_I2C_Mem_Write(&hi2c2, (devAddress<<1), RegisterAddress, 1, (uint8_t*)addr, 2, I2C_TIMEOUT);
}

static uint16_t ReadMem(uint8_t devAddress, uint8_t RegisterAddress)
{
  uint8_t Value[2];

  HAL_I2C_Mem_Read(&hi2c2, (devAddress<<1), RegisterAddress, 1, &Value, 2, I2C_TIMEOUT);

    // Check for I2C errors after all operations
    if (hi2c2.ErrorCode != HAL_I2C_ERROR_NONE) {
        restartI2C();
    }

  return ((Value[0] << 8) | Value[1]);
}


static void WriteByte(uint8_t devAddress, uint8_t RegisterAddress, uint8_t Value)
{
  HAL_I2C_Mem_Write(&hi2c2, (devAddress<<1), RegisterAddress, 1, &Value, 1, I2C_TIMEOUT);
}

static uint16_t ReadByte(uint8_t devAddress, uint8_t RegisterAddress)
{
  uint8_t Value;

  HAL_I2C_Mem_Read(&hi2c2, (devAddress<<1), RegisterAddress, 1, &Value, 1, I2C_TIMEOUT);

      // Check for I2C errors after all operations
    if (hi2c2.ErrorCode != HAL_I2C_ERROR_NONE) {
        restartI2C();
    }
  return Value;
}

#ifdef IMU_MPU6050

void refreshIMUValues() {  
    IMU_Accel[0] = signNumber16(ReadMem(IMU_MPU6050_I2C_ADDRESS, IMU_MPU6050_ACCEL_XOUT_H)) * IMU_GRAVITATIONAL_ACCELERATION / accelDivisor;  // Convert to m/s^2
    IMU_Accel[1] = signNumber16(ReadMem(IMU_MPU6050_I2C_ADDRESS, IMU_MPU6050_ACCEL_YOUT_H)) * IMU_GRAVITATIONAL_ACCELERATION / accelDivisor; 
    IMU_Accel[2] = signNumber16(ReadMem(IMU_MPU6050_I2C_ADDRESS, IMU_MPU6050_ACCEL_ZOUT_H)) * IMU_GRAVITATIONAL_ACCELERATION / accelDivisor; 

    IMU_Gyro[0] = signNumber16(ReadMem(IMU_MPU6050_I2C_ADDRESS, IMU_MPU6050_GYRO_XOUT_H)) * IMU_DPS2RAD / gyroDivisor;  // Convert to rad/s
    IMU_Gyro[1] = signNumber16(ReadMem(IMU_MPU6050_I2C_ADDRESS, IMU_MPU6050_GYRO_YOUT_H)) * IMU_DPS2RAD / gyroDivisor;
    IMU_Gyro[2] = signNumber16(ReadMem(IMU_MPU6050_I2C_ADDRESS, IMU_MPU6050_GYRO_ZOUT_H)) * IMU_DPS2RAD / gyroDivisor;

    IMU_Temp = ((int16_t)ReadMem(IMU_MPU6050_I2C_ADDRESS, IMU_MPU6050_TEMP_OUT_H)) / 340.0f + 36.53f;  // Convert to °C

    #ifdef IMU_DYNAMIC_FSR
    calibrateIMU();
    #endif
}

void initIMU() {
    uint8_t wake = 0;
    HAL_I2C_Mem_Write(&hi2c2, (IMU_MPU6050_I2C_ADDRESS << 1) + 0, IMU_MPU6050_PWR_MGMT_1, 1, &wake, 1, I2C_TIMEOUT);
    uint8_t sampleRate = 0x07; // Set sample rate to 1 kHz
    HAL_I2C_Mem_Write(&hi2c2, (IMU_MPU6050_I2C_ADDRESS << 1) + 0, IMU_MPU6050_SMPLRT_DIV, 1, &sampleRate, 1, I2C_TIMEOUT);
    uint8_t accelConfig = IMU_MPU6050_ACCEL_FS_2; // Full range: ±2g
    HAL_I2C_Mem_Write(&hi2c2, (IMU_MPU6050_I2C_ADDRESS << 1) + 0, IMU_MPU6050_ACCEL_CONFIG, 1, &accelConfig, 1, I2C_TIMEOUT);
    uint8_t gyroConfig = IMU_MPU6050_GYRO_FS_250; // Full range: ±250°/s
    HAL_I2C_Mem_Write(&hi2c2, (IMU_MPU6050_I2C_ADDRESS << 1) + 0, IMU_MPU6050_GYRO_CONFIG, 1, &gyroConfig, 1, I2C_TIMEOUT);
    HAL_I2C_Mem_Read(&hi2c2, (IMU_MPU6050_I2C_ADDRESS << 1) + 1, IMU_MPU6050_WHO_AM_I, 1, &checkIMU, 1, I2C_TIMEOUT);
}

void calibrateIMU() {
    static uint8_t lastAccelConfig = IMU_MPU6050_ACCEL_FS_2; // Default ±2g
    static uint8_t lastGyroConfig = IMU_MPU6050_GYRO_FS_250; // Default ±250°/s

    uint8_t accelConfig = lastAccelConfig;
    uint8_t gyroConfig = lastGyroConfig;

    // Check each accelerometer axis
    float accelMaxMag = 0;
    for (int i = 0; i < 3; i++) {
        float absAccel = fabsf(IMU_Accel[i] / IMU_GRAVITATIONAL_ACCELERATION); // Convert m/s² to g
        accelMaxMag = (absAccel > accelMaxMag) ? absAccel : accelMaxMag;
    }


    accelConfig = IMU_MPU6050_ACCEL_FS_2;
    accelDivisor = 16384.0f;
    // Dynamic adjustment of accelerometer FSR
    if (accelMaxMag >= 1.75f && accelMaxMag < 3.75f) { // Near ±2g limit, switch to ±4g
        accelConfig = IMU_MPU6050_ACCEL_FS_4;
        accelDivisor = 8192.0f;
    } if (accelMaxMag >= 3.75f && accelMaxMag < 7.75f) { // Near ±4g limit, switch to ±8g
        accelConfig = IMU_MPU6050_ACCEL_FS_8;
        accelDivisor = 4096.0f;
    } if (accelMaxMag >= 7.75f) { // Near ±8g limit, switch to ±16g
        accelConfig = IMU_MPU6050_ACCEL_FS_16;
        accelDivisor = 2048.0f;
    }

    // Check each gyroscope axis
    float gyroMaxMag = 0;
    for (int i = 0; i < 3; i++) {
        float absGyro = fabsf(IMU_Gyro[i]); // Gyro is already in radians
        gyroMaxMag = (absGyro > gyroMaxMag) ? absGyro : gyroMaxMag;
    }

    // Dynamic adjustment of gyroscope FSR (values in radians/s)
    if (gyroMaxMag >= 4.3633f && gyroMaxMag < 8.7266f) { // Near ±250°/s limit, switch to ±500°/s
        gyroConfig = IMU_MPU6050_GYRO_FS_500;
        gyroDivisor = 65.5f;
    } else if (gyroMaxMag >= 8.7266f && gyroMaxMag < 17.4533f) { // Near ±500°/s limit, switch to ±1000°/s
        gyroConfig = IMU_MPU6050_GYRO_FS_1000;
        gyroDivisor = 32.8f;
    } else if (gyroMaxMag >= 17.4533f && gyroMaxMag < 34.9066f) { // Near ±1000°/s limit, switch to ±2000°/s
        gyroConfig = IMU_MPU6050_GYRO_FS_2000;
        gyroDivisor = 16.4f;
    } else { // Default to ±250°/s for maximum resolution
        gyroConfig = IMU_MPU6050_GYRO_FS_250;
        gyroDivisor = 131.0f;
    }

    // Apply new configurations if they changed
    if (lastAccelConfig != accelConfig) {
        HAL_I2C_Mem_Write(&hi2c2, (IMU_MPU6050_I2C_ADDRESS << 1) + 0, IMU_MPU6050_ACCEL_CONFIG, 1, &accelConfig, 1, I2C_TIMEOUT);
        lastAccelConfig = accelConfig;
    }

    if (lastGyroConfig != gyroConfig) {
        HAL_I2C_Mem_Write(&hi2c2, (IMU_MPU6050_I2C_ADDRESS << 1) + 0, IMU_MPU6050_GYRO_CONFIG, 1, &gyroConfig, 1, I2C_TIMEOUT);
        lastGyroConfig = gyroConfig;
    }

    // Update the divisor values globally
    accelDivisor = (accelConfig == IMU_MPU6050_ACCEL_FS_16) ? 2048.0f :
                   (accelConfig == IMU_MPU6050_ACCEL_FS_8) ? 4096.0f :
                   (accelConfig == IMU_MPU6050_ACCEL_FS_4) ? 8192.0f : 16384.0f;

    gyroDivisor = (gyroConfig == IMU_MPU6050_GYRO_FS_2000) ? 16.4f :
                  (gyroConfig == IMU_MPU6050_GYRO_FS_1000) ? 32.8f :
                  (gyroConfig == IMU_MPU6050_GYRO_FS_500) ? 65.5f : 131.0f;
}

#endif // IMU_MPU6050


#ifdef IMU_ICM42605

float _aRes, _gRes;

static uint8_t getChipID()
{
  uint8_t c = ReadByte(ICM42605_ADDRESS, ICM42605_WHO_AM_I);
  return c;
}

static float getAres(uint8_t Ascale) {
  switch (Ascale)
  {
    // Possible accelerometer scales (and their register bit settings) are:
    // 2 Gs (00), 4 Gs (01), 8 Gs (10), and 16 Gs  (11).
    case AFS_2G:
      _aRes = 2.0f / 32768.0f;
      return _aRes;
      break;
    case AFS_4G:
      _aRes = 4.0f / 32768.0f;
      return _aRes;
      break;
    case AFS_8G:
      _aRes = 8.0f / 32768.0f;
      return _aRes;
      break;
    case AFS_16G:
      _aRes = 16.0f / 32768.0f;
      return _aRes;
      break;
  }
}

static float getGres(uint8_t Gscale) {
  switch (Gscale)
  {
    case GFS_15_125DPS:
      _gRes = 15.125f / 32768.0f;
      return _gRes;
      break;
    case GFS_31_25DPS:
      _gRes = 31.25f / 32768.0f;
      return _gRes;
      break;
    case GFS_62_5DPS:
      _gRes = 62.5f / 32768.0f;
      return _gRes;
      break;
    case GFS_125DPS:
      _gRes = 125.0f / 32768.0f;
      return _gRes;
      break;
    case GFS_250DPS:
      _gRes = 250.0f / 32768.0f;
      return _gRes;
      break;
    case GFS_500DPS:
      _gRes = 500.0f / 32768.0f;
      return _gRes;
      break;
    case GFS_1000DPS:
      _gRes = 1000.0f / 32768.0f;
      return _gRes;
      break;
    case GFS_2000DPS:
      _gRes = 2000.0f / 32768.0f;
      return _gRes;
      break;
  }
}


static void resetICM42605()
{
  // reset device
  uint8_t temp = ReadByte(ICM42605_ADDRESS, ICM42605_DEVICE_CONFIG);
  WriteByte(ICM42605_ADDRESS, ICM42605_DEVICE_CONFIG, temp | 0x01); // Set bit 0 to 1 to reset ICM42605

}


static uint8_t statusICM42605()
{
  // reset device
  uint8_t temp = ReadByte(ICM42605_ADDRESS, ICM42605_INT_STATUS);
  return temp;
}



static void initICM42605(uint8_t Ascale, uint8_t Gscale, uint8_t AODR, uint8_t GODR)
{
  uint8_t temp = ReadByte(ICM42605_ADDRESS, ICM42605_PWR_MGMT0); // make sure not to disturb reserved bit values
  WriteByte(ICM42605_ADDRESS, ICM42605_PWR_MGMT0, temp | 0x0F);  // enable gyro and accel in low noise mode

   temp = ReadByte(ICM42605_ADDRESS, ICM42605_GYRO_CONFIG0);
  WriteByte(ICM42605_ADDRESS, ICM42605_GYRO_CONFIG0, temp | GODR | Gscale << 5); // gyro full scale and data rate

   temp = ReadByte(ICM42605_ADDRESS, ICM42605_ACCEL_CONFIG0);
  WriteByte(ICM42605_ADDRESS, ICM42605_ACCEL_CONFIG0, temp | AODR | Ascale << 5); // set accel full scale and data rate

   temp = ReadByte(ICM42605_ADDRESS, ICM42605_GYRO_CONFIG1);
  WriteByte(ICM42605_ADDRESS, ICM42605_GYRO_CONFIG1, temp | 0xD0); // set temperature sensor low pass filter to 5Hz, use first order gyro filter

  //  temp = ReadByte(ICM42605_ADDRESS, ICM42605_INT_CONFIG);
  // WriteByte(ICM42605_ADDRESS, ICM42605_INT_CONFIG, temp | 0x18 | 0x03 ); // set both interrupts active high, push-pull, pulsed

  //  temp = ReadByte(ICM42605_ADDRESS, ICM42605_INT_CONFIG1);
  // WriteByte(ICM42605_ADDRESS, ICM42605_INT_CONFIG1, temp & ~(0x10) ); // set bit 4 to zero for proper function of INT1 and INT2
 
  //  temp = ReadByte(ICM42605_ADDRESS, ICM42605_INT_SOURCE0);
  // WriteByte(ICM42605_ADDRESS, ICM42605_INT_SOURCE0, temp | 0x08 ); // route data ready interrupt to INT1
 
  //  temp = ReadByte(ICM42605_ADDRESS, ICM42605_INT_SOURCE3);
  // WriteByte(ICM42605_ADDRESS, ICM42605_INT_SOURCE3, temp | 0x01 ); // route AGC interrupt interrupt to INT2

  // Select Bank 4
   temp = ReadByte(ICM42605_ADDRESS, ICM42605_REG_BANK_SEL);
  WriteByte(ICM42605_ADDRESS, ICM42605_REG_BANK_SEL, temp | 0x04 ); // select Bank 4

   temp = ReadByte(ICM42605_ADDRESS, ICM42605_APEX_CONFIG5);
  WriteByte(ICM42605_ADDRESS, ICM42605_APEX_CONFIG5, temp & ~(0x07) ); // select unitary mounting matrix

   temp = ReadByte(ICM42605_ADDRESS, ICM42605_REG_BANK_SEL);
  WriteByte(ICM42605_ADDRESS, ICM42605_REG_BANK_SEL, temp & ~(0x07) ); // select Bank 0
}

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
void refreshIMUValues() {
    // initIMU();
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);

    IMU_Accel[0] = signNumber16(ReadMem(ICM42605_ADDRESS, ICM42605_ACCEL_DATA_X1)) * IMU_GRAVITATIONAL_ACCELERATION / accelDivisor;  // Convert to m/s^2
    IMU_Accel[1] = signNumber16(ReadMem(ICM42605_ADDRESS, ICM42605_ACCEL_DATA_Y1)) * IMU_GRAVITATIONAL_ACCELERATION / accelDivisor; 
    IMU_Accel[2] = signNumber16(ReadMem(ICM42605_ADDRESS, ICM42605_ACCEL_DATA_Z1)) * IMU_GRAVITATIONAL_ACCELERATION / accelDivisor; 

    IMU_Gyro[0] = signNumber16(ReadMem(ICM42605_ADDRESS, ICM42605_GYRO_DATA_X1)) * IMU_DPS2RAD / gyroDivisor;  // Convert to rad/s
    IMU_Gyro[1] = signNumber16(ReadMem(ICM42605_ADDRESS, ICM42605_GYRO_DATA_Y1)) * IMU_DPS2RAD / gyroDivisor;
    IMU_Gyro[2] = signNumber16(ReadMem(ICM42605_ADDRESS, ICM42605_GYRO_DATA_Z1)) * IMU_DPS2RAD / gyroDivisor;

    IMU_Temp = ((int16_t)ReadMem(ICM42605_ADDRESS, ICM42605_TEMP_DATA1)) / 132.48f + 25.0f;  // Convert to °C

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    #ifdef IMU_DYNAMIC_FSR
    calibrateIMU();
    #endif
}

void initIMU() {
  // resetICM42605(); // Reset the ICM42605
  initICM42605(AFS_2G, GFS_2000DPS, AODR_100Hz, GODR_100Hz); // Initialize with ±2g and ±125°/s
}

void calibrateIMU() {
    static uint8_t lastAccelConfig = AFS_2G; // Default ±2g
    static uint8_t lastGyroConfig = GFS_2000DPS; // Default ±125°/s

    uint8_t accelConfig = lastAccelConfig;
    uint8_t gyroConfig = lastGyroConfig;

    // Check each accelerometer axis
    float accelMaxMag = 0;
    for (int i = 0; i < 3; i++) {
        float absAccel = fabsf(IMU_Accel[i] / IMU_GRAVITATIONAL_ACCELERATION); // Convert m/s² to g
        accelMaxMag = (absAccel > accelMaxMag) ? absAccel : accelMaxMag;
    }


    accelConfig = AFS_2G;
    accelDivisor = 16384.0f;
    // Dynamic adjustment of accelerometer FSR
    if (accelMaxMag >= 1.75f && accelMaxMag < 3.75f) { // Near ±2g limit, switch to ±4g
        accelConfig = AFS_4G;
        accelDivisor = 8192.0f;
    } if (accelMaxMag >= 3.75f && accelMaxMag < 7.75f) { // Near ±4g limit, switch to ±8g
        accelConfig = AFS_8G;
        accelDivisor = 4096.0f;
    } if (accelMaxMag >= 7.75f) { // Near ±8g limit, switch to ±16g
        accelConfig = AFS_16G;
        accelDivisor = 2048.0f;
    }

    // Check each gyroscope axis
    float gyroMaxMag = 0;
    for (int i = 0; i < 3; i++) {
        float absGyro = fabsf(IMU_Gyro[i]); // Gyro is already in radians
        gyroMaxMag = (absGyro > gyroMaxMag) ? absGyro : gyroMaxMag;
    }


    // Dynamic adjustment of gyroscope FSR (values in radians/s)
    if (gyroMaxMag >= 0.0165f && gyroMaxMag < 0.0329f) { // Near ±15.125°/s limit, switch to ±31.25°/s
        gyroConfig = GFS_31_25DPS;
        gyroDivisor = 32768.0f / 31.25f; // ≈ 1048.96
    } else if (gyroMaxMag >= 0.0329f && gyroMaxMag < 0.0658f) { // Near ±31.25°/s limit, switch to ±62.5°/s
        gyroConfig = GFS_62_5DPS;
        gyroDivisor = 32768.0f / 62.5f; // ≈ 524.29
    } else if (gyroMaxMag >= 0.0658f && gyroMaxMag < 0.1316f) { // Near ±62.5°/s limit, switch to ±125°/s
        gyroConfig = GFS_125DPS;
        gyroDivisor = 32768.0f / 125.0f; // ≈ 262.14
    } else if (gyroMaxMag >= 0.1316f && gyroMaxMag < 0.2632f) { // Near ±125°/s limit, switch to ±250°/s
        gyroConfig = GFS_250DPS;
        gyroDivisor = 32768.0f / 250.0f; // ≈ 131.07
    } else if (gyroMaxMag >= 0.4363f && gyroMaxMag < 0.8727f) { // Near ±250°/s limit, switch to ±500°/s
        gyroConfig = GFS_500DPS;
        gyroDivisor = 32768.0f / 500.0f; // ≈ 65.54
    } else if (gyroMaxMag >= 0.8727f && gyroMaxMag < 1.7453f) { // Near ±500°/s limit, switch to ±1000°/s
        gyroConfig = GFS_1000DPS;
        gyroDivisor = 32768.0f / 1000.0f; // ≈ 32.77
    } else if (gyroMaxMag >= 1.7453f && gyroMaxMag < 3.4907f) { // Near ±1000°/s limit, switch to ±2000°/s
        gyroConfig = GFS_2000DPS;
        gyroDivisor = 32768.0f / 2000.0f; // ≈ 16.38
    } else if (gyroMaxMag < 0.0165f) { // Default to ±15.125°/s for maximum resolution
        gyroConfig = GFS_15_125DPS;
        gyroDivisor = 32768.0f / 15.125f; // ≈ 2166.98
    } else {
        gyroConfig = GFS_250DPS;
        gyroDivisor = 131.0f;
    }

    // Apply new configurations if they changed
    // resetICM42605(); // Reset the ICM42605 to apply new settings
    initICM42605(accelConfig, gyroConfig, AODR_1000Hz, GODR_1000Hz);
   

    // Update the divisor values globally
    accelDivisor = (accelConfig == AFS_16G) ? 2048.0f :
                   (accelConfig == AFS_8G) ? 4096.0f :
                   (accelConfig == AFS_4G) ? 8192.0f : 16384.0f;

    gyroDivisor = (gyroConfig == GFS_2000DPS) ? 16.4f :
                  (gyroConfig == GFS_1000DPS) ? 32.8f :
                  (gyroConfig == GFS_500DPS) ? 65.5f : 131.0f;
}

#endif // IMU_ICM42605