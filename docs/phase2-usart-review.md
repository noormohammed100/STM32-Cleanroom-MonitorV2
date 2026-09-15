# Phase 2 USART Review

## Existing functionality

- [ ] USART2 initialisation
- [ ] Single-byte transmit
- [ ] String transmit
- [ ] `printf` output
- [ ] Single-byte receive
- [ ] Full-duplex transmit/receive
- [ ] Receive interrupt
- [ ] Receive buffer
- [ ] Non-blocking read
- [ ] Timeout-aware read
- [ ] Error handling
- [ ] PC terminal test

## Baseline observations

- USART: USART2
- TX pin: PA2
- RX pin: PA3
- Alternate function: AF7
- Baud rate: 115200
- Peripheral clock assumption: 16 MHz APB1
- Terminal settings: 8 data bits, no parity, 1 stop bit
- Test date:
- Firmware commit:

## How to verify

1. Build and flash the current version.
2. Open a PC terminal at 115200 8-N-1.
3. Type a character. Confirm it is echoed.
4. Send `1`. Confirm the LED turns on.
5. Send another character. Confirm the LED turns off.
6. Send nothing for at least one minute. Confirm the firmware does not stop.

## Baseline test results

| Step | Test | Expected | Actual | Status |
|---|---|---|---|---|
| 1 | Type a character | Echo received | | |
| 2 | Send `1` | LED on | | |
| 3 | Send another character | LED off | | |
| 4 | No input for 1 minute | Firmware continues | | |

## Notes

- Current ISR calls `uart_callback()`.
- `uart_callback()` echoes using `uart2_write()` and controls the LED directly.
- This means the ISR can block on transmission.
- No ring buffer or non-blocking receive API exists yet.