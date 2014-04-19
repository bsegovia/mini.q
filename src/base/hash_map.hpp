/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - hash_map.hpp -> exposes hash map class (i.e. std::unordered_map like)
 -------------------------------------------------------------------------*/
#pragma once
#include "algorithm.hpp"
#include "allocator.hpp"
#include "hash.hpp"
#include "stl.hpp"

namespace q {

// TLoadFactor4 - controls hash map load. 4 means 100% load, ie. hashmap will
// grow when number of items == capacity. Default value of 6 means it grows when
// number of items == capacity * 3/2 (6/4). Higher load == tighter maps, but
// bigger risk of collisions.
template<typename TKey, typename TValue,
class THashFunc = q::hash<TKey>,
int TLoadFactor4 = 6,
class TKeyEqualFunc = q::equal_to<TKey>,
class TAllocator = q::allocator>
class hash_map
{
public:
  typedef q::pair<TKey, TValue> value_type;

private:
  struct node {
    static const hash_value_t kUnusedHash  = 0xFFFFFFFF;
    static const hash_value_t kDeletedHash = 0xFFFFFFFE;

    node(): hash(kUnusedHash) {}

    INLINE bool is_unused() const   { return hash == kUnusedHash; }
    INLINE bool is_deleted() const  { return hash == kDeletedHash; }
    INLINE bool is_occupied() const { return hash < kDeletedHash; }

    hash_value_t hash;
    value_type data;
  };
  template<typename TNodePtr, typename TPtr, typename TRef>
  class node_iterator {
    friend class hash_map;
  public:
    typedef forward_iterator_tag    iterator_category;

    explicit node_iterator(TNodePtr node, const hash_map* map)
      : m_node(node), m_map(map)
    {}
    template<typename UNodePtr, typename UPtr, typename URef>
    node_iterator(const node_iterator<UNodePtr, UPtr, URef>& rhs)
      : m_node(rhs.node()), m_map(rhs.get_map())
    {}

    TRef operator*() const {
      assert(m_node != 0);
      return m_node->data;
    }
    TPtr operator->() const { return &m_node->data; }
    INLINE TNodePtr node() const { return m_node; }

    node_iterator& operator++() {
      assert(m_node != 0);
      ++m_node;
      move_to_next_occupied_node();
      return *this;
    }
    node_iterator operator++(int) {
      node_iterator copy(*this);
      ++(*this);
      return copy;
    }

    INLINE bool operator==(const node_iterator& rhs) const {
      return rhs.m_node == m_node;
    }
    bool operator!=(const node_iterator& rhs) const {
      return !(rhs == *this);
    }

    const hash_map* get_map() const { return m_map; }
  private:
    void move_to_next_occupied_node() {
      // @todo: save nodeEnd in constructor?
      TNodePtr nodeEnd = m_map->m_nodes + m_map->bucket_count();
      for (/**/; m_node < nodeEnd; ++m_node)
        if (m_node->is_occupied())
          break;
    }
    TNodePtr m_node;
    const hash_map *m_map;
  };

public:
  typedef TKey key_type;
  typedef TValue mapped_type;
  typedef TAllocator allocator_type;
  typedef node_iterator<node*, value_type*, value_type&> iterator;
  typedef node_iterator<const node*, const value_type*, const value_type&> const_iterator;
  typedef int size_type;
  static const size_type kNodeSize = sizeof(node);
  static const size_type kInitialCapacity = 64;

