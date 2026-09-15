/*
 * systick.c
 *
 *  Created on: 7 Aug 2026
 *      Author: noork
 */
#include "stm32f4xx.h"
#define SYSTICK_LOAD_VAL 16000 //because we are trying to create 1ms delay
#define CTRL_ENABLE (1U<<0)
#define CTRL_CLKSRC (1U<<2)
#define CTRL_COUNTFLAG (1U<<16)
#define one_hz_LOAD 16000000
#define CTRL_IE (1U<<1)
void systickDelayMS(int delay)
{
	/*configure systick*/
	//reload with number of clocks per meillisecond
	SysTick->LOAD = SYSTICK_LOAD_VAL;

	//clear SysTick ccurrent value register
	SysTick->VAL = 0;

	//Enable systick and select internal clock source
    SysTick->CTRL = CTRL_ENABLE|CTRL_CLKSRC;

for(int i=0; i<delay; i++)
{
	//wait until count flag is set
	while((SysTick->CTRL & CTRL_COUNTFLAG) == 0){}
}

SysTick->CTRL = 0;

}

void systick_1hz_interrupt(void)
{
	//configure LOAD for 1Hz
	SysTick->LOAD = one_hz_LOAD -1;
	//clear systick current value register
	SysTick->VAL = 0;
	//Enable systick, systick interrupt and select internal clock source
	    SysTick->CTRL = CTRL_ENABLE|CTRL_CLKSRC |CTRL_IE;
}
