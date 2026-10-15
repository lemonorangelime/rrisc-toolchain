org 0x00000000

extern externsym
_start:
	mov r0, externsym	; generate an absolute reference
	mov r0, [r0]
stall:
	jmp stall
	jmp externsym	; generate a relative reference