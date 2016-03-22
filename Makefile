CC=g++
CFLAGS=-g -O $$(sdl-config --cflags) \
  -Wall -Wuninitialized -Wshadow -Wno-non-virtual-dtor -Wno-delete-non-virtual-dtor -Wno-multichar
OBJ=testsw.o SDL_widgets.o

.SUFFIXES=
.PHONY: all hello make-waves bouncy-tune

#all: testsw
all: testsw hello make-waves bouncy-tune

testsw: $(OBJ)
	$(CC) $(OBJ) -o $@ $$(sdl-config --libs) -lSDL_gfx -lSDL_ttf

%.o: %.cpp
	$(CC) -c $< $(CFLAGS)

hello make-waves bouncy-tune:
	make -C $@

SDL_widgets.o: SDL_widgets.h sw-pixmaps.h config.h
testsw.o: SDL_widgets.h icon.xpm
