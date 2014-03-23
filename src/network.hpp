/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - network.hpp -> shared network code shared by client and server
 -------------------------------------------------------------------------*/
#pragma once
#include "base/sys.hpp"
#include "base/math.hpp"

namespace q {

// network messages codes, c2s, c2c, s2c
enum {
  SV_INITS2C, SV_INITC2S, SV_POS, SV_TEXT, SV_SOUND, SV_CDIS,
  SV_DIED, SV_DAMAGE, SV_SHOT, SV_FRAGS,
  SV_TIMEUP, SV_EDITENT, SV_MAPRELOAD, SV_ITEMACC,
  SV_MAPCHANGE, SV_ITEMSPAWN, SV_ITEMPICKUP, SV_DENIED,
  SV_PING, SV_PONG, SV_CLIENTPING, SV_GAMEMODE,
  SV_EDITH, SV_EDITT, SV_EDITS, SV_EDITD, SV_EDITE,
  SV_SENDMAP, SV_RECVMAP, SV_SERVMSG, SV_ITEMLIST, SV_EXT,
  SV_CUBE
};

enum { CS_ALIVE, CS_DEAD, CS_LAGGED, CS_EDITING };

enum {
  MAXCLIENTS = 256, // in a multiplayer game, can be arbitrarily changed
  MAXTRANS = 5000, // max amount of data to swallow in 1 go
  CUBE_SERVER_PORT = 28765,
  CUBE_SERVINFO_PORT = 28766,
  PROTOCOL_VERSION = 122 // bump when protocol changes
};

char msgsizelookup(int msg);
void putint(u8 *&p, int n);
int getint(u8 *&p);
void sendstring(const char *t, u8 *&p);

template <typename T> INLINE T getvec(u8 *&p) {
  T v;
  loopi(T::channelnum) v[i] = (typename T::scalar)(getint(p));
  return v;
}

static const float DMF = 16.0f;// quantizes positions
static const float DAF = 1.0f; // quantizes angles
static const float DVF = 100.0f; // quantizes velocities
} /* namespace q */

