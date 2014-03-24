CXX=clang++
FLAGS=-MP -MD -Wall -fvisibility=hidden -I./ `sdl-config --cflags` -I./
CXXCOMMONFLAGS=-fno-exceptions -fno-unwind-tables -fno-rtti -std=c++11 -fvisibility-inlines-hidden -Wno-invalid-offsetof
DEBUGFLAGS=-O0 -DMEMORY_DEBUGGER -g
OPTFLAGS=-O2 -DMEMORY_DEBUGGER -g -fsanitize=address
RELFLAGS=-fomit-frame-pointer -O3 -DNDEBUG -DRELEASE -g
CXXFLAGS= $(CXXCOMMONFLAGS) $(FLAGS) $(OPTFLAGS)
LIBS=`sdl-config --libs` -lSDL_image -lSDL_mixer -lz -fsanitize=address

ENET_OBJS=\
  enet/callbacks.o\
  enet/host.o\
  enet/list.o\
  enet/packet.o\
  enet/peer.o\
  enet/protocol.o\
  enet/unix.o

BASE_OBJS=\
  base/console.o\
  base/math.o\
  base/script.o\
  base/stl.o\
  base/sys.o\
  base/task.o

GAME_OBJS=\
  client.o\
  bvh.o\
  csg.o\
  demo.o\
  editing.o\
  entities.o\
  game.o\
  iso.o\
  md2.o\
  menu.o\
  monster.o\
  network.o\
  ogl.o\
  physics.o\
  qef.o\
  renderer.o\
  server.o\
  serverbrowser.o\
  serverms.o\
  serverutil.o\
  shaders.o\
  rt.o\
  sound.o\
  sky.o\
  text.o\
  ui.o\
  weapon.o\
  world.o

CLIENT_OBJS=\
  $(ENET_OBJS)\
  $(BASE_OBJS)\
  $(GAME_OBJS)\
  mini.q.o

RT_OBJS=\
  $(ENET_OBJS)\
  $(BASE_OBJS)\
  $(GAME_OBJS)\
  mini.q.rt.o

SERVER_OBJS=\
  $(ENET_OBJS)\
  base/math.o\
  base/stl.o\
  base/sys.o\
  network.o\
  server.o\
  serverms.o\
  standalone.o

MAYAOBJ_OBJS=\
  base/sys.o\
  base/stl.o\
  obj.o

SHADERS=$(shell ls data/shaders/*[glsl,decl])
all: mini.q.server mini.q.rt mini.q importobj compress_chars importobj

-include $(CLIENT_OBJS:.o=.d)
-include $(SERVER_OBJS:.o=.d)
-include $(RT_OBJS:.o=.d)

## build embedded shader source
shaders.hxx shaders.cxx: $(SHADERS)
	./scripts/stringify_all_shaders.sh shaders.cxx shaders.hxx shader.dep

## build font header
font.hxx: compress_chars data/font8x16.png
	./compress_chars data/font8x16.png 8 16 0 > font.hxx

mini.q: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(CLIENT_OBJS) $(LIBS)

mini.q.rt: $(RT_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q.rt $(RT_OBJS) $(LIBS)

mini.q.server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q.server $(SERVER_OBJS) $(LIBS)

importobj: $(MAYAOBJ_OBJS)
	$(CXX) $(CXXFLAGS) -o importobj $(MAYAOBJ_OBJS) $(LIBS)

compress_chars: compress_chars.o
	$(CXX) $(CXXFLAGS) -o compress_chars compress_chars.o $(LIBS)

clean:
	rm -rf mini.q mini.q.server mini.q.rt importobj\
		compress_chars *.o *.d ./enet/*.o ./enet/*.d \
		base/*.o base/*.d oprofile_data

