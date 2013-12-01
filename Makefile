#CXX=clang++
#CXX=~/src/emscripten/em++
CXXOPTFLAGS=-Wall -Os -DNDEBUG -std=c++11
CXXDEBUGFLAGS=-Wall -O0 -g -std=c++11
CXXFLAGS=$(CXXOPTFLAGS) `sdl-config --cflags`
LIBS=`sdl-config --libs` -lSDL_image
OBJS=con.o game.o mini.q.o ogl.o script.o sys.o text.o
#OBJS=blob.o
HEADERS=con.hpp game.hpp ogl.hpp script.hpp sys.hpp text.hpp
all: mini.q

blob.o: blob.cpp $(HEADERS)
con.o: con.cpp $(HEADERS)
game.o: game.cpp $(HEADERS)
ogl.o: ogl.cpp $(HEADERS)
script.o: script.cpp $(HEADERS)
sys.o: sys.cpp $(HEADERS)
mini.q.o: mini.q.cpp $(HEADERS)

clean:
	-rm -f $(OBJS) mini.q

mini.q: $(OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(OBJS) $(LIBS)