  hash_map() : m_nodes(&ms_emptyNode),
    m_size(0),
    m_capacity(0),
    m_capacityMask(0),
    m_numUsed(0) {
    assert((kInitialCapacity & (kInitialCapacity - 1)) == 0); // must be power-of-two
  }
  explicit hash_map(const allocator_type& allocator) : m_nodes(&ms_emptyNode),
    m_size(0),
    m_capacity(0),
    m_capacityMask(0),
    m_numUsed(0),
    m_allocator(allocator)
  {}
  explicit hash_map(size_type initial_bucket_count,
                    const allocator_type& allocator = allocator_type())
    : m_nodes(&ms_emptyNode),
      m_size(0),
      m_capacity(0),
      m_capacityMask(0),
      m_numUsed(0),
      m_allocator(allocator)
  { reserve(initial_bucket_count); }
  hash_map(size_type initial_bucket_count, const THashFunc& hashFunc,
           const allocator_type& allocator = allocator_type())
    : m_nodes(&ms_emptyNode),
      m_size(0),
      m_capacity(0),
      m_capacityMask(0),
      m_numUsed(0),
      m_hashFunc(hashFunc),
      m_allocator(allocator)
  { reserve(initial_bucket_count); }
  hash_map(const hash_map& rhs, const allocator_type& allocator = allocator_type())
    : m_nodes(&ms_emptyNode),
      m_size(0),
      m_capacity(0),
      m_capacityMask(0),
      m_numUsed(0),
      m_allocator(allocator)
  { *this = rhs; }
  explicit hash_map(noinitializetype) {}
  ~hash_map() { delete_nodes(); }

  iterator begin() {
    iterator it(m_nodes, this);
    it.move_to_next_occupied_node();
    return it;
  }
  const_iterator begin() const {
    const_iterator it(m_nodes, this);
    it.move_to_next_occupied_node();
    return it;
  }
  iterator end()             { return iterator(m_nodes + m_capacity, this); }
  const_iterator end() const { return const_iterator(m_nodes + m_capacity, this); }

  // @note: added for compatiblity sake.
  // personally, I consider it "risky". use find/insert for more
  // explicit operations.
  mapped_type& operator[](const key_type& key) {
    hash_value_t hash;
    node* n = find_for_insert(key, &hash);
    if (n == 0 || !n->is_occupied())
      return insert_at(value_type(key, TValue()), n, hash).first->second;
    return n->data.second;
  }
  // @note: does not copy allocator.
  hash_map& operator=(const hash_map &rhs) {
    assert(invariant());
    if (&rhs != this) {
      clear();
      if (m_capacity < rhs.bucket_count()) {
        delete_nodes();
        m_nodes = allocate_nodes(rhs.bucket_count());
        m_capacity = rhs.bucket_count();
        m_capacityMask = m_capacity - 1;
      }
      rehash(m_capacity, m_nodes, rhs.m_capacity, rhs.m_nodes, false);
      m_size = rhs.size();
      m_numUsed = rhs.m_numUsed;
    }
    assert(invariant());
    return *this;
  }
  void swap(hash_map& rhs) {
    if (&rhs != this) {
      assert(invariant());
      assert(m_allocator == rhs.m_allocator);
      q::swap(m_nodes, rhs.m_nodes);
      q::swap(m_size, rhs.m_size);
      q::swap(m_capacity, rhs.m_capacity);
      q::swap(m_capacityMask, rhs.m_capacityMask);
      q::swap(m_numUsed, rhs.m_numUsed);
      q::swap(m_hashFunc, rhs.m_hashFunc);
      q::swap(m_keyEqualFunc, rhs.m_keyEqualFunc);
      assert(invariant());
    }
  }

  q::pair<iterator, bool> insert(const value_type& v) {
    typedef q::pair<iterator, bool> ret_type_t;
    assert(invariant());
    if (m_numUsed * TLoadFactor4 >= m_capacity * 4)
      grow();

    hash_value_t hash=0;
    node *n = find_for_insert(v.first, &hash);
    if (n->is_occupied()) {
      assert(hash == n->hash && m_keyEqualFunc(v.first, n->data.first));
      return ret_type_t(iterator(n, this), false);
    }
    if (n->is_unused())
      ++m_numUsed;
    q::copy_construct(&n->data, v);
    n->hash = hash;
    ++m_size;
    assert(invariant());
    return ret_type_t(iterator(n, this), true);
  }

