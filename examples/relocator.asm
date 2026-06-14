org 0x0000

RELOCATION_ADDRESS equ 0x00A0

mov r0, payload
mov r1, payload_end
mov r3, RELOCATION_ADDRESS
cpyloop:
mov r2, [r0]
mov [r3], r2
add r0, 1
add r3, 1
blt r1, r0, cpyloop

jmpabs RELOCATION_ADDRESS

payload:
xor r0, r0
add r1, r0, 0x0000
add r2, r0, 0x0001
loop:
add r3, r1, r2
add r1, r2, r0
add r2, r3, r0
jmp loop
payload_end:
