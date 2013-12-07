#CXX=clang++
#CXX=~/src/emscripten/em++
CXXOPTFLAGS=-Wall -Os -DNDEBUG -std=c++11
CXXDEBUGFLAGS=-Wall -O0 -g -std=c++11
CXXFLAGS=$(CXXOPTFLAGS) -I./ `sdl-config --cflags`
LIBS=`sdl-config --libs` -lSDL_image
OBJS=con.o game.o mini.q.o ogl.o renderer.o script.o shaders.o sys.o text.o enet.o
HEADERS=con.hpp game.hpp ogl.hpp script.hpp shaders.hpp sys.hpp text.hpp
all: mini.q compress_chars

SHADERS=data/shaders/fixed_vp.glsl\
        data/shaders/fixed_fp.glsl\
        data/shaders/font_fp.glsl\
        data/shaders/dfrm_fp.glsl

## build embedded shader source
shaders.cpp: $(SHADERS)
	./scripts/stringify_all_shaders.sh shaders.cpp shaders.hpp

enet.o:\
  enet/callbacks.h\
  enet/enet.h\
  enet/list.h\
  enet/memory.h\
  enet/protocol.h\
  enet/time.h\
  enet/types.h\
  enet/unix.h\
  enet/utility.h\
  enet/win32.h\
  enet/callbacks.c\
  enet/host.c\
  enet/list.c\
  enet/memory.c\
  enet/packet.c\
  enet/peer.c\
  enet/protocol.c\
  enet/unix.c\
  enet/win32.c

blob.o: blob.cpp $(HEADERS)
con.o: con.cpp $(HEADERS)
game.o: game.cpp $(HEADERS)
mini.q.o: mini.q.cpp $(HEADERS)
ogl.o: ogl.cpp $(HEADERS)
shaders.o: shaders.cpp $(HEADERS)
script.o: script.cpp $(HEADERS)
sys.o: sys.cpp $(HEADERS)
text.o: text.cpp font.hxx $(HEADERS)

## build font header
#font.hxx: compress_chars data/font8x8.png
#	./compress_chars data/font8x8.png 8 8 > font.hxx
font.hxx: compress_chars data/font8x16.png
	./compress_chars data/font8x16.png 8 16 > font.hxx

mini.q: $(OBJS) $(ENET_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(OBJS) $(ENET_OBJS) $(LIBS)

compress_chars: compress_chars.o
	$(CXX) $(CXXFLAGS) -o compress_chars compress_chars.o $(LIBS)

clean:
	-rm -f $(OBJS) mini.q compress_chars compress_chars.o


