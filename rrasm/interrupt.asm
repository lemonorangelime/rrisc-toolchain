mov r0, 0x0100
mov sp, r0	; set up stack through `sp`
mov r0, interrupt_handler
mov iva, r0	; set up interrupt handler through `iva` control register

mov r0, 0x4321	; set r0 to something meaningful so we can check if it's preserved
dd 0xfefefefe	; invalid opcode, should trigger exception

mov r0, pit_handler
mov iva, r0

mov r1, 0x0001
mov r0, 32 ; interrupt after 32 clocks
out r1, r0
mov r1, 0x0000
mov r0, 0x0001 ; activate pit
out r1, r0

mov r15, 0x1234	; signal stall in gtkwave
stall:
jmp stall	; stall

interrupt_handler:
	push r0		; we will clobber r0
	push r1		; and r1
	mov r1, sp	; grab stack pointer into r1
	mov r0,  [r1 + 0x0002]	; interrupt address
	mov r14, [r1 + 0x0003]	; interrupt reason
	add r0, 1	; skip faulting instruction
	mov [r1 + 0x0002], r0	; write that back to stack
	pop r1
	pop r0
	iret

pit_handler: ; ack
	push r0
	push r1
	mov r1, 0x0000
	mov r0, 0x0000 ; ack interrupt
	out r1, r0
	pop r1
	pop r0
	iret