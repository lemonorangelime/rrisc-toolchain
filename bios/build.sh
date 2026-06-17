../rrasm/rrasm bios.asm -r -f elf -o bios.elf
rrasm biosjmp.asm -r -f elf -o biosjmp.elf
rrld bios.elf biosjmp.elf -f bin -o bios.bin
rrld bios.elf biosjmp.elf -f vmd -o bios.vmd
rrld bios.elf biosjmp.elf -f mi -o bios.mi
