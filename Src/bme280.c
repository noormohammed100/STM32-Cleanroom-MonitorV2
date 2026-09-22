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

#define BME280_DATA_START_REG  0xF7
#define BME280_DATA_FRAME_LEN  8

#define BME280_REG_CALIB_1    0x88
#define BME280_CALIB_1_LEN    26

#define BME280_REG_CALIB_2    0xE1
#define BME280_CALIB_2_LEN    7

typedef struct {
    /* Temperature calibration */
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    /* Pressure calibration */
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    /* Humidity calibration */
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_data_t;
static bme280_calib_data_t g_calib;

typedef enum {
    BME280_OK,
	BME280_Initialised,
	BUS_ERROR,
	ERROR_ID_MISMATCH
} BME280_STATUS;

typedef struct {
    int32_t raw_temp;
    int32_t raw_press;
    int32_t raw_hum;
} bme280_raw_data_t;


uint8_t data_reg[91];
char data;


BME280_STATUS read_chip_ID(void);
BME280_STATUS BME280_read_calibration(void);
static int32_t BME280_compensate_T_int32(int32_t adc_T);
static uint32_t BME280_compensate_P_int32(int32_t adc_P);
static uint32_t BME280_compensate_H_int32(int32_t adc_H);



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

BME280_STATUS BME280_read(bme280_raw_data_t *raw_data)
{
	i2c_status_t status;
	BME280_STATUS ID_Check;

	uint8_t buffer[BME280_DATA_FRAME_LEN];

	ID_Check = read_chip_ID();
	if(ID_Check != BME280_OK)
	{
		return ID_Check;
	}

	status = I2C_burstRead (BME280_ADDRESS,BME280_DATA_START_REG,BME280_DATA_FRAME_LEN,(char*) buffer);
	if(status != I2C_OK)
		{
			return BUS_ERROR;
		}
	/* Pressure: registers 0xF7 (MSB), 0xF8 (LSB), 0xF9 (XLSB bits [7:4]) */
	raw_data->raw_press = (int32_t)(((uint32_t)buffer[0] << 12) |
	                                    ((uint32_t)buffer[1] << 4)  |
	                                    ((uint32_t)buffer[2] >> 4));

	    /* Temperature: registers 0xFA (MSB), 0xFB (LSB), 0xFC (XLSB bits [7:4]) */
	raw_data->raw_temp = (int32_t)(((uint32_t)buffer[3] << 12) |
	                                   ((uint32_t)buffer[4] << 4)  |
	                                   ((uint32_t)buffer[5] >> 4));

	    /* Humidity: registers 0xFD (MSB), 0xFE (LSB) */
	raw_data->raw_hum = (int32_t)(((uint32_t)buffer[6] << 8) |
	                                  (uint32_t)buffer[7]);

	return BME280_OK;
}

BME280_STATUS BME280_read_calibration(void)
{
    uint8_t b1[BME280_CALIB_1_LEN]; // 26 bytes: 0x88 - 0xA1
    uint8_t b2[BME280_CALIB_2_LEN]; // 7 bytes:  0xE1 - 0xE7
    i2c_status_t status;

    /* 1. Burst-read block 1 (Temperature, Pressure, H1) */
    status = I2C_burstRead(BME280_ADDRESS, BME280_REG_CALIB_1, BME280_CALIB_1_LEN, (char *)b1);
    if (status != I2C_OK)
    {
        return BUS_ERROR;
    }

    /* 2. Burst-read block 2 (H2 - H6) */
    status = I2C_burstRead(BME280_ADDRESS, BME280_REG_CALIB_2, BME280_CALIB_2_LEN, (char *)b2);
    if (status != I2C_OK)
    {
        return BUS_ERROR;
    }

    /* 3. Unpack Temperature Coefficients */
    g_calib.dig_T1 = (uint16_t)((b1[1] << 8) | b1[0]);
    g_calib.dig_T2 = (int16_t) ((b1[3] << 8) | b1[2]);
    g_calib.dig_T3 = (int16_t) ((b1[5] << 8) | b1[4]);

    /* 4. Unpack Pressure Coefficients */
    g_calib.dig_P1 = (uint16_t)((b1[7]  << 8) | b1[6]);
    g_calib.dig_P2 = (int16_t) ((b1[9]  << 8) | b1[8]);
    g_calib.dig_P3 = (int16_t) ((b1[11] << 8) | b1[10]);
    g_calib.dig_P4 = (int16_t) ((b1[13] << 8) | b1[12]);
    g_calib.dig_P5 = (int16_t) ((b1[15] << 8) | b1[14]);
    g_calib.dig_P6 = (int16_t) ((b1[17] << 8) | b1[16]);
    g_calib.dig_P7 = (int16_t) ((b1[19] << 8) | b1[18]);
    g_calib.dig_P8 = (int16_t) ((b1[21] << 8) | b1[20]);
    g_calib.dig_P9 = (int16_t) ((b1[23] << 8) | b1[22]);

    /* 5. Unpack Humidity Coefficients */
    g_calib.dig_H1 = b1[25];
    g_calib.dig_H2 = (int16_t) ((b2[1] << 8) | b2[0]);
    g_calib.dig_H3 = b2[2];
    g_calib.dig_H4 = (int16_t) (((int8_t)b2[3] << 4) | (b2[4] & 0x0F));
    g_calib.dig_H5 = (int16_t) (((int8_t)b2[5] << 4) | (b2[4] >> 4));
    g_calib.dig_H6 = (int8_t)  b2[6];

    return BME280_OK;
}


// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
int32_t t_fine;
static int32_t BME280_compensate_T_int32(int32_t adc_T)
{
int32_t var1, var2, T;
var1 = ((((adc_T>>3) - ((int32_t)g_calib.dig_T1<<1))) * ((int32_t)g_calib.dig_T2)) >> 11;
var2 = (((((adc_T>>4) - ((int32_t)g_calib.dig_T1)) * ((adc_T>>4) - ((int32_t)g_calib.dig_T1))) >> 12) *
((int32_t)g_calib.dig_T3)) >> 14;
t_fine = var1 + var2;
T = (t_fine * 5 + 128) >> 8;
return T;
}
// Returns pressure in Pa as unsigned 32 bit integer. Output value of “96386” equals 96386 Pa= 963.86 hPa
static uint32_t BME280_compensate_P_int32(int32_t adc_P)
{
	int32_t var1, var2;
	uint32_t p;
var1 = (((int32_t)t_fine)>>1) - (int32_t)64000;
var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((int32_t)g_calib.dig_P6);
var2 = var2 + ((var1*((int32_t)g_calib.dig_P5))<<1);
var2 = (var2>>2)+(((int32_t)g_calib.dig_P4)<<16);
var1 = (((g_calib.dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((int32_t)g_calib.dig_P2) * var1)>>1))>>18;
var1 =((((32768+var1))*((int32_t)g_calib.dig_P1))>>15);
if (var1 == 0)
{
return 0; // avoid exception caused by division by zero
}
p = (((uint32_t)(((int32_t)1048576)-adc_P)-(var2>>12)))*3125;
if (p < 0x80000000)
{
p = (p << 1) / ((uint32_t)var1);
}
else
{
p = (p / (uint32_t)var1) * 2;
}
var1 = (((int32_t)g_calib.dig_P9) * ((int32_t)(((p>>3) * (p>>3))>>13)))>>12;
var2 = (((int32_t)(p>>2)) * ((int32_t)g_calib.dig_P8))>>13;
p = (uint32_t)((int32_t)p + ((var1 + var2 + g_calib.dig_P7) >> 4));
return p;
}


