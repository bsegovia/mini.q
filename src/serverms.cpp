/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - serverms.cpp -> code to handle master server
 -------------------------------------------------------------------------*/
#include "server.hpp"
#include "network.hpp"
#include "base/sys.hpp"
#include "enet/enet.h"

namespace q {
namespace server {

static ENetSocket mssock = ENET_SOCKET_NULL;

void httpgetsend(ENetAddress &ad, const char *hostname, const char *req,
                 const char *ref, const char *agent)
{
  if (ad.host==ENET_HOST_ANY) {
    printf("looking up %s...\n", hostname);
    enet_address_set_host(&ad, hostname);
    if (ad.host==ENET_HOST_ANY) return;
  }
  if (mssock!=ENET_SOCKET_NULL) enet_socket_destroy(mssock);
  mssock = enet_socket_create(ENET_SOCKET_TYPE_STREAM, NULL);
  if (mssock==ENET_SOCKET_NULL) { printf("could not open socket\n"); return; }
  if (enet_socket_connect(mssock, &ad)<0) { printf("could not connect\n"); return; }
  ENetBuffer buf;
  fixedstring httpget(fmt, "GET %s HTTP/1.0\nHost: %s\nReferer: %s\nUser-Agent: %s\n\n", req, hostname, ref, agent);
  buf.data = httpget.c_str();
  buf.dataLength = strlen((const char*) buf.data);
  printf("sending request to %s...\n", hostname);
  enet_socket_send(mssock, NULL, &buf, 1);
}

void httpgetrecieve(ENetBuffer &buf) {
  if (mssock==ENET_SOCKET_NULL)
    return;
  enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
  if (enet_socket_wait(mssock, &events, 0) >= 0 && events) {
    int len = enet_socket_receive(mssock, NULL, &buf, 1);
    if (len<=0) {
      enet_socket_destroy(mssock);
      mssock = ENET_SOCKET_NULL;
      return;
    }
    buf.data = ((char *)buf.data)+len;
    ((char*)buf.data)[0] = 0;
    buf.dataLength -= len;
  }
}

u8 *stripheader(u8 *b) {
  char *s = strstr((char *)b, "\n\r\n");
  if (!s) s = strstr((char *)b, "\n\n");
  return s ? (u8 *)s : b;
}

static ENetAddress masterserver = { ENET_HOST_ANY, 80 };
static int updmaster = 0;
static fixedstring masterbase;
static fixedstring masterpath;
static u8 masterrep[MAXTRANS];
static ENetBuffer masterb;

void updatemasterserver(int seconds) {
  if (seconds>updmaster) { // send alive signal to masterserver every hour of uptime
    fixedstring path(fmt, "%sregister.do?action=add", masterpath.c_str());
    httpgetsend(masterserver, masterbase.c_str(), path.c_str(),
                "mini.q.server", "mini.q server");
    masterrep[0] = 0;
    masterb.data = masterrep;
    masterb.dataLength = MAXTRANS-1;
    updmaster = seconds+60*60;
  }
}

void checkmasterreply(void) {
  bool busy = mssock!=ENET_SOCKET_NULL;
  httpgetrecieve(masterb);
  if (busy && mssock==ENET_SOCKET_NULL)
    printf("masterserver reply: %s\n", stripheader(masterrep));
}

u8 *retrieveservers(u8 *buf, int buflen) {
  fixedstring path(fmt, "%sretrieve.do?item=list", masterpath.c_str());
  httpgetsend(masterserver, masterbase.c_str(), path.c_str(),
              "mini.q.server", "mini.q server");
  ENetBuffer eb;
  buf[0] = 0;
  eb.data = buf;
  eb.dataLength = buflen-1;
  while (mssock!=ENET_SOCKET_NULL) httpgetrecieve(eb);
  return stripheader(buf);
}

static ENetSocket pongsock = ENET_SOCKET_NULL;
static fixedstring serverdesc;

void serverms(int mode, int numplayers, int minremain, char *smapname, int seconds, bool isfull) {
  checkmasterreply();
  updatemasterserver(seconds);

  // reply all server info requests
  ENetBuffer buf;
  ENetAddress addr;
  u8 pong[MAXTRANS], *p;
  int len;
  enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
  buf.data = pong;
  while (enet_socket_wait(pongsock, &events, 0) >= 0 && events) {
    buf.dataLength = sizeof(pong);
    len = enet_socket_receive(pongsock, &addr, &buf, 1);
    if (len < 0) return;
    p = &pong[len];
    putint(p, PROTOCOL_VERSION);
    putint(p, mode);
    putint(p, numplayers);
    putint(p, minremain);
    fixedstring mname;
    strcpy_s(mname, isfull ? "[FULL] " : "");
    strcat_s(mname, smapname);
    sendstring(mname.c_str(), p);
    sendstring(serverdesc.c_str(), p);
    buf.dataLength = p - pong;
    enet_socket_send(pongsock, &addr, &buf, 1);
  }
}

void servermsinit(const char *master, const char *sdesc, bool listen) {
  const char *mid = strstr(master, "/");
  if (!mid) mid = master;
  strcpy_s(masterpath, mid);
  strn0cpy(masterbase.c_str(), master, mid-master+1);
  strcpy_s(serverdesc, sdesc);

  if (listen) {
    ENetAddress address = { ENET_HOST_ANY, CUBE_SERVINFO_PORT };
    pongsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM, &address);
    if (pongsock == ENET_SOCKET_NULL) sys::fatal("could not create server info socket\n");
  }
}
} /* namespace server */
} /* namespace q */

