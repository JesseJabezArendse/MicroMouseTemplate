//********************************************************************
//*                          Micro Mouse                             *
//*                          IMU Library                             *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      09-06-2025                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This header file provides an interface for communicating with    *
//* Inertial Measurement Unit sensors on the Micro Mouse robot.      *
//* It supports two IMU types: IMU_MPU6050 and ICM42605, selectable     *
//* through preprocessor directives. The library handles sensor      *
//* initialization, calibration, and reading of accelerometer and    *
//* gyroscope data for motion tracking and orientation sensing.      *
//*                                                                  *
//********************************************************************


#ifndef IMU_H
#define IMU_H

#include "stm32l4xx.h"
#include "main.h"

//====================================================================
// GLOBAL CONSTANTS
//====================================================================
#define IMU_GRAVITATIONAL_ACCELERATION 9.80665f // Standard gravity in m/s²
#define IMU_DPS2RAD 0.01745329251994329576923690768489f // Conversion factor from degrees per second to radians per second
//====================================================================
#ifndef COMPILED_BY_SIMULINK
#define IMU_ICM42605
// #define IMU_MPU6050
// #define IMU_DYNAMIC_FSR
#endif // COMPILED_BY_SIMULINK


#ifdef IMU_MPU6050
// https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf

#define IMU_MPU6050_I2C_PORT &hi2c2
#define IMU_MPU6050_I2C_ADDRESS IMU_MPU6050_I2C_ADDRESS_H

// MPU-6050 I2C Address
#define IMU_MPU6050_I2C_ADDRESS_L       0x68  // Default I2C address when AD0 is LOW
#define IMU_MPU6050_I2C_ADDRESS_H       0x69  // Alternate I2C address when AD0 is HIGH

