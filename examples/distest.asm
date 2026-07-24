jrel relative ; force relative
relative:
jmpabs 0x1234 ; force absolute
nop
add r1, r2, r3
mov r4, sp
mov sp, r5
mov [r6], r7
mov r8, [r9]
mov [r10 + 0xb00b], r11
mov r12, [r13 + 0x0bb0]
