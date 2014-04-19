/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - sound.cpp -> sound related code
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "base/vector.hpp"
#include "base/allocator.hpp"
#include <SDL2/SDL_mixer.h>

#define MAXCHAN 32
#define SOUNDFREQ 22050
#define MAXVOL MIX_MAX_VOLUME

namespace q {
namespace sound {

VARP(soundvol, 0, 255, 255);
VARP(musicvol, 0, 128, 255);
VAR(soundbufferlen, 128, 1024, 4096);
VAR(stereo, 0, 1, 1);

static bool nosound = false;
static Mix_Music *mod = NULL;
static void *stream = NULL;
static vector<Mix_Chunk*> samples;
static cvector snames;
static int soundsatonce = 0;
static int lastsoundmillis = 0;
static struct { vec3f loc; bool inuse; } soundlocs[MAXCHAN];

void stop(void) {
  if (nosound) return;
  if (mod) {
    Mix_HaltMusic();
    Mix_FreeMusic(mod);
    mod = NULL;
  }
  if (stream) stream = NULL;
}

void start(void) {
  memset(soundlocs, 0, sizeof(soundlocs));
  if (Mix_OpenAudio(SOUNDFREQ, MIX_DEFAULT_FORMAT, 2, soundbufferlen) < 0) {
    con::out("sound init failed (SDL_mixer): %s", (size_t)Mix_GetError());
    nosound = true;
  }
  Mix_AllocateChannels(MAXCHAN);
}

static void music(const char *name) {
  if (nosound) return;
  stop();
  if (soundvol && musicvol) {
    fixedstring sn;
    strcpy_s(sn, "packages/");
    strcat_s(sn, name);
    if ((mod = Mix_LoadMUS(sys::path(sn)))) {
      Mix_PlayMusic(mod, -1);
      Mix_VolumeMusic((musicvol*MAXVOL)/255);
    }
  }
}

static void registersound(const char *name) {
  static linear_allocator<4096,sizeof(char)> lin;
  loopv(snames) if (strcmp(snames[i], name)==0) return;
  snames.add(lin.newstring(name));
  samples.add(NULL);
}

void finish(void) {
  if (nosound) return;
  stop();
  Mix_CloseAudio();
}

static void updatechanvol(int chan, const vec3f *loc) {
  int vol = soundvol, pan = 255/2;
  if (loc) {
    const auto v = game::player1->o-*loc;
    const auto dist = length(v);

    // simple mono distance attenuation
    vol -= int(int(dist)*3*soundvol/255);
    if (stereo && (v.x != 0.f || v.y != 0.f)) {
      // relative angle of sound along X-Z axis
      const auto yaw = -atan2(v.x,v.z)-game::player1->ypr.x*float(pi)/180.0f;
      // range is from 0 (left) to 255 (right)
      pan = int(255.9f*(0.5f*sin(yaw)+0.5f));
    }
  }
  vol = (vol*MAXVOL)/255;

#if !defined(__JAVASCRIPT__)
  // crashed (out of bound access) happens here.
  // I do not know why unfortunately
  Mix_Volume(chan, vol);
  Mix_SetPanning(chan, 255-pan, pan);
#endif /* __JAVASCRIPT__ */
}

static void newsoundloc(int chan, const vec3f *loc) {
  assert(chan>=0 && chan<MAXCHAN);
  soundlocs[chan].loc = *loc;
  soundlocs[chan].inuse = true;
}

void updatevol(void) {
  if (nosound) return;
  loopi(MAXCHAN) if (soundlocs[i].inuse) {
    if (Mix_Playing(i))
      updatechanvol(i, &soundlocs[i].loc);
    else
      soundlocs[i].inuse = false;
  }
}

void playc(int n) {
  // client::addmsg(0, 2, SV_SOUND, n);
  play(n);
}

void play(int n, const vec3f *loc) {
  if (nosound) return;
  if (!soundvol) return;
  if (int(game::lastmillis())==int(lastsoundmillis))
    soundsatonce++;
  else
    soundsatonce = 1;
  lastsoundmillis = int(game::lastmillis());
  if (soundsatonce>5) return;  // avoid bursts of sounds with heavy packetloss
  if (n<0 || n>=samples.length()) {
    con::out("unregistered sound: %d", n);
    return;
  }

  if (!samples[n]) {
    sprintf_sd(buf)("data/sounds/%s.wav", snames[n]);
    samples[n] = Mix_LoadWAV(sys::path(buf));
    if (!samples[n]) {
      con::out("failed to load sample: %s", buf);
      return;
    }
  }

  const int chan = Mix_PlayChannel(-1, samples[n], 0);
  if (chan<0) return;
  if (loc) newsoundloc(chan, loc);
  updatechanvol(chan, loc);
}

static void sound(int n) { play(n, NULL); }

CMD(music, ARG_1STR);
CMD(registersound, ARG_1STR);
CMD(sound, ARG_1INT);
} /* namespace sound */
} /* namespace q */

