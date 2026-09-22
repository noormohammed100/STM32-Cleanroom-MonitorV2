#include "stm32f4xx.h"
#include "I2C.h"
#include "systick.h"
#define GPIOBEN                 (1U << 1)
#define I2C1EN                  (1U << 21)
#define I2C_100KHZ              80
#define SD_MODE_MAX_RISE_TIME   17

#define CR1_PE                  (1U << 0)
#define CR1_START               (1U << 8)
#define CR1_STOP                (1U << 9)
#define CR1_ACK                 (1U << 10)
#define CR1_SWRST               (1U << 15)

#define SR1_SB                  (1U << 0)
#define SR1_ADDR                (1U << 1)
#define SR1_BTF                 (1U << 3)
#define SR1_RXNE                (1U << 6)
#define SR1_TxE                 (1U << 7)
#define SR2_BUSY                (1U << 1)

#define WAIT_CLEAR      0
#define WAIT_SET        1

#define I2C_TIMEOUT_MS        1000





static i2c_status_t wait_flag(volatile uint32_t *reg, uint32_t bit,
                               int wait_until_set, uint32_t timeout_ms)
{
    uint32_t start = systick_millis();

    while (1) {
        int bit_is_set = (*reg & bit) != 0;
        if (bit_is_set == wait_until_set) {
            return I2C_OK;
        }
        if ((systick_millis() - start) >= timeout_ms) {
            return I2C_TIMEOUT;
        }
    }
}

i2c_status_t I2C_init(void)
{
    // 1. Enable Clock to GPIOB and I2C1
    RCC->AHB1ENR |= GPIOBEN;
    RCC->APB1ENR |= I2C1EN;

    // 2. Configure PB8 (SCL) and PB9 (SDA) as Alternate Function Open-Drain with Pull-ups
    GPIOB->MODER &= ~((3U << 16) | (3U << 18));
    GPIOB->MODER |=  ((2U << 16) | (2U << 18)); // AF mode

    GPIOB->OTYPER |= (1U << 8) | (1U << 9);      // Open-Drain

    GPIOB->PUPDR &= ~((3U << 16) | (3U << 18));
    GPIOB->PUPDR |=  ((1U << 16) | (1U << 18)); // Pull-up

    // Set AF4 (I2C1) on PB8 and PB9 (AFRH bits 3:0 and 7:4)
    GPIOB->AFR[1] &= ~((0xFU << 0) | (0xFU << 4));
    GPIOB->AFR[1] |=  ((4U << 0) | (4U << 4));

    // 3. Reset and Configure I2C1
    I2C1->CR1 |= CR1_SWRST;
    I2C1->CR1 &= ~CR1_SWRST;

    I2C1->CR2 = 16;                           // 16 MHz APB1 clock
    I2C1->CCR = I2C_100KHZ;                   // Standard mode 100 kHz
    I2C1->TRISE = SD_MODE_MAX_RISE_TIME;      // Rise time
    I2C1->CR1 |= CR1_PE;                      // Peripheral Enable

    return I2C_OK;
}

