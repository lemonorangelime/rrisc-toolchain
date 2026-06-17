rrasm bios.asm -f elf -o bios.elf
rrasm biosjmp.asm -f elf -o biosjmp.elf
rrld bios.elf biosjmp.elf -f bin -o bios.bin
rrld bios.elf biosjmp.elf -f vmd -o bios.vmd
rrld bios.elf biosjmp.elf -f mi -o bios.mi
