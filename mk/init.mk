# Shut up GNU make
.SILENT:

XGETTEXT_MSGIG_BUGS = "sharybin@nm.ru"
POMAKEFILEDEPS = "POTFILES.in"
OBJECTIVE_DIRECTORIES = 
OBJECTIVE_LIBS = 
OBJECTIVE_LIBS_NOINST = 
OBJECTIVE_BINS = 
OBJECTIVE_DATA = 
SUBDIRS = 
HEADERS = 
LIBADD = 
V = 0
VERBOSE ?= $(V)
VERBOSITY = 0
SHOW_CFLAGS ?= $(VERBOSE)

LIBDIR = $(libdir)
BINDIR = $(bindir)
INCLUDEDIR = $(pkgincludedir)

srcdir=$(top_builddir)/src

CFLAGS += -Wall -I$(srcdir) -I.
CXXFLAGS += -Wall -I$(srcdir) -I.
