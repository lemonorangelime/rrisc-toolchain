MAX equ 0xffff
MIN equ 1

mov r15, 1
mov r0, MIN
mov r1, MAX

ascent:
shl r15, 1
blt r1, r15, ascent

descent:
shr r15, 1
beq r0, r15, ascent
jmp descent
