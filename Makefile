#CXX=clang++
#CXXDEBUGFLAGS=-Wall -O0 -g -std=c++11 -fsanitize=address
#CXX=~/src/emscripten/em++
CXXOPTFLAGS=-Wall -Os -DNDEBUG -std=c++11
CXXDEBUGFLAGS=-Wall -O0 -g -std=c++11
CXXFLAGS=$(CXXDEBUGFLAGS) -Wno-invalid-offsetof -I./ `sdl-config --cflags`
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
  net.hpp\
  ogl.hpp\
  physics.hpp\
  script.hpp\
  shaders.hpp\
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

blob.o: blob.cpp $(HEADERS)
con.o: con.cpp $(HEADERS)
game.o: game.cpp $(HEADERS)
mini.q.o: mini.q.cpp $(HEADERS)
net.o: net.cpp net.hpp stl.hpp sys.hpp
ogl.o: ogl.cpp $(HEADERS)
physics.o: physics.cpp $(HEADERS)
shaders.o: shaders.cpp $(HEADERS)
script.o: script.cpp $(HEADERS)
stl.o: stl.cpp $(HEADERS)
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

