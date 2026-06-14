; interact with various I/O devices and verify correctness

PIT_NEVER_INTERRUPTED equ 0b0000000000000001

xor r0, r0 ; ensure cleanlyness of registers we will use
xor r1, r1
xor r14, r14
xor r15, r15

mov r0, interrupt_handler
mov iva, r0

mov r0, 0x080
mov sp, r0

mov r0, 0x0001
mov r1, 24
out r0, r1
mov r0, 0x0000
mov r1, 0x0001
out r0, r1

mov r0, 4
xor r1, r1
loop:
	add r1, 1
	blt r0, r1, loop
xor r0, r0
mov r0, 0xffff
beq r0, r15, .continue ; i want bne r0, 15, pit_error because this should be negated, cant, must do this
or r14, PIT_NEVER_INTERRUPTED
.continue:

; add new tests here hew new I/O devices

done:	; ON END:
	; r0  = 0xff00 (Stall Signal)
	; r14 = Error Bitmap
	mov r0, 0xff00 ; signal end with 0xff00 in r0 and stall
stall:
	jmp stall

interrupt_handler:
	push r0
	push r1
	mov r1, sp
	mov r0, [r1 + 0x0003]
	xor r1, r1
	beq r0, r1, ack_pit
exit_int:
	pop r1
	pop r0
	iret
ack_pit:
	out r1, r1	; r1 happens to be zero, both need to be zero
	mov r15, 0xffff	; signal pit ack to loop (see loop:)
	jmp exit_int