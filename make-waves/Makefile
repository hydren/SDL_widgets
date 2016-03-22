CC=g++ -O
CFLAGS=-g $$(sdl-config --cflags) -I.. \
  -Wuninitialized -Wshadow -Wall -Wno-non-virtual-dtor -Wno-multichar -Wno-delete-non-virtual-dtor
OBJ= make-waves.o dump-wave.o ../sdl-widgets.o

.SUFFIXES=

make-waves: $(OBJ)
	$(CC) $(OBJ) -o $@ $$(sdl-config --libs) -lSDL_gfx -lSDL_ttf

../sdl-widgets.o: ../sdl-widgets.cpp ../sdl-widgets.h ../sw-pixmaps.h
	make -C .. sdl-widgets.o

%.o: %.cpp
	$(CC) -c $< $(CFLAGS)

make-waves.o: ../sdl-widgets.h dump-wave.h templates.h filter-test.cpp 
dump-wave.o: dump-wave.h