i2c_status_t I2C_byteRead(char saddr, char maddr, char* data)
{
    volatile int tmp;

    i2c_status_t status;

    status = wait_flag(&I2C1->SR2, SR2_BUSY, WAIT_CLEAR, I2C_TIMEOUT_MS);
        if (status != I2C_OK)
        	{
        	return status;
        	}

    // START condition
    I2C1->CR1 |= CR1_START;
    status = wait_flag(&I2C1->SR1, SR1_SB, WAIT_SET, I2C_TIMEOUT_MS);
            if (status != I2C_OK)
            	{
            	return status;
            	}

    // Slave address + Write
    I2C1->DR = (saddr << 1);
    status = wait_flag(&I2C1->SR1, SR1_ADDR, WAIT_SET, I2C_TIMEOUT_MS);
                if (status != I2C_OK)
                	{
                	return status;
                	}
    tmp = I2C1->SR2; // Clear ADDR

    // Memory address
    status = wait_flag(&I2C1->SR1, SR1_TxE, WAIT_SET, I2C_TIMEOUT_MS);
                if (status != I2C_OK)
                	{
                	return status;
                	}
    I2C1->DR = maddr;
    status = wait_flag(&I2C1->SR1, SR1_TxE, WAIT_SET, I2C_TIMEOUT_MS);
                if (status != I2C_OK)
                	{
                	return status;
                	}

    // Restart condition
    I2C1->CR1 |= CR1_START;
    status = wait_flag(&I2C1->SR1, SR1_SB, WAIT_SET, I2C_TIMEOUT_MS);
                if (status != I2C_OK)
                	{
                	return status;
                	}

    // Slave address + Read
    I2C1->DR = (saddr << 1) | 1;
    status = wait_flag(&I2C1->SR1, SR1_ADDR, WAIT_SET, I2C_TIMEOUT_MS);
                if (status != I2C_OK)
                	{
                	return status;
                	}

    // Single-byte read protocol: Clear ACK BEFORE clearing ADDR
    I2C1->CR1 &= ~CR1_ACK;
    tmp = I2C1->SR2; // Clear ADDR to start single reception

    // Generate STOP condition
    I2C1->CR1 |= CR1_STOP;

    // Read byte
    status = wait_flag(&I2C1->SR1, SR1_RXNE, WAIT_SET, I2C_TIMEOUT_MS);
                if (status != I2C_OK)
                	{
                	return status;
                	}
    *data = (uint8_t)I2C1->DR;

    // Wait until STOP condition completes
    status = wait_flag(&I2C1->CR1, CR1_STOP, WAIT_CLEAR, I2C_TIMEOUT_MS);
                if (status != I2C_OK)
                	{
                	return status;
                	}
                return I2C_OK;
}

i2c_status_t I2C_burstRead(char saddr, char maddr, int n, char* data)
{
    volatile int tmp;

    i2c_status_t status;

    status = wait_flag(&I2C1->SR2, SR2_BUSY, WAIT_CLEAR, I2C_TIMEOUT_MS);
                if (status != I2C_OK)
                	{
                	return status;
                	}

    // START condition
    I2C1->CR1 |= CR1_START;
    status = wait_flag(&I2C1->SR1, SR1_SB, WAIT_SET, I2C_TIMEOUT_MS);
                    if (status != I2C_OK)
                    	{
                    	return status;
                    	}

    // Slave address + Write
    I2C1->DR = (saddr << 1);
    status = wait_flag(&I2C1->SR1, SR1_ADDR, WAIT_SET, I2C_TIMEOUT_MS);
                        if (status != I2C_OK)
                        	{
                        	return status;
                        	}
    tmp = I2C1->SR2;

    // Memory address
    status = wait_flag(&I2C1->SR1, SR1_TxE, WAIT_SET, I2C_TIMEOUT_MS);
                        if (status != I2C_OK)
                        	{
                        	return status;
                        	}
    I2C1->DR = maddr;
    status = wait_flag(&I2C1->SR1, SR1_TxE, WAIT_SET, I2C_TIMEOUT_MS);
                        if (status != I2C_OK)
                        	{
                        	return status;
                        	}

    // Restart condition
    I2C1->CR1 |= CR1_START;
    status = wait_flag(&I2C1->SR1, SR1_SB, WAIT_SET, I2C_TIMEOUT_MS);
                        if (status != I2C_OK)
                        	{
                        	return status;
                        	}

    // Slave address + Read
    I2C1->DR = (saddr << 1) | 1;
    status = wait_flag(&I2C1->SR1, SR1_ADDR, WAIT_SET, I2C_TIMEOUT_MS);
                        if (status != I2C_OK)
                        	{
                        	return status;
                        	}

    // Enable ACK and clear ADDR
    I2C1->CR1 |= CR1_ACK;
    tmp = I2C1->SR2;

    while (n > 0)
    {
        if (n == 3)
        {
            // Wait for BTF: Byte N-2 in DR, Byte N-1 in shift register
            status = wait_flag(&I2C1->SR1, SR1_BTF, WAIT_SET, I2C_TIMEOUT_MS);
                                if (status != I2C_OK)
                                	{
                                	return status;
                                	}
            I2C1->CR1 &= ~CR1_ACK;            // Send NACK on the last byte
            *data++ = (uint8_t)I2C1->DR;         // Read Byte N-2
            n--;

            // Wait for next BTF: Byte N-1 in DR, Byte N in shift register
            status = wait_flag(&I2C1->SR1, SR1_BTF, WAIT_SET, I2C_TIMEOUT_MS);
                                if (status != I2C_OK)
                                	{
                                	return status;
                                	}
            I2C1->CR1 |= CR1_STOP;            // Generate STOP
            *data++ = (uint8_t)I2C1->DR;         // Read Byte N-1
            n--;

            status = wait_flag(&I2C1->SR2, SR2_BUSY, WAIT_SET, I2C_TIMEOUT_MS);
                                if (status != I2C_OK)
                                	{
                                	return status;
                                	}
            *data++ = (uint8_t)I2C1->DR;         // Read Byte N
            n--;
            break;
        }
        else
        {
            status = wait_flag(&I2C1->SR1, SR1_RXNE, WAIT_SET, I2C_TIMEOUT_MS);
                                if (status != I2C_OK)
                                	{
                                	return status;
                                	}
            *data++ = (uint8_t)I2C1->DR;
            n--;
        }
    }

    status = wait_flag(&I2C1->CR1, CR1_STOP, WAIT_CLEAR, I2C_TIMEOUT_MS);
                        if (status != I2C_OK)
                        	{
                        	return status;
                        	}
                        return I2C_OK;
}

