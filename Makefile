#CXX=clang++
#CXX=~/src/emscripten/em++
CXXOPTFLAGS=-Wall -Os -DNDEBUG -std=c++11
CXXDEBUGFLAGS=-Wall -O0 -g -std=c++11
CXXFLAGS=$(CXXOPTFLAGS) `sdl-config --cflags`
LIBS=`sdl-config --libs`
OBJS=con.o game.o mini.q.o ogl.o script.o sys.o
#OBJS=blob.o
HEADERS=con.hpp game.hpp ogl.hpp script.hpp sys.hpp
all: mini.q

blob.o: blob.cpp $(HEADERS) Makefile
con.o: con.cpp $(HEADERS) Makefile
game.o: game.cpp $(HEADERS) Makefile
ogl.o: ogl.cpp $(HEADERS) Makefile
script.o: script.cpp $(HEADERS) Makefile
sys.o: sys.cpp $(HEADERS) Makefile
mini.q.o: mini.q.cpp $(HEADERS) Makefile

clean:
	-rm -f $(OBJS) mini.q

mini.q: $(OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(OBJS) $(LIBS)

