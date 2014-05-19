/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - list.cpp -> implements list container
 - mostly taken from https://code.google.com/p/qstl/
 -------------------------------------------------------------------------*/
#include "list.hpp"

namespace q {
namespace internal {
void list_base_node::link_before(list_base_node* nextnode) {
  assert(!in_list());
  prev = nextnode->prev;
  prev->next = this;
  nextnode->prev = this;
  next = nextnode;
}
void list_base_node::unlink() {
  assert(in_list());
  prev->next = next;
  next->prev = prev;
  next = prev = this;
}
} /* namespace internal */
} /* namespace q */

