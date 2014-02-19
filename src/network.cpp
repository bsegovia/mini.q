/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - network.cpp -> implements network code shared by client and server
 -------------------------------------------------------------------------*/
#include "network.hpp"
#include "sys.hpp"

// all network traffic is in 32bit ints, which are then compressed using the
// following simple scheme (assumes that most values are small).
namespace q {
// size inclusive message token, 0 for variable or not-checked sizes
static const char msgsizesl[] = {
  SV_INITS2C, 4, SV_INITC2S, 0, SV_POS, 12, SV_TEXT, 0, SV_SOUND, 2, SV_CDIS, 2,
  SV_EDITH, 7, SV_EDITT, 7, SV_EDITS, 6, SV_EDITD, 6, SV_EDITE, 6,
  SV_DIED, 2, SV_DAMAGE, 4, SV_SHOT, 8, SV_FRAGS, 2,
  SV_MAPCHANGE, 0, SV_ITEMSPAWN, 2, SV_ITEMPICKUP, 3, SV_DENIED, 2,
  SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2, SV_GAMEMODE, 2,
  SV_TIMEUP, 2, SV_EDITENT, 10, SV_MAPRELOAD, 2, SV_ITEMACC, 2,
  SV_SENDMAP, 0, SV_RECVMAP, 1, SV_SERVMSG, 0, SV_ITEMLIST, 0,
  SV_EXT, 0, SV_CUBE, 14,
  -1
};

char msgsizelookup(int msg) {
  for (const char *p = msgsizesl; *p>=0; p += 2) if (*p==msg) return p[1];
  return -1;
}

void putint(u8 *&p, int n) {
  if (n<128 && n>-127) { *p++ = n; }
  else if (n<0x8000 && n>=-0x8000) { *p++ = 0x80; *p++ = n; *p++ = n>>8;  }
  else { *p++ = 0x81; *p++ = n; *p++ = n>>8; *p++ = n>>16; *p++ = n>>24; };
}

int getint(u8 *&p) {
  int c = *((char *)p);
  p++;
  if (c==-128) { int n = *p++; n |= *((char *)p)<<8; p++; return n;}
  else if (c==-127) { int n = *p++; n |= *p++<<8; n |= *p++<<16; return n|(*p++<<24); } 
  else return c;
}

void sendstring(const char *t, u8 *&p) {
  while (*t) putint(p, *t++);
  putint(p, 0);
}
} /* namespace q */

