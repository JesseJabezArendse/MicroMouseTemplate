/*
 * INA219.c
 *
 *  Created on: Dec 30, 2020
 *  Updated on: Jan 29, 2022
 *       Author: Piotr Smolen <komuch@gmail.com>
 *       Updated by: Brandon Thibeaux <github: thibeaux>
 */

#include "main.h"
#include "INA219.h"


INA219_t ina219;
int8_t batteryLife;
int16_t Vbattery, Vshunt, Current, config, Power;
float miliwattAVG,miliWattTime,totalPowerUsed;
enum BatteryState batteryState;
bool isFirst;

uint16_t ina219_calibrationValue;
int16_t ina219_CurrentDivider_mA;
int16_t ina219_PowerMultiplier_mW;

/*
 * @brief:		Read a register from the IN219 sensor.
 * @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @param:		register address in hexadecimal
 * @retval:		16 bit unsigned integer that represents the register's contents.
 */
uint16_t Read16(INA219_t *ina219, uint8_t Register)
{
	uint8_t Value[2];

	HAL_I2C_Mem_Read(ina219->ina219_i2c, (INA219_ADDRESS<<1), Register, 1, Value, 2, I2C_TIMEOUT);

    // Check for I2C errors after all operations
    if (ina219->ina219_i2c->ErrorCode != HAL_I2C_ERROR_NONE) {
        restartI2C(ina219->ina219_i2c);
    }

	return ((Value[0] << 8) | Value[1]);
}

/*
 * @brief:		Write to a register on the IN219 sensor.
 * @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @param:		Register address in hexadecimal
 * @param:		16 bit integer in hexadecimal that is the value you want to write to the register.
 * @retval:		HAL_StatusTypeDef, this will include an enum value representing
 * 				if the I2C transmission was successful or not.
 * 				typedef enum
				{
				  HAL_OK       = 0x00U,
				  HAL_ERROR    = 0x01U,
				  HAL_BUSY     = 0x02U,
				  HAL_TIMEOUT  = 0x03U
				} HAL_StatusTypeDef;
 */
HAL_StatusTypeDef Write16(INA219_t *ina219, uint8_t Register, uint16_t Value)
{
	uint8_t addr[2];
	addr[0] = (Value >> 8) & 0xff;  // upper byte
	addr[1] = (Value >> 0) & 0xff; // lower byte
	return HAL_I2C_Mem_Write(ina219->ina219_i2c, (INA219_ADDRESS<<1), Register, 1, (uint8_t*)addr, 2, I2C_TIMEOUT);
}

/*
 * @brief: 		This function will read the battery voltage level being read.
 * @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @retval:		Returns voltage level in mili-volts
 */
uint16_t INA219_ReadBusVoltage(INA219_t *ina219)
{
	uint16_t result = Read16(ina219, INA219_REG_BUSVOLTAGE);

	return ((result >> 3  ) * 4);

}

/*
 *  @brief:	  	Gets the raw Current value (16-bit signed integer, so +-32767)
 *  @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 *  @retval:	The raw Current reading
 */
int16_t INA219_ReadCurrent_raw(INA219_t *ina219)
{
	int16_t result = Read16(ina219, INA219_REG_CURRENT);

	return (result );
}

/*
 * @brief:  	Gets the Current value in mA, taking into account the
 *          	config settings and Current LSB
 * @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @return: 	The Current reading convereted to milliamps
 */
int16_t INA219_ReadCurrent(INA219_t *ina219)
{
	int16_t result = INA219_ReadCurrent_raw(ina219);

	return (result / ina219_CurrentDivider_mA );
}

/*
 * @brief: 		This function will read the shunt voltage level.
 * @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @retval:		Returns voltage level in mili-volts. This value represents the difference
 * 				between the voltage of the Power supply and the bus voltage after the shunt
 * 				resistor.
 */