// Register Map
#define IMU_MPU6050_SELF_TEST_X         0x0D  // Self-test register for accelerometer X-axis
#define IMU_MPU6050_SELF_TEST_Y         0x0E  // Self-test register for accelerometer Y-axis
#define IMU_MPU6050_SELF_TEST_Z         0x0F  // Self-test register for accelerometer Z-axis
#define IMU_MPU6050_SELF_TEST_A         0x10  // Self-test register for accelerometer
#define IMU_MPU6050_SMPLRT_DIV          0x19  // Sample rate divider
#define IMU_MPU6050_CONFIG              0x1A  // Configuration register (DLPF settings)
#define IMU_MPU6050_GYRO_CONFIG         0x1B  // Gyroscope configuration (FS_SEL)
#define IMU_MPU6050_ACCEL_CONFIG        0x1C  // Accelerometer configuration (AFS_SEL)
#define IMU_MPU6050_FIFO_EN             0x23  // FIFO enable register
#define IMU_MPU6050_I2C_MST_CTRL        0x24  // I2C master control register
#define IMU_MPU6050_I2C_SLV0_ADDR       0x25  // I2C slave 0 address
#define IMU_MPU6050_I2C_SLV0_REG        0x26  // I2C slave 0 register
#define IMU_MPU6050_I2C_SLV0_CTRL       0x27  // I2C slave 0 control
#define IMU_MPU6050_I2C_SLV1_ADDR       0x28  // I2C slave 1 address
#define IMU_MPU6050_I2C_SLV1_REG        0x29  // I2C slave 1 register
#define IMU_MPU6050_I2C_SLV1_CTRL       0x2A  // I2C slave 1 control
#define IMU_MPU6050_I2C_SLV2_ADDR       0x2B  // I2C slave 2 address
#define IMU_MPU6050_I2C_SLV2_REG        0x2C  // I2C slave 2 register
#define IMU_MPU6050_I2C_SLV2_CTRL       0x2D  // I2C slave 2 control
#define IMU_MPU6050_I2C_SLV3_ADDR       0x2E  // I2C slave 3 address
#define IMU_MPU6050_I2C_SLV3_REG        0x2F  // I2C slave 3 register
#define IMU_MPU6050_I2C_SLV3_CTRL       0x30  // I2C slave 3 control
#define IMU_MPU6050_I2C_SLV4_ADDR       0x31  // I2C slave 4 address
#define IMU_MPU6050_I2C_SLV4_REG        0x32  // I2C slave 4 register
#define IMU_MPU6050_I2C_SLV4_DO         0x33  // I2C slave 4 data out
#define IMU_MPU6050_I2C_SLV4_CTRL       0x34  // I2C slave 4 control
#define IMU_MPU6050_I2C_SLV4_DI         0x35  // I2C slave 4 data in
#define IMU_MPU6050_I2C_MST_STATUS      0x36  // I2C master status
#define IMU_MPU6050_INT_PIN_CFG         0x37  // Interrupt pin configuration
#define IMU_MPU6050_INT_ENABLE          0x38  // Interrupt enable register
#define IMU_MPU6050_INT_STATUS          0x3A  // Interrupt status register
#define IMU_MPU6050_ACCEL_XOUT_H        0x3B  // Accelerometer X-axis high byte
#define IMU_MPU6050_ACCEL_XOUT_L        0x3C  // Accelerometer X-axis low byte
#define IMU_MPU6050_ACCEL_YOUT_H        0x3D  // Accelerometer Y-axis high byte
#define IMU_MPU6050_ACCEL_YOUT_L        0x3E  // Accelerometer Y-axis low byte
#define IMU_MPU6050_ACCEL_ZOUT_H        0x3F  // Accelerometer Z-axis high byte
#define IMU_MPU6050_ACCEL_ZOUT_L        0x40  // Accelerometer Z-axis low byte
#define IMU_MPU6050_TEMP_OUT_H          0x41  // Temperature high byte
#define IMU_MPU6050_TEMP_OUT_L          0x42  // Temperature low byte
#define IMU_MPU6050_GYRO_XOUT_H         0x43  // Gyroscope X-axis high byte
#define IMU_MPU6050_GYRO_XOUT_L         0x44  // Gyroscope X-axis low byte
#define IMU_MPU6050_GYRO_YOUT_H         0x45  // Gyroscope Y-axis high byte
#define IMU_MPU6050_GYRO_YOUT_L         0x46  // Gyroscope Y-axis low byte
#define IMU_MPU6050_GYRO_ZOUT_H         0x47  // Gyroscope Z-axis high byte
#define IMU_MPU6050_GYRO_ZOUT_L         0x48  // Gyroscope Z-axis low byte
#define IMU_MPU6050_SIGNAL_PATH_RESET   0x68  // Signal path reset register
#define IMU_MPU6050_USER_CTRL           0x6A  // User control register
#define IMU_MPU6050_PWR_MGMT_1          0x6B  // Power management 1 register
#define IMU_MPU6050_PWR_MGMT_2          0x6C  // Power management 2 register
#define IMU_MPU6050_FIFO_COUNTH         0x72  // FIFO count high byte
#define IMU_MPU6050_FIFO_COUNTL         0x73  // FIFO count low byte
#define IMU_MPU6050_FIFO_R_W            0x74  // FIFO read/write register
#define IMU_MPU6050_WHO_AM_I            0x75  // WHO_AM_I register (default value: 0x68)

// Configuration Bits
#define IMU_MPU6050_GYRO_FS_250         0x00  // ±250°/s
#define IMU_MPU6050_GYRO_FS_500         0x08  // ±500°/s
#define IMU_MPU6050_GYRO_FS_1000        0x10  // ±1000°/s
#define IMU_MPU6050_GYRO_FS_2000        0x18  // ±2000°/s

#define IMU_MPU6050_ACCEL_FS_2          0x00  // ±2g
#define IMU_MPU6050_ACCEL_FS_4          0x08  // ±4g
#define IMU_MPU6050_ACCEL_FS_8          0x10  // ±8g
#define IMU_MPU6050_ACCEL_FS_16         0x18  // ±16g

#define IMU_MPU6050_DLPF_CFG_260HZ      0x00  // 260Hz bandwidth
#define IMU_MPU6050_DLPF_CFG_184HZ      0x01  // 184Hz bandwidth
#define IMU_MPU6050_DLPF_CFG_94HZ       0x02  // 94Hz bandwidth
#define IMU_MPU6050_DLPF_CFG_44HZ       0x03  // 44Hz bandwidth
#define IMU_MPU6050_DLPF_CFG_21HZ       0x04  // 21Hz bandwidth
#define IMU_MPU6050_DLPF_CFG_10HZ       0x05  // 10Hz bandwidth
#define IMU_MPU6050_DLPF_CFG_5HZ        0x06  // 5Hz bandwidth


#define IMU_MPU6050_X_AXIS 0
#define IMU_MPU6050_Y_AXIS 1
#define IMU_MPU6050_Z_AXIS 2

#endif

