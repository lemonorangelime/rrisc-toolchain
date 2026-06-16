str:
	db "hello!", 0x00, 0x00

buffer:
	dd 0x12345678, 0x87654321, 0x11aabb22, 0xee99ff00

empty:
	dd 0x00000000, 0x00000000, 0x00000000, 0x00000000

extern main
; stdlib functions
extern byteswap
extern strlen
extern memcpy32
extern memcpy
extern alpha_composite
main:
	mov r0, 0x1234
	shl r0, 16
	or r0, 0x5678
	call byteswap

	mov r8, r0
	mov r0, str
	call strlen

	mov r9, r0
	mov r0, empty
	mov r1, buffer
	mov r2, 4
	call memcpy32
	mov r4, [r0]
	mov r5, [r0 + 1]
	mov r6, [r0 + 2]
	mov r7, [r0 + 3]

	mov r0, empty
	mov r1, str
	mov r2, 1
	call memcpy
	mov r3, [r0]

	mov r0, 0xff00
	shl r0, 16
	or r0, 0x00ff
	xor r1, r1
	mov r1, 0x80ff
	shl r1, 16
	call alpha_composite

	ret