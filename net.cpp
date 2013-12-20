/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - net.cpp -> implements a simple UDP interface
 -------------------------------------------------------------------------*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "sys.hpp"
#include "net.hpp"

namespace q {
namespace net {

static const u32 MTU = 1400;

// easy way to push data in a fixed size array
struct buffer {
  INLINE buffer(void) : m_len(0) {}
  INLINE buffer(const buffer &other) : m_len(other.m_len) {
    if (m_len!=0) memcpy(m_data, other.m_data, m_len);
  }
  buffer &operator=(const buffer &other) {
    m_len = other.m_len;
    if (m_len!=0) memcpy(m_data, other.m_data, m_len);
    return *this;
  }
  template <typename T> INLINE void append(const T &x) {
    assert(m_len + sizeof(T) <= MTU);
    (T&)m_data[m_len]=x;
    m_len+=sizeof(T);
  }
  INLINE void copy(const char *src, u32 sz) {
    assert(m_len + sz <= MTU);
    memcpy(m_data+m_len, src, sz);
    m_len+=sz;
  }
  u32 m_len;
  char m_data[MTU];
};

// very basic udp socket interface
struct socket {
  socket(u16 port = 0); // port == 0 means client (no bind operation)
  ~socket();
  int receive(buffer &buf) const;
  int receive(address &addr, buffer &buf) const;
  int send(const address &addr, const buffer &buf) const;
  int fd;
};

address::address(u32 addr, u16 port) : m_ip(addr), m_port(port) {}
address::address(const char *addr, u16 port) :
  m_ip(ntohl(inet_addr(addr))), m_port(port) {}

/*-------------------------------------------------------------------------
 - unix boiler plate
 -------------------------------------------------------------------------*/
static const int reuseaddr = 1;
static const int rcvbuffersz = 256 * 1024;
static const int sndbuffersz = 256 * 1024;

socket::socket(u16 port) : fd(::socket(PF_INET, SOCK_DGRAM, 0)) {
#define TRY(EXP) if (EXP == -1) sys::fatal("net: unable to start socket");
  TRY (fcntl(fd, F_SETFL, O_NONBLOCK | fcntl(fd, F_GETFL)));
  TRY (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseaddr, sizeof(int)));
  TRY (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&rcvbuffersz, sizeof(int)));
  TRY (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&sndbuffersz, sizeof(int)));
#undef TRY
  if (port != 0) {
    struct sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(port);
    bind(fd, (sockaddr*) &servaddr, sizeof(servaddr));
  }
}

socket::~socket() { if (fd != -1) close(fd); }

INLINE sockaddr_in makeaddress(const address &addr) {
  sockaddr_in out;
  bzero(&out, sizeof(out));
  out.sin_family = AF_INET;
  out.sin_addr.s_addr = htonl(addr.m_ip);
  out.sin_port = htons(addr.m_port);
  return out;
}

int socket::receive(address &addr, buffer &buf) const {
  const auto outaddr = makeaddress(addr);
  socklen_t addrlen = sizeof(outaddr);
  const int flags = MSG_NOSIGNAL;
  buf.m_len = recvfrom(fd, buf.m_data, MTU, flags, (sockaddr*)&outaddr, &addrlen);
  if (buf.m_len != u32(-1)) {
    addr.m_ip = ntohl(outaddr.sin_addr.s_addr);
    addr.m_port = ntohs(outaddr.sin_port);
    return buf.m_len;
  }
  if (errno != EWOULDBLOCK) sys::fatal("net: error receiving data on socket");
  return buf.m_len = 0;
}

int socket::receive(buffer &buf) const {
  const int flags = MSG_NOSIGNAL;
  buf.m_len = recvfrom(fd, buf.m_data, MTU, flags, NULL, NULL);
  if (buf.m_len != u32(-1)) return buf.m_len;
  if (errno != EWOULDBLOCK) sys::fatal("net: error receiving data on socket");
  return buf.m_len = 0;
}

int socket::send(const address &addr, const buffer &buf) const {
  const auto outaddr = makeaddress(addr);
  const socklen_t addrlen = sizeof(outaddr);
  const auto len = sendto(fd, buf.m_data, buf.m_len, MSG_NOSIGNAL,
    (const sockaddr*) &outaddr, addrlen);
  if (len != -1) return len;
  if (errno != EWOULDBLOCK) sys::fatal("net: error sending data on socket");
  return 0;
}

static u32 seed = 0;
static u64 basetime;
static u64 gettime64() {
  struct timeval timeval;
  gettimeofday (&timeval, NULL);
  return timeval.tv_sec * 1000ull + timeval.tv_usec / 1000ull;
}
void start() {
  basetime = gettime64();
  seed = time(NULL);
  srand(seed);
}
void end() {}
static u32 gettime() { return u32(gettime64() - basetime); }

static void send(const char *fmt, vector<char> &msg, va_list args) {
  while (char ch = *fmt) {
    switch (ch) {
#define SEND(CHAR, TYPE, VA_ARG_TYPE) case CHAR: {\
  union { TYPE x; char bytes[sizeof(TYPE)]; };\
  x = TYPE(va_arg(args, VA_ARG_TYPE));\
  loopi(sizeof(TYPE)) msg.add(bytes[i]);\
  break;\
}
      SEND('c', char, int)
      SEND('s', short, int)
      SEND('i', int, int)
#undef SEND
    }
    ++fmt;
  }
}

static u32 rcv(const char *fmt, const char *msg, u32 &offset, u32 sz, va_list args) {
  const u32 initial_offset = offset;
  while (char ch = *fmt) {
    switch (ch) {
#define RCV(CHAR, TYPE) case CHAR: {\
  const auto dst = va_arg(args, char*);\
  if (offset+sizeof(TYPE)>sz) return 0;\
  loopi(sizeof(TYPE)) dst[i] = msg[offset++];\
  break;\
}
      RCV('c', char)
      RCV('s', short)
      RCV('i', int)
#undef RCV
      default: sys::fatal("net: unknown format string");
    }
    ++fmt;
  }
  return offset - initial_offset;
}

/*-------------------------------------------------------------------------
 - implements network protocol
 -------------------------------------------------------------------------*/
struct internal_header;
struct internal_server;
struct internal_channel;

struct internal_header {
  static INLINE const internal_header &get(const buffer&);
  INLINE u32 rel() const { return ntohl(m_seq_rel) >> 31; }
  INLINE u32 seq() const { return ntohl(m_seq_rel) & 0x7fffffff; }
  INLINE u32 ackrel() const { return ntohl(m_ackseq_ackrel) >> 31; }
  INLINE u32 ackseq() const { return ntohl(m_ackseq_ackrel) & 0x7fffffff; }
  u32 m_seq_rel;       // 31 bits for sequence. 1 bit to know if reliable or not
  u32 m_ackseq_ackrel; // 31 bits to acknowledge sequence. 1 bit for reliable ack
  u16 m_qport;         // handles port renaming by routers
};
static const u32 HEADER_SZ = sizeof(u32[2]) + sizeof(u16);
static const u32 MAX_MSG_SZ = MTU - HEADER_SZ;

struct internal_channel {
  internal_channel(u32 ip, u16 port, u16 qport, internal_server *owner);
  internal_channel(u32 ip, u16 port, u16 qport, u32 maxtimeout);
  ~internal_channel();
  void init(u32 ip, u16 port, u16 qport);
  bool istimedout(u32 curtime = gettime());
  void atomic();
  void send(bool reliable, const char *fmt, va_list args);
  int rcv(const char *fmt, va_list args);
  void flush();
  void flush(buffer &buf, const address &dstaddr, bool reliable);
  void remove_sent_reliable();
  bool handle_reception(const buffer &buf);
  bool is_client_channel() const { return m_owner == NULL; }
  /* internal boilerplate */
  internal_server *m_owner; // set only for server->client communication
  socket *m_sock;           // owned by the server for a server channel

  /* incoming message */
  buffer m_incoming;     // message to process
  u32 m_incoming_offset; // offset where to pull data from msg

  /* outgoing messages (reliable and unreliable) */
  enum { REL, UNREL, MSG_NUM };
  vector<char> m_msg[MSG_NUM]; // current reliable and unreliable messages
  vector<u32> m_size[MSG_NUM]; // size of each message inside m_msg
  u32 m_rel_sent_size;         // what we sent to remote (if 0, we can send a new rel)
  u32 m_atomic_size[MSG_NUM];  // current size of the atomic group
  u32 m_atomic;                // group several sends in one atomic entity

  /* connection information and status */
  u32 m_maxtimeout;   // millis after which we consider channel is dead
  u32 m_ip;           // ip of the remote machine
  u16 m_port;         // actual port of the remote machine
  u16 m_qport;        // qport of the remote to fight port renaming by routers
  u32 m_seq:31;       // sequence number for the next packet to send to remote
  u32 m_ack:1;        // ack bit that remote needs to match
  u32 m_rel_seq:31;   // sequence number of the last rel msg we sent
  u32 m_resend_rel:1; // if 1, we need to resend the reliable data
  u32 m_remote_seq:31;// the larger sequence number we got from this remote machine
  u32 m_remote_ack:1; // last reliable ack we got from the remote machine
  u32 m_remote_update;// time of last message from remote
};

struct internal_server {
  internal_server(u32 maxclients, u32 timedout, u16 port);
  void remove(internal_channel*);
  internal_channel *active();
  internal_channel *timedout();
  internal_channel *findchannel(u32 ip, u16 qport) const;
  vector<internal_channel*> m_channels;
  socket m_sock;
  u32 m_maxclients;
  u32 m_maxtimeout;
};

INLINE const internal_header &internal_header::get(const buffer &buf) {
  assert(buf.m_len >= HEADER_SZ);
  return (const internal_header&) buf.m_data[0];
}

static void chan_make_header(internal_channel *c, buffer &buf, bool rel) {
  buf.append(htonl(c->m_seq++ | ((rel?1:0)<<31)));
  buf.append(htonl(c->m_remote_seq | (c->m_remote_ack<<31)));
  buf.append(c->m_qport);
}

internal_channel::internal_channel(u32 ip, u16 port, u16 qport, internal_server *owner) :
  m_owner(owner), m_sock(&owner->m_sock), m_maxtimeout(owner->m_maxtimeout)
{init(ip, port, qport);}

internal_channel::internal_channel(u32 ip, u16 port, u16 qport, u32 maxtimeout) :
  m_owner(NULL), m_sock(new socket), m_maxtimeout(maxtimeout)
{init(ip, port, qport);}

void internal_channel::init(u32 ip, u16 port, u16 qport) {
  m_incoming_offset = 0;
  m_ip = ip;
  m_port = port;
  m_qport = qport;
  m_seq = 1;
  m_ack = 0;
  m_resend_rel = m_rel_seq = 0;
  m_remote_seq = m_remote_ack = 0;
  m_remote_update = gettime();
  m_rel_sent_size = 0;
  m_atomic_size[REL] = m_atomic_size[UNREL] = m_atomic = 0;
}

internal_channel::~internal_channel() {
  if (m_owner != NULL)
    m_owner->remove(this);
  else
    delete m_sock;
}

void internal_channel::atomic() {
  m_atomic ^= 1;
  if (m_atomic) return;
  loopi(MSG_NUM) {
    if (m_atomic_size[i] == 0) continue;
    if (m_atomic_size[i] > MAX_MSG_SZ) sys::fatal("net: message is too large");
    m_size[i].add(m_atomic_size[i]);
    m_atomic_size[i] = 0;
  }
}

bool internal_channel::istimedout(u32 curtime) {
  return curtime-m_remote_update > m_maxtimeout;
}

bool internal_channel::handle_reception(const buffer &buf) {
  const auto &header = internal_header::get(buf);
  if (istimedout() || header.seq()<=m_remote_seq)
    return false;
  m_incoming = buf;
  m_incoming_offset = HEADER_SZ;

  // update the information about remote
  if (header.rel()) m_remote_ack ^= 1;
  m_remote_seq = header.seq();
  m_remote_update = gettime();

  // look if remote got the latest reliable command; if so, we remove the
  // reliable data we sent since we do not need it anymore. Otherwise, we
  // resend the data we sent previously and possibly more if we can (i.e. we
  // also append the reliable data we got at the same time)
  if (header.ackseq() >= m_rel_seq) {
    m_rel_sent_size = 0;
    if (header.ackrel() == m_ack) remove_sent_reliable();
  }
  return true;
}

void internal_channel::remove_sent_reliable() {
  auto &rel = m_msg[REL];
  auto &relsize = m_size[REL];

  // scan and find the first item we did not send
  u32 tot = 0, idx = 0;
  for (auto r : rel) {
    if (tot + r > m_rel_sent_size) break;
    else {
      tot += r;
      ++idx;
    }
  }
  assert(tot == m_rel_sent_size);
  m_rel_sent_size = 0;

  // remove reliable data from the left of the stream
  const auto len = rel.length() - tot;
  if (len) memcpy(&rel[0], &rel[tot], len);
  rel.setsize(rel.length()-tot);

  // only keep the length of the items we still need to send
  const auto szlen = relsize.length()-idx;
  if (szlen) memcpy(&relsize[0], &relsize[idx], szlen*sizeof(u32));
  relsize.setsize(szlen);
}

void internal_channel::send(bool reliable, const char *fmt, va_list args) {
  auto &msg = m_msg[reliable?REL:UNREL];
  const auto offset = msg.length();
  net::send(fmt, msg, args);
  const auto idx = reliable?REL:UNREL;
  const auto len = msg.length()-int(offset);
  if (m_atomic)
    m_atomic_size[idx] += len;
  else if (len > int(MAX_MSG_SZ))
    sys::fatal("net: message is too large");
  else if (len > 0)
    m_size[idx].add(len);
}

int internal_channel::rcv(const char *fmt, va_list args) {
  // are we already done with this channel? if so, try to get something from the
  // socket
  if (m_incoming.m_len <= m_incoming_offset) {
    buffer buf;
    if (!is_client_channel()) return 0; // socket is handled by server
    if (m_sock->receive(buf)<= 0 || !handle_reception(buf)) return 0;
  }
  const char *msg = m_incoming.m_data;
  const u32 len = m_incoming.m_len;
  return net::rcv(fmt, msg, m_incoming_offset, len, args);
}

void internal_channel::flush(buffer &buf, const address &dstaddr, bool reliable) {
  auto &msg = m_msg[reliable?REL:UNREL];
  auto &msgsize = m_size[reliable?REL:UNREL];

  assert(buf.m_len == 0 || !reliable);
  u32 tot = 0, offset = 0;
  loopv(msgsize) {
    u32 next = 0;
    if (buf.m_len + tot == 0) chan_make_header(this, buf, reliable);
    if (i != msgsize.length()-1) next = msgsize[i+1];
    tot += msgsize[i];
    m_rel_sent_size += reliable ? msgsize[i] : 0;
    if (buf.m_len + tot + next > MTU) {
      buf.copy(&msg[offset], tot);
      m_sock->send(dstaddr, buf);
      offset += tot;
      buf.m_len = tot = 0;
      if (reliable) return;
    }
  }
  if (tot > 0) buf.copy(&msg[offset], tot);
}

void internal_channel::flush() {
  // skip it completely if channel timed out
  if (istimedout()) {
    m_incoming.m_len = 0;
    loopi(MSG_NUM) m_msg[i].setsize(0);
    loopi(MSG_NUM) m_size[i].setsize(0);
    return;
  }

  buffer buf;
  const address dstaddr(m_ip, m_port);
  if (m_rel_sent_size == 0) flush(buf, dstaddr, true);
  flush(buf, dstaddr, false);
  if (buf.m_len) m_sock->send(dstaddr, buf);
  m_msg[UNREL].setsize(0);
  m_size[UNREL].setsize(0);
}

internal_server::internal_server(u32 maxclients, u32 maxtimeout, u16 port) :
  m_sock(port), m_maxclients(maxclients), m_maxtimeout(maxtimeout) {}

internal_channel *internal_server::findchannel(u32 ip, u16 qport) const {
  for (auto c : m_channels) if (c->m_ip==ip && c->m_qport==qport) return c;
  return NULL;
}

internal_channel *internal_server::active() {
  for (;;) {
    buffer buf;
    address addr;
    const int len = m_sock.receive(addr, buf);
    if (len <= 0) break;
    const auto &header = internal_header::get(buf);

    // allocate a new channel if not already handled
    internal_channel *c;
    if ((c = findchannel(addr.m_ip, header.m_qport)) == NULL) {
      if (u32(m_channels.length()) == m_maxclients) continue;
      c = m_channels.add(new internal_channel(addr.m_ip, addr.m_port, header.m_qport, this));
    }
    if (!c->handle_reception(buf)) continue;
    return c;
  }
  return NULL;
}

internal_channel *internal_server::timedout() {
  const u32 curtime = gettime();
  for (auto c : m_channels) if (c->istimedout(curtime)) return c;
  return NULL;
}

void internal_server::remove(internal_channel *chan) {
  u32 idx = 0, n = u32(m_channels.length());
  for (; idx < n; ++idx) if (chan == m_channels[idx]) break;
  if (idx == n) return;
  m_channels[idx] = m_channels[n-1];
  m_channels.setsize(n-1);
}

/*-------------------------------------------------------------------------
 - local interface
 -------------------------------------------------------------------------*/
static server  * const local_server = (server*) 0x1;
static channel * const local_s2c = (channel*) 0x2;
static channel * const local_c2s = (channel*) 0x3;
static vector<char> s2c, c2s;
static u32 s2c_offset = 0, c2s_offset = 0;
static const u32 compacting_size = 1024*1024;

static u32 localrcv(const char *fmt, vector<char> &msg, u32 &offset, va_list args) {
  const auto read = rcv(fmt, &msg[0], offset, msg.length(), args);
  if (offset >= compacting_size) {
    const u32 compacted_sz = msg.length()-offset;
    vector<char> compacted(compacted_sz);
    memcpy(&compacted[0], &msg[offset], compacted_sz);
    compacted.moveto(msg);
    offset = 0;
  }
  return read;
}

static INLINE bool islocal(const channel *c) {
  return c == local_s2c || c == local_c2s;
}

/*-------------------------------------------------------------------------
 - public interface
 -------------------------------------------------------------------------*/
#define CAST(TYPE, X) ((internal_##TYPE *)X)

/* channel interface */
channel *channel::create(local_type) { return local_c2s; }
channel *channel::create(const address &addr, u32 maxtimeout) {
  const u32 qport = rand()&0xffff;
  return (channel*) (new internal_channel(addr.m_ip, addr.m_port, qport, maxtimeout));
}
void channel::destroy(channel *c) { if (!islocal(c)) delete CAST(channel, c); }
void channel::send(bool reliable, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  if (this == local_s2c)
    net::send(fmt, s2c, args);
  else if (this == local_c2s)
    net::send(fmt, c2s, args);
  else
    CAST(channel,this)->send(reliable, fmt, args);
  va_end(args);
}
void channel::atomic() { if (!islocal(this)) CAST(channel,this)->atomic(); }
int channel::rcv(const char *fmt, ...) {
  int len;
  va_list args;
  va_start(args, fmt);
  if (this == local_s2c)
    len = localrcv(fmt, c2s, c2s_offset, args);
  else if (this == local_c2s)
    len = localrcv(fmt, s2c, s2c_offset, args);
  else
    len = CAST(channel,this)->rcv(fmt, args);
  va_end(args);
  return len;
}
void channel::flush() { if (!islocal(this)) CAST(channel,this)->flush(); }

/* server interface */
server *server::create(local_type) { return local_server; }
server *server::create(u32 maxclients, u32 maxtimeout, u16 port) {
  return (server*) (new internal_server(maxclients, maxtimeout, port));
}
void server::destroy(server *s) {
  if (s != local_server) delete CAST(server,s);
}
channel *server::active() {
  if (this == local_server)
    return u32(c2s.length()) == c2s_offset ? NULL : local_s2c;
  else
    return (channel*) CAST(server,this)->active();
}
channel *server::timedout() {
  if (this == local_server)
    return NULL;
  else
    return (channel*) CAST(server,this)->timedout();
}

#undef CAST
} /* namespace net */
} /* namespace q */

