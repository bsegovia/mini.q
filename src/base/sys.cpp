/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - sys.cpp -> implements platform specific code, stdlib and main function
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "intrusive_list.hpp"
#if defined(__UNIX__)
#include <unistd.h>
#endif
#include <malloc.h>

namespace q {
namespace sys {

int scrw = 1024, scrh = 1024;
static int islittleendian_ = 1;
void initendiancheck(void) { islittleendian_ = *((char*)&islittleendian_); }
int islittleendian(void) { return islittleendian_; }
void endianswap(void *memory, int stride, int length) {
  if (*((char *)&stride)) return;
  loop(w, length) loop(i, stride/2) {
    u8 *p = (u8 *)memory+w*stride;
    u8 t = p[i];
    p[i] = p[stride-i-1];
    p[stride-i-1] = t;
  }
}

#if defined __WIN32__
u32 threadnumber() {
#if (_WIN32_WINNT >= 0x0601)
  int groups = GetActiveProcessorGroupCount();
  int totalProcessors = 0;
  for (int i = 0; i < groups; i++) 
    totalProcessors += GetActiveProcessorCount(i);
  return totalProcessors;
#else
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#endif
}
#else
u32 threadnumber() { return sysconf(_SC_NPROCESSORS_CONF); }
#endif

#if defined(MEMORY_DEBUGGER)
struct DEFAULT_ALIGNED memblock : intrusive_list_node {
  INLINE memblock(size_t sz, const char *file, int linenum) :
    file(file), linenum(linenum), allocnum(0), size(sz)
  {rbound() = lbound() = 0xdeadc0de;}
  const char *file;
  u32 linenum, allocnum, size, bound;
  INLINE u32 &rbound(void) {return *(u32*)((char*)this+sizeof(memblock)+size); }
  INLINE u32 &lbound(void) {return bound;}
};

static intrusive_list<memblock> *memlist = NULL;
static SDL_mutex *memmutex = NULL;
static u32 memallocnum = 0;
static bool memfirstalloc = true;

static void memlinkblock(memblock *node) {
  if (memmutex) SDL_LockMutex(memmutex);
  node->allocnum = memallocnum++;
  memlist->push_back(node);
  if (memmutex) SDL_UnlockMutex(memmutex);
}
static void memunlinkblock(memblock *node) {
  if (memmutex) SDL_LockMutex(memmutex);
  unlink(node);
  if (memmutex) SDL_UnlockMutex(memmutex);
}

#define MEMOUT(S,B)\
  fixedstring S(fmt, "file: %s, line %i, size %i bytes, alloc %i",\
                     (B)->file, (B)->linenum, (B)->size, (B)->allocnum)
static void memcheckbounds(memblock *node) {
  if (node->lbound() != 0xdeadc0de || node->rbound() != 0xdeadc0de) {
    fprintf(stderr, "memory corruption detected (alloc %i)\n", node->allocnum);
    DEBUGBREAK;
    _exit(EXIT_FAILURE);
  }
}
static void memoutputalloc(void) {
  size_t sz = 0;
  if (memlist != NULL) {
    for (auto it = memlist->begin(); it != memlist->end(); ++it) {
      MEMOUT(unfreed, it);
      fprintf(stderr, "unfreed allocation: %s\n", unfreed.c_str());
      sz += it->size;
    }
    if (sz > 0) fprintf(stderr, "total unfreed: %fKB \n", float(sz)/1000.f);
    delete memlist;
    _exit(EXIT_FAILURE);
  }
}

static void memonfirstalloc(void) {
  if (memfirstalloc) {
    memlist = new intrusive_list<memblock>();
    atexit(memoutputalloc);
    memfirstalloc = false;
  }
}
#undef MEMOUT

void memstart(void) { memmutex = SDL_CreateMutex(); }
void *memalloc(size_t sz, const char *filename, int linenum) {
  memonfirstalloc();
  if (sz) {
    sz = ALIGN(sz, DEFAULT_ALIGNMENT);
    auto ptr = malloc(sz+sizeof(memblock)+sizeof(u32));
    auto block = new (ptr) memblock(sz, filename, linenum);
    memlinkblock(block);
    return (void*) (block+1);
  } else
    return NULL;
}

void memfree(void *ptr) {
  memonfirstalloc();
  if (ptr) {
    auto block = (memblock*)((char*)ptr-sizeof(memblock));
    memcheckbounds(block);
    memunlinkblock(block);
    free(block);
  }
}

void *memrealloc(void *ptr, size_t sz, const char *filename, int linenum) {
  memonfirstalloc();
  auto block = ptr==NULL?(memblock*)NULL:(memblock*)((char*)ptr-sizeof(memblock));
  if (ptr) {
    memcheckbounds(block);
    memunlinkblock(block);
  }
  if (sz) {
    sz = ALIGN(sz, DEFAULT_ALIGNMENT);
    auto ptr = realloc(block, sz+sizeof(memblock)+sizeof(u32));
    block = new (ptr) memblock(sz, filename, linenum);
    memlinkblock(block);
    return (void*) (block+1);
  } else if (ptr)
    free(block);
  return NULL;
}
#else
void memstart(void) {}
void *memalloc(size_t sz, const char*, int) {return malloc(sz);}
void *memrealloc(void *ptr, size_t sz, const char *, int) {return realloc(ptr,sz);}
void memfree(void *ptr) {if (ptr) free(ptr);}
#endif // defined(MEMORY_DEBUGGER)

void *memalignedalloc(size_t size, size_t align, const char *file, int lineno) {
  if (size == 0) return NULL;
  auto base = (char*)memalloc(size+align+sizeof(int), file, lineno);
  assert(NULL != base);

  auto unaligned = base + sizeof(int);
  auto aligned = unaligned + align - ((size_t)unaligned & (align-1));
  ((int*)aligned)[-1] = (int)((size_t)aligned-(size_t)base);
  return aligned;
}

void memalignedfree(const void* ptr) {
  if (ptr == NULL) return;
  int ofs = ((int*)ptr)[-1];
  free((char*)ptr-ofs);
}

char *path(char *s) {
  for (char *t = s; (t = strpbrk(t, "/\\")) != 0; *t++ = PATHDIV);
  return s;
}

char *loadfile(const char *fn, int *size) {
  auto f = fopen(fn, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  u32 len = ftell(f);
  fseek(f, 0, SEEK_SET);
  auto buf = (char *) MALLOC(len+1);
  if (!buf) return NULL;
  buf[len] = 0;
  size_t rlen = fread(buf, 1, len, f);
  fclose(f);
  if (len!=rlen || len<=0) {
    FREE(buf);
    return NULL;
  }
  if (size!=NULL) *size = len;
  return buf;
}

void quit(const char *msg) {
#if defined(RELEASE)
#if defined(__WIN32__)
  ExitProcess(EXIT_SUCCESS);
#else
  _exit(EXIT_SUCCESS);
#endif
#else
  if (msg && strlen(msg)) {
#if defined(__WIN32__)
    MessageBox(NULL, msg, "cube fatal error", MB_OK|MB_SYSTEMMODAL);
#else
    printf("%s\n", msg);
#endif // __WIN32__
  } else
    q::finish();
  SDL_Quit();
  exit(msg && strlen(msg) ? EXIT_FAILURE : EXIT_SUCCESS);
#endif
}

void fatal(const char *s, const char *o) {
  assert(0);
  fixedstring m(fmt, "%s%s (%s)\n", s, o, SDL_GetError());
  quit(m.c_str());
}

void keyrepeat(bool on) {
  // TODO SDL_EnableKeyRepeat(on ? SDL_DEFAULT_REPEAT_DELAY : 0, SDL_DEFAULT_REPEAT_INTERVAL);
}

#if defined(__WIN32__)
float millis() {
  LARGE_INTEGER freq, val;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&val);
  static double first = double(val.QuadPart) / double(freq.QuadPart) * 1e3;
  return float(double(val.QuadPart) / double(freq.QuadPart) * 1e3 - first);
}
#else
float millis() {
  struct timeval tp; gettimeofday(&tp,NULL);
  static double first = double(tp.tv_sec)*1e3 + double(tp.tv_usec)*1e-3;
  return float(double(tp.tv_sec)*1e3 + double(tp.tv_usec)*1e-3 - first);
}
#endif

void writebmp(const int *data, int w, int h, const char *filename) {
  int x, y;
  FILE *fp = fopen(filename, "wb");
  assert(fp);
  struct bmphdr {
    int filesize;
    short as0, as1;
    int bmpoffset;
    int headerbytes;
    int w;
    int h;
    short nplanes;
    short bpp;
    int compression;
    int sizeraw;
    int hres;
    int vres;
    int npalcolors;
    int nimportant;
  };

  const char magic[2] = { 'B', 'M' };
  char *raw = (char *) MALLOC(w*h*sizeof(int));
  assert(raw);
  char *p = raw;

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      int c = *data++;
      *p++ = ((c >> 16) & 0xff);
      *p++ = ((c >> 8) & 0xff);
      *p++ = ((c >> 0) & 0xff);
    }
    while (x & 3) {
      *p++ = 0;
      x++;
    } // pad to dword
  }
  int sizeraw = p - raw;
  int scanline = (w * 3 + 3) & ~3;
  assert(sizeraw == scanline * h);

