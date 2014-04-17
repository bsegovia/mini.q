#pragma once
#include "sys.hpp"
#include "stl.hpp"

namespace q {
namespace internal {
  struct rb_tree_base {
    typedef int size_type;
    enum color_e {
      red,
      black
    };
  };
  template<typename TKey>
  struct rb_tree_key_wrapper {
    TKey key;
    rb_tree_key_wrapper() {}
    rb_tree_key_wrapper(const TKey& key_): key(key_) {}
    const TKey& get_key() const { return key; }
  };
  template<typename TKey>
  struct rb_tree_traits {
    typedef TKey key_type;
    typedef rb_tree_key_wrapper<TKey> value_type;
  };
} // internal

template<class TTreeTraits, class TAllocator = q::allocator>
class rb_tree_base : public internal::rb_tree_base {
public:
  typedef typename TTreeTraits::key_type  key_type;
  typedef typename TTreeTraits::value_type value_type;
  typedef TAllocator allocator_type;

  struct node {
    INLINE node(void) {}
    INLINE node(color_e color, node* left, node* right, node* parent)
      : left(left), parent(parent), right(right), color(color)
    {}
    node *left;
    node *parent;
    node *right;
    value_type value;
    color_e color;
  };

  explicit INLINE rb_tree_base(const allocator_type& allocator = allocator_type())
    : m_size(0), m_allocator(allocator)
  {
    ms_sentinel.color = black;
    ms_sentinel.left = &ms_sentinel;
    ms_sentinel.right = &ms_sentinel;
    ms_sentinel.parent = &ms_sentinel;
    m_root    = &ms_sentinel;
  }
  INLINE ~rb_tree_base() { clear(); }

  node* insert(const value_type& v) {
    node* iter(m_root);
    node* parent(&ms_sentinel);
    while (iter != &ms_sentinel) {
      parent = iter;
      if (iter->value.get_key() < v.get_key())
        iter = iter->right;
      else if (v.get_key() < iter->value.get_key())
        iter = iter->left;
      else // v.key == iter->key
        return iter;
    }

    auto new_node = alloc_node();
    new_node->color = red;
    new_node->value = v;
    new_node->left = &ms_sentinel;
    new_node->right = &ms_sentinel;
    new_node->parent = parent;
    if (parent != &ms_sentinel) {
      if (v.get_key() < parent->value.get_key())
        parent->left = new_node;
      else
        parent->right = new_node;
    } else // empty tree
      m_root = new_node;

    rebalance(new_node);
    validate();
    ++m_size;
    return new_node;
  }

  node* find_node(const key_type &key) {
    node* iter(m_root);
    while (iter != &ms_sentinel) {
      const key_type& iter_key = iter->value.get_key();
      if (iter_key < key)
        iter = iter->right;
      else if (key < iter_key)
        iter = iter->left;
      else // key == iter->key
        return iter;
    }
    return 0; // not found
  }

  size_type erase(const key_type& key) {
    node* toErase = find_node(key);
    size_type erased(0);
    if (toErase != 0) {
      erase(toErase);
      erased = 1;
    }
    return erased;
  }
  void erase(node* n) {
    assert(m_size > 0);
    node* toErase;
    // "end" node, can be just unlinked from the tree (less than 2 children)
    if (n->left == &ms_sentinel || n->right == &ms_sentinel)
      toErase = n;
    else {
      // cannot remove node, it is part of the tree, needs to overwrite the value
      // and remove successor.
      toErase = n->right;
      while (toErase->left != &ms_sentinel)
        toErase = toErase->left;
    }

    node* eraseChild = (toErase->left != &ms_sentinel ? toErase->left : toErase->right);
    eraseChild->parent = toErase->parent;
    if (toErase->parent != &ms_sentinel)
    {
      if (toErase == toErase->parent->left)
        toErase->parent->left = eraseChild;
      else
        toErase->parent->right = eraseChild;
    } else // we are erasing root of the tree.
      m_root = eraseChild;

    // Branching is probably worse than key copy anyway.
    // $$$ unless key is very expensive?
    //if (toErase != n)
    n->value = toErase->value;

    if (toErase->color == black)
      rebalance_after_erase(eraseChild);

    free_node(toErase, false);

    validate();
    --m_size;
  }

