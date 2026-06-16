PREFIX?=/usr/local/bin/

SRC:=rrld
PATHNAME:=rrld

default:
	@echo "Pick target 'install' or 'uninstall'"

install:
	cp ${SRC} ${PREFIX}/${PATHNAME}
	chmod 775 ${PREFIX}/${PATHNAME}

uninstall:
	rm ${PREFIX}/${PATHNAME}
