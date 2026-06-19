#define my_macro_func(op_a, op_b) \
	mov r0, op_b \
	mov r1, op_a \
	xor op_a, op_b \
	add op_a, a

#define a 0x1234
#ifdef a
mov r0, 0xffff
mov r1, 0x5555
#endif

#ifdef b
mov r15, 0x0000
mov r14, 0x1111
#else
mov r3, 0x2222
mov r4, 0x3333
#endif

#ifdef b
mov r5, 0x1212
#elifdef a
mov r5, 0x3434
#else

#ifdef b
mov r6, 0x9999
#elifndef c
mov r6, 0x8888
#endif

#ifndef c
mov r7, 0x7777
#endif

mov r0, a
my_macro_func r0, r1