int16_t INA219_ReadShuntVolage(INA219_t *ina219)
{
	int16_t result = (int16_t)Read16(ina219, INA219_REG_SHUNTVOLTAGE);

	return (result / 100);
}
/*
 * @brief: 	This reads the Power register then multiplies it by the Power multiplier.
 * 			Power multiplier is initialize in the calibration function.
 * @param:	Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @retval:	Returns Power level in mili-watts
 */
uint16_t INA219_ReadPower(INA219_t *ina219)
{
	uint16_t result = Read16(ina219, INA219_REG_POWER );
	result = result * ina219_PowerMultiplier_mW; // Power is the Power register times the Power_LSB (Power multiplier)
	return (result);
}

/*
 * @brief:		This takes a minimum and maximum value an turn the Current voltage
 * 				level into a percentage to give you a reference on how much battery life is left on your device.
 * @param:	<object pointer> <maximum mili-voltage level> <minumum mili-voltage level>
 * @retval: 	Percentage floating point value
 * @example: 	GetBatteryLife(&ina219, 6000, 4000)
 * 				returns 75
 */
uint8_t GetBatteryLife(uint16_t Vbattery ,float batteryMax, float batteryMin)
{
	float  percentageLife = 0.0f;
	percentageLife = (Vbattery - batteryMin) / (batteryMax - batteryMin);
	if(percentageLife >= 0 )
	{
		return percentageLife * 100;
	}
	else
	{
		return 0;
	}
}

/*
 * @brief: get Delta time in mili-seconds which is the difference between the last time you called this function and now
 */
int lastTime,deltaTime,now;
int INA219_GetDeltaTime_ms()
{
	  int now = HAL_GetTick();
	  deltaTime = now -lastTime;
	  lastTime = now;

	  return deltaTime;
}

/*
 * @brief:		The goal is to add up all the Power used since the system turned on. WE do this by adding up samples from
 * 				INA219_GetMiliWattsDeltaTime every time this function is called. This returns a total used amount of mW seconds.
 */
float totalEnergy;
float INA219_GetTotalPowerUsed(INA219_t *ina219)
{
	float sample = INA219_GetMiliWattsDeltaTime(ina219); // for some reason it comes out in a really small number so multiply this by 1000 to get real figure.
	totalEnergy += (sample*1000);
	return totalEnergy;
}
/*
 * @brief:		The goal of this function is to give you the amount of mili-watts being
 * 				consumed every time the function is called or every change in time.
 * @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @retval:		floating point number giving back a value for mili-watts seconds
 */
float INA219_GetMiliWattsDeltaTime(INA219_t *ina219)
{
	float energy = 0;
	energy = INA219_GetDeltaTime_ms() * INA219_ReadPower(ina219);
	return energy/1000; // convert mili-seconds to seconds.
}

/*
 * @brief:		The goal of this function is to give you the average mW amount
 * @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @retval:		floating point number giving back a value for mili-watts
 */
float sample[10]; // this can be set to any number. REmeber the bigger the sample amount the longer it takes to become accurate.
int index = 0;
float INA219_GetAVGMiliWatt(INA219_t *ina219)
{
	// Energy = Power * Time
	float energy = 0;
	if (!isFirst)
	{
		isFirst = true;
		sample[0] = INA219_ReadPower(ina219);
		index = 1;
	}
	else
	{
		if(index != 10)
		{
			sample[index] = INA219_ReadPower(ina219);
			index++;
		}
		else
		{
			index = 0;
			sample[index] = INA219_ReadPower(ina219);
		}

	}
	for(int i=0; i< (sizeof(sample)/sizeof(sample[0])); i++)
	{
		energy += sample[i];
	}
	energy = energy / (sizeof(sample)/sizeof(sample[0]));



	return energy;
}

