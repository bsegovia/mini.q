/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - intrusive_list.hpp -> exposes and implements intrusive list container
 - mostly taken from https://code.google.com/p/rdestl/
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"

namespace q {
struct intrusive_list_node {
  INLINE intrusive_list_node() { next = prev = this; }
  INLINE bool in_list() const  { return this != next; }
  intrusive_list_node *next;
  intrusive_list_node *prev;
};
void append(intrusive_list_node *node, intrusive_list_node *prev);
void prepend(intrusive_list_node *node, intrusive_list_node *next);
void link(intrusive_list_node *node, intrusive_list_node *next);
void unlink(intrusive_list_node *node);

template<typename pointer, typename reference>
struct intrusive_list_iterator {
  INLINE intrusive_list_iterator(): m_node(0) {}
  INLINE intrusive_list_iterator(pointer iterNode) : m_node(iterNode) {}
  INLINE reference operator*() const {
    GBE_assert(m_node);
    return *m_node;
  }
  INLINE pointer operator->() const { return m_node; }
  INLINE pointer node() const { return m_node; }
  INLINE intrusive_list_iterator& operator++() {
    m_node = static_cast<pointer>(m_node->next);
    return *this;
  }
  INLINE intrusive_list_iterator& operator--() {
    m_node = static_cast<pointer>(m_node->prev);
    return *this;
  }
  INLINE intrusive_list_iterator operator++(int) {
    intrusive_list_iterator copy(*this);
    ++(*this);
    return copy;
  }
  INLINE intrusive_list_iterator operator--(int) {
    intrusive_list_iterator copy(*this);
    --(*this);
    return copy;
  }
  INLINE bool operator== (const intrusive_list_iterator& rhs) const {
    return rhs.m_node == m_node;
  }
  INLINE bool operator!= (const intrusive_list_iterator& rhs) const {
    return !(rhs == *this);
  }
private:
  pointer m_node;
};

struct intrusive_list_base {
public:
  typedef size_t size_type;
  INLINE void pop_back() { unlink(m_root.prev); }
  INLINE void pop_front() { unlink(m_root.next); }
  INLINE bool empty() const  { return !m_root.in_list(); }
  size_type size() const;
protected:
  intrusive_list_base();
  INLINE ~intrusive_list_base() {}
  intrusive_list_node m_root;
private:
  intrusive_list_base(const intrusive_list_base&);
  intrusive_list_base& operator=(const intrusive_list_base&);
};

template<class T> struct intrusive_list : public intrusive_list_base {
  typedef T node_type;
  typedef T value_type;
  typedef intrusive_list_iterator<T*, T&> iterator;
  typedef intrusive_list_iterator<const T*, const T&> const_iterator;

  intrusive_list() : intrusive_list_base() {
    intrusive_list_node* testNode((T*)0);
    static_cast<void>(sizeof(testNode));
  }

  void push_back(value_type* v) { link(v, &m_root); }
  void push_front(value_type* v) { link(v, m_root.next); }

  iterator begin()  { return iterator(upcast(m_root.next)); }
  iterator end()    { return iterator(upcast(&m_root)); }
  iterator rbegin() { return iterator(upcast(m_root.prev)); }
  iterator rend()   { return iterator(upcast(&m_root)); }
  const_iterator begin() const  { return const_iterator(upcast(m_root.next)); }
  const_iterator end() const    { return const_iterator(upcast(&m_root)); }
  const_iterator rbegin() const { return const_iterator(upcast(m_root.prev)); }
  const_iterator rend() const   { return const_iterator(upcast(&m_root)); }

  INLINE value_type *front() { return upcast(m_root.next); }
  INLINE value_type *back()  { return upcast(m_root.prev); }
  INLINE const value_type *front() const { return upcast(m_root.next); }
  INLINE const value_type *back() const  { return upcast(m_root.prev); }

  iterator insert(iterator pos, value_type* v) {
    link(v, pos.node());
    return iterator(v);
  }
  iterator erase(iterator it) {
    iterator it_erase(it);
    ++it;
    unlink(it_erase.node());
    return it;
  }
  iterator erase(iterator first, iterator last) {
    while (first != last) first = erase(first);
    return first;
  }

  INLINE void clear() { erase(begin(), end()); }
  INLINE void fastclear() { m_root.next = m_root.prev = &m_root; }
  static void remove(value_type* v) { unlink(v); }
private:
  static INLINE node_type* upcast(intrusive_list_node* n) {
    return static_cast<node_type*>(n);
  }
  static INLINE const node_type* upcast(const intrusive_list_node* n) {
    return static_cast<const node_type*>(n);
  }
};
} /* namespace q */

