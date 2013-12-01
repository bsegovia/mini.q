/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - compress_chars.cpp -> load a png for the font and make a bitstring
 -------------------------------------------------------------------------*/
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#define FATAL(...) do {\
  fprintf(stderr, __VA_ARGS__);\
  fprintf(stderr, "\n");\
  exit(EXIT_FAILURE);\
} while (0)

int main(int argc, const char *argv[]) {
  if (argc != 2) FATAL("usage: %s filename", argv[0]);
  auto s = IMG_Load(argv[1]);
  if (s == NULL) FATAL("unable to load font file %s", argv[1]);
  if (s->w%32 != 0) FATAL("unsupported width");
  if (s->format->BitsPerPixel != 32) FATAL("unsupported pixel format");
  uint32_t *src = (uint32_t*)(s->pixels);
  uint32_t k = 0;
  printf("static const int fontw = %d, fonth = %d;\n", s->w, s->h);
  printf("static const int charxnum = %d;\n", s->w/8);
  printf("static const u32 fontdata[] = {\n");
  for (auto y = 0; y < s->h; ++y) {
    for (auto x = 0; x < s->w/32; ++x, ++k) {
      uint32_t out = 0;
      for (auto i = 0; i < 32; ++i)
        out |= ((src[y*s->w+32*x+i]!=0?1:0) << i);
      printf("0x%08x, ", out);
      if (k%8==7)printf("\n");
    }
  }
  printf("};\n");
  SDL_FreeSurface(s);
  return 0;
}

