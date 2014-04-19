/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - serverbrowser.cpp -> implements routines to list servers
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "enet/enet.h"
#include "base/vector.hpp"
#include <SDL2/SDL_thread.h>

namespace q {
namespace browser {

struct resolverthread {
  SDL_Thread *thread;
  char *query;
  int starttime;
  volatile bool alive;
};

struct resolverresult {
  char *query;
  ENetAddress address;
};

struct serverinfo {
  fixedstring name;
  fixedstring full;
  fixedstring map;
  fixedstring sdesc;
  int mode, numplayers, ping, protocol, minremain;
  ENetAddress address;
};

static vector<resolverthread> resolverthreads;
static vector<char *> resolverqueries;
static vector<resolverresult> resolverresults;
static SDL_mutex *resolvermutex = NULL;
static SDL_sem *resolversem = NULL;
static int resolverlimit = 1000;

static int resolverloop(void * data) {
  auto rt = (resolverthread *) data;
  while (rt->alive) {
    SDL_SemWait(resolversem);
    SDL_LockMutex(resolvermutex);
    if (resolverqueries.empty()) {
      SDL_UnlockMutex(resolvermutex);
      continue;
    }
    rt->query = resolverqueries.pop();
    rt->starttime = int(game::lastmillis());
    SDL_UnlockMutex(resolvermutex);
    ENetAddress address = {ENET_HOST_ANY, CUBE_SERVINFO_PORT};
    enet_address_set_host(&address, rt->query);
    SDL_LockMutex(resolvermutex);
    resolverresult &rr = resolverresults.add();
    rr.query = rt->query;
    rr.address = address;
    rt->query = NULL;
    rt->starttime = 0;
    SDL_UnlockMutex(resolvermutex);
  }
  return 0;
}

static void resolverinit(int threads, int limit) {
  resolverlimit = limit;
  resolversem = SDL_CreateSemaphore(0);
  resolvermutex = SDL_CreateMutex();

  while (threads > 0) {
    resolverthread &rt = resolverthreads.add();
    rt.query = NULL;
    rt.alive = true;
    rt.starttime = 0;
    rt.thread = SDL_CreateThread(resolverloop, "browser thread", &rt);
    --threads;
  }
}

static void resolverstop(resolverthread &rt, bool restart) {
  SDL_LockMutex(resolvermutex);
  int status = 0;
  rt.alive = false;
  SDL_WaitThread(rt.thread, &status);
  rt.query = NULL;
  rt.starttime = 0;
  rt.thread = NULL;
  if (restart) rt.thread = SDL_CreateThread(resolverloop, "browser thread", &rt);
  SDL_UnlockMutex(resolvermutex);
}

static void resolverclear(void) {
  SDL_LockMutex(resolvermutex);
  resolverqueries.setsize(0);
  resolverresults.setsize(0);
  while (SDL_SemTryWait(resolversem) == 0);
  loopv(resolverthreads) {
    resolverthread &rt = resolverthreads[i];
    resolverstop(rt, true);
  }
  SDL_UnlockMutex(resolvermutex);
}

static void resolverquery(char *name) {
  SDL_LockMutex(resolvermutex);
  resolverqueries.add(name);
  SDL_SemPost(resolversem);
  SDL_UnlockMutex(resolvermutex);
}

static bool resolvercheck(char **name, ENetAddress *address) {
  SDL_LockMutex(resolvermutex);
  if (!resolverresults.empty()) {
    resolverresult &rr = resolverresults.pop();
    *name = rr.query;
    *address = rr.address;
    SDL_UnlockMutex(resolvermutex);
    return true;
  }
  loopv(resolverthreads) {
    resolverthread &rt = resolverthreads[i];
      if (rt.query) {
        if (game::lastmillis() - rt.starttime > resolverlimit) {
          resolverstop(rt, true);
          *name = rt.query;
          SDL_UnlockMutex(resolvermutex);
          return true;
        }
      }
  }
  SDL_UnlockMutex(resolvermutex);
    return false;
}

static vector<serverinfo> servers;
static ENetSocket pingsock = ENET_SOCKET_NULL;
static int lastinfo = 0;

const char *getservername(int n) { return servers[n].name.c_str(); }

void addserver(const char *servername) {
  loopv(servers) if (strcmp(servers[i].name.c_str(), servername)==0) return;
  // serverinfo &si = servers.insert(0, serverinfo());
  servers.insert(0, serverinfo());
  serverinfo &si = *servers.begin();
  strcpy_s(si.name, servername);
  si.full[0] = 0;
  si.mode = 0;
  si.numplayers = 0;
  si.ping = 9999;
  si.protocol = 0;
  si.minremain = 0;
  si.map[0] = 0;
  si.sdesc[0] = 0;
  si.address.host = ENET_HOST_ANY;
  si.address.port = CUBE_SERVINFO_PORT;
}
CMD(addserver, ARG_1STR);

static void pingservers(void) {
  ENetBuffer buf;
  u8 ping[MAXTRANS];
  u8 *p;
  loopv(servers) {
    serverinfo &si = servers[i];
    if (si.address.host == ENET_HOST_ANY) continue;
    p = ping;
    putint(p, int(game::lastmillis()));
    buf.data = ping;
    buf.dataLength = p - ping;
    enet_socket_send(pingsock, &si.address, &buf, 1);
  }
  lastinfo = int(game::lastmillis());
}

static void checkresolver(void) {
  char *name = NULL;
  ENetAddress addr = {ENET_HOST_ANY, CUBE_SERVINFO_PORT};
  while (resolvercheck(&name, &addr)) {
    if (addr.host == ENET_HOST_ANY) continue;
    loopv(servers) {
      serverinfo &si = servers[i];
      if (name == si.name.c_str()) {
        si.address = addr;
        addr.host = ENET_HOST_ANY;
        break;
      }
    }
  }
}

static void checkpings(void) {
  enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
  ENetBuffer buf;
  ENetAddress addr;
  u8 ping[MAXTRANS], *p;
  char text[MAXTRANS];
  buf.data = ping;
  buf.dataLength = sizeof(ping);
  while (enet_socket_wait(pingsock, &events, 0) >= 0 && events) {
    if (enet_socket_receive(pingsock, &addr, &buf, 1) <= 0) return;
    loopv(servers) {
      serverinfo &si = servers[i];
      if (addr.host == si.address.host) {
        p = ping;
        si.ping = int(game::lastmillis()) - getint(p);
        si.protocol = getint(p);
        if (si.protocol!=PROTOCOL_VERSION) si.ping = 9998;
        si.mode = getint(p);
        si.numplayers = getint(p);
        si.minremain = getint(p);
        sgetstr();
        strcpy_s(si.map, text);
        sgetstr();
        strcpy_s(si.sdesc, text);
        break;
      }
    }
  }
}

int sicompare(const serverinfo &a, const serverinfo &b) {
  return a.ping>b.ping ? 1 : (a.ping<b.ping ? -1 : strcmp(a.name.c_str(), b.name.c_str()));
}

void refreshservers(void) {
  checkresolver();
  checkpings();
  if (game::lastmillis() - lastinfo >= 5000) pingservers();
  servers.sort((void*)sicompare);
  int maxmenu = 16;
  loopv(servers) {
    serverinfo &si = servers[i];
    if (si.address.host != ENET_HOST_ANY && si.ping != 9999) {
      if (si.protocol!=PROTOCOL_VERSION)
        sprintf_s(si.full)("%s [different cube protocol]", si.name.c_str());
      else
        sprintf_s(si.full)("%d\t%d\t%s, %s: %s %s",
          si.ping, si.numplayers, si.map[0] ? si.map.c_str() : "[unknown]",
          game::modestr(si.mode),
          si.name.c_str(), si.sdesc.c_str());
    } else
      sprintf_s(si.full)(si.address.host != ENET_HOST_ANY ?
        "%s [waiting for server response]" :
        "%s [unknown host]\t", si.name.c_str());
    si.full[50] = 0; // cut off too long server descriptions
    menu::manual(1, i, si.full.c_str());
    if (!--maxmenu) return;
  }
}

static void servermenu(void) {
  if (pingsock == ENET_SOCKET_NULL) {
    pingsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM, NULL);
    resolverinit(1, 1000);
  }
  resolverclear();
  loopv(servers) resolverquery(servers[i].name.c_str());
  refreshservers();
  menu::set(1);
}
CMD(servermenu, ARG_NONE);

static void updatefrommaster(void) {
  const int MAXUPD = 32000;
  u8 buf[MAXUPD];
  auto reply = (char*) server::retrieveservers(buf, MAXUPD);
  if (!*reply ||
      strstr((char *)reply, "<html>") ||
      strstr((char *)reply, "<HTML>"))
    con::out("master server not replying");
  else {
    servers.setsize(0);
    script::execstring(reply);
  }
  servermenu();
}

void writeservercfg(void) {
  FILE *f = fopen("servers.cfg", "w");
  if (!f) return;
  fprintf(f, "// servers connected to are added here automatically\n\n");
  loopvrev(servers) fprintf(f, "addserver %s\n", servers[i].name.c_str());
  fclose(f);
}
CMD(updatefrommaster, ARG_NONE);
} /* namespace browser */
} /* namespace q */

