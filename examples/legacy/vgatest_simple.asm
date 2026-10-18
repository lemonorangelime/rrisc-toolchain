FB equ 0x1a80
FB_END equ 0x4000

#ifndef IMAGE_FILE
#define IMAGE_FILE "output.raw"
#endif

_start:
	mov r0, FB
	mov r1, FB_END
	mov r2, payload
_start.loop:
	mov r3, [r2]
	mov [r0], r3
	add r0, 1
	add r2, 1
	bneq r1, r0, _start.loop
	ret

payload:
	incbin IMAGE_FILE
payload_raw:
