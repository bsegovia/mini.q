FLAGS=-MP -MD -Wall -fvisibility=hidden -I./ `sdl2-config --cflags`
CXXCOMMONFLAGS=-fno-exceptions -fno-rtti -std=c++11 -fvisibility-inlines-hidden -Wno-invalid-offsetof
# avx2 stuff for gcc -> -mavx2 -mbmi -mlzcnt -mfma -mf16c

ifneq (,$(findstring g++, $(CXX)))
FLAGS+=-fabi-version=6
endif
ifeq (,$(findstring icc, $(CXX)))
CXXCOMMONFLAGS+=-fno-unwind-tables
endif
ifneq (,$(findstring icc, $(CXX)))
FLAGS+=-vec-
endif

DEBUGFLAGS=-O0 -DMEMORY_DEBUGGER -g
OPTFLAGS=-O2 -DMEMORY_DEBUGGER -g -fsanitize=address
RELFLAGS=-fomit-frame-pointer -O3 -DNDEBUG -DRELEASE

CXXFLAGS=$(CXXCOMMONFLAGS) $(FLAGS) $(DEBUGFLAGS)
CXXSSEFLAGS=$(CXXFLAGS) -msse -msse2
CXXAVXFLAGS=$(CXXFLAGS) -mavx
LIBS=`sdl2-config --libs` -lSDL2_image -lSDL2_mixer -lz -lm -lstdc++ -fsanitize=address

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
  base/intrusive_list.o\
  base/hash.o\
  base/math.o\
  base/script.o\
  base/string.o\
  base/sys.o\
  base/sse.o\
  base/task.o

LUA_OBJS=\
  base/lua/lapi.o\
  base/lua/lauxlib.o\
  base/lua/lbaselib.o\
  base/lua/lcode.o\
  base/lua/ldblib.o\
  base/lua/ldebug.o\
  base/lua/ldo.o\
  base/lua/ldump.o\
  base/lua/lfunc.o\
  base/lua/lgc.o\
  base/lua/linit.o\
  base/lua/liolib.o\
  base/lua/llex.o\
  base/lua/lmathlib.o\
  base/lua/lmem.o\
  base/lua/loadlib.o\
  base/lua/lobject.o\
  base/lua/lopcodes.o\
  base/lua/loslib.o\
  base/lua/lparser.o\
  base/lua/lstate.o\
  base/lua/lstring.o\
  base/lua/lstrlib.o\
  base/lua/ltable.o\
  base/lua/ltablib.o\
  base/lua/ltm.o\
  base/lua/lundump.o\
  base/lua/lvm.o\
  base/lua/lzio.o\
  base/lua/print.o

GAME_OBJS=\
  client.o\
  bvh.o\
  csg.o\
  csgscalar.o\
  csgsse.o\
  csgavx.o\
  demo.o\
  editing.o\
  entities.o\
  game.o\
  geom.o\
  iso.o\
  md2.o\
  menu.o\
  mini.q.o\
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
  rtscalar.o\
  rtsse.o\
  rtavx.o\
  sound.o\
  sky.o\
  text.o\
  ui.o\
  weapon.o\
  world.o

CLIENT_OBJS=\
  $(LUA_OBJS)\
  $(ENET_OBJS)\
  $(BASE_OBJS)\
  $(GAME_OBJS)\
  main.o

RT_OBJS=\
  $(LUA_OBJS)\
  $(ENET_OBJS)\
  $(BASE_OBJS)\
  $(GAME_OBJS)\
  mini.q.rt.o

ISO_OBJS=\
  $(LUA_OBJS)\
  $(ENET_OBJS)\
  $(BASE_OBJS)\
  $(GAME_OBJS)\
  mini.q.iso.o

SERVER_OBJS=\
  $(LUA_OBJS)\
  $(ENET_OBJS)\
  base/math.o\
  base/string.o\
  base/sys.o\
  base/intrusive_list.o\
  network.o\
  server.o\
  serverms.o\
  standalone.o

MAYAOBJ_OBJS=\
  base/sys.o\
  base/string.o\
  base/intrusive_list.o\
  obj.o

SHADERS=$(shell ls data/shaders/*[glsl,decl])
all: mini.q.server mini.q.rt mini.q.iso mini.q importobj compress_chars importobj

%.o: %.cpp
	$(CXX) $(CXXSSEFLAGS) -c $< -o $@

%avx.o: %avx.cpp
	$(CXX) $(CXXAVXFLAGS) -c $< -o $@

-include $(MAYAOBJ_OBJS:.o=.d)
-include $(CLIENT_OBJS:.o=.d)
-include $(SERVER_OBJS:.o=.d)
-include $(RT_OBJS:.o=.d)
-include $(ISO_OBJS:.o=.d)
-include $(LUA_OBJS:.o=.d)
-include $(GAME_OBJS:.o=.d)
-include $(BASE_OBJS:.o=.d)

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

mini.q.iso: $(ISO_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q.iso $(ISO_OBJS) $(LIBS)

mini.q.server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q.server $(SERVER_OBJS) $(LIBS)

importobj: $(MAYAOBJ_OBJS)
	$(CXX) $(CXXFLAGS) -o importobj $(MAYAOBJ_OBJS) $(LIBS)

compress_chars: compress_chars.o
	$(CXX) $(CXXFLAGS) -o compress_chars compress_chars.o $(LIBS)

clean:
	rm -rf mini.q mini.q.server mini.q.rt importobj\
		compress_chars *.o *.d ./enet/*.o ./enet/*.d\
		base/*.o base/*.d base/lua/*.o base/lua/*.d\
		oprofile_data

