include RMCIOS-build-scripts/utilities.mk

SOURCES:=*.c
FILENAME:=std-module
GCC?=${TOOL_PREFIX}gcc
DLLTOOL?=${TOOL_PREFIX}dlltool
MAKE?=make
INSTALLDIR:=..${/}..
export

compile:
	$(MAKE) -f RMCIOS-build-scripts${/}module_dll.mk compile TOOL_PREFIX=${TOOL_PREFIX}

install:
	-${MKDIR} "${INSTALLDIR}${/}modules"
	${COPY} python-module.dll ${INSTALLDIR}${/}modules
	${COPY} *.py ${INSTALLDIR}${/}

