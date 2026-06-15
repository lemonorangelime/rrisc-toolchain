; stdlib functions
extern byteswap
extern strlen

str:
	db "hello!", 0x00, 0x00

extern main
main:
	mov r0, 0x1234
	shl r0, 16
	or r0, 0x5678
	call byteswap
	mov r1, r0
	mov r0, str
	call strlen
	ret