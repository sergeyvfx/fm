# Tests build part of makefile

.SILENT:

top_builddir = ..

include ${top_builddir}/mk/rules.mk
include ${top_builddir}/mk/init.mk

SUBDIRS = vfs

OBJECTS = ${SOURCES:.c=.o}

include ${top_builddir}/mk/objective.mk

# Run-tests part of makefile

all:
	./test-all.sh || exit 1

clean-tests:
	rm -f .tests-passed > /dev/null 2>&1

clean: clean-tests
