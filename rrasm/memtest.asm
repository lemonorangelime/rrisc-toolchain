org 0x0000

mov r0, 0x0010
mov r1, 0xBABE
mov r2, 0xABBA
nop

mov [r0], r1
mov [r0 + 0x0001], r2

nop
mov r3, [r0]
mov r3, [r0 + 0x0001]