/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - serverutil.cpp -> helper functions and main entry for server
 -------------------------------------------------------------------------*/
#include "client.hpp"
#include "server.hpp"
#include "network.hpp"
#include "enet/enet.h"

namespace q {
#if defined(STANDALONE)
void finish() {}
#endif
namespace server {

static fixedstring copyname;
static int copysize;
static u8 *copydata = NULL;

void sendmaps(int n, const char *mapname, int mapsize, u8 *mapdata) {
  if (mapsize <= 0 || mapsize > 256*256) return;
  strcpy_s(copyname, mapname);
  copysize = mapsize;
  if (copydata) FREE(copydata);
  copydata = (u8*)MALLOC(mapsize);
  memcpy(copydata, mapdata, mapsize);
}

ENetPacket *recvmap(int n) {
  if (!copydata) return NULL;
  ENetPacket *packet = enet_packet_create(NULL, MAXTRANS + copysize, ENET_PACKET_FLAG_RELIABLE);
  u8 *start = packet->data;
  u8 *p = start+2;
  putint(p, SV_RECVMAP);
  sendstring(copyname, p);
  putint(p, copysize);
  memcpy(p, copydata, copysize);
  p += copysize;
  *(u16 *)start = ENET_HOST_TO_NET_16(p-start);
  enet_packet_resize(packet, p-start);
  return packet;
}
} /* namespace server */

#if defined(STANDALONE)
void client::localservertoclient(u8 *buf, int len) {}
void fatal(const char *s) {
  server::clean();
  printf("servererror: %s\n", s);
  exit(EXIT_FAILURE);
}

static int main(int argc, char* argv[]) {
  int uprate = 0, maxcl = 4;
  const char *sdesc = "", *ip = "", *master = NULL, *passwd = "";

  for (int i = 1; i<argc; i++) {
    char *a = &argv[i][2];
    if (argv[i][0]=='-') switch (argv[i][1]) {
      case 'u': uprate = atoi(a); break;
      case 'n': sdesc  = a; break;
      case 'i': ip     = a; break;
      case 'm': master = a; break;
      case 'p': passwd = a; break;
      case 'c': maxcl  = atoi(a); break;
      default: printf("WARNING: unknown commandline option\n");
    }
  }
  if (enet_initialize()<0)
    fatal("unable to initialise network module");
  server::init(true, uprate, sdesc, ip, master, passwd, maxcl);
  return 0;
}
#endif
} /* namespace q */

#if defined(STANDALONE)
#include "game.cpp"
int main(int argc, char *argv[]) { return q::main(argc,argv); }
#endif

