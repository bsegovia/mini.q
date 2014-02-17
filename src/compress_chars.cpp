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

template <typename T>
void loaddata(SDL_Surface *s, bool invert) {
  uint32_t k = 0;
  T *src = (T*) s->pixels;
  const bool pos = invert ? 0 : 1;
  const bool neg = invert ? 1 : 0;
  for (auto y = 0; y < s->h; ++y) {
    for (auto x = 0; x < s->w/32; ++x, ++k) {
      uint32_t out = 0;
      for (auto i = 0; i < 32; ++i)
        out |= ((src[y*s->w+32*x+i]!=0?pos:neg) << i);
      printf("0x%08x, ", out);
      if (k%8==7)printf("\n");
    }
  }
}
int main(int argc, const char *argv[]) {
  if (argc != 5) FATAL("usage: %s filename width height invert", argv[0]);
  auto s = IMG_Load(argv[1]);
  auto fontw = atoi(argv[2]);
  auto fonth = atoi(argv[3]);
  auto invert = atoi(argv[4]);
  if (s == NULL) FATAL("unable to load font file %s", argv[1]);
  if (s->w%32 != 0) FATAL("unsupported width");
  if (s->format->BitsPerPixel != 8 && s->format->BitsPerPixel != 32)
    FATAL("unsupported pixel format (%d bpp)", s->format->BitsPerPixel);
  printf("static const int fontw = %d, fonth = %d;\n", s->w, s->h);
  printf("static const vec2f fontwh(%f, %f);\n", float(s->w), float(s->h));
  printf("static const int fontcol = %d;\n", s->w/fontw);
  printf("static const int charw = %d;\n", fontw);
  printf("static const int charh = %d;\n", fonth);
  printf("static const vec2f charwh(%f,%f);\n", float(fontw), float(fonth));
  printf("static const u32 fontdata[] = {\n");
  if (s->format->BitsPerPixel == 8)
    loaddata<uint8_t>(s, invert);
  else
    loaddata<uint32_t>(s, invert);
  printf("};\n");

  SDL_FreeSurface(s);
  return 0;
}