  struct bmphdr hdr;
  hdr.filesize = scanline * h + sizeof(hdr) + 2;
  hdr.as0 = 0;
  hdr.as1 = 0;
  hdr.bmpoffset = sizeof(hdr) + 2;
  hdr.headerbytes = 40;
  hdr.w = w;
  hdr.h = h;
  hdr.nplanes = 1;
  hdr.bpp = 24;
  hdr.compression = 0;
  hdr.sizeraw = sizeraw;
  hdr.hres = 0;
  hdr.vres = 0;
  hdr.npalcolors = 0;
  hdr.nimportant = 0;
  fwrite(&magic[0], 1, 2, fp);
  fwrite(&hdr, 1, sizeof(hdr), fp);
  fwrite(raw, 1, hdr.sizeraw, fp);
  fclose(fp);
  FREE(raw);
}

enum {eax=0, ebx=1, ecx=2, edx=3};
template <int lvl, int bit, int reg> static INLINE bool has() {
  int flags[]={0,0,0,0};
  __cpuid(flags, lvl);
  return (flags[reg] & (1<bit)) != 0;
}
template <int lvl, int bit, int reg> static INLINE bool hasex() {
  int flags[]={0,0,0,0};
  __cpuid_count(flags, lvl, 0);
  return (flags[reg] & (1<bit)) != 0;
}
static INLINE int check_xcr0_ymm() {
  u32 xcr0;
#if defined(__MSVC__)
  xcr0 = u32(_xgetbv(0))
#else
  asm ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#endif
  return ((xcr0 & 6) == 6); // checking if xmm and ymm state are enabled in XCR0
}

