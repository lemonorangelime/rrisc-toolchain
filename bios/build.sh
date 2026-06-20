link() {
	rrld build/${1} build/biosjmp.elf -f bin -o build/${2}.bin -w unknown-vmembase,large-bin
	rrld build/${1} build/biosjmp.elf -f vmd -o build/${2}.vmd -w unknown-vmembase,large-bin
	rrld build/${1} build/biosjmp.elf -f mi -o build/${2}.mi -w unknown-vmembase,large-bin
}

mkdir -p build/
rrasm bios.asm -f elf -o build/bios.elf -w no-entry
rrasm bios.asm -d NO_SUPPORT_UNICODE -f elf -o build/bios_ascii.elf -w no-entry
rrasm bios.asm -d VERY_SMALL -f elf -o build/bios_small.elf -w no-entry
rrasm bios.asm -d MULLESS -f elf -o build/bios_mulless.elf -w no-entry

rrasm bios.asm -f bin -o build/bios_raw.bin -w no-entry
rrasm bios.asm -d NO_SUPPORT_UNICODE -f bin -o build/bios_ascii_raw.bin -w no-entry
rrasm bios.asm -d VERY_SMALL -f bin -o build/bios_small_raw.bin -w no-entry
rrasm bios.asm -d MULLESS -f bin -o build/bios_mulless_raw.bin -w no-entry

rrasm biosjmp.asm -f elf -o build/biosjmp.elf
link bios.elf bios
link bios_ascii.elf bios_ascii
link bios_small.elf bios_small
link bios_mulless.elf bios_mulless

../rrasm/rrasm biosupdate.asm -f elf -o build/biosupdate.elf
../rrasm/rrasm biosupdate.asm -f roadrun -o build/biosupdate.roadrun
../rrasm/rrasm biosupdate.asm -f vmd -o build/biosupdate.vmd
../rrasm/rrasm biosupdate.asm -f bin -o build/biosupdate.bin
../rrasm/rrasm biosupdate.asm -f mi -o build/biosupdate.mi
