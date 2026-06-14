PREFIX?=/usr/local/bin/

SRC:=rrasm
PATHNAME:=rrasm

default:
	@echo "Pick target 'install' or 'uninstall'"

install:
	cp ${SRC} ${PREFIX}/${PATHNAME}
	chmod 775 ${PREFIX}/${PATHNAME}

uninstall:
	rm ${PREFIX}/${PATHNAME}