i2c_status_t I2C1_burstWrite(char saddr, char maddr, int n, char* data)
{
    volatile int tmp;

    i2c_status_t status;

    status = wait_flag(&I2C1->SR2, SR2_BUSY, WAIT_CLEAR, I2C_TIMEOUT_MS);
                            if (status != I2C_OK)
                            	{
                            	return status;
                            	}


    // START condition
    I2C1->CR1 |= CR1_START;

    status = wait_flag(&I2C1->SR1, SR1_SB, WAIT_SET, I2C_TIMEOUT_MS);
                            if (status != I2C_OK)
                            	{
                            	return status;
                            	}

    // Slave address + Write
    I2C1->DR = (saddr << 1);

    status = wait_flag(&I2C1->SR1, SR1_ADDR, WAIT_SET, I2C_TIMEOUT_MS);
                               if (status != I2C_OK)
                               	{
                               	return status;
                               	}
    tmp = I2C1->SR2;

    // Memory address
    status = wait_flag(&I2C1->SR1, SR1_TxE, WAIT_SET, I2C_TIMEOUT_MS);
                               if (status != I2C_OK)
                               	{
                               	return status;
                               	}
    I2C1->DR = maddr;

    for (int i = 0; i < n; i++)
    {

      status = wait_flag(&I2C1->SR1, SR1_TxE, WAIT_SET, I2C_TIMEOUT_MS);
                               if (status != I2C_OK)
                                {
                                  return status;
                                }
        I2C1->DR = *data++;
    }

    // Wait until last byte is fully shifted out onto the wire
    status = wait_flag(&I2C1->SR1, SR1_BTF, WAIT_SET, I2C_TIMEOUT_MS);
                               if (status != I2C_OK)
                               	{
                               	return status;
                               	}

    // STOP condition
    I2C1->CR1 |= CR1_STOP;
    status = wait_flag(&I2C1->CR1, CR1_STOP, WAIT_CLEAR, I2C_TIMEOUT_MS);
                               if (status != I2C_OK)
                               	{
                               	return status;
                               	}
                               return I2C_OK;
}