/*
 * @brief:		The goal of this function is to make sure the battery is working as we
 * 				specify it to. If there is a discrepancy that is found in this check we
 * 				update the battery's state to alert the main function to take action.
 * @param:		Pointer to the device object that was made from the struct. EX:  (&ina219)
 * @param:		Floating point value for the percent of battery life we want to
 * 				dedicate as a threshold that indicates a "LOW" state
 * @param:		Floating point value that represents out batteries Current life percentage ratio
 * @retval:		We will return an enum state that represents the battery's state. This tells
 * 				the program that called the health check function what state our battery is
 * 				at and whether we have entered a "LOW" state. This way the program can take
 * 				appropriate action.
 */
enum BatteryState INA219_HealthCheck(INA219_t *ina219,float batteryPercentageThreshold,float batteryPercentage)
{
	switch(batteryState)
	{
		case (Battery_START):
			/* Enter your start up functionality here */
			batteryState = Battery_OK;
			break;
		case (Battery_OK):
			/* Enter your battery OK state functionality here */
			if(batteryPercentage > batteryPercentageThreshold) // is battery life below given threshold?
			{
				batteryState = Battery_OK;
			}
			else
			{
				 batteryState = Battery_LOW;
			}
			break;
		case (Battery_LOW):
			/* Enter your battery LOW state functionality here */
			if(batteryPercentage > batteryPercentageThreshold) // is battery life below given threshold?
			{
				batteryState = Battery_OK;
			}
			else
			{
				 batteryState = Battery_LOW;
			}
			break;
		default:
			/*
			 * If program encounters a bug or a value outside what is expected we go here.
			 * Feel free to add functionality if needed.
			*/
			batteryState = Battery_START;
			break;
	}
	return batteryState;

}

void INA219_Reset(INA219_t *ina219)
{
	Write16(ina219, INA219_REG_CONFIG, INA219_CONFIG_RESET);
	HAL_Delay(1);
}

void INA219_setCalibration(INA219_t *ina219, uint16_t CalibrationData)
{
	Write16(ina219, INA219_REG_CALIBRATION, CalibrationData);
}

uint16_t INA219_getConfig(INA219_t *ina219)
{
	uint16_t result = Read16(ina219, INA219_REG_CONFIG);
	return result;
}

void INA219_setConfig(INA219_t *ina219, uint16_t Config)
{
	Write16(ina219, INA219_REG_CONFIG, Config);
}

void INA219_setCalibration_32V_2A(INA219_t *ina219)
{
	uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
				 INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
				 INA219_CONFIG_SADCRES_12BIT_1S_532US |
				 INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;

	ina219_calibrationValue = 4096;
	ina219_CurrentDivider_mA = 10; // Current LSB = 100uA per bit (1000/100 = 10)
	ina219_PowerMultiplier_mW = 2; // Power LSB = 1mW per bit (2/1)

	INA219_setCalibration(ina219, ina219_calibrationValue);
	INA219_setConfig(ina219, config);
}

void INA219_setCalibration_32V_1A(INA219_t *ina219)
{
	uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
						INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
						INA219_CONFIG_SADCRES_12BIT_1S_532US |
						INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;

	ina219_calibrationValue = 10240;
	ina219_CurrentDivider_mA = 25;    // Current LSB = 40uA per bit (1000/40 = 25)
	ina219_PowerMultiplier_mW = 0.8f; // Power LSB = 800uW per bit

	INA219_setCalibration(ina219, ina219_calibrationValue);
	INA219_setConfig(ina219, config);
}

void INA219_setCalibration_16V_400mA(INA219_t *ina219)
{
	uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
						INA219_CONFIG_GAIN_1_40MV | INA219_CONFIG_BADCRES_12BIT |
						INA219_CONFIG_SADCRES_12BIT_128S_69MS |
						INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;

	ina219_calibrationValue = 8192;
	ina219_CurrentDivider_mA = 20;    // Current LSB = 50uA per bit (1000/50 = 20)
	ina219_PowerMultiplier_mW = 1.0f; // Power LSB = 1mW per bit

	INA219_setCalibration(ina219, ina219_calibrationValue);
	INA219_setConfig(ina219, config);
}

