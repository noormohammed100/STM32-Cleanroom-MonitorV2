/*
 * uart.c
 *
 *  Created on: 3 Aug 2026
 *      Author: noork
 */
#include "uart.h"
#include <stddef.h>



#define GPIOAEN     (1U << 0)
#define USART2_EN   (1U << 17)

#define SYS_FREQ       16000000U
#define APB1_CLK       SYS_FREQ
#define USART_BAUDRATE 115200U

/* Correct USART_CR1 bit masks */
#define USART_CR1_RE  (1U << 2)  /* Receiver Enable (Bit 2) */
#define USART_CR1_TE  (1U << 3)  /* Transmitter Enable (Bit 3) */
#define USART_CR1_UE  (1U << 13) /* USART Enable (Bit 13) */

/* USART_SR bit masks */
#define USART_SR_ORE  (1U << 3)  /* Overrun Error (Bit 3) */

#define USART_SR_TXE  (1U << 7)  /* Transmit Data Register Empty (Bit 7) */

#define USART_CR1_RXNEIE (1U<<5)
#define USART_SR_RXNE (1U << 5)  /* Read Data Register Not Empty (Bit 5) */

#define USART_SR_PE  (1U << 0)  /* Parity error */
#define USART_SR_FE  (1U << 1)  /* Framing error */
#define USART_SR_NE  (1U << 2)  /* Noise error */

static volatile uint8_t rx_byte;
static volatile uint8_t rx_byte_ready;

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate);
static uint16_t compute_uart_bd(uint32_t PeriphClk, uint32_t BaudRate);
bool uart2_read_byte(uint8_t *byte);

#define UART2_RX_BUFFER_SIZE  64U

//ring counter
static volatile uint8_t  rx_buffer[UART2_RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0U;   /* Write index (ISR) */
static volatile uint16_t rx_tail = 0U;   /* Read index (main) */
static volatile uint32_t rx_overflow = 0U;

//error counters
static volatile uint32_t uart2_overrun_errors = 0U;
static volatile uint32_t uart2_framing_errors = 0U;
static volatile uint32_t uart2_noise_errors = 0U;
static volatile uint32_t uart2_parity_errors = 0U;

static void usart2_store_byte(uint8_t byte)
{
	uint16_t next_head = (uint16_t)((rx_head + 1U) % UART2_RX_BUFFER_SIZE);
	if (next_head == rx_tail)
	    {
	        /* Buffer full: discard new byte and count overflow */
	        rx_overflow++;
	        return;
	    }

	    rx_buffer[rx_head] = byte;
	    rx_head = next_head;
}
void uart2_write(int ch)
{
	/* Make sure transmit data register is empty */
	while (!(USART2->SR & USART_SR_TXE)) {}
	USART2->DR = (ch & 0xFF);
}

char uart2_read(void)
{
	/* Handle and clear Overrun Error (ORE) if set to prevent RX lockup */
	if (USART2->SR & USART_SR_ORE)
	{
		(void)USART2->SR;
		(void)USART2->DR;
	}

	/* Wait until receive data register is not empty */
	while (!(USART2->SR & USART_SR_RXNE)) {}

	return (char)(USART2->DR & 0xFF);
}

bool uart2_read_nonblocking(uint8_t *byte)
{
    if (byte == NULL)
    {
        return false;
    }

    if (rx_tail == rx_head)
    {
        /* Buffer empty */
        return false;
    }

    *byte = rx_buffer[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1U) % UART2_RX_BUFFER_SIZE);

    return true;
}

uint32_t uart2_rx_overflow_count(void)
{
    return rx_overflow;
}

/* Optional helper for Task 7 */
uint32_t uart2_error_count(void)
{
    return uart2_overrun_errors + uart2_framing_errors +
           uart2_noise_errors + uart2_parity_errors;
}

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate)
{
	USARTx->BRR = compute_uart_bd(PeriphClk, BaudRate);
}

static uint16_t compute_uart_bd(uint32_t PeriphClk, uint32_t BaudRate)
{
	return ((PeriphClk + (BaudRate / 2U)) / BaudRate);
}

int __io_putchar(int ch)
{
	uart2_write(ch);
	return ch;
}

