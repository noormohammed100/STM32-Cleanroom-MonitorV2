/*
 * uart.h
 *
 *  Created on: 3 Aug 2026
 *      Author: noork
 */

#ifndef UART_H_
#define UART_H/*
 * uart.h
 *
 *  Created on: 3 Aug 2026
 *      Author: noork
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <stdbool.h>
#include <stm32f4xx.h>

void uart2_write_string(const char *string);
bool uart2_read_nonblocking(uint8_t *byte);
uint32_t uart2_rx_overflow_count(void);
uint32_t uart2_error_count(void);
void uart2_tx_init (void);
char uart2_read(void);
void uart2_rxtx_init(void);
void uart2_write(int ch);
void uart2_rxtx_interrupt_init(void);


#endif /* UART_H_ */
_

#include <stdint.h>
#include "stm32f4xx.h"

void uart2_tx_init (void);
char uart2_read(void);
void uart2_rxtx_init(void);
void uart2_write(int ch);

#endif /* UART_H_ */
