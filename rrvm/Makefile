PREFIX?=/usr/local/bin/

CC:=gcc
CCFLAGS:=-Iinclude -Wl,"-Tlink.ld"

SRC:=$(shell find src/ -name '*.c')
BIN:=rrvm
PATHNAME:=rrvm

default: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CCFLAGS) $^ -o $@
clean:
	rm $(BIN)

install: $(BIN)
	cp ${BIN} ${PREFIX}/${PATHNAME}
	chmod 775 ${PREFIX}/${PATHNAME}

uninstall:
	rm ${PREFIX}/${PATHNAME}
