#define VGA_CTRL_BASE     0x1a60
#define VGA_CTRL_RESET    0x000000ff
#define VGA_CTRL_ADVANCED_MODE 0xc0000000
#define VGA_CTRL_COMPAT_MODE   0x80000000

#define VGA_ADVANCED_VRAM_BASE  0x1a80

#define VGA_ADVANCED_REG_CTRL   0x1a60
#define VGA_ADVANCED_REG_WIDTH  0x1a61
#define VGA_ADVANCED_REG_HEIGHT 0x1a62

#define VGA_ADVANCED_CTRL_MODESET 0x00000000

_start:
	xor r15, r15
	mov r14, 1
	shl r14, 16

	mov r0, VGA_CTRL_ADVANCED_MODE
	mov [r15 + VGA_CTRL_BASE], r0
	mov r0, [r15 + VGA_CTRL_BASE] ; test if supported
	shr r0, 24
	and r0, 1
	beq r0, r15, .exit

	; advanced mode graphics, no bios support here...
	mov r0, 640
	mov r1, 480
	mov r2, VGA_ADVANCED_CTRL_MODESET
	mov [r15 + VGA_ADVANCED_REG_WIDTH], r0
	mov [r15 + VGA_ADVANCED_REG_HEIGHT], r1
	mov [r15 + VGA_ADVANCED_REG_CTRL], r2

	shl r0, 1
	shl r1, 1
	xor r6, r6
	xor r7, r7
	mov r13, r14

.divloop1:
	add r6, 1
	sub r13, r0
	bgt r0, r13, .divloop1

.divloop2:
	add r7, 1
	sub r14, r1
	bgt r1, r14, .divloop2

	shr r0, 1
	shr r1, 1
	shl r6, 1
	shl r7, 1

	mov r13, 0xff
	mull r6, r13
	mull r7, r13
	shl r13, 24

	xor r8, r8
	xor r9, r9

	mov r14, 0xff0000
	xor r11, r11
	mov r2, VGA_ADVANCED_VRAM_BASE
.scanloop:
	xor r12, r12
	mull r8, r7, r11
	and r8, r14
.pixloop:
	mull r9, r6, r12
	shr r9, 16
	or r9, r8
	or r9, r13
	mov [r2], r9
	add r2, 1
	add r12, 1
	bneq r0, r12, .pixloop
	add r11, 1
	bneq r1, r11, .scanloop

.exit:
	; return to bios
	mov r0, VGA_CTRL_COMPAT_MODE
	mov r1, VGA_CTRL_RESET
	mov [r15 + VGA_CTRL_BASE], r0
	mov [r15 + VGA_CTRL_BASE], r1
	ret
