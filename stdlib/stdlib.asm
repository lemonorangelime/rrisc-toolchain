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

extern wordcpy
extern memcpy32
wordcpy: ; alias
memcpy32: ; memcpy32(dest, src, words) - copy N words from src to dest
	xor r15, r15
	beq r2, r15, memcpy32.exit
	push r2
	push r3

	xor r3, r3
	mov r13, r0
	mov r14, r1
memcpy32.copyloop:
	mov r15, [r14]
	mov [r13], r15
	add r13, 1
	add r14, 1
	sub r2, 1
	bgt r3, r2, memcpy32.copyloop

	pop r3
	pop r2
memcpy32.exit:
	ret

extern alpha_composite
alpha_composite: ; alpha_composite(bottom, top) - composite top pixel with bottom pixel
	push r2
	push r3
	push r4

	shr r13, r1, 24		; uint32_t alpha = top >> ALPHA_SHIFT;
	xor r14, r13, 0xff	; uint32_t inverted = CHANNEL_MASK - alpha;

	shr r15, r1, 16		; uint32_t top_r = (top >> RED_SHIFT) & CHANNEL_MASK;
	and r15, 0xff
	mul r15, r13		; [r15 = (alpha * top_r)]
	shr r4, r0, 16		; uint32_t bottom_r = (bottom >> RED_SHIFT) & CHANNEL_MASK;
	and r4, 0xff
	mul r4, r14		; [r4 = (inverted * bottom_r)]
	add r3, r5, r4		; tmp = (alpha * top_r) + (inverted * bottom_r) + 256;
	add r3, 256
	and r3, 0xff00		; output |= (tmp << 8) & RED_MASK;
	shl r2, r3, 8

	shr r15, r1, 8		; uint32_t top_g = (top >> GREEN_SHIFT) & CHANNEL_MASK;
	and r15, 0xff
	mul r15, r13		; [r15 = (alpha * top_g)]
	shr r4, r0, 8		; uint32_t bottom_g = (bottom >> GREEN_SHIFT) & CHANNEL_MASK;
	and r4, 0xff
	mul r4, r14		; [r4 = (inverted * bottom_g)]
	add r3, r5, r4		; tmp = (alpha * top_g) + (inverted * bottom_g) + 256;
	add r3, 256
	and r3, 0xff00		; ; output |= tmp & GREEN_MASK;
	or r2, r3

	and r15, r1, 0xff	; uint32_t top_b = (top >> BLUE_SHIFT) & CHANNEL_MASK;
	mul r15, r13		; [r15 = (alpha * top_b)]
	and r4, r0, 0xff	; uint32_t bottom_b = (bottom >> BLUE_SHIFT) & CHANNEL_MASK;
	mul r4, r14		; [r4 = (inverted * bottom_b)]
	add r3, r15, r4		; tmp = (alpha * top_b) + (inverted * bottom_b) + 256;
	add r3, 256
	shr r3, 8		; output |= tmp >> 8;
	or r2, r3

	mov r3, 0xff		; output |= ALPHA_MASK;
	shl r3, 24
	or r2, r3

	mov r0, r2

	pop r4
	pop r3
	pop r2
	ret

extern bytecpy
extern memcpy
bytecpy: ; alias
memcpy: ; memcpy(dest, src, words) - copy N bytes(!) from src to dest
	push r2 ; enter

	push r2 ; copy words
	shr r2, 2
	call memcpy32
	add r13, r0, r2 ; convinently r0 + (r2 >> 2) will skip the pre-copied words
	add r14, r1, r2 ; same
	pop r2

	xor r15, r15 ; early exit if no remaining bytes
	and r2, 0b11
	beq r2, r15, memcpy.exit

	push r3
	mov r15, 1 ; generate source mask in r2
	mov r3, 32
	shl r2, 3
	sub r3, r3, r2
	shl r2, r15, r2
	sub r2, 1
	shl r2, r3

	; r3 = destination mask
	sub r3, r15, 2 ; this works because we set r15 to 1 up there
	xor r3, r2

	mov r15, [r1] ; copy remaining bytes
	and r15, r2
	mov r2, [r0]
	and r2, r3
	or r15, r2
	mov [r0], r15

	pop r3

memcpy.exit:
	pop r2
	ret

extern strlen
strlen: ; strlen(s) - return byte(!) length of s
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

	xor r3, r3
	sub r3, 8
	mov r2, 24
strlen.shiftloop:
	shr r1, r14, r2
	and r1, 0xff
	beq r1, r15, strlen.exit
	sub r2, 8
	add r0, 1
	bneq r3, r2, strlen.shiftloop
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