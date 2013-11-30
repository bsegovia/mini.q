#CXX=clang++
#CXX=~/src/emscripten/em++
CXXOPTFLAGS=-Wall -Os -DNDEBUG -Wall -std=c++11
CXXDEBUGFLAGS=-Wall -O0 -g -DNDEBUG -std=c++11
CXXFLAGS=$(CXXDEBUGFLAGS) -I../enet/include `sdl-config --cflags`
LIBS=`sdl-config --libs`
OBJS=mini.q.o

all: mini.q

mini.q.o: mini.q.cpp mini.q.hpp Makefile

clean:
	-rm -f $(OBJS) mini.q

mini.q: $(OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(OBJS) $(LIBS)

