/*
 * I2C.h
 *
 *  Created on: 19 Aug 2026
 *      Author: noork
 */

#ifndef I2C_H_
#define I2C_H_

typedef enum {
    I2C_OK = 0,
    I2C_TIMEOUT,
} i2c_status_t;

i2c_status_t I2C_init(void);
i2c_status_t I2C_byteRead(char saddr, char maddr, char* data);
i2c_status_t I2C_burstRead(char saddr, char maddr,int n, char* data);
i2c_status_t I2C1_burstWrite(char saddr, char maddr, int n, char* data);


#endif /* I2C_H_ */
