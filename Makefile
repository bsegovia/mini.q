#CXX=clang++
#CXX=~/src/emscripten/em++
CXXOPTFLAGS=-Wall -Os -DNDEBUG -std=c++11
CXXDEBUGFLAGS=-Wall -O0 -g -std=c++11
CXXFLAGS=$(CXXDEBUGFLAGS) `sdl-config --cflags`
LIBS=`sdl-config --libs` -lSDL_image
OBJS=con.o game.o mini.q.o ogl.o renderer.o script.o shaders.o sys.o text.o
#OBJS=blob.o
HEADERS=con.hpp game.hpp ogl.hpp script.hpp shaders.hpp sys.hpp text.hpp
all: mini.q compress_chars

SHADERS=data/shaders/fixed_vp.glsl\
        data/shaders/fixed_fp.glsl\
        data/shaders/font_fp.glsl

shaders.cpp: $(SHADERS)
	./scripts/stringify_all_shaders.sh shaders.cpp shaders.hpp

blob.o: blob.cpp $(HEADERS)
con.o: con.cpp $(HEADERS)
game.o: game.cpp $(HEADERS)
mini.q.o: mini.q.cpp $(HEADERS)
ogl.o: ogl.cpp $(HEADERS)
shaders.o: shaders.cpp $(HEADERS)
script.o: script.cpp $(HEADERS)
sys.o: sys.cpp $(HEADERS)
text.o: text.cpp font.hxx $(HEADERS)
font.hxx: compress_chars data/font8x8.png
	./compress_chars data/font8x8.png > font.hxx

clean:
	-rm -f $(OBJS) mini.q

mini.q: $(OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(OBJS) $(LIBS)

compress_chars: compress_chars.o
	$(CXX) $(CXXFLAGS) -o compress_chars compress_chars.o $(LIBS)

