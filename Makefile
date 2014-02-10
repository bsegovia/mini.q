#CXX=clang++
FLAGS=-Wall -fno-exceptions -fno-unwind-tables -fvisibility=hidden -fvisibility-inlines-hidden -fno-rtti -std=c++11 -Wno-invalid-offsetof -I./ `sdl-config --cflags`
CXXDEBUGFLAGS=-O0 -DMEMORY_DEBUGGER -g
CXXOPTFLAGS=-O2 -DMEMORY_DEBUGGER -g
CXXRELFLAGS=-fomit-frame-pointer -Os -DNDEBUG -DRELEASE

CXXFLAGS=$(FLAGS) $(CXXDEBUGFLAGS)
#CXXFLAGS=$(CXXOPTFLAGS) -std=c++11 -Wno-invalid-offsetof -I./ `sdl-config --cflags` -fsanitize=address
#CXXFLAGS=$(FLAGS) $(CXXRELFLAGS)
#CXXFLAGS=$(CXXOPTFLAGS) -std=c++11 -Wno-invalid-offsetof -I./ `sdl-config --cflags`
LIBS=`sdl-config --libs` -g -lSDL_image -lSDL_mixer
OBJS=\
  con.o\
  csg.o\
  game.o\
  iso.o\
  md2.o\
  math.o\
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
  text.o\
  ui.o
all: mini.q compress_chars

include Makefile.dep

SHADERS=data/shaders/fixed_vp.glsl\
        data/shaders/fixed_fp.glsl\
        data/shaders/blit_vp.glsl\
        data/shaders/blit_fp.glsl\
        data/shaders/deferred_vp.glsl\
        data/shaders/deferred_fp.glsl\
        data/shaders/debugunsplit_vp.glsl\
        data/shaders/debugunsplit_fp.glsl\
        data/shaders/simple_material_vp.glsl\
        data/shaders/simple_material_fp.glsl\
        data/shaders/split_deferred_vp.glsl\
        data/shaders/split_deferred_fp.glsl\
        data/shaders/forward_vp.glsl\
        data/shaders/forward_fp.glsl\
        data/shaders/font_fp.glsl\
        data/shaders/fxaa_vp.glsl\
        data/shaders/fxaa_fp.glsl\
        data/shaders/fxaa.glsl\
        data/shaders/noise2D.glsl\
        data/shaders/noise3D.glsl\
        data/shaders/noise4D.glsl

## build embedded shader source
shaders.cxx: $(SHADERS)
	./scripts/stringify_all_shaders.sh shaders.cxx

## build font header
font.hxx: compress_chars data/font8x16.png
	./compress_chars data/font8x16.png 8 16 0 > font.hxx

mini.q: $(OBJS) $(ENET_OBJS)
	$(CXX) $(CXXFLAGS) -o mini.q $(OBJS) $(ENET_OBJS) $(LIBS)

compress_chars: compress_chars.o
	$(CXX) $(CXXFLAGS) -o compress_chars compress_chars.o $(LIBS)

clean:
	rm -f $(OBJS) mini.q compress_chars compress_chars.o

