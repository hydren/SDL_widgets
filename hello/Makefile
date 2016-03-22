CC=g++
CFLAGS=-g $(shell sdl-config --cflags)
LFLAGS=$(shell sdl-config --libs) -lSDL_gfx -lSDL_ttf

.SUFFIXES=

hello: hello.o ../sdl-widgets.o
	$(CC) $@.o ../sdl-widgets.o -o $@ $(LFLAGS)

%.o: %.cpp
	$(CC) -c $< -I.. $(CFLAGS)

../sdl-widgets.o: ../sdl-widgets.cpp ../sdl-widgets.h ../sw-pixmaps.h
	make -C .. sdl-widgets.o

hello.o: ../sdl-widgets.h