  size_type erase(const key_type& key) {
    node* n = lookup(key);
    if (n != (m_nodes + m_capacity) && n->is_occupied()) {
      erase_node(n);
      return 1;
    }
    return 0;
  }
  void erase(iterator it) {
    assert(it.get_map() == this);
    if (it != end()) {
      assert(!empty());
      erase_node(it.node());
    }
  }
  void erase(iterator from, iterator to) {
    for (/**/; from != to; ++from) {
      node* n = from.node();
      if (n->is_occupied())
        erase_node(n);
    }
  }

  iterator find(const key_type& key) {
    node* n = lookup(key);
    return iterator(n, this);
  }
  const_iterator find(const key_type& key) const {
    const node* n = lookup(key);
    return const_iterator(n, this);
  }

  void clear() {
    node* endNode = m_nodes + m_capacity;
    for (node* iter = m_nodes; iter != endNode; ++iter) {
      if (iter) {
        if (iter->is_occupied())
          q::destruct(&iter->data);
        // we can make them unused, because we clear whole hash_map, so we can
        // guarantee there'll be no holes.
        iter->hash = node::kUnusedHash;
      }
    }
    m_size = 0;
    m_numUsed = 0;
  }

  void reserve(size_type min_size) {
    size_type newCapacity = (m_capacity == 0 ? kInitialCapacity : m_capacity);
    while (newCapacity < min_size) newCapacity *= 2;
    if (newCapacity > m_capacity) grow(newCapacity);
  }

  size_type bucket_count() const  { return m_capacity; }
  size_type size() const          { return m_size; }
  size_type empty() const         { return size() == 0; }
  size_type nonempty_bucket_count() const { return m_numUsed; }
  size_type used_memory() const {
    return bucket_count() * kNodeSize;
  }

  const allocator_type& get_allocator() const { return m_allocator; }
  void set_allocator(const allocator_type& allocator) {
    m_allocator = allocator;
  }

private:
  void grow() {
    const int newCapacity = (m_capacity == 0 ? kInitialCapacity : m_capacity * 2);
    grow(newCapacity);
  }
  void grow(int new_capacity) {
    assert((new_capacity & (new_capacity - 1)) == 0); // must be power-of-two
    node* newNodes = allocate_nodes(new_capacity);
    rehash(new_capacity, newNodes, m_capacity, m_nodes, true);
    if (m_nodes != &ms_emptyNode)
      m_allocator.deallocate(m_nodes, sizeof(node) * m_capacity);
    m_capacity = new_capacity;
    m_capacityMask = new_capacity - 1;
    m_nodes = newNodes;
    m_numUsed = m_size;
    assert(m_numUsed < m_capacity);
  }
  q::pair<iterator, bool> insert_at(const value_type& v, node* n,
                                    hash_value_t hash) {
    assert(invariant());
    if (n == 0 || m_numUsed * TLoadFactor4 >= m_capacity * 4)
      return insert(v);

    assert(!n->is_occupied());
    if (n->is_unused())
      ++m_numUsed;
    q::copy_construct(&n->data, v);
    n->hash = hash;
    ++m_size;
    assert(invariant());
    return q::pair<iterator, bool>(iterator(n, this), true);
  }
  node* find_for_insert(const key_type& key, hash_value_t* out_hash) {
    if (m_capacity == 0)
      return 0;

    const hash_value_t hash = hash_func(key);
    *out_hash = hash;
    u32 i = hash & m_capacityMask;

    node* n = m_nodes + i;
    if (n->hash == hash && m_keyEqualFunc(key, n->data.first))
      return n;

    node* freeNode(0);
    if (n->is_deleted())
      freeNode = n;
    u32 numProbes(0);

    // guarantees loop termination
    assert(m_numUsed < m_capacity);
    while (!n->is_unused()) {
      ++numProbes;
      i = (i + numProbes) & m_capacityMask;
      n = m_nodes + i;
      if (compare_key(n, key, hash))
        return n;
      if (n->is_deleted() && freeNode == 0)
        freeNode = n;
    }
    return freeNode ? freeNode : n;
  }
  node* lookup(const key_type& key) const {
    const hash_value_t hash = hash_func(key);
    u32 i = hash & m_capacityMask;
    node* n = m_nodes + i;
    if (n->hash == hash && m_keyEqualFunc(key, n->data.first))
      return n;
    u32 numProbes(0);

    // guarantees loop termination
    assert(m_capacity == 0 || m_numUsed < m_capacity);
    while (!n->is_unused())
    {
      ++numProbes;
      i = (i + numProbes) & m_capacityMask;
      n = m_nodes + i;

      if (compare_key(n, key, hash))
        return n;
    }
    return m_nodes + m_capacity;
  }

