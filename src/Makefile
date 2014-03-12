#CXX=clang++
FLAGS=-MP -MD -Wall -fvisibility=hidden -I./ `sdl-config --cflags` -I./
CXXCOMMONFLAGS=-fno-exceptions -fno-unwind-tables -fno-rtti -std=c++11 -fvisibility-inlines-hidden -Wno-invalid-offsetof
DEBUGFLAGS=-O0 -DMEMORY_DEBUGGER -g
OPTFLAGS=-O2 -DMEMORY_DEBUGGER -g -fsanitize=address
RELFLAGS=-fomit-frame-pointer -O3 -DNDEBUG -DRELEASE -g

CXXFLAGS= $(CXXCOMMONFLAGS) $(FLAGS) $(RELFLAGS)
LIBS=`sdl-config --libs` -lSDL_image -lSDL_mixer -lz #-fsanitize=address

ENET_OBJS=\
  enet/callbacks.o\
  enet/host.o\
  enet/list.o\
  enet/packet.o\
  enet/peer.o\
  enet/protocol.o\
  enet/unix.o

CLIENT_OBJS=\
  $(ENET_OBJS)\
  client.o\
  con.o\
  csg.o\
  demo.o\
  editing.o\
  entities.o\
  game.o\
  iso.o\
  md2.o\
  math.o\
  menu.o\
  mini.q.o\
  monster.o\
  network.o\
  ogl.o\
  physics.o\
  qef.o\
  renderer.o\
  script.o\
  server.o\
  serverbrowser.o\
  serverms.o\
  serverutil.o\
  shaders.o\
  sound.o\
  stl.o\
  sky.o\
  sys.o\
  task.o\
  text.o\
  ui.o\
  weapon.o\
  world.o

SERVER_OBJS=\
  $(ENET_OBJS)\
  math.o\
  stl.o\
  sys.o\
  network.o\
  server.o\
  serverms.o\
  standalone.o

SHADERS=$(shell ls data/shaders/*[glsl,decl])
all: mini.q.server mini.q compress_chars

-include $(CLIENT_OBJS:.o=.d)
-include $(SERVER_OBJS:.o=.d)

## build embedded shader source
shaders.hxx shaders.cxx: $(SHADERS)
	./scripts/stringify_all_shaders.sh shaders.cxx shaders.hxx shader.dep

## build font header
font.hxx: compress_chars data/font8x16.png
	./compress_chars data/font8x16.png 8 16 0 > font.hxx

mini.q: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(CLIENT_OBJS) $(LIBS)

mini.q.server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q.server $(SERVER_OBJS) $(LIBS)

compress_chars: compress_chars.o
	$(CXX) $(CXXFLAGS) -o compress_chars compress_chars.o $(LIBS)

clean:
	rm -rf mini.q mini.q.server compress_chars *.o *.d ./enet/*.o ./enet/*.d oprofile_data

