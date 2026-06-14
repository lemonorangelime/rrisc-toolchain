; test various I/O devices and instructions to verify correctness and locate hardware bugs

JAL_UNEXPECTED_PC equ 0b0000000000000010 ; jal put unexpected value into register
JAL_BRANCH_NEVER_TAKEN equ 0b0000000000000100 ; jal did not jump
BEQ_TOOK_WRONG_BRANCH equ 0b0000000001000000 ; beq took the wrong branch
BLT_TOOK_WRONG_BRANCH equ 0b0000000010000000 ; blt took the wrong branch
INCORRECT_ENDIANNESS equ 0b0000000100000000 ; nebulous endianness problem

xor r0, r0 ; ensure cleanlyness of registers we will use
xor r1, r1
xor r14, r14
xor r15, r15

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
