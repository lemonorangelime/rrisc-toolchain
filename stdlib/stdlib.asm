jmp _start

extern byteswap
byteswap:
	mov r15, r0
	mov r14, 0xff
	xor r0, r0

	shr r13, r15, 24
	and r13, r14
	or r0, r13

	shr r13, r15, 8
	shl r14, 8
	and r13, r14
	or r0, r13

	shl r13, r15, 8
	shl r14, 8
	and r13, r14
	or r0, r13

	shl r13, r15, 24
	shl r14, 8
	and r13, r14
	or r0, r13
	ret

extern strlen
strlen:
	push r1
	push r2
	push r3
	mov r13, r0
	xor r0, r0
	xor r15, r15

	; count characters in word
	; r0 = count, r1 = byte, r2 = byte offset, r13 = addr, r15 = zero
strlen.readloop:
	mov r14, [r13]

	; our processor is big endian...
	push r0
	push r13
	push r15
	mov r0, r14
	call byteswap
	mov r14, r0
	pop r15
	pop r13
	pop r0

	xor r3, r3
	mov r3, 32
	xor r2, r2
strlen.shiftloop:
	and r1, r14, 0xff
	beq r1, r15, strlen.exit
	shr r14, 8
	add r2, 8
	add r0, 1
	blt r3, r2, strlen.shiftloop
	add r13, 1
	jmp strlen.readloop

strlen.exit:
	pop r3
	pop r2
	pop r1
	ret

extern main
_start:
	; setup a reasonable stack
	mov r0, 0x0100
	mov sp, r0
	xor r0, r0

	call main
loop:
	hlt
	jmp loop