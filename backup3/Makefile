OUTPUT = ../librengtk.a
COMPILEEXTRAS = -pedantic -Wno-long-long -Wall -ansi `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gtkglext-1.0`
EXTRAS = -g -DDEBUG

COMPILE = g++ $(COMPILEEXTRAS) $(CFLAGS) $(EXTRAS) -c -o

OBJECTS = objects/gtkwrapper.o objects/gtkcairowrapper.o \
	#objects/gtkglextwrapper.o

all: $(OBJECTS)
	ar r $(OUTPUT) $(OBJECTS)

release:
	make EXTRAS="-O2"

clean:
	rm -f $(OUTPUT)
	rm -f objects/*

objects/gtkwrapper.o: gtkwrapper.cxx gtkwrapper.h
	$(COMPILE) $@ gtkwrapper.cxx

objects/gtkglextwrapper.o: gtkglextwrapper.cxx gtkglextwrapper.h
	$(COMPILE) $@ gtkglextwrapper.cxx

objects/gtkcairowrapper.o: gtkcairowrapper.cxx gtkcairowrapper.h
	$(COMPILE) $@ gtkcairowrapper.cxx
