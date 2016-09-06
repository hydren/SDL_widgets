CC=g++
CFLAGS=-g $$(sdl-config --cflags) \
  -O -Wuninitialized -Wshadow -Wall -Wno-non-virtual-dtor -Wno-delete-non-virtual-dtor -Wno-multichar

.SUFFIXES=

bouncy-tune: bouncy-tune.o dump-wave.o ../SDL_widgets.o
	$(CC) $@.o dump-wave.o ../SDL_widgets.o -o $@ $$(sdl-config --libs) -lSDL_gfx -lSDL_ttf

../SDL_widgets.o: ../SDL_widgets.cpp ../SDL_widgets.h ../sw-pixmaps.h
	make -C .. SDL_widgets.o

%.o: %.cpp
	$(CC) -c $< -I.. $(CFLAGS)

bouncy-tune.o: ../SDL_widgets.h dump-wave.h templates.h
dump-wave.o: dump-wave.h
