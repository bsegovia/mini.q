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

HEADERS=\
  con.hpp\
  game.hpp\
  iso.hpp\
  iso_mc.hpp\
  md2.hpp\
  net.hpp\
  ogl.hpp\
  physics.hpp\
  qef.hpp\
  renderer.hpp\
  script.hpp\
  shaders.hpp\
  sound.hpp\
  stl.hpp\
  sys.hpp\
  task.hpp\
  text.hpp
all: mini.q compress_chars

SHADERS=data/shaders/fixed_vp.glsl\
        data/shaders/fixed_fp.glsl\
        data/shaders/font_fp.glsl\
        data/shaders/dfrm_fp.glsl

## build embedded shader source
shaders.cpp: $(SHADERS)
	./scripts/stringify_all_shaders.sh shaders.cpp shaders.hpp

con.o: con.cpp $(HEADERS)
game.o: game.cpp $(HEADERS)
iso.o: iso.cpp iso.hpp math.hpp stl.hpp sys.hpp
iso_mc.o: iso_mc.cpp iso_mc.hpp iso.hpp math.hpp stl.hpp sys.hpp
md2.o: md2.cpp $(HEADERS)
mini.q.o: mini.q.cpp $(HEADERS)
net.o: net.cpp net.hpp stl.hpp sys.hpp
ogl.o: ogl.cpp ogl.hxx $(HEADERS)
qef.o: qef.cpp qef.hpp sys.hpp
physics.o: physics.cpp $(HEADERS)
renderer.o: renderer.cpp $(HEADERS)
shaders.o: shaders.cpp $(HEADERS)
sound.o: sound.cpp $(HEADERS)
script.o: script.cpp $(HEADERS)
stl.o: stl.cpp math.hpp stl.hpp sys.hpp
sys.o: sys.cpp $(HEADERS)
task.o: task.cpp stl.hpp sys.hpp task.hpp
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
	rm -f $(OBJS) mini.q compress_chars compress_chars.o

