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

xor r0, r0 ; ensure cleanlyness of registers we will use
xor r1, r1
xor r13, r13
xor r14, r14
xor r15, r15

xor r0, r0
xor r1, r1
mov r14, JAL_BRANCH_NEVER_TAKEN ; we are about to check
jal r0, skip
jal_expected:
	jmp done
skip:
	add r13, 1
	mov r14, JAL_UNEXPECTED_PC ; we are about to check
	mov r1, jal_expected
	beq r0, r1, .jalfine
	jmp done
.jalfine:
add r13, 1

xor r0, r1
xor r1, r1
mov r0, 0x01
mov r1, 0x02
mov r14, BLT_TOOK_WRONG_BRANCH ; we are about to check
blt r1, r0, .bltcontinue
jmp done
.bltcontinue:
add r13, 1
blt r0, r1, done
add r13, 1
mov r14, BEQ_TOOK_WRONG_BRANCH ; we are about to check
beq r0, r1, done
add r13, 1
mov r0, 0x1234
mov r1, 0x1234
beq r0, r1, .beqfine
jmp done
.beqfine:
add r13, 1

xor r0, r0
xor r1, r1
xor r2, r2
or r0, 0x12
shl r0, 8
or r0, 0x34
mov r1, 0x1234
mov r14, INCORRECT_ENDIANNESS ; we are about to check
beq r0, r1, .endcheckcontinue
jmp done
.endcheckcontinue:
add r13, 1
mov [r2 + scratch], r0 ; this seemingly strange check is intended to check if endianness changes during memory read/write (mostly for debugging vm)
mov r0, [r2 + scratch]
beq r0, r1, .endiannessfine
jmp done
.endiannessfine:
add r13, 1

; add new tests here hew new I/O devices
; MAKE SURE TO SET r14

okay:
	xor r14, r14
	mov r14, 0xffff
	shl r14, 16
	or r14, 0xffff
	or r13, r14
done:	; ON END
	;
	; r0 = 0xff00 (Stall Signal)
	; r13 = Error Index
	; r14 = Error Code
	mov r0, 0xff00 ; signal end with 0xff00 in r0 and stall
stall:
	jmp stall


scratch:
