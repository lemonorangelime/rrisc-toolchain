PREFIX?=/usr/local/bin/
SHARE_DIR?=/usr/local/share/rrasm/

PROGRAM:=rrasm
SHARED:=rrasm.py arch
PATHNAME:=rrasm

default:
	@echo "Pick target 'install' or 'uninstall'"

mkdir:
	mkdir -p ${SHARE_DIR}

install: mkdir
	cp ${PROGRAM} ${PREFIX}/${PATHNAME}
	chmod 775 ${PREFIX}/${PATHNAME}

	cp -r ${SHARED} ${SHARE_DIR}

uninstall:
	rm ${PREFIX}/${PATHNAME} ${SHARE_DIR}
