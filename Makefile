CXX=clang++
CXXOPTFLAGS=-Wall -O2 -DMEMORY_DEBUGGER -g
CXXDEBUGFLAGS=-Wall -O0 -DMEMORY_DEBUGGER -g
#CXX=~/src/emscripten/em++
#CXXOPTFLAGS=-Wall -DMEMORY_DEBUGGER -pg -O3 -DNDEBUG -std=c++11
#CXXDEBUGFLAGS=-Wall -DMEMORY_DEBUGGER -O0 -g -std=c++11

CXXFLAGS=$(CXXDEBUGFLAGS) -std=c++11 -Wno-invalid-offsetof -I./ `sdl-config --cflags`
#CXXFLAGS=$(CXXOPTFLAGS) -std=c++11 -Wno-invalid-offsetof -I./ `sdl-config --cflags` -fsanitize=address
LIBS=`sdl-config --libs` -g -lGL -lSDL_image -lSDL_mixer
OBJS=\
  con.o\
  csg.o\
  game.o\
  iso.o\
  md2.o\
  mini.q.o\
  net.o\
  ogl.o\
  physics.o\
  qef.o\
  renderer.o\
  script.o\
  shaders.o\
  sound.o\
  stl.o\
  sys.o\
  task.o\
  text.o\
  ui.o
all: mini.q compress_chars

include Makefile.dep

SHADERS=data/shaders/fixed_vp.glsl\
        data/shaders/fixed_fp.glsl\
        data/shaders/font_fp.glsl\
        data/shaders/dfrm_fp.glsl

## build embedded shader source
shaders.cpp: $(SHADERS)
	./scripts/stringify_all_shaders.sh shaders.cpp shaders.hpp

## build font header
font.hxx: compress_chars data/font8x16.png
	./compress_chars data/font8x16.png 8 16 0 > font.hxx

mini.q: $(OBJS) $(ENET_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(OBJS) $(ENET_OBJS) $(LIBS)

compress_chars: compress_chars.o
	$(CXX) $(CXXFLAGS) -o compress_chars compress_chars.o $(LIBS)

clean:
	rm -f $(OBJS) mini.q compress_chars compress_chars.o

