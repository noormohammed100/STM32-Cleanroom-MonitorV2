/*
 * systick.h
 *
 *  Created on: 7 Aug 2026
 *      Author: noork
 */

#ifndef SYSTICK_H_
#define SYSTICK_H_
#include <stdint.h>
#define SYSTEM_CORE_CLOCK_HZ (16000000UL)
#define SYSTICK_TICK_HZ      (1000UL)
#define SYSTICK_LOAD_VAL ((SYSTEM_CORE_CLOCK_HZ / SYSTICK_TICK_HZ) - 1UL)
void systick_init(void);
uint32_t systick_millis(void);


#endif /* SYSTICK_H_ */
