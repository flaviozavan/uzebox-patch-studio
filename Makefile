CXXFLAGS=-Wall -Wextra -O2 `wx-config --cflags`
CXXFLAGS+=`sdl2-config --cflags`
LDLIBS=`wx-config --libs` `sdl2-config --libs`
OBJECTS=upsgrid.o filereader.o patchdata.o structdata.o

ifneq (, $(findstring MINGW, $(shell uname)))
	LDLIBS+=-lSDL2_mixer
	CXXFLAGS+=-std=gnu++14
	OBJECTS+=windows.res
else
	LDLIBS+=`pkg-config --libs SDL2_mixer`
	CXXFLAGS+=`pkg-config --cflags SDL2_mixer`
	CXXFLAGS+=-std=c++14
endif


all: uzebox-patch-studio

uzebox-patch-studio: $(OBJECTS)

windows.res: windows.rc
	windres windows.rc -O coff -o windows.res

clean:
	rm -f uzebox-patch-studio $(OBJECTS)
