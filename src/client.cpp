#include "mini.q.hpp"
#include <enet/enet.h>

namespace q {
namespace client {

static ENetHost *clienthost = NULL;
static int connecting = 0;
static int connattempts = 0;
static int disconnecting = 0;
static int clientnum = -1; // our client id in the game
static bool c2sinit = false; // whether we need to tell the other clients our stats
static string ctext;
static vector<ivector> messages; // collect c2s messages conveniently
static int lastupdate = 0, lastping = 0;
static string toservermap;
static bool senditemstoserver = false; // after a map change (server doesn't have map)
static string clientpassword;

int getclientnum(void) { return clientnum; }

bool multiplayer(void) {
  if (clienthost) // check not correct on listen server?
    con::out("operation not available in multiplayer");
  return clienthost!=NULL;
}

void neterr(const char *s) {
  con::out("illegal network message (%s)", s);
  disconnect();
}

bool allowedittoggle(void) {
  const bool allow = !clienthost || game::mode()==1;
  if (!allow)
    con::out("editing in multiplayer requires coopedit mode (1)");
  return allow; 
}

VARF(rate, 0, 0, 25000, 
  if (clienthost && (!rate || rate>1000))
    enet_host_bandwidth_limit (clienthost, rate, rate));

static void throttle(void);
VARF(throttle_interval, 0, 5, 30, throttle());
VARF(throttle_accel, 0, 2, 32, throttle());
VARF(throttle_decel, 0, 2, 32, throttle());

static void throttle(void) {
  if (!clienthost || connecting) return;
  assert(ENET_PEER_PACKET_THROTTLE_SCALE==32);
  enet_peer_throttle_configure(clienthost->peers,
                               throttle_interval*1000,
                               throttle_accel, throttle_decel);
}

static void newname(const char *name) {
  c2sinit = false;
  strn0cpy(game::player1->name, name, 16);
}
CMDN(name, newname, ARG_1STR);

static void newteam(const char *name) {
  c2sinit = false;
  strn0cpy(game::player1->team, name, 5);
}
CMDN(team, newteam, ARG_1STR);

void writeclientinfo(FILE *f) {
  fprintf((FILE*) f, "name \"%s\"\nteam \"%s\"\n",
    game::player1->name, game::player1->team);
}

void connect(const char *servername) {
  disconnect(1);  // reset state
  browser::addserver(servername);
  con::out("attempting to connect to %s", servername);
  ENetAddress address = {ENET_HOST_ANY, CUBE_SERVER_PORT};
  if (enet_address_set_host(&address, servername) < 0) {
    con::out("could not resolve server %s", servername);
    return;
  }

  clienthost = enet_host_create(NULL, 1, rate, rate);

  if (clienthost) {
    enet_host_connect(clienthost, &address, 1); 
    enet_host_flush(clienthost);
    connecting = game::lastmillis();
    connattempts = 0;
  } else {
    con::out("could not connect to server");
    disconnect();
  }
}
CMD(connect, ARG_1STR);

void disconnect(int onlyclean, int async) {
  if (clienthost) {
    if (!connecting && !disconnecting) {
      enet_peer_disconnect(clienthost->peers);
      enet_host_flush(clienthost);
      disconnecting = game::lastmillis();
    }
    if (clienthost->peers->state != ENET_PEER_STATE_DISCONNECTED) {
      if (async) return;
      enet_peer_reset(clienthost->peers);
    }
    enet_host_destroy(clienthost);
  }

  if (clienthost && !connecting)
    con::out("disconnected");
  clienthost = NULL;
  connecting = 0;
  connattempts = 0;
  disconnecting = 0;
  clientnum = -1;
  c2sinit = false;
  game::player1->lifesequence = 0;
  loopv(game::players)
    game::zapdynent(game::players[i]);

  server::localdisconnect();

  if (!onlyclean) {
    demo::stop();
    server::localconnect();
  }
}

void trydisconnect(void) {
  if (!clienthost) {
    con::out("not connected");
    return;
  }
  if (connecting) {
    con::out("aborting connection attempt");
    disconnect();
    return;
  }
  con::out("attempting to disconnect...");
  disconnect(0, !disconnecting);
}
CMDN(disconnect, trydisconnect, ARG_NONE);

void toserver(const char *text) {
  con::out("%s:\f %s", game::player1->name, text);
  strn0cpy(ctext, text, 80);
}
CMDN(say, toserver, ARG_VARI);

static void echo(const char *text) { con::out("%s", text); }
CMD(echo, ARG_VARI);

void addmsg(int rel, int num, int type, ...) {
  if (demo::playing()) return;
  if (num!=msgsizelookup(type)) {
    sprintf_sd(s)("inconsistant msg size for %d (%d != %d)",
      type, num, msgsizelookup(type));
    sys::fatal(s);
  }
  if (messages.length()==128) {
    con::out("command flood protection (type %d)", type);
    return;
  }
  ivector &msg = messages.add();
  msg.add(num);
  msg.add(rel);
  msg.add(type);
  va_list marker;
  va_start(marker, type);
  loopi(num-1) msg.add(va_arg(marker, int));
  va_end(marker);
}

void server_err(void) {
  con::out("server network error, disconnecting...");
  disconnect();
}

void password(const char *p) { strcpy_s(clientpassword, p); }
CMD(password, ARG_1STR);

bool netmapstart(void) {
  senditemstoserver = true;
  return clienthost!=NULL;
}

void initclientnet(void) {
  ctext[0] = 0;
  toservermap[0] = 0;
  clientpassword[0] = 0;
  newname("unnamed");
  newteam("red");
}

void sendpackettoserv(void *packet) {
  if (clienthost) {
    enet_host_broadcast(clienthost, 0, (ENetPacket *)packet);
    enet_host_flush(clienthost);
  }
  else
    server::localclienttoserver((ENetPacket *)packet);
}

void c2sinfo(const game::dynent *d) {
  // we haven't had a welcome message from the server yet
  if (clientnum<0) return;
  // do not update faster than 25fps
  if (game::lastmillis()-lastupdate<40) return;
  ENetPacket *packet = enet_packet_create (NULL, MAXTRANS, 0);
  u8 *start = packet->data;
  u8 *p = start+2;
  bool serveriteminitdone = false;
  if (toservermap[0]) { // suggest server to change map
    // do this exclusively as map change may invalidate rest of update
    packet->flags = ENET_PACKET_FLAG_RELIABLE;
    putint(p, SV_MAPCHANGE);
    sendstring(toservermap, p);
    toservermap[0] = 0;
    putint(p, game::nextmode());
  } else {
    putint(p, SV_POS);
    putint(p, clientnum);
    // quantize coordinates to 1/16th of a cube, between 1 and 3 bytes
    putint(p, (int)(d->o.x*DMF));
    putint(p, (int)(d->o.y*DMF));
    putint(p, (int)(d->o.z*DMF));
    putint(p, (int)(d->ypr.x*DAF));
    putint(p, (int)(d->ypr.y*DAF));
    putint(p, (int)(d->ypr.z*DAF));
    // quantize to 1/100, almost always 1 byte
    putint(p, (int)(d->vel.x*DVF));
    putint(p, (int)(d->vel.y*DVF));
    putint(p, (int)(d->vel.z*DVF));
    // pack rest in 1 byte: strafe:2, move:2, onfloor:1, state:3
    putint(p, (d->strafe&3) |
              ((d->move&3)<<2) |
              (((int)d->onfloor)<<4) |
              ((edit::mode() ? CS_EDITING : d->state)<<5));

    if (senditemstoserver) {
      packet->flags = ENET_PACKET_FLAG_RELIABLE;
      putint(p, SV_ITEMLIST);
      if (!m_noitems) game::putitems(p);
      putint(p, -1);
      senditemstoserver = false;
      serveriteminitdone = true;
    }
    // player chat, not flood protected for now
    if (ctext[0]) {
      packet->flags = ENET_PACKET_FLAG_RELIABLE;
      putint(p, SV_TEXT);
      sendstring(ctext, p);
      ctext[0] = 0;
    }
    // tell other clients who I am
    if (!c2sinit) {
      packet->flags = ENET_PACKET_FLAG_RELIABLE;
      c2sinit = true;
      putint(p, SV_INITC2S);
      sendstring(game::player1->name, p);
      sendstring(game::player1->team, p);
      putint(p, game::player1->lifesequence);
    }
    // send messages collected during the previous frames
    loopv(messages) {
      ivector &msg = messages[i];
      if (msg[1]) packet->flags = ENET_PACKET_FLAG_RELIABLE;
      loopi(msg[0]) putint(p, msg[i+2]);
    }
    messages.setsize(0);
    if (game::lastmillis()-lastping>250) {
      putint(p, SV_PING);
      putint(p, game::lastmillis());
      lastping = game::lastmillis();
    }
  }

  *(u16 *)start = ENET_HOST_TO_NET_16(p-start);
  enet_packet_resize(packet, p-start);
  demo::incomingdata(start, p-start, true);
  if (clienthost) {
    enet_host_broadcast(clienthost, 0, packet);
    enet_host_flush(clienthost);
  } else
    server::localclienttoserver(packet);
  lastupdate = game::lastmillis();
  if (serveriteminitdone)
    demo::loadgamerest();  // hack
}

// update the position of other clients in the game in our world don't care
// if he's in the scenery or other players, just don't overlap with our
// client
static void updatepos(game::dynent *d) {
  const float r = game::player1->radius+d->radius;
  const float dx = game::player1->o.x-d->o.x;
  const float dy = game::player1->o.y-d->o.y;
  const float dz = game::player1->o.z-d->o.z;
  const float rz = game::player1->aboveeye+d->eyeheight;
  const float fx = abs(dx);
  const float fy = abs(dy);
  const float fz = abs(dz);

  if (fx<r && fy<r && fz<rz && d->state!=CS_DEAD) {
    if (fx<fy)
      d->o.y += dy<0 ? r-fy : -(r-fy);  // push aside
    else
      d->o.x += dx<0 ? r-fx : -(r-fx);
  }
  const int lagtime = game::lastmillis()-d->lastupdate;
  if (lagtime) {
    d->plag = (d->plag*5+lagtime)/6;
    d->lastupdate = game::lastmillis();
  }
}

// process forced map change from the server
static void changemapserv(const char *name, int mode) {
  game::setmode(mode);
  // TODO world::load(name);
}

void localservertoclient(u8 *buf, int len) {
  if (ENET_NET_TO_HOST_16(*(u16 *)buf) != len)
    neterr("packet length");
  demo::incomingdata(buf, len);

  u8 *end = buf+len;
  u8 *p = buf+2;
  char text[MAXTRANS];
  int cn = -1, type;
  game::dynent *d = NULL;
  bool mapchanged = false;

  while (p<end) switch (type = getint(p)) {
    case SV_INITS2C: {
      cn = getint(p);
      const int prot = getint(p);
      if (prot!=PROTOCOL_VERSION) {
        con::out("you are using a different game protocol (you: %d, server: %d)",
                 PROTOCOL_VERSION, prot);
        disconnect();
        return;
      }
      toservermap[0] = 0;
      clientnum = cn; // we are now fully connected
      if (!getint(p)) // we are the first client on this server, set map
      strcpy_s(toservermap, game::getclientmap());
      sgetstr();
      if (text[0] && strcmp(text, clientpassword)) {
        con::out("you need to set the correct password to join this server!");
        disconnect();
        return;
      }
      if (getint(p)==1)
        con::out("server is FULL, disconnecting...");
    }
    break;
    case SV_POS: {
      cn = getint(p);
      d = game::getclient(cn);
      if (!d) return;
      d->o     = getvec<vec3f>(p)/DMF;
      d->o    += vec3f(0.5f/DMF); // avoid false intersection with ground
      d->ypr.x   = getint(p)/DAF;
      d->ypr.y = getint(p)/DAF;
      d->ypr.z  = getint(p)/DAF;
      d->vel   = getvec<vec3f>(p)/DVF;
      int f = getint(p);
      d->strafe = (f&3)==3 ? -1 : f&3;
      f >>= 2;
      d->move = (f&3)==3 ? -1 : f&3;
      d->onfloor = (f>>2)&1;
      int state = f>>3;
      if (state==CS_DEAD && d->state!=CS_DEAD) d->lastaction = game::lastmillis();
      d->state = state;
      if (!demo::playing()) updatepos(d);
    }
    break;
    case SV_SOUND:
      sound::play(getint(p), &d->o);
    break;
    case SV_TEXT:
      sgetstr();
      con::out("%s:\f %s", d->name, text);
    break;
    case SV_MAPCHANGE:
      sgetstr();
      changemapserv(text, getint(p));
      mapchanged = true;
    break;
    case SV_ITEMLIST: {
      int n;
      if (mapchanged) {
        senditemstoserver = false;
        game::resetspawns();
      }
      while ((n = getint(p))!=-1)
        if (mapchanged)
          game::setspawn(n, true);
    }
    break;
#if 0 // TODO
    case SV_MAPRELOAD: {
      getint(p);
      sprintf_sd(nextmapalias)("nextmap_%s", game::getclientmap());
      char *map = script::getalias(nextmapalias); // look up map in the cycle
      changemap(map ? map : game::getclientmap());
    }
    break;
#endif
    case SV_INITC2S: {
      sgetstr();
      if (d->name[0]) { // already connected
        if (strcmp(d->name, text))
          con::out("%s is now known as %s", d->name, text);
      } else { // new client
        c2sinit = false; // send new players my info again 
        con::out("connected: %s", text);
      }
      strcpy_s(d->name, text);
      sgetstr();
      strcpy_s(d->team, text);
      d->lifesequence = getint(p);
    }
    break;
    case SV_CDIS:
      cn = getint(p);
      if (!(d = game::getclient(cn))) break;
      con::out("player %s disconnected",
                   d->name[0] ? d->name : "[incompatible client]"); 
      game::zapdynent(game::players[cn]);
    break;
    case SV_SHOT: {
      const int gun = getint(p);
      const vec3f s = getvec<vec3f>(p)/DMF, e = getvec<vec3f>(p)/DMF;
      if (gun==game::GUN_SG) game::createrays(s, e);
      game::shootv(gun, s, e, d);
    }
    break;
    case SV_DAMAGE: {
      const int target = getint(p);
      const int damage = getint(p);
      const int ls = getint(p);
      if (target==clientnum) {
        if (ls==game::player1->lifesequence)
          game::selfdamage(damage, cn, d);
      } else
        sound::play(sound::PAIN1+rnd(5), &game::getclient(target)->o);
    }
    break;
    case SV_DIED: {
      const int actor = getint(p);
      if (actor==cn)
        con::out("%s suicided", d->name);
      else if (actor==clientnum) {
        int frags;
        if (isteam(game::player1->team, d->team)) {
          frags = -1;
          con::out("you fragged a teammate (%s)", d->name);
        } else {
          frags = 1;
          con::out("you fragged %s", d->name);
        }
        addmsg(1, 2, SV_FRAGS, game::player1->frags += frags);
      } else {
        const game::dynent * const a = game::getclient(actor);
        if (a) {
          if (isteam(a->team, d->name))
            con::out("%s fragged his teammate (%s)", a->name, d->name);
          else
            con::out("%s fragged %s", a->name, d->name);
        }
      }
      sound::play(sound::DIE1+rnd(2), &d->o);
      d->lifesequence++;
    }
    break;
    case SV_FRAGS:
      game::players[cn]->frags = getint(p);
    break;
    case SV_ITEMPICKUP:
      game::setspawn(getint(p), false);
      getint(p);
    break;
    case SV_ITEMSPAWN: {
      u32 i = getint(p);
      game::setspawn(i, true);
      if (i>=u32(game::ents.length()))
        break;
      const auto &e = game::ents[i];
      const vec3f v(float(e.x),float(e.y),float(e.z));
      sound::play(sound::ITEMSPAWN, &v); 
    }
    break;
    case SV_ITEMACC:
      game::realpickup(getint(p), game::player1);
    break;
    case SV_EDITH:
    case SV_EDITT:
    case SV_EDITS:
    case SV_EDITD:
    case SV_EDITE: {
#if 0
      const int x  = getint(p);
      const int y  = getint(p);
      const int xs = getint(p);
      const int ys = getint(p);
      const int v  = getint(p);
      block b = { x, y, xs, ys };

      switch (type) {
        case SV_EDITH: edit::editheightxy(v!=0, getint(p), b); break;
        case SV_EDITT: edit::edittexxy(v, getint(p), b); break;
        case SV_EDITS: edit::edittypexy(v, b); break;
        case SV_EDITD: edit::setvdeltaxy(v, b); break;
        case SV_EDITE: edit::editequalisexy(v!=0, b); break;
      }
#endif
    }
    break;
    // Coop edit of ent
    case SV_EDITENT: {
      u32 i = getint(p);
      while (u32(game::ents.length()) <= i)
        game::ents.add().type = game::NOTUSED;
      game::ents[i].type = getint(p);
      game::ents[i].x = getint(p);
      game::ents[i].y = getint(p);
      game::ents[i].z = getint(p);
      game::ents[i].attr1 = getint(p);
      game::ents[i].attr2 = getint(p);
      game::ents[i].attr3 = getint(p);
      game::ents[i].attr4 = getint(p);
      game::ents[i].spawned = false;
    }
    break;
    case SV_PING:
      getint(p);
    break;
    case SV_PONG: 
      addmsg(0, 2, SV_CLIENTPING, game::player1->ping =
        (game::player1->ping*5+game::lastmillis()-getint(p))/6);
    break;
    case SV_CLIENTPING:
      game::players[cn]->ping = getint(p);
    break;
    case SV_GAMEMODE:
      game::setnextmode(getint(p));
    break;
    case SV_TIMEUP:
      game::timeupdate(getint(p));
    break;
#if 0
    case SV_CUBE: {
      const auto xyz = getvec<vec3i>(p);
      const auto pos = getvec<vec3<s8>>(p);
      const u8 mat = getint(p);
      const auto tex = getvec<world::cubetex>(p);
      edit::setcube(xyz, world::brickcube(pos,mat,tex), false);
    }
    break;
    // A new map is received
    case SV_RECVMAP:
    {
      sgetstr();
      con::out("received map \"%s\" from server, reloading..", text);
      const int mapsize = getint(p);
      world::writemap(text, mapsize, p);
      p += mapsize;
      changemapserv(text, game::mode());
      break;
    }
#endif
    case SV_SERVMSG:
      sgetstr();
      con::out("%s", text);
    break;
    case SV_EXT:
      for (int n = getint(p); n; n--)
        getint(p);
    break;
    default:
      neterr("type");
    return;
  }
}

void gets2c(void) {
  ENetEvent event;
  if (!clienthost) return;
  if (connecting && game::lastmillis()/3000 > connecting/3000) {
    con::out("attempting to connect...");
    connecting = game::lastmillis();
    ++connattempts; 
    if (connattempts > 3) {
      con::out("could not connect to server");
      disconnect();
      return;
    }
  }
  while (clienthost!=NULL && enet_host_service(clienthost, &event, 0)>0)
    switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
        con::out("connected to server");
        connecting = 0;
        throttle();
      break;
      case ENET_EVENT_TYPE_RECEIVE:
        if (disconnecting)
          con::out("attempting to disconnect...");
        else
          localservertoclient(event.packet->data, event.packet->dataLength);
        enet_packet_destroy(event.packet);
      break;
      case ENET_EVENT_TYPE_DISCONNECT:
        if (disconnecting)
          disconnect();
        else
          server_err();
      return;
      default:
      break;
    }
}

void changemap(const char *name) { strcpy_s(toservermap, name); }
CMDN(map, changemap, ARG_1STR);
} /* namespace client */
} /* namespace q */

