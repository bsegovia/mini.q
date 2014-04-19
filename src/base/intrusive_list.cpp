/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - intrusive_list.cpp -> implements shared intrusive_list routines
 -------------------------------------------------------------------------*/
#include "intrusive_list.hpp"

namespace q {
intrusive_list_base::intrusive_list_base(void) : m_root() {}
intrusive_list_base::size_type intrusive_list_base::size(void) const {
  size_type numnodes(0);
  const intrusive_list_node* iter = &m_root;
  do {
    iter = iter->next;
    ++numnodes;
  } while (iter != &m_root);
  return numnodes - 1;
}
void append(intrusive_list_node *node, intrusive_list_node *prev) {
  assert(!node->in_list());
  node->next = prev->next;
  node->next->prev = node;
  prev->next = node;
  node->prev = prev;
}
void prepend(intrusive_list_node *node, intrusive_list_node *next) {
  assert(!node->in_list());
  node->prev = next->prev;
  node->prev->next = node;
  next->prev = node;
  node->next = next;
}
void link(intrusive_list_node* node, intrusive_list_node* nextNode) {
  prepend(node, nextNode);
}
void unlink(intrusive_list_node* node) {
  assert(node->in_list());
  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->next = node->prev = node;
}
} /* namespace q */