void INA219_setPowerMode(INA219_t *ina219, uint8_t Mode)
{
	uint16_t config = INA219_getConfig(ina219);

	switch (Mode) {
		case INA219_CONFIG_MODE_POWERDOWN:
			config = (config & ~INA219_CONFIG_MODE_MASK) | (INA219_CONFIG_MODE_POWERDOWN & INA219_CONFIG_MODE_MASK);
			INA219_setConfig(ina219, config);
			break;

		case INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED:
			config = (config & ~INA219_CONFIG_MODE_MASK) | (INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED & INA219_CONFIG_MODE_MASK);
			INA219_setConfig(ina219, config);
			break;

		case INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS:
			config = (config & ~INA219_CONFIG_MODE_MASK) | (INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS & INA219_CONFIG_MODE_MASK);
			INA219_setConfig(ina219, config);
			break;

		case INA219_CONFIG_MODE_ADCOFF:
			config = (config & ~INA219_CONFIG_MODE_MASK) | (INA219_CONFIG_MODE_ADCOFF & INA219_CONFIG_MODE_MASK);
			INA219_setConfig(ina219, config);
			break;
	}
}

void INA219_setCalibration_MaxRes(INA219_t *ina219)
{
	uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
					  INA219_CONFIG_GAIN_1_40MV |
					  INA219_CONFIG_BADCRES_12BIT |
					  INA219_CONFIG_SADCRES_12BIT_128S_69MS |
					  INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;

	ina219_calibrationValue = 8192; // For 50uA/bit, Rshunt = 0.1Ω (corrected from 0.05Ω)
	ina219_CurrentDivider_mA = 20;   // 1000/50uA
	ina219_PowerMultiplier_mW = 1;   // 1mW/bit

	INA219_setCalibration(ina219, ina219_calibrationValue);
	INA219_setConfig(ina219, config);
}

uint8_t initINA219()
{
	isFirst = true; // set global var used by INA219_GetMiliWattMinutes
	ina219.ina219_i2c = INA219_I2C_BUS;
	ina219.Address = INA219_ADDRESS;

	ina219_CurrentDivider_mA = 0;
	ina219_PowerMultiplier_mW = 0;

	uint8_t ina219_isReady = HAL_I2C_IsDeviceReady(ina219.ina219_i2c, (ina219.Address << 1), 3, 10);

	if(ina219_isReady == HAL_OK)
	{
		INA219_Reset(&ina219);
		INA219_setCalibration_MaxRes(&ina219);
		ina219.initialized = 1;
		return 1;
	}

	else
	{
		return 0;
	}
}

void refreshINA219Values()
{
	if (!ina219.initialized) { return; }
	Vbattery = INA219_ReadBusVoltage(&ina219);
	batteryLife = GetBatteryLife(Vbattery, 4200.0f, 3000.0f);

	Vshunt = INA219_ReadShuntVolage(&ina219);
	Current = INA219_ReadCurrent(&ina219);
	Power = INA219_ReadPower(&ina219);

	ina219.busVoltage_mV  = Vbattery;
	ina219.shuntVoltage_mV = Vshunt;
	ina219.current_mA     = Current;
	ina219.power_mW       = Power;
	ina219.batteryLife    = batteryLife;

	__disable_irq();


	// Detect charging: if Vshunt > threshold (e.g., 100mV), assume charging
	// You may want to tune this threshold for your setup
	// if (Vshunt > 100) {
	// 	// Calculate Current as Power/Vbattery (in mA)
	// 	if (Vbattery > 0) {
	// 		Current = (int16_t)((float)Power / ((float)Vbattery / 1000.0f)); // Vbattery in mV to V
	// 		Vshunt = (uint16_t)(Current * 0.05f); // Rshunt = 0.05 Ohm, Vshunt in mV
	// 	}
	// 	// Indicate charging by making Current and Power negative
	// 	Current = -Current;
	// 	Power = -Power;
	// }

	__enable_irq();
}
