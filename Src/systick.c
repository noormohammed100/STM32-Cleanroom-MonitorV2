/*
 * systick.c
 *
 *  Created on: 7 Aug 2026
 *      Author: noork
 */
#include "stm32f4xx.h"
#include "systick.h"
#define CTRL_ENABLE (1U<<0)
#define CTRL_CLKSRC (1U<<2)
#define CTRL_IE (1U<<1)
static volatile uint32_t g_millis = 0U;
void systick_init(void)
{
	 if (SYSTICK_LOAD_VAL > 0x00FFFFFFUL)
	    {
	        while (1) { }
	    }
	/*configure systick*/
		//reload with number of clocks per meillisecond
		SysTick->LOAD = SYSTICK_LOAD_VAL;

		//clear SysTick ccurrent value register
		SysTick->VAL = 0;

		//Enable systick and select internal clock source
	    SysTick->CTRL = CTRL_ENABLE|CTRL_CLKSRC|CTRL_IE;
}
void SysTick_Handler(void)
{
	g_millis++;
}
uint32_t systick_millis(void)
{
	return g_millis;
}
