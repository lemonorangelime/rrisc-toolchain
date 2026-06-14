mov r0, 0x0080
mov sp, r0
mov r1, sp
; expected: sp = 0x0080, r1 = 0x0080

nop
mov r2, 0x1234
mov r3, 0x4321
push r2
push r3
xor r2, r2
xor r3, r3
pop r2
pop r3
; expected: r2 = 0x4321, r3 = 0x1234 (swapped)

stall:
	jmp stall