bool hasfeature(cpufeature feature) {
  switch (feature) {
    case CPU_SSE:   return has<1,25,edx>();
    case CPU_SSE2:  return has<1,26,edx>();
    case CPU_SSE3:  return has<1, 0,ecx>();
    case CPU_SSSE3: return has<1, 9,ecx>();
    case CPU_SSE41: return has<1,19,ecx>();
    case CPU_SSE42: return has<1,20,ecx>();
    case CPU_AVX:   return has<1,28,ecx>();
    case CPU_AVX2:  return hasex<7, 5,ebx>();
    case CPU_BMI1:  return hasex<7, 3,ebx>();
    case CPU_BMI2:  return hasex<7, 8,ebx>();
    case CPU_LZCNT: return has<0x80000001,5,ecx>();
    case CPU_FMA:   return has<1,12,ecx>();
    case CPU_F16C:  return has<1,29,ecx>();
    case CPU_YMM:   return check_xcr0_ymm();
    default: return false;
  }
}
const char *featurename(cpufeature feature) {
  switch (feature) {
    case CPU_SSE:   return "sse";
    case CPU_SSE2:  return "sse2";
    case CPU_SSE3:  return "sse3";
    case CPU_SSSE3: return "ssse3";
    case CPU_SSE41: return "sse4.1";
    case CPU_SSE42: return "sse4.2";
    case CPU_AVX:   return "avx";
    case CPU_AVX2:  return "avx2";
    case CPU_BMI1:  return "bmi1";
    case CPU_BMI2:  return "bmi2";
    case CPU_LZCNT: return "lzcnt";
    case CPU_FMA:   return "fma";
    case CPU_F16C:  return "f16c";
    case CPU_YMM:   return "ymmstate";
    default:        return "unknown";
  };
}

void textinput(bool on) {
  if(on)
    SDL_StartTextInput();
  else
    SDL_StopTextInput();
}
} /* namespace sys */
} /* namespace q */

