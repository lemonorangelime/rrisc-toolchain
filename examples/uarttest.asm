UART_RX equ 0xfff1
UART_TX equ 0xfff2
UART_STAT equ 0xfff0

UART_STAT_RX_READY equ 0b00000001
UART_STAT_TX_READY equ 0b00000010

_start:
	mov r0, UART_RX
	mov r1, UART_STAT
	mov r2, UART_STAT_RX_READY
_start.poll_uart:
	mov r3, [r1] ; poll RX_READY bit
	and r3, r2
	bneq r3, r2, _start.poll_uart
	mov r4, [r0] ; read
	jmp _start