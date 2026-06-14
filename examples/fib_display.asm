mov r0, 0x80
mov sp, r0
mov r0, timer_int
mov iva, r0
xor r0, r0

mov r15, 0x0008		; 7-segment display I/O address

mov r6, stall	; fake iret stack (don't want to duplicate PIT programming sequence)
push r6
push r6

add r1, r0, 0x0000
add r2, r0, 0x0001
jmp pit_program

stall:
jmp stall

timer_int:
	add r3, r1, r2 ; calculate one step of fib
	add r1, r2, r0
	add r2, r3, r0
	out r15, r1 ; output that to display
pit_program: ; now re-program pit
	mov r4, 0x0001
	mov r5, 0x02fa ; interrupt after 50000000 clocks (1 second)
	shl r5, 16
	or r5, 0xf080
	out r4, r5
	xor r5, r5
	mov r4, 0x0000
	mov r5, 0x0001 ; activate pit
	out r4, r5
	iret
