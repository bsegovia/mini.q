/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - server.hpp -> implements server specific code
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include <enet/enet.h>
#include <time.h>

namespace q {
namespace server {

enum { ST_EMPTY, ST_LOCAL, ST_TCPIP };

struct drawarray { // server side version of "dynent" type
  int type;
  ENetPeer *peer;
  string hostname;
  string mapvote;
  string name;
  int modevote;
};

static vector<drawarray> clients;
static int maxclients = 8;
static string smapname;

struct server_entity { // server side version of "entity" type
  bool spawned;
  int spawnsecs;
};

static vector<server_entity> sents;
static bool notgotitems = true; // true when map has changed and waiting for clients to send item
static int mode = 0;

void restoreserverstate(vector<game::entity> &ents) { // hack: called from savegame code, only works in SP
  loopv(sents) {
    sents[i].spawned = ents[i].spawned;
    sents[i].spawnsecs = 0;
  }
}

static int interm = 0, minremain = 0, mapend = 0;
static bool mapreload = false;
static const char *serverpassword = "";
static bool isdedicated;
static ENetHost * serverhost = NULL;
static int bsend = 0, brec = 0, laststatus = 0, lastsec = 0;

#define MAXOBUF 100000

void process(ENetPacket *packet, int sender);
void multicast(ENetPacket *packet, int sender);
void disconnect_client(int n, const char *reason);

void send(int n, ENetPacket *packet) {
  if (!packet) return;
  switch (clients[n].type) {
    case ST_TCPIP:
      enet_peer_send(clients[n].peer, 0, packet);
      bsend += packet->dataLength;
      break;
    case ST_LOCAL:
      client::localservertoclient(packet->data, packet->dataLength);
      break;
  }
}

void send2(bool rel, int cn, int a, int b) {
  ENetPacket *packet = enet_packet_create(NULL, 32, rel ? ENET_PACKET_FLAG_RELIABLE : 0);
  u8 *start = packet->data;
  u8 *p = start+2;
  putint(p, a);
  putint(p, b);
  *(u16 *)start = ENET_HOST_TO_NET_16(p-start);
  enet_packet_resize(packet, p-start);
  if (cn<0) process(packet, -1);
  else send(cn, packet);
  if (packet->referenceCount==0) enet_packet_destroy(packet);
};

void sendservmsg(const char *msg) {
  ENetPacket *packet = enet_packet_create(NULL, MAXDEFSTR+10, ENET_PACKET_FLAG_RELIABLE);
  u8 *start = packet->data;
  u8 *p = start+2;
  putint(p, SV_SERVMSG);
  sendstring(msg, p);
  *(u16 *)start = ENET_HOST_TO_NET_16(p-start);
  enet_packet_resize(packet, p-start);
  multicast(packet, -1);
  if (packet->referenceCount==0) enet_packet_destroy(packet);
}

void disconnect_client(int n, const char *reason) {
  printf("client::disconnecting client (%s) [%s]\n", clients[n].hostname, reason);
  enet_peer_disconnect(clients[n].peer);
  clients[n].type = ST_EMPTY;
  send2(true, -1, SV_CDIS, n);
}

void resetitems() { sents.setsize(0); notgotitems = true; };

// server side item pickup, acknowledge first client that gets it
void pickup(u32 i, int sec, int sender) {
  if (i>=(u32)sents.size()) return;
  if (sents[i].spawned) {
    sents[i].spawned = false;
    sents[i].spawnsecs = sec;
    send2(true, sender, SV_ITEMACC, i);
  }
}

void resetvotes() { loopv(clients) clients[i].mapvote[0] = 0; }

bool vote(char *map, int reqmode, int sender) {
  strcpy_s(clients[sender].mapvote, map);
  clients[sender].modevote = reqmode;
  int yes = 0, no = 0; 
  loopv(clients) if (clients[i].type!=ST_EMPTY) {
    if (clients[i].mapvote[0]) { if (strcmp(clients[i].mapvote, map)==0 && clients[i].modevote==reqmode) yes++; else no++; }
    else no++;
  }
  if (yes==1 && no==0) return true;  // single player
  sprintf_sd(msg)("%s suggests %s on map %s (set map to vote)",
    clients[sender].name, game::modestr(reqmode), map);
  sendservmsg(msg);
  if (yes/(float)(yes+no) <= 0.5f) return false;
  sendservmsg("vote passed");
  resetvotes();
  return true;
}

// server side processing of updates: does very little and most state is tracked
// client only could be extended to move more gameplay to server (at expense of
// lag)
void process(ENetPacket * packet, int sender) { // sender may be -1
  const u16 len = *(u16*) packet->data;
  if (ENET_NET_TO_HOST_16(len)!=packet->dataLength) {
    disconnect_client(sender, "packet length");
    return;
  }

  u8 *end = packet->data+packet->dataLength;
  u8 *p = packet->data+2;
  char text[MAXTRANS];
  int cn = -1, type;

  while (p<end) switch (type = getint(p)) {
    case SV_TEXT:
      sgetstr();
    break;
    case SV_INITC2S:
      sgetstr();
      strcpy_s(clients[cn].name, text);
      sgetstr();
      getint(p);
    break;
    case SV_MAPCHANGE: {
      sgetstr();
      int reqmode = getint(p);
      if (reqmode<0) reqmode = 0;
      if (smapname[0] && !mapreload && !vote(text, reqmode, sender)) return;
      mapreload = false;
      mode = reqmode;
      minremain = mode&1 ? 15 : 10;
      mapend = lastsec+minremain*60;
      interm = 0;
      strcpy_s(smapname, text);
      resetitems();
      sender = -1;
    }
    break;
    case SV_ITEMLIST: {
      int n;
      while ((n = getint(p))!=-1) if (notgotitems) {
        server_entity se = { false, 0 };
        while (sents.size()<=n) sents.add(se);
        sents[n].spawned = true;
      }
      notgotitems = false;
    }
    break;
    case SV_ITEMPICKUP: {
      const int n = getint(p);
      pickup(n, getint(p), sender);
    }
    break;
    case SV_PING:
      send2(false, cn, SV_PONG, getint(p));
    break;
    case SV_POS: {
      cn = getint(p);
      if (cn<0 || cn>=clients.size() || clients[cn].type==ST_EMPTY) {
        disconnect_client(sender, "client num");
        return;
      }
      int size = msgsizelookup(type);
      assert(size!=-1);
      loopi(size-2) getint(p);
    }
    break;
    case SV_SENDMAP: {
      sgetstr();
      int mapsize = getint(p);
      sendmaps(sender, text, mapsize, p);
    }
    return;
    case SV_RECVMAP:
      send(sender, recvmap(sender));
    return;
    case SV_EXT:   // allows for new features that require no server updates 
      for (int n = getint(p); n; n--) getint(p);
    break;
    default: {
      const int size = msgsizelookup(type);
      if (size==-1) { disconnect_client(sender, "tag type"); return; };
      loopi(size-1) getint(p);
    }
  }

  if (p>end) { disconnect_client(sender, "end of packet"); return; };
  multicast(packet, sender);
}

void send_welcome(int n) {
  auto packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
  u8 *start = packet->data;
  u8 *p = start+2;
  putint(p, SV_INITS2C);
  putint(p, n);
  putint(p, PROTOCOL_VERSION);
  putint(p, smapname[0]);
  sendstring(serverpassword, p);
  putint(p, clients.size()>maxclients);
  if (smapname[0]) {
    putint(p, SV_MAPCHANGE);
    sendstring(smapname, p);
    putint(p, mode);
    putint(p, SV_ITEMLIST);
    loopv(sents) if (sents[i].spawned) putint(p, i);
    putint(p, -1);
  }
  *(u16 *)start = ENET_HOST_TO_NET_16(p-start);
  enet_packet_resize(packet, p-start);
  send(n, packet);
}

void multicast(ENetPacket *packet, int sender) {
  loopv(clients) {
    if (i==sender) continue;
    send(i, packet);
  }
}

void localclienttoserver(ENetPacket *packet) {
  process(packet, 0);
  if (!packet->referenceCount) enet_packet_destroy (packet);
}

drawarray &addclient(void) {
  loopv(clients) if (clients[i].type==ST_EMPTY) return clients[i];
  return clients.add();
}

void checkintermission(void) {
  if (!minremain) {
    interm = lastsec+10;
    mapend = lastsec+1000;
  }
  send2(true, -1, SV_TIMEUP, minremain--);
}

void startintermission() { minremain = 0; checkintermission(); };

void resetserverifempty(void) {
  loopv(clients) if (clients[i].type!=ST_EMPTY) return;
  clients.setsize(0);
  smapname[0] = 0;
  resetvotes();
  resetitems();
  mode = 0;
  mapreload = false;
  minremain = 10;
  mapend = lastsec+minremain*60;
  interm = 0;
}

int nonlocalclients = 0;
int lastconnect = 0;

// main server update, called from cube main loop in sp, or dedicated server loop
void slice(int seconds, unsigned int timeout) {
  loopv(sents) { // spawn entities when timer reached
    if (sents[i].spawnsecs && (sents[i].spawnsecs -= seconds-lastsec)<=0) {
      sents[i].spawnsecs = 0;
      sents[i].spawned = true;
      send2(true, -1, SV_ITEMSPAWN, i);
    }
  }

  lastsec = seconds;

  if ((mode>1 || (mode==0 && nonlocalclients)) && seconds>mapend-minremain*60) checkintermission();
  if (interm && seconds>interm) {
    interm = 0;
    loopv(clients) if (clients[i].type!=ST_EMPTY) {
      send2(true, i, SV_MAPRELOAD, 0);    // ask a client to trigger map reload
      mapreload = true;
      break;
    }
  }

  resetserverifempty();

  if (!isdedicated) return;     // below is network only

  int numplayers = 0;
  loopv(clients) if (clients[i].type!=ST_EMPTY) ++numplayers;
  serverms(mode, numplayers, minremain, smapname, seconds, clients.size()>=maxclients);

  if (seconds-laststatus>60) { // display bandwidth stats, useful for server ops
    nonlocalclients = 0;
    loopv(clients) if (clients[i].type==ST_TCPIP) nonlocalclients++;
    laststatus = seconds;
    if (nonlocalclients || bsend || brec) printf("status: %d remote clients, %.1f send, %.1f rec (K/sec)\n", nonlocalclients, bsend/60.0f/1024, brec/60.0f/1024);
    bsend = brec = 0;
  }

  ENetEvent event;
  if (enet_host_service(serverhost, &event, timeout) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT: {
        drawarray &c = addclient();
        c.type = ST_TCPIP;
        c.peer = event.peer;
        c.peer->data = (void *)(&c-&clients[0]);
        char hn[1024];
        strcpy_s(c.hostname, (enet_address_get_host(&c.peer->address, hn, sizeof(hn))==0) ? hn : "localhost");
        printf("client connected (%s)\n", c.hostname);
        send_welcome(lastconnect = &c-&clients[0]);
        break;
      }
      case ENET_EVENT_TYPE_RECEIVE:
        brec += event.packet->dataLength;
        process(event.packet, (intptr_t)event.peer->data); 
        if (event.packet->referenceCount==0) enet_packet_destroy(event.packet);
      break;
      case ENET_EVENT_TYPE_DISCONNECT: 
        if ((intptr_t)event.peer->data<0) break;
        printf("client::disconnected client (%s)\n", clients[(intptr_t)event.peer->data].hostname);
        clients[(intptr_t)event.peer->data].type = ST_EMPTY;
        send2(true, -1, SV_CDIS, (intptr_t)event.peer->data);
        event.peer->data = (void *)-1;
      break;
      default: break;
    }
    if (numplayers>maxclients)
      disconnect_client(lastconnect, "maxclients reached");
  }
#if !defined(__WIN32__)
  fflush(stdout);
#endif
}

void clean(void) { if (serverhost) enet_host_destroy(serverhost); }

void localdisconnect(void) {
  loopv(clients) if (clients[i].type==ST_LOCAL) clients[i].type = ST_EMPTY;
}

void localconnect(void) {
  drawarray &c = addclient();
  c.type = ST_LOCAL;
  strcpy_s(c.hostname, "local");
  send_welcome(&c-&clients[0]); 
}

void init(bool dedicated, int uprate, const char *sdesc, const char *ip, const char *master, const char *passwd, int maxcl) {
  serverpassword = passwd;
  maxclients = maxcl;
  servermsinit(master ? master : "wouter.fov120.com/cube/masterserver/", sdesc, dedicated);

  if ((isdedicated = dedicated) != 0) {
    ENetAddress address = { ENET_HOST_ANY, CUBE_SERVER_PORT };
    if (*ip && enet_address_set_host(&address, ip)<0)
      printf("WARNING: server ip not resolved");
    serverhost = enet_host_create(&address, MAXCLIENTS, 0, uprate);
    if (!serverhost)
      sys::fatal("could not create server host\n");
    loopi(MAXCLIENTS) serverhost->peers[i].data = (void *)-1;
  }

  resetserverifempty();

  if (isdedicated) { // do not return, this becomes main loop
#if defined(__WIN32__)
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
    printf("dedicated server started, waiting for clients...\nCtrl-C to exit\n\n");
    atexit(clean);
    atexit(enet_deinitialize);
    for (;;) slice(/*enet_time_get_sec()*/time(NULL), 5);
  }
}
} /* namespace server */
} /* namespace q */