  static void rehash(int new_capacity, node* new_nodes,
                     int capacity, const node* nodes, bool destruct_original) {
    const node* it = nodes;
    const node* itEnd = nodes + capacity;
    const u32 mask = new_capacity - 1;
    while (it != itEnd) {
      if (it->is_occupied()) {
        const hash_value_t hash = it->hash;
        u32 i = hash & mask;

        node* n = new_nodes + i;
        u32 numProbes(0);
        while (!n->is_unused()) {
          ++numProbes;
          i = (i + numProbes) & mask;
          n = new_nodes + i;
        }
        q::copy_construct(&n->data, it->data);
        n->hash = hash;
        if (destruct_original)
          q::destruct(&it->data);
      }
      ++it;
    }
  }

  node *allocate_nodes(int n) {
    node* buckets = static_cast<node*>(m_allocator.allocate(n * sizeof(node)));
    node* iterBuckets(buckets);
    node* end = iterBuckets + n;
    for (/**/; iterBuckets != end; ++iterBuckets)
      iterBuckets->hash = node::kUnusedHash;
    return buckets;
  }
  void delete_nodes() {
    node* it = m_nodes;
    node* itEnd = it + m_capacity;
    while (it != itEnd) {
      if (it && it->is_occupied())
        q::destruct(&it->data);
      ++it;
    }
    if (m_nodes != &ms_emptyNode)
      m_allocator.deallocate(m_nodes, sizeof(node) * m_capacity);

    m_capacity = 0;
    m_capacityMask = 0;
    m_size = 0;
  }
  void erase_node(node* n) {
    assert(!empty());
    assert(n->is_occupied());
    q::destruct(&n->data);
    n->hash = node::kDeletedHash;
    --m_size;
  }

  INLINE hash_value_t hash_func(const key_type& key) const {
    const hash_value_t h = m_hashFunc(key) & 0xFFFFFFFD;
    return h;
  }

  bool invariant() const {
    assert((m_capacity & (m_capacity - 1)) == 0);
    assert(m_numUsed >= m_size);
    return true;
  }

  INLINE bool compare_key(const node* n, const key_type& key, hash_value_t hash) const {
    return (n->hash == hash && m_keyEqualFunc(key, n->data.first));
  }

  node* m_nodes;
  int m_size;
  int m_capacity;
  u32 m_capacityMask;
  int m_numUsed;
  THashFunc m_hashFunc;
  TKeyEqualFunc m_keyEqualFunc;
  TAllocator m_allocator;
  static node ms_emptyNode;
};

// holy ...
template<typename TKey, typename TValue,
         class THashFunc, int TLoadFactor4,
         class TKeyEqualFunc, class TAllocator>
typename hash_map<TKey, TValue, THashFunc, TLoadFactor4, TKeyEqualFunc, TAllocator>::node
  hash_map<TKey, TValue, THashFunc, TLoadFactor4, TKeyEqualFunc, TAllocator>::ms_emptyNode;
} /* namespace q */

