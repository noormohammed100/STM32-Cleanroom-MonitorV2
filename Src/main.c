#include <stdio.h>
#include "stm32f4xx.h"
#include <stdint.h>
#include "uart.h"
#include "systick.h"


#define GPIOAEN (1U<<0)
#define PIN5 (1U<<5)
#define LED_PIN PIN5

static void callback_systick(void);

int main (void)
{
RCC->AHB1ENR = GPIOAEN;
GPIOA->MODER |= (1U<<10);
GPIOA->MODER &=~ (1U<<11);

systick_1hz_interrupt();
uart2_tx_init();


while(1)
	{




	}

}
static void callback_systick(void)
{
	printf("A second just passed \n\r");
			GPIOA->ODR ^= LED_PIN;
}
void SysTick_Handler(void)
{
	callback_systick();
}