  void clear()
  {
    if (!empty())
    {
      free_node(m_root, true);
      m_root = &ms_sentinel;
      m_size = 0;
    }
  }

  // @NOTE: swapping trees with different allocator types results in undefined behavior.
  void swap(rb_tree_base& other)
  {
    if (&other != this) {
      validate();
      assert(m_allocator == other.m_allocator);
      q::swap(m_root, other.m_root);
      q::swap(m_size, other.m_size);
      validate();
    }
  }

  INLINE bool empty() const { return m_size == 0; }
  INLINE size_type size() const { return m_size; }

  typedef void (*TravFunc)(node* n, int left, int depth);

  void traverse_node(node* n, TravFunc func, int depth) {
    int left(-1);
    if (n->parent != &ms_sentinel)
      left = n->parent->left == n;
    func(n, left, depth);
    if (n->left != &ms_sentinel)
      traverse_node(n->left, func, depth + 1);
    if (n->right != &ms_sentinel)
      traverse_node(n->right, func, depth + 1);
  }

  void traverse(TravFunc func) {
    int depth(0);
    traverse_node(m_root, func, depth);
  }

  node* get_begin_node() const {
    node* iter(0);
    if (m_root != &ms_sentinel) {
      iter = m_root;
      while (iter->left != &ms_sentinel)
        iter = iter->left;
    }
    return iter;
  }

  node* find_next_node(node* n) const {
    node* next(0);
    if (n != 0) {
      if (n->right != &ms_sentinel) {
        next = n->right;
        while (next->left != &ms_sentinel)
          next = next->left;
      } else if (n->parent != &ms_sentinel) {
        if (n == n->parent->left)
          return n->parent;
        else {
          next = n;
          while (next->parent != &ms_sentinel) {
            if (next == next->parent->right)
              next = next->parent;
            else
              return next->parent;
          }
          next = 0;
        }
      } else // 'n' is root.
        assert(n == m_root);
    }
    return next;
  }

  size_type num_nodes(const node* n) const {
    return n == &ms_sentinel ? 0 : 1 + num_nodes(n->left) + num_nodes(n->right);
  }
  INLINE void rebalance(node* new_node) {
    assert(new_node->color == red);
    node* iter(new_node);
    while (iter->parent->color == red)
    {
      node* grandparent(iter->parent->parent);
      if (iter->parent == grandparent->left)
      {
        node* uncle = grandparent->right;
        // Both parent and uncle are red.
        // Repaint both, make grandparent red.
        if (uncle->color == red)
        {
          iter->parent->color = black;
          uncle->color = black;
          grandparent->color = red;
          iter = grandparent;
        }
        else 
        {
          if (iter == iter->parent->right)
          {
            iter = iter->parent;
            rotate_left(iter);
          }
          grandparent = iter->parent->parent;
          iter->parent->color = black;
          grandparent->color = red;
          rotate_right(grandparent);
        }
      }
      else
      {
        node* uncle = grandparent->left;
        if (uncle->color == red)
        {
          grandparent->color = red;
          iter->parent->color = black;
          uncle->color = black;
          iter = grandparent;
        }
        else
        {
          if (iter == iter->parent->left)
          {
            iter = iter->parent;
            rotate_right(iter);
          }
          grandparent = iter->parent->parent;
          iter->parent->color = black;
          grandparent->color = red;
          rotate_left(grandparent);
        }
      }
    }
    m_root->color = black;
  }

