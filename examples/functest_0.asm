org 0x0000

nop ; entry point cant be 0

extern functest_helper
extern _start
_start:
	mov r15, 0x80
	mov sp, r15
	call func
	call func
	call func

	call functest_helper

stall:
	jmp stall

func:
	add r0, 1
	ret