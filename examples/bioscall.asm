BIOS_FN_TABLE equ 0x1600

call_bios:
	push r8
	ret
_start:
	mov r8, BIOS_FN_TABLE
	mov r8, [r8]
	mov r0, 0
	mov r1, 0
	mov r2, 'H'
	call call_bios
	mov r0, 8
	mov r2, 'e'
	call call_bios
	mov r0, 16
	mov r2, 'l'
	call call_bios
	mov r0, 24
	mov r2, 'l'
	call call_bios
	mov r0, 32
	mov r2, 'o'
	call call_bios
	mov r0, 40
	mov r2, '!'
	call call_bios
	mov r0, 48
	call call_bios
	ret
