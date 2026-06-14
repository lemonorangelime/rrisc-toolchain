; test various I/O devices and instructions to verify correctness and locate hardware bugs

PIT_NEVER_INTERRUPTED equ 0b0000000000000001 ; programed pit to interrupt, never did (in reasonable amount of timea)
JAL_UNEXPECTED_PC equ 0b0000000000000010 ; jal put unexpected value into register
JAL_BRANCH_NEVER_TAKEN equ 0b0000000000000100 ; jal did not jump
PUSH_UNEXPECTED_SP equ 0b0000000000001000 ; unexpected stack pointer after push
POP_UNEXPECTED_SP equ 0b0000000000010000 ; unexpected stack pointer after pop
STACK_CORRUPTED equ 0b0000000000100000 ; value on stack did not match value pushed
BEQ_TOOK_WRONG_BRANCH equ 0b0000000001000000 ; beq took the wrong branch
BLT_TOOK_WRONG_BRANCH equ 0b0000000010000000 ; blt took the wrong branch
INCORRECT_ENDIANNESS equ 0b0000000100000000 ; nebulous endianness problem
SPURIOUS_INTERRUPT equ 0b1000000000000000 ; received interrupt when none was expected

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

mov r0, PIT_POLL_CYCLES
xor r1, r1
loop:
	add r1, 1
	blt r0, r1, loop
xor r0, r0
mov r0, 0xffff
beq r0, r15, .continue ; i want bne r0, 15, pit_error because this should be negated, cant, must do this
or r14, PIT_NEVER_INTERRUPTED
.continue:

xor r0, r0
xor r1, r1
call r0, skip
jal_expected:
	or r14, JAL_BRANCH_NEVER_TAKEN
skip:
	mov r1, jal_expected
	beq r0, r1, .jalfine
	or r14, JAL_UNEXPECTED_PC
.jalfine:

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
or r14, PUSH_UNEXPECTED_SP
.pushfine:

xor r0, r0
xor r1, r1
mov r0, 0x1234
shl r0, 16
or r0, 0x5678
pop r1
beq r0, r1, .popcontinue
or r14, STACK_CORRUPTED
.popcontinue:
xor r0, r0
mov r0, 0x80
mov r1, sp
beq r0, r1, .popfine
or r14, POP_UNEXPECTED_SP
.popfine:

xor r0, r1
xor r1, r1
mov r0, 0x01
mov r1, 0x02
blt r1, r0, .bltcontinue
or r14, BLT_TOOK_WRONG_BRANCH
.bltcontinue:
blt r0, r1, .bltwrong
jmp .bltfine
.bltwrong:
or r14, BLT_TOOK_WRONG_BRANCH
.bltfine:

beq r0, r1, .beq_wrong
jmp .beq_continue
.beq_wrong:
or r14, BEQ_TOOK_WRONG_BRANCH
.beq_continue:
mov r0, 0x1234
mov r1, 0x1234
beq r0, r1, .beqfine
or r14, BEQ_TOOK_WRONG_BRANCH
.beqfine:

xor r0, r0
xor r1, r1
xor r2, r2
or r0, 0x12
shl r0, 8
or r0, 0x34
mov r1, 0x1234
beq r0, r1, .endcheckcontinue
or r14, INCORRECT_ENDIANNESS
.endcheckcontinue:
mov [r2 + scratch], r0 ; this seemingly strange check is intended to check if endianness changes during memory read/write (mostly for debugging vm)
mov r0, [r2 + scratch]
beq r0, r1, .endiannessfine
or r14, INCORRECT_ENDIANNESS
.endiannessfine:

; add new tests here hew new I/O devices

done:	; ON END:
	; r0  = 0xff00 (Stall Signal)
	; r14 = Error Bitmap
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