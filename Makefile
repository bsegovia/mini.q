#CXX=clang++
#CXXDEBUGFLAGS=-Wall -O0 -DMEMORY_DEBUGGER -g -std=c++11 -fsanitize=address
#CXX=~/src/emscripten/em++
CXXOPTFLAGS=-Wall -DMEMORY_DEBUGGER -Os -DNDEBUG -std=c++11
CXXDEBUGFLAGS=-Wall -DMEMORY_DEBUGGER -O0 -g -std=c++11
CXXFLAGS=$(CXXOPTFLAGS) -Wno-invalid-offsetof -I./ `sdl-config --cflags`
LIBS=`sdl-config --libs` -lSDL_image -lSDL_mixer
OBJS=\
  con.o\
  game.o\
  iso.o\
  iso_dc.o\
  iso_mc.o\
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
  text.o
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
#font.hxx: compress_chars data/font8x8.png
#	./compress_chars data/font8x8.png 8 8 > font.hxx
font.hxx: compress_chars data/font8x16.png
	./compress_chars data/font8x16.png 8 16 > font.hxx

mini.q: $(OBJS) $(ENET_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(OBJS) $(ENET_OBJS) $(LIBS)

compress_chars: compress_chars.o
	$(CXX) $(CXXFLAGS) -o compress_chars compress_chars.o $(LIBS)

clean:
	rm -f $(OBJS) mini.q compress_chars compress_chars.o

