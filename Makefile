CXXFLAGS=-Wall -Wextra -O2 `wx-config --cflags`
CXXFLAGS+=`sdl2-config --cflags`
LDLIBS=`wx-config --libs` `sdl2-config --libs`

ifneq (, $(findstring MINGW, $(shell uname)))
  LDLIBS+=-lSDL2_mixer
  CXXFLAGS+=-std=gnu++14
else
  LDLIBS+=`pkg-config --libs SDL2_mixer`
  CXXFLAGS+=`pkg-config --cflags SDL2_mixer`
  CXXFLAGS+=-std=c++14
endif

all: uzebox-patch-studio

uzebox-patch-studio: grid.o filereader.o patchdata.o structdata.o

clean:
	rm -f uzebox-patch-studio grid.o filereader.o patchdata.o structdata.o
