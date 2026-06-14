; test various I/O devices and instructions to verify correctness and locate hardware bugs

PIT_NEVER_INTERRUPTED equ 0 ; programed pit to interrupt, never did (in reasonable amount of time)
JAL_UNEXPECTED_PC equ 1 ; jal put unexpected value into register
JAL_BRANCH_NEVER_TAKEN equ 2 ; jal did not jump
STP_UNEXPECTED_SP equ 3 ; unexpected stack pointer after stp
PUSH_UNEXPECTED_SP equ 4 ; unexpected stack pointer after push
POP_UNEXPECTED_SP equ 5 ; unexpected stack pointer after pop
STACK_CORRUPTED equ 6 ; value on stack did not match value pushed
BEQ_TOOK_WRONG_BRANCH equ 7 ; beq took the wrong branch
BLT_TOOK_WRONG_BRANCH equ 8 ; blt took the wrong branch
INCORRECT_ENDIANNESS equ 9 ; nebulous endianness problem
SPURIOUS_INTERRUPT equ 10 ; received interrupt when none was expected or incorrect interrupt source

OKAY equ 0xffffffff ; everything okay

PIT_CLOCK_DELAY equ 24 ; how many cycles to program pit for
PIT_POLL_CYCLES equ 4 ; how long to wait for the pit to finally interrupt

xor r0, r0 ; ensure cleanlyness of registers we will use
xor r1, r1
xor r14, r14
xor r15, r15

mov r0, interrupt_handler
mov iva, r0

mov r0, 0x080
mov sp, r0

mov r0, 0x0001
mov r1, PIT_CLOCK_DELAY
out r0, r1
mov r0, 0x0000
mov r1, 0x0001
out r0, r1
mov r13, 0xff00

mov r14, PIT_NEVER_INTERRUPTED
mov r0, PIT_POLL_CYCLES
xor r1, r1
loop:
	add r1, 1
	blt r0, r1, loop
xor r0, r0
mov r0, 0xffff
beq r0, r15, .continue ; i want bne r0, 15, pit_error because this should be negated, cant, must do this
jmp done
.continue:

mov r14, JAL_BRANCH_NEVER_TAKEN
xor r0, r0
xor r1, r1
call r0, skip
jal_expected:
	jmp done
skip:
	mov r14, JAL_UNEXPECTED_PC
	mov r1, jal_expected
	beq r0, r1, .jalfine
	jmp done
.jalfine:

mov r14, PUSH_UNEXPECTED_SP
xor r0, r0
xor r1, r1
xor r2, r2
mov r2, 0x1234
shl r2, 16
or r2, 0x5678
mov r0, 0x080
mov sp, r0
push r2
mov r1, sp
add r1, 1
beq r0, r1, .pushfine
jmp done
.pushfine:

mov r14, STACK_CORRUPTED
xor r0, r0
xor r1, r1
mov r0, 0x1234
shl r0, 16
or r0, 0x5678
pop r1
beq r0, r1, .popcontinue
jmp done
.popcontinue:
mov r14, POP_UNEXPECTED_SP
xor r0, r0
mov r0, 0x80
mov r1, sp
beq r0, r1, .popfine
jmp done
.popfine:

xor r0, r1
xor r1, r1
mov r0, 0x01
mov r1, 0x02
mov r14, BLT_TOOK_WRONG_BRANCH
blt r1, r0, .bltcontinue
jmp done
.bltcontinue:
mov r14, BLT_TOOK_WRONG_BRANCH
blt r0, r1, done

mov r14, BEQ_TOOK_WRONG_BRANCH
beq r0, r1, done
mov r0, 0x1234
mov r1, 0x1234
mov r14, BEQ_TOOK_WRONG_BRANCH
beq r0, r1, .beqfine
jmp done
.beqfine:

mov r14, INCORRECT_ENDIANNESS
xor r0, r0
xor r1, r1
xor r2, r2
or r0, 0x12
shl r0, 8
or r0, 0x34
mov r1, 0x1234
beq r0, r1, .endcheckcontinue
jmp done
.endcheckcontinue:
mov [r2 + scratch], r0 ; this seemingly strange check is intended to check if endianness changes during memory read/write (mostly for debugging vm)
mov r0, [r2 + scratch]
beq r0, r1, .endiannessfine
jmp done
.endiannessfine:

; add new tests here hew new I/O devices
; MAKE SURE TO SET r14

xor r14, r14
mov r14, 0xffff
shl r14, 16
or r14, 0xffff
done:	; ON END:
	; r0  = 0xff00 (Stall Signal)
	; r14 = Error code
	mov r0, 0xff00 ; signal end with 0xff00 in r0 and stall
stall:
	jmp stall

scratch:

interrupt_handler:
	push r0
	push r1
	mov r1, sp
	mov r0, [r1 + 0x0003]
	xor r1, r1
	beq r0, r1, ack_pit
exit_int:
	xor r1, r1
	and r1, r13, 0xff
	beq r1, r0, was_int_expected
	or r14, SPURIOUS_INTERRUPT
	jmp exit_int.end
was_int_expected:
	xor r0, r0
	xor r1, r1
	shr r1, r13, 8
	and r1, 0xff
	mov r0, 0xff
	beq r1, r0, exit_int.end
	or r14, SPURIOUS_INTERRUPT
exit_int.end:
	pop r1
	pop r0
	iret
ack_pit:
	out r1, r1	; r1 happens to be zero, both need to be zero
	mov r15, 0xffff	; signal pit ack to loop (see loop:)
	jmp exit_int