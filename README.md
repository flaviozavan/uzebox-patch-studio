Uzebox Patch Studio
===================

A cross-platform graphical editor for Uzebox sound patches.

----------

Compiling on Linux
-------------

1. Install wxWidgets, SDL2 and SDL2_mixer
**Arch Linux:** pacman -S wxgtk sdl2 sdl2_mixer
**Ubuntu:** apt-get install libwxgtk3.0-dev libsdl2-dev libsdl2-mixer-dev
2. cd to Uzebox Patch Studio's directory
3. make

Compiling on Windows
-------------

1. Install mingw/msys. Make sure to install the compiler and other developments
tools
2. Download SDL2's source code
3. Extract it to your msys home
4. Open msys, go to SDL2's source code directory
5. mkdir release && cd release && ../configure && make && make install
6. Download SDL2_mixer's source code.
7. Extract it to your msys home
8. Open msys, go to SDL2_mixer's source code directory
9. mkdir release && cd release && ../configure && make && make install
10. Download wxWidgets' (3.0.x) source code
11. Extract it to your msys home
12. Open msys, go to wxWdigets' source code directory
13. mkdir release && cd release && ../configure && make MONOLITHIC=1 SHARED=1
UNICODE=1 BUILD=release && make install
14. cd to uzebox-patch-studio's directory
15. make
16. Try running the exe outside of msys
17. You are done if there are no missing dlls
18. Find the missing dll in mingw
19. Copy it to the same directory as the exe
20. Return to step 16

Compiling on Mac OS
-------------

1. Install wxmac, SDL2 and SDL2 Mixer using homebrew, ports or similar
2. cd to Uzebox Patch Studio's directory
3. make
