ASM:=rrasm
ASMFLAGS:=-r -f elf

SRC:=stdlib.asm
OUTPUT:=stdlib.o

TESTBIN := test.vmd

default: $(TESTBIN) $(OUTPUT)

clean:
	rm $(OUTPUT) $(TESTBIN) test.o

test.o: test.asm
	rrasm $^ -r -f elf -o $@

$(TESTBIN): test.o $(OUTPUT)
	rrld $(OUTPUT) test.o -r -f vmd -o $(TESTBIN)

$(OUTPUT): $(SRC)
	$(ASM) $^ $(ASMFLAGS) -o $@
