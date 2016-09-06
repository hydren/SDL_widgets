CC=g++
CFLAGS=-g $(shell sdl-config --cflags)
LFLAGS=$(shell sdl-config --libs) -lSDL_gfx -lSDL_ttf

.SUFFIXES=

hello: hello.o ../SDL_widgets.o
	$(CC) $@.o ../SDL_widgets.o -o $@ $(LFLAGS)

%.o: %.cpp
	$(CC) -c $< -I.. $(CFLAGS)

../SDL_widgets.o: ../SDL_widgets.cpp ../SDL_widgets.h ../sw-pixmaps.h
	make -C .. SDL_widgets.o

hello.o: ../SDL_widgets.h