void uart2_tx_init(void)
{
	/* 1. Clock access to GPIOA */
	RCC->AHB1ENR |= GPIOAEN;

	/* 2. Configure PA2 as Alternate Function (10) */
	GPIOA->MODER &= ~(3U << 4);  /* Clear MODER2 (bits 5:4) */
	GPIOA->MODER |=  (2U << 4);  /* Set AF mode */

	/* 3. Configure PA2 AF07 (USART2_TX) in AFRL */
	GPIOA->AFR[0] &= ~(0xFU << 8); /* Clear bits 11:8 */
	GPIOA->AFR[0] |=  (7U << 8);   /* Set AF07 */

	/* 4. Clock access to USART2 module */
	RCC->APB1ENR |= USART2_EN;

	/* 5. Set baud rate */
	uart_set_baudrate(USART2, APB1_CLK, USART_BAUDRATE);

	/* 6. Enable Transmitter and USART module */
	USART2->CR1 = USART_CR1_TE;
	USART2->CR1 |= USART_CR1_UE;
}

void uart2_rxtx_init(void)
{
	/* 1. Clock access to GPIOA */
	RCC->AHB1ENR |= GPIOAEN;

	/* 2. Configure PA2 (TX) & PA3 (RX) as Alternate Function (10) */
	GPIOA->MODER &= ~((3U << 4) | (3U << 6)); /* Clear MODER2 & MODER3 */
	GPIOA->MODER |=  ((2U << 4) | (2U << 6)); /* Set AF mode */

	/* 3. Configure PA2 & PA3 to AF07 (USART2) in AFRL */
	GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12)); /* Clear bits 11:8 and 15:12 */
	GPIOA->AFR[0] |=  ((7U << 8)  | (7U << 12));   /* Set AF07 for PA2 & PA3 */

	/* 4. Clock access to USART2 module */
	RCC->APB1ENR |= USART2_EN;

	/* 5. Set baud rate */
	uart_set_baudrate(USART2, APB1_CLK, USART_BAUDRATE);

	/* 6. Enable Transmitter, Receiver, and USART module */
	USART2->CR1 = USART_CR1_TE | USART_CR1_RE;
	USART2->CR1 |= USART_CR1_UE;
}
void uart2_rxtx_interrupt_init(void)
{

	/*  Clock access to GPIOA */
	RCC->AHB1ENR |= GPIOAEN;

	/*  Configure PA2 (TX) & PA3 (RX) as Alternate Function (10) */
	GPIOA->MODER &= ~((3U << 4) | (3U << 6)); /* Clear MODER2 & MODER3 */
	GPIOA->MODER |=  ((2U << 4) | (2U << 6)); /* Set AF mode */

	/*  Configure PA2 & PA3 to AF07 (USART2) in AFRL */
	GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12)); /* Clear bits 11:8 and 15:12 */
	GPIOA->AFR[0] |=  ((7U << 8)  | (7U << 12));   /* Set AF07 for PA2 & PA3 */

	/*  Clock access to USART2 module */
	RCC->APB1ENR |= USART2_EN;

	/*  Set baud rate */
	uart_set_baudrate(USART2, APB1_CLK, USART_BAUDRATE);

	/*  Enable Transmitter, Receiver, and USART module */
	USART2->CR1 = USART_CR1_TE | USART_CR1_RE;
	//Enable RXNE Interrupt
	USART2->CR1 |= USART_CR1_RXNEIE;
	//Enable UART Interrupt in NVIC
	NVIC_EnableIRQ(USART2_IRQn);
	USART2->CR1 |= USART_CR1_UE;
}

void uart2_write_string(const char *string)
{
    if (string == NULL)
    {
        return;
    }

    while (*string != '\0')
    {
        uart2_write(*string);
        string++;
    }
}
void USART2_IRQHandler(void)
{
    uint32_t status = USART2->SR;

    /* Handle overrun error */
      if (status & USART_SR_ORE)
      {
          uart2_overrun_errors++;
      }

      /* Handle framing error */
      if (status & USART_SR_FE)
      {
          uart2_framing_errors++;
      }

      /* Handle noise error */
      if (status & USART_SR_NE)
      {
          uart2_noise_errors++;
      }

      /* Handle parity error */
      if (status & USART_SR_PE)
      {
          uart2_parity_errors++;
      }
    if (status & USART_SR_ORE)
    {
        uart2_overrun_errors++;
    }

    /* Normal receive */
    if (status & USART_SR_RXNE)
    {
        uint8_t byte = (uint8_t)USART2->DR;
        usart2_store_byte(byte);
    }
    else if (status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE))
      {
          /*
           * If an error flag is set but RXNE is not, we still must
           * read DR once to clear the error condition on STM32F4.
           */
          (void)USART2->DR;
      }

    }
