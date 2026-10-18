#define BIOS_FN_TABLE 0x1600

#define BACKGROUND_COLOUR 0x00800080
#define VGA_PALETTE_BASE 0x1a70

hello_world: db "Hello World!!!!", 0x00
hello_world32: dd "Hey!!! Γεία Σoυ!!" ; γεία σου !!! :D
hello_world32_size equ 17

extern _start
_start:
	xor r15, r15
        mov r0, BACKGROUND_COLOUR ; assign colour 1 to magenta (legacy 16 colour mode)
        mov [r15 + VGA_PALETTE_BASE + 1], r0

	mov r9, BIOS_FN_TABLE
	mov r8, [r9 + 3] ; BIOS function 3, fill_screen()
	mov r0, 1 ; colour 1 (16 colour mode)
	call r8

	mov r8, [r9 + 4] ; BIOS function 4, print_str()
	mov r0, 0 ; X
	mov r1, 0 ; Y
	mov r2, hello_world ; string ptr
	call r8

	mov r8, [r9 + 5] ; BIOS function 5, print_str32()
	mov r0, 0 ; X
	mov r1, 8 ; Y
	mov r2, hello_world32 ; string32 ptr
	mov r3, hello_world32_size ; character length
	call r8

	; stall long engugh to appear on screen
	mov r0, 500000
	xor r1, r1
.stall: sub r0, 1
	bneq r0, r1, .stall

	ret