#ifndef ICM42605_h
#define ICM42605_h

#define ICM42605_ADDRESS                   0b1101000   // Address of ICM42605 accel/gyro when ADO = HIGH


/* ICM42605 registers
https://store.invensense.com/datasheets/invensense/DS-ICM-42605v1-2.pdf
*/
// Bank 0
#define ICM42605_DEVICE_CONFIG             0x11
#define ICM42605_DRIVE_CONFIG              0x13
#define ICM42605_INT_CONFIG                0x14
#define ICM42605_FIFO_CONFIG               0x16
#define ICM42605_TEMP_DATA1                0x1D
#define ICM42605_TEMP_DATA0                0x1E
#define ICM42605_ACCEL_DATA_X1             0x1F
#define ICM42605_ACCEL_DATA_X0             0x20
#define ICM42605_ACCEL_DATA_Y1             0x21
#define ICM42605_ACCEL_DATA_Y0             0x22
#define ICM42605_ACCEL_DATA_Z1             0x23
#define ICM42605_ACCEL_DATA_Z0             0x24
#define ICM42605_GYRO_DATA_X1              0x25
#define ICM42605_GYRO_DATA_X0              0x26
#define ICM42605_GYRO_DATA_Y1              0x27
#define ICM42605_GYRO_DATA_Y0              0x28
#define ICM42605_GYRO_DATA_Z1              0x29
#define ICM42605_GYRO_DATA_Z0              0x2A
#define ICM42605_TMST_FSYNCH               0x2B
#define ICM42605_TMST_FSYNCL               0x2C
#define ICM42605_INT_STATUS                0x2D
#define ICM42605_FIFO_COUNTH               0x2E
#define ICM42605_FIFO_COUNTL               0x2F
#define ICM42605_FIFO_DATA                 0x30
#define ICM42605_APEX_DATA0                0x31
#define ICM42605_APEX_DATA1                0x32
#define ICM42605_APEX_DATA2                0x33
#define ICM42605_APEX_DATA3                0x34
#define ICM42605_APEX_DATA4                0x35
#define ICM42605_APEX_DATA5                0x36
#define ICM42605_INT_STATUS2               0x37
#define ICM42605_INT_STATUS3               0x38
#define ICM42605_SIGNAL_PATH_RESET         0x4B
#define ICM42605_INTF_CONFIG0              0x4C
#define ICM42605_INTF_CONFIG1              0x4D
#define ICM42605_PWR_MGMT0                 0x4E
#define ICM42605_GYRO_CONFIG0              0x4F
#define ICM42605_ACCEL_CONFIG0             0x50
#define ICM42605_GYRO_CONFIG1              0x51
#define ICM42605_GYRO_ACCEL_CONFIG0        0x52
#define ICM42605_ACCEL_CONFIG1             0x53
#define ICM42605_TMST_CONFIG               0x54
#define ICM42605_APEX_CONFIG0              0x56
#define ICM42605_SMD_CONFIG                0x57
#define ICM42605_FIFO_CONFIG1              0x5F
#define ICM42605_FIFO_CONFIG2              0x60
#define ICM42605_FIFO_CONFIG3              0x61
#define ICM42605_FSYNC_CONFIG              0x62
#define ICM42605_INT_CONFIG0               0x63
#define ICM42605_INT_CONFIG1               0x64
#define ICM42605_INT_SOURCE0               0x65
#define ICM42605_INT_SOURCE1               0x66
#define ICM42605_INT_SOURCE3               0x68
#define ICM42605_INT_SOURCE4               0x69
#define ICM42605_FIFO_LOST_PKT0            0x6C
#define ICM42605_FIFO_LOST_PKT1            0x6D
#define ICM42605_SELF_TEST_CONFIG          0x70
#define ICM42605_WHO_AM_I                  0x75
#define ICM42605_REG_BANK_SEL              0x76

