PREFIX?=/usr/local/bin/

CC:=gcc
CCFLAGS:=-Iinclude -Wl,"-Tlink.ld" -lSDL2 -Wno-shift-overflow

SRC:=$(shell find src/ -name '*.c')
BIN:=rrvm
PATHNAME:=rrvm

default: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CCFLAGS) $^ -o $@
clean:
	rm -f $(BIN)

install: $(BIN)
	cp ${BIN} ${PREFIX}/${PATHNAME}
	chmod 775 ${PREFIX}/${PATHNAME}

uninstall:
	rm ${PREFIX}/${PATHNAME}
