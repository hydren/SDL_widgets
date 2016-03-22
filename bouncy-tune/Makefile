CC=g++
CFLAGS=-g $$(sdl-config --cflags) -std=c++0x \
  -O -Wuninitialized -Wshadow -Wall -Wno-non-virtual-dtor -Wno-delete-non-virtual-dtor -Wno-multichar

.SUFFIXES=

bouncy-tune: bouncy-tune.o dump-wave.o ../sdl-widgets.o
	$(CC) $@.o dump-wave.o ../sdl-widgets.o -o $@ $$(sdl-config --libs) -lSDL_gfx -lSDL_ttf

../sdl-widgets.o: ../sdl-widgets.cpp ../sdl-widgets.h ../sw-pixmaps.h
	make -C .. sdl-widgets.o

%.o: %.cpp
	$(CC) -c $< -I.. $(CFLAGS)

bouncy-tune.o: ../sdl-widgets.h dump-wave.h templates.h
dump-wave.o: dump-wave.h