// Bank 1
#define ICM42605_SENSOR_CONFIG0            0x03
#define ICM42605_GYRO_CONFIG_STATIC2       0x0B
#define ICM42605_GYRO_CONFIG_STATIC3       0x0C
#define ICM42605_GYRO_CONFIG_STATIC4       0x0D
#define ICM42605_GYRO_CONFIG_STATIC5       0x0E
#define ICM42605_GYRO_CONFIG_STATIC6       0x0F
#define ICM42605_GYRO_CONFIG_STATIC7       0x10
#define ICM42605_GYRO_CONFIG_STATIC8       0x11
#define ICM42605_GYRO_CONFIG_STATIC9       0x12
#define ICM42605_GYRO_CONFIG_STATIC10      0x13
#define ICM42605_XG_ST_DATA                0x5F
#define ICM42605_YG_ST_DATA                0x60
#define ICM42605_ZG_ST_DATA                0x61
#define ICM42605_TMSTVAL0                  0x62
#define ICM42605_TMSTVAL1                  0x63
#define ICM42605_TMSTVAL2                  0x64
#define ICM42605_INTF_CONFIG4              0x7A
#define ICM42605_INTF_CONFIG5              0x7B
#define ICM42605_INTF_CONFIG6              0x7C

// Bank 2
#define ICM42605_ACCEL_CONFIG_STATIC2      0x03
#define ICM42605_ACCEL_CONFIG_STATIC3      0x04
#define ICM42605_ACCEL_CONFIG_STATIC4      0x05
#define ICM42605_XA_ST_DATA                0x3B
#define ICM42605_YA_ST_DATA                0x3C
#define ICM42605_ZA_ST_DATA                0x3D

// Bank 4
#define ICM42605_GYRO_ON_OFF_CONFIG        0x0E
#define ICM42605_APEX_CONFIG1              0x40
#define ICM42605_APEX_CONFIG2              0x41
#define ICM42605_APEX_CONFIG3              0x42
#define ICM42605_APEX_CONFIG4              0x43
#define ICM42605_APEX_CONFIG5              0x44
#define ICM42605_APEX_CONFIG6              0x45
#define ICM42605_APEX_CONFIG7              0x46
#define ICM42605_APEX_CONFIG8              0x47
#define ICM42605_APEX_CONFIG9              0x48
#define ICM42605_ACCEL_WOM_X_THR           0x4A
#define ICM42605_ACCEL_WOM_Y_THR           0x4B
#define ICM42605_ACCEL_WOM_Z_THR           0x4C
#define ICM42605_INT_SOURCE6               0x4D
#define ICM42605_INT_SOURCE7               0x4E
#define ICM42605_INT_SOURCE8               0x4F
#define ICM42605_INT_SOURCE9               0x50
#define ICM42605_INT_SOURCE10              0x51
#define ICM42605_OFFSET_USER0              0x77
#define ICM42605_OFFSET_USER1              0x78
#define ICM42605_OFFSET_USER2              0x79
#define ICM42605_OFFSET_USER3              0x7A
#define ICM42605_OFFSET_USER4              0x7B
#define ICM42605_OFFSET_USER5              0x7C
#define ICM42605_OFFSET_USER6              0x7D
#define ICM42605_OFFSET_USER7              0x7E
#define ICM42605_OFFSET_USER8              0x7F

#define AFS_2G  0x03
#define AFS_4G  0x02
#define AFS_8G  0x01
#define AFS_16G 0x00  // default

#define GFS_2000DPS   0x00 // default
#define GFS_1000DPS   0x01
#define GFS_500DPS    0x02
#define GFS_250DPS    0x03
#define GFS_125DPS    0x04
#define GFS_62_5DPS   0x05
#define GFS_31_25DPS  0x06
#define GFS_15_125DPS 0x07

#define AODR_8000Hz   0x03
#define AODR_4000Hz   0x04
#define AODR_2000Hz   0x05
#define AODR_1000Hz   0x06 // default
#define AODR_200Hz    0x07
#define AODR_100Hz    0x08
#define AODR_50Hz     0x09
#define AODR_25Hz     0x0A
#define AODR_12_5Hz   0x0B
#define AODR_6_25Hz   0x0C
#define AODR_3_125Hz  0x0D
#define AODR_1_5625Hz 0x0E
#define AODR_500Hz    0x0F

#define GODR_8000Hz  0x03
#define GODR_4000Hz  0x04
#define GODR_2000Hz  0x05
#define GODR_1000Hz  0x06 // default
#define GODR_200Hz   0x07
#define GODR_100Hz   0x08
#define GODR_50Hz    0x09
#define GODR_25Hz    0x0A
#define GODR_12_5Hz  0x0B
#define GODR_500Hz   0x0F

#endif

//====================================================================
// FUNCTION DECLARATIONS
//====================================================================
void refreshIMUValues();
void initIMU();
void calibrateIMU();
//====================================================================

#endif

//********************************************************************
// END OF PROGRAM
//********************************************************************
