FB equ 0x1a80
FB_WIDTH_WORDS equ 40
FB_HEIGHT equ 240
FB_END equ 0x4000

genpixel:
	shr r13, r2, 2
	shr r14, r2, 3
	or r13, r14
	shl r14, r13, 4
	or r13, r14
	shl r14, r13, 4
	or r13, r14
	mov [r0], r13
	ret

extern main
main:
	mov r0, FB
	mov r5, 0 ; scanline
	mov r3, FB_HEIGHT
main.next_scanline:
	mov r1, FB_WIDTH_WORDS
	mov r2, 0 ; pixel
main.fill_scanline:
	call genpixel
	add r2, 1
	add r0, 1
	bneq r1, r2, main.fill_scanline
	add r5, 1
	bneq r5, r3, main.next_scanline
	ret