CXXFLAGS=-Wall -Wextra -O2 -std=c++14 `wx-config --cflags`
LDLIBS=`wx-config --libs`

all: uzebox-patch-studio

uzebox-patch-studio: grid.o input.o

clean:
	rm -f uzebox-patch-studio grid.o input.o
