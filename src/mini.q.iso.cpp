/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.iso.cpp -> test iso surface extractor
 -------------------------------------------------------------------------*/
#include "base/console.hpp"
#include "base/task.hpp"
#include "base/string.hpp"
#include "base/script.hpp"
#include "base/sys.hpp"
#include "csg.hpp"
#include "iso.hpp"
#include "mini.q.hpp"

using namespace q;
static void outputcpufeatures() {
  using namespace sys;
  fixedstring features("cpu: ");
  loopi(CPU_FEATURE_NUM) {
    const auto name = featurename(cpufeature(i));
    const auto hasit = hasfeature(cpufeature(i));
    strcat_s(features, hasit ? name : "");
    if (hasit && i != int(CPU_FEATURE_NUM)-1) strcat_s(features, " ");
  }
  con::out(features.c_str());
}

static const float CELLSIZE = 0.1f;
int main(int argc, const char **argv) {
  outputcpufeatures();

  con::out("init: memory debugger");
  sys::memstart();

  con::out("init: tasking system");
#if defined(__X86__) || defined(__X86_64__)
  // flush to zero and no denormals
  _mm_setcsr(_mm_getcsr() | (1<<15) | (1<<6));
#endif
  const u32 threadnum = sys::threadnumber() - 1;
  con::out("init: tasking system: %d threads created", threadnum);
  task::start(&threadnum, 1);

  con::out("init: isosurface module");
  iso::start();
  con::out("init: csg module");
  csg::start();
  // load the csg function
  script::execscript(argv[1] ? argv[1] : "data/csg.lua");
  const auto node = csg::makescene();

  // build the mesh
  assert(node != NULL);
  const auto start = sys::millis();
  const auto m = iso::dc(vec3f(0.15f), 4096, CELLSIZE, *node);
  const auto end = sys::millis();
  printf("time %f ms\n", float(end-start));
  geom::store("simple.mesh", m);
#if !defined(NDEBUG)
  finish();
#endif
}
