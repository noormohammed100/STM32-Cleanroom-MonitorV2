\# Phase 3 test record — SysTick and event scheduling



\## Configuration

\- Board: STM32 Nucleo-F411RE

\- LED: LD2 / PA5

\- Core clock: 16 MHz HSI (verified against RM0383 reset-state section)

\- SysTick clock source: HCLK (CTRL\_CLKSOURCE = 1)

\- SysTick period: 1 ms

\- SysTick LOAD value: 15,999



\## Clock verification

Source: RM0383, Reset state section:

"After reset, the CPU clock frequency is 16 MHz and 0 wait state is

configured in the FLASH\_ACR register."



\- HCLK = 16 MHz (no PLL, no AHB prescaler at reset)

\- SysTick uses HCLK (bit 2 of SysTick->CTRL set in systick.c)

\- Cycles per 1 ms = 16,000,000 / 1,000 = 16,000

\- SysTick->LOAD  = 16,000 - 1 = 15,999



\## Tests



| ID | Procedure | Expected | Actual | Pass |

|----|-----------|----------|--------|------|

| P3-T01 | Break in SysTick\_Handler | Handler reached repeatedly | \[your observation] | \[ ] |

| P3-T02 | Watch g\_millis in debugger | Increments \~1000 per second | \[your observation] | \[ ] |

| P3-T03 | Observe LD2 | Toggles every \~500 ms | LD2 toggles at 500 ms on/off | ✅ |

| P3-T04 | Breakpoint on \_\_NOP() in 2 s test event | Hits every \~2 s while LED continues | Breakpoint hit every \~2 s; LED toggled continuously between hits | ✅ |

| P3-T05 | 5-minute run | No freeze, no reset | \[fill in after you actually run it] | \[ ] |



\## Problems and fixes

\[Fill in anything real. If nothing, write "None."]



\## Build date

\[Actual date you flash and test]