  void rebalance_after_erase(node* n)
  {
    node* iter(n);
    while (iter != m_root && iter->color == black)
    {
      if (iter == iter->parent->left)
      {
        node* sibling = iter->parent->right;
        if (sibling->color == red)
        {
          sibling->color = black;
          iter->parent->color = red;
          rotate_left(iter->parent);
          sibling = iter->parent->right;
        }
        if (sibling->left->color == black &&
            sibling->right->color == black)
        {
          sibling->color = red;
          iter = iter->parent;
        }
        else
        {
          if (sibling->right->color == black)
          {
            sibling->left->color = black;
            sibling->color = red;
            rotate_right(sibling);
            sibling = iter->parent->right;
          }
          sibling->color = iter->parent->color;
          iter->parent->color = black;
          sibling->right->color = black;
          rotate_left(iter->parent);
          iter = m_root;
        }
      }
      else // iter == right child
      {
        node* sibling = iter->parent->left;
        if (sibling->color == red)
        {
          sibling->color = black;
          iter->parent->color = red;
          rotate_right(iter->parent);
          sibling = iter->parent->left;
        }
        if (sibling->left->color == black &&
            sibling->right->color == black)
        {
          sibling->color = red;
          iter = iter->parent;
        }
        else
        {
          if (sibling->left->color == black)
          {
            sibling->right->color = black;
            sibling->color = red;
            rotate_left(sibling);
            sibling = iter->parent->left;
          }
          sibling->color = iter->parent->color;
          iter->parent->color = black;
          sibling->left->color = black;
          rotate_right(iter->parent);
          iter = m_root;
        }
      }
    }
    iter->color = black;
  }

  void validate() const {
#if STL_DEBUG
    assert(m_root->color == black);
    validate_node(m_root);
#endif
  }
  void validate_node(node* n) const {
    // - we're child of our parent.
    assert(n->parent == &ms_sentinel ||
        n->parent->left == n || n->parent->right == n);
    // - both children of red node are black
    if (n->color == red) {
      assert(n->left->color == black);
      assert(n->right->color == black);
    }
    if (n->left != &ms_sentinel)
      validate_node(n->left);
    if (n->right != &ms_sentinel)
      validate_node(n->right);
  }

  // n's right child replaces n and the right child's left child
  // becomes n's right child.
  void rotate_left(node *n) {
    // Right child's left child becomes n's right child.
    node* rightChild = n->right;
    n->right = rightChild->left;
    if (n->right != &ms_sentinel)
      n->right->parent = n;

    // n's right child replaces n
    rightChild->parent = n->parent;
    if (n->parent == &ms_sentinel)
    {
      m_root = rightChild;
    }
    else
    {
      if (n == n->parent->left)
        n->parent->left = rightChild;
      else
        n->parent->right = rightChild;
    }
    rightChild->left = n;
    n->parent = rightChild;
  }
  void rotate_right(node* n)
  {
    node* leftChild(n->left);
    n->left = leftChild->right;
    if (n->left != &ms_sentinel)
      n->left->parent = n;

    leftChild->parent = n->parent;
    if (n->parent == &ms_sentinel)
    {
      m_root = leftChild;
    }
    else
    {
      // Substitute us in the parent list with left child.
      if (n == n->parent->left)
        n->parent->left = leftChild;
      else
        n->parent->right = leftChild;
    }
    leftChild->right = n;
    n->parent = leftChild;
  }
  node* alloc_node()
  {
    node* mem = (node*)m_allocator.allocate(sizeof(node));
    return new (mem) node();
  }
  void free_node(node* n, bool recursive)
  {
    if (recursive)
    {
      if (n->left != &ms_sentinel)
        free_node(n->left, true);
      if (n->right != &ms_sentinel)
        free_node(n->right, true);
    }
    if (n != &ms_sentinel)
    {
      n->~node();
      m_allocator.deallocate(n, sizeof(node));
    }
  }

private:
  // block copy for the time being
  rb_tree_base(const rb_tree_base&);
  rb_tree_base& operator=(const rb_tree_base&);

  node *m_root;
  size_type m_size;
  allocator_type m_allocator;
  static node ms_sentinel;
};

#if defined(__CLANG__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
#endif
template<class TTreeTraits, class TAllocator>
typename rb_tree_base<TTreeTraits, TAllocator>::node
  rb_tree_base<TTreeTraits, TAllocator>::ms_sentinel(
    internal::rb_tree_base::black, &ms_sentinel, &ms_sentinel, &ms_sentinel);
#if defined(__CLANG__)
#pragma clang diagnostic pop
#endif

template<typename TKey, class TAllocator = q::allocator>
class rb_tree : public rb_tree_base<internal::rb_tree_traits<TKey>, TAllocator> {
public:
  explicit rb_tree(TAllocator allocator = TAllocator()):
    rb_tree_base<internal::rb_tree_traits<TKey>, TAllocator>(allocator) {}
};
} /* namespace q */

