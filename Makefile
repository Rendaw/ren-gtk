NAME = gtk
OBJECTS = objects/gtkwrapper.o objects/gtkcairowrapper.o
COMPILE_EXTRA = `pkg-config --cflags gtk+-2.0`
LINK_EXTRA = -lren-general `pkg-config --libs gtk+-2.0`

include ../library-tools/Makefile.include

objects/gtkwrapper.o objects/gtkwrapper.debug.o: gtkwrapper.cxx gtkwrapper.h
	$(COMPILE) $@ gtkwrapper.cxx

objects/gtkcairowrapper.o objects/gtkcairowrapper.debug.o: gtkcairowrapper.cxx gtkcairowrapper.h
	$(COMPILE) $@ gtkcairowrapper.cxx
