/*
 * bme280.c
 *
 *  Created on: 17 Sept 2026
 *      Author: noork
 */
#include "I2C.h"
#include <stdint.h>

#define BME280_ADDRESS       	 0x76
#define BME280_CHIP_ID_REG   	 0xD0
#define BME280_CHIP_ID       	 0x60

#define BME280_CONFIG        0xF5
#define BME280_CTRL_HUM      0xF2
#define BME280_CTRL_MEAS     0xF4


typedef enum {
    BME280_OK,
	BME280_Initialised,
	BUS_ERROR,
	ERROR_ID_MISMATCH
} BME280_STATUS;

BME280_STATUS read_chip_ID (void)
{
	uint8_t data;
	i2c_status_t status;
	status = I2C_byteRead(BME280_ADDRESS,BME280_CHIP_ID_REG,(char *)&data);
	if(status != I2C_OK)
	{
		return BUS_ERROR;
	}
	else if (data == BME280_CHIP_ID)
	{
		return BME280_OK;
	}
	else
	{
		return ERROR_ID_MISMATCH;
	}
}


BME280_STATUS BME280_init()
{
	i2c_status_t status;
	BME280_STATUS ID_Check;
	uint8_t reg_val;

	ID_Check = read_chip_ID();

	if(ID_Check != BME280_OK)
	{
		return ID_Check;
	}


	reg_val = 0xA0;
	status = I2C1_burstWrite (BME280_ADDRESS,BME280_CONFIG,1,(char*)&reg_val);
	if(status != I2C_OK)
		{
			return BUS_ERROR;
		}
	reg_val = 0x01;
	status = I2C1_burstWrite (BME280_ADDRESS,BME280_CTRL_HUM,1,(char*)&reg_val);
	if(status != I2C_OK)
		{
			return BUS_ERROR;
		}
	reg_val = 0x27;
	status = I2C1_burstWrite (BME280_ADDRESS,BME280_CTRL_MEAS,1,(char*)&reg_val);
	if(status != I2C_OK)
		{
			return BUS_ERROR;
		}
	return BME280_Initialised;
}


