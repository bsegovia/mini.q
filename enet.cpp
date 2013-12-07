/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - enet.cpp -> contains the complete enet stack
 -------------------------------------------------------------------------*/

#define HAS_SOCKLEN_T
#define HAS_FCNTL
#include "enet/callbacks.c"
#include "enet/host.c"
#include "enet/list.c"
#include "enet/memory.c"
#include "enet/packet.c"
#include "enet/peer.c"
#include "enet/protocol.c"
#include "enet/unix.c"

