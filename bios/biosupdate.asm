org 0x0000

#define BIOS_FILE "build/bios_raw.bin"

_start:
	mov r0, payload
	mov r1, payload_end
	mov r3, 0x1600
_start.updatebios:
	mov r4, [r0]
	mov [r3], r4
	add r0, 1
	add r3, 1
	blt r1, r0, _start.updatebios

	mov r3, 0x1600
	mov r4, [r3 + 6]
	push r4
	ret

payload:
incbin BIOS_FILE
payload_end: