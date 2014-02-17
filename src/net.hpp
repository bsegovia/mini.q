#include "sys.hpp"

namespace q {
namespace net {

// ipv4 address
struct address {
  INLINE address() {}
  address(u32 addr, u16 port);
  address(const char *addr, u16 port);
  u32 m_ip;   // ip in host order
  u16 m_port; // port in host order
};

// to initialize a local server and channel
static const struct local_type {local_type(){}} local;

// two way communication between host and remote
struct channel {
  static channel *create(local_type);
  static channel *create(const address &addr, u32 maxtimeout);
  static void destroy(channel*);
  void atomic();
  void send(bool reliable, const char *fmt, ...);
  int rcv(const char *fmt, ...);
  void flush();
};

// handle multiple incoming connections. there is no explicit disconnection but
// simply time out notifications
struct server {
  static server *create(local_type);
  static server *create(u32 maxclients, u32 maxtimeout, u16 port);
  static void destroy(server*);
  channel *active();
  channel *timedout();
};

void start();
void end();

} /* namespace net */
} /* namespace q */

