db future, 0x00, 0x00, 0x00
dw future, 0x0000
dd future
dq future
future:

addi r0, 1
mov r0, [r0 + 't']
db "test", "γεία"
mov r0, "hi"
mov [r0 + "hi"], r0
mov r0, [r0 + "hi"]
mov r0, [r0 + 'a']
mov r0, [r0 + 0x12]
mov r1, r0
mov r14, r4