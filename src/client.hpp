/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - client.cpp -> exposes client game routines
 -------------------------------------------------------------------------*/
#pragma once
#include "entities.hpp"
#include "base/sys.hpp"

namespace q {
namespace client {

// process any updates from the server
void localservertoclient(u8 *buf, int len);
// connect to the given server
void connect(const char *servername);
// disconned from the current server
void disconnect(int onlyclean = 0, int async = 0);
// send the given message to the server
void toserver(const char *text);
// add a new message to send to the server
void addmsg(int reliable, int n, int type, ...);
// is it a multiplayer game?
bool multiplayer(void);
// toggle edit mode (restricted by client code)
bool allowedittoggle(void);
// set the enetpacket to the server
void sendpackettoserv(void *packet);
// process (enet) events from the server
void gets2c(void);
// send updates to the server
void c2sinfo(const game::dynent *d);
// triggers disconnection when invalid message is processed
void neterr(const char *s);
// create a default profile for the client
void initclientnet(void);
// indicate if the map can be started in mp
bool netmapstart(void);
// return our client id in the game
int getclientnum(void);
// outputs name and team in the given file
void writeclientinfo(FILE*);
// request map change, server may ignore
void changemap(const char *name);

} /* namespace client */
} /* namespace q */

