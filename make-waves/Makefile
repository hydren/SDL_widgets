CC=g++ -O
CFLAGS=-g $$(sdl-config --cflags) -I.. \
  -Wuninitialized -Wshadow -Wall -Wno-non-virtual-dtor -Wno-multichar -Wno-delete-non-virtual-dtor
OBJ= make-waves.o dump-wave.o ../SDL_widgets.o

.SUFFIXES=

make-waves: $(OBJ)
	$(CC) $(OBJ) -o $@ $$(sdl-config --libs) -lSDL_gfx -lSDL_ttf

../SDL_widgets.o: ../SDL_widgets.cpp ../SDL_widgets.h ../sw-pixmaps.h
	make -C .. SDL_widgets.o

%.o: %.cpp
	$(CC) -c $< $(CFLAGS)

make-waves.o: ../SDL_widgets.h dump-wave.h templates.h filter-test.cpp 
dump-wave.o: dump-wave.h
