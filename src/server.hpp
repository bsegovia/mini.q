/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - server.hpp -> exposes server specific code
 -------------------------------------------------------------------------*/
#pragma once
#include "entities.hpp"

typedef struct _ENetPacket ENetPacket;

namespace q {
namespace server {

void init(bool dedicated, int uprate, const char *sdesc, const char *ip, const char *master, const char *passwd, int maxcl);
void clean(void);
void localconnect(void);
void localdisconnect(void);
void localclienttoserver(struct _ENetPacket *);
void slice(int seconds, unsigned int timeout);
void startintermission(void);
void restoreserverstate(vector<game::entity> &ents);
u8 *retrieveservers(u8 *buf, int buflen);
void serverms(int mode, int numplayers, int minremain, char *smapname, int seconds, bool isfull);
void servermsinit(const char *master, const char *sdesc, bool listen);
void sendmaps(int n, const char *mapname, int mapsize, u8 *mapdata);
ENetPacket *recvmap(int n);

} /* namespace server */
} /* namespace q */

#define sgetstr() do { \
char *t = text; \
do { \
  *t = getint(p); \
} while(*t++); \
} while (0) // used by networking

