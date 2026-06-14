_start:
	xor r0, r0
	xor r1, r1
	or r0, 0x0012
	or r1, 0x0034
	add r0, r1

	STATIC equ 0x1234	; static symbol
	add r0, STATIC

	jmp symbol	; reference future symbol
symbol:

	mov [r1 + STATIC], r0

	xor r2, r2
	xor r3, r3
	add r3, 8
loop:
	add r0, 1
	add r2, 1
	blt r3, r2, loop

	hlt