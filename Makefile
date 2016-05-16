CXXFLAGS=-Wall -Wextra -O2 -std=c++14 `wx-config --cflags`
CXXFLAGS+=`sdl2-config --cflags` `pkg-config --cflags SDL2_mixer`
LDLIBS=`wx-config --libs` `sdl2-config --libs` `pkg-config --libs SDL2_mixer`

all: uzebox-patch-studio

uzebox-patch-studio: grid.o input.o generate.o

clean:
	rm -f uzebox-patch-studio grid.o input.o generate.o
