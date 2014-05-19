/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - vector.hpp -> implements resizeable vector class
 - mostly taken from https://code.google.com/p/qstl/
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "allocator.hpp"
#include "algorithm.hpp"

namespace q {
struct base_vector {
  typedef int size_type;
  static const size_type npos = size_type(-1);
};

template<typename T, class TAllocator>
struct standard_vector_storage {
  explicit standard_vector_storage(const TAllocator& allocator)
  :  m_begin(0),
    m_end(0),
    m_capacityEnd(0),
    m_allocator(allocator)
  {/**/ }
  explicit standard_vector_storage(noinitializetype) {}

  void reallocate(base_vector::size_type newCapacity, base_vector::size_type oldSize) {
    T* newBegin = static_cast<T*>(m_allocator.allocate(newCapacity * sizeof(T)));
    const base_vector::size_type newSize = oldSize < newCapacity ? oldSize : newCapacity;
    // Copy old data if needed.
    if (m_begin) {
      q::copy_construct_n(m_begin, newSize, newBegin);
      destroy(m_begin, oldSize);
    }
    m_begin = newBegin;
    m_end = m_begin + newSize;
    m_capacityEnd = m_begin + newCapacity;
    assert(invariant());
  }

  // Reallocates memory, doesnt copy contents of old buffer.
  void reallocate_discard_old(base_vector::size_type newCapacity) {
    assert(newCapacity > base_vector::size_type(m_capacityEnd - m_begin));
    T* newBegin = static_cast<T*>(m_allocator.allocate(newCapacity * sizeof(T)));
    const base_vector::size_type currSize((base_vector::size_type)(m_end - m_begin));
    if (m_begin)
      destroy(m_begin, currSize);
    m_begin = newBegin;
    m_end = m_begin + currSize;
    m_capacityEnd = m_begin + newCapacity;
    assert(invariant());
  }

  void destroy(T* ptr, base_vector::size_type n) {
    q::destruct_n(ptr, n);
    m_allocator.deallocate(ptr, n * sizeof(T));
  }

  void reset() {
    if (m_begin)
      m_allocator.deallocate(m_begin, (m_end - m_begin) * sizeof(T));
    m_begin = m_end = 0;
    m_capacityEnd = 0;
  }
  bool invariant() const { return m_end >= m_begin; }
  INLINE void record_high_watermark() {}
  T *m_begin;
  T *m_end;
  T *m_capacityEnd;
  TAllocator m_allocator;
};

template<typename T,
         class TAllocator = q::allocator,
         class TStorage = standard_vector_storage<T, TAllocator>>
class vector : public base_vector, private TStorage {
private:
  using TStorage::m_begin;
  using TStorage::m_end;
  using TStorage::m_capacityEnd;
  using TStorage::m_allocator;
  using TStorage::invariant;
  using TStorage::reallocate;

public:
  typedef T value_type;
  typedef T *iterator;
  typedef const T *const_iterator;
  typedef TAllocator allocator_type;
  static const size_type kInitialCapacity = 16;

  explicit vector(const allocator_type& allocator = allocator_type())
  : TStorage(allocator)
  {}
  explicit vector(size_type initialSize, const allocator_type& allocator = allocator_type())
  : TStorage(allocator) {resize(initialSize);}
  vector(const T* first, const T* last, const allocator_type& allocator = allocator_type())
  : TStorage(allocator) {assign(first, last);}

  // @note: allocator is not copied from rhs.
  // @note: will not perform default constructor for newly created objects.
  vector(const vector& rhs, const allocator_type& allocator = allocator_type())
  : TStorage(allocator) {
    if(rhs.size() == 0) // nothing to do
      return;
    reallocate_discard_old(rhs.capacity());
    q::copy_construct_n(rhs.m_begin, rhs.size(), m_begin);
    m_end = m_begin + rhs.size();
    TStorage::record_high_watermark();
    assert(invariant());
  }
  explicit vector(noinitializetype n) : TStorage(n) {}
  ~vector() {
    if (TStorage::m_begin != 0)
      TStorage::destroy(TStorage::m_begin, size());
  }

  // @note:  allocator is not copied!
  // @note:  will not perform default constructor for newly created objects,
  //      just initialize with copy ctor of elements of rhs.
  vector& operator=(const vector& rhs) {
    copy(rhs);
    return *this;
  }

  void copy(const vector& rhs)
  {
    const size_type newSize = rhs.size();
    if (newSize > capacity())
      reallocate_discard_old(rhs.capacity());
    q::copy_construct_n(rhs.m_begin, newSize, m_begin);
    m_end = m_begin + newSize;
    TStorage::record_high_watermark();
    assert(invariant());
  }

  // @note: swap() not provided for the time being.
  iterator begin() {return m_begin;}
  iterator end()   {return m_end;}
  const_iterator begin() const {return m_begin;}
  const_iterator end() const   {return m_end;}
  size_type size() const     {return size_type(m_end - m_begin);}
  bool empty() const         {return m_begin == m_end;}
  size_type capacity() const {return size_type(m_capacityEnd - m_begin);}

  T* data()              {return empty() ? 0 : m_begin;}
  const T* data() const  {return empty() ? 0 : m_begin;}

  T& front() {
    assert(!empty());
    return *begin();
  }
  const T& front() const {
    assert(!empty());
    return *begin();
  }
  T& back() {
    assert(!empty());
    return *(end() - 1);
  }
  const T& back() const {
    assert(!empty());
    return *(end() - 1);
  }

  INLINE T& operator[](size_type i) {return at(i);}
  INLINE const T& operator[](size_type i) const {return at(i);}

  T& at(size_type i) {
    assert(i < size());
    return m_begin[i];
  }

  const T& at(size_type i) const {
    assert(i < size());
    return m_begin[i];
  }

  void push_back(const T& v) {
    if (m_end < m_capacityEnd)
      q::copy_construct(m_end++, v);
    else {
      grow();
      q::copy_construct(m_end++, v);
    }
    TStorage::record_high_watermark();
  }

  // @note: extension. Use instead of push_back(T()) or resize(size() + 1).
  void push_back() {
    if (m_end == m_capacityEnd)
      grow();
    q::construct(m_end);
    ++m_end;
    TStorage::record_high_watermark();
  }

  void pop_back() {
    assert(!empty());
    --m_end;
    q::destruct(m_end);
  }

  void assign(const T* first, const T* last) {
    // Iterators cant be in range!
    assert(!validate_iterator(first));
    assert(!validate_iterator(last));

    const size_type count = size_type(last - first);
    assert(count > 0);
    clear();
    if (m_begin + count > m_capacityEnd)
      reallocate_discard_old(compute_new_capacity(count));

    q::copy_n(first, count, m_begin);
    m_end = m_begin + count;
    TStorage::record_high_watermark();
    assert(invariant());
  }

  void insert(size_type index, size_type n, const T& val) {
    assert(invariant());
    const size_type indexEnd = index + n;
    const size_type prevSize = size();
    if (m_end + n > m_capacityEnd)
      reallocate(compute_new_capacity(prevSize + n), prevSize);

    // Past 'end', needs to copy construct.
    if (indexEnd > prevSize) {
      const size_type numCopy    = prevSize - index;
      const size_type numAppend  = indexEnd - prevSize;
      assert(numCopy >= 0 && numAppend >= 0);
      assert(numAppend + numCopy == n);
      iterator itOut = m_begin + prevSize;
      for (size_type i = 0; i < numAppend; ++i, ++itOut)
        q::copy_construct(itOut, val);
      q::copy_construct_n(m_begin + index, numCopy, itOut);
      for (size_type i = 0; i < numCopy; ++i)
        m_begin[index + i] = val;
    } else {
      q::copy_construct_n(m_end - n, n, m_end);
      iterator insertPos = m_begin + index;
      q::move_n(insertPos, prevSize - indexEnd, insertPos + n);
      for (size_type i = 0; i < n; ++i)
        insertPos[i] = val;
    }
    m_end += n;
    TStorage::record_high_watermark();
  }

  // @pre validate_iterator(it)
  // @note use push_back for maximum efficiency if it == end()!
  void insert(iterator it, size_type n, const T& val) {
    assert(validate_iterator(it));
    assert(invariant());
    insert(size_type(it - m_begin), n, val);
  }

  iterator insert(iterator it, const T& val) {
    assert(validate_iterator(it));
    assert(invariant());
    // @todo: optimize for toMove==0 --> push_back here?
    const size_type index = (size_type)(it - m_begin);
    if (m_end == m_capacityEnd) {
      grow();
      it = m_begin + index;
    } else
      q::construct(m_end);

    // @note: conditional vs empty loop, what's better?
    if (m_end > it) {
      if(!has_trivial_copy<T>::value) {
        const size_type prevSize = size();
        assert(index <= prevSize);
        const size_type toMove = prevSize - index;

        q::internal::move_n(it, toMove, it + 1, int_to_type<has_trivial_copy<T>::value>());
      } else {
        assert(it < m_end);
        const size_t n = reinterpret_cast<uintptr_t>(m_end) - reinterpret_cast<uintptr_t>(it);
        memmove(it + 1, it, n);
      }
    }
    *it = val;
    ++m_end;
    assert(invariant());
    TStorage::record_high_watermark();
    return it;
  }

  // @pre validate_iterator(it)
  // @pre it != end()
  iterator erase(iterator it) {
    assert(validate_iterator(it));
    assert(it != end());
    assert(invariant());

    // Move everything down, overwriting *it
    if (it + 1 < m_end)
    {
      move_down_1(it, int_to_type<has_trivial_copy<T>::value>());
    }
    --m_end;
    q::destruct(m_end);
    return it;
  }

  iterator erase(iterator first, iterator last) {
    assert(validate_iterator(first));
    assert(validate_iterator(last));
    assert(invariant());
    if (last <= first)
      return end();

    const size_type indexFirst = size_type(first - m_begin);
    const size_type toRemove = size_type(last - first);
    if (toRemove > 0)
    {
      move_down(last, first, int_to_type<has_trivial_copy<T>::value>());
      shrink(size() - toRemove);
    }
    return m_begin + indexFirst;
  }

  // *Much* quicker version of erase, doesnt preserve collection oqr.
  INLINE void erase_unoqred(iterator it) {
    assert(validate_iterator(it));
    assert(it != end());
    assert(invariant());
    const iterator itNewEnd = end() - 1;
    if (it != itNewEnd)
      *it = *itNewEnd;
    pop_back();
  }

  void resize(size_type n) {
    if (n > size())
      insert(m_end, n - size(), value_type());
    else
      shrink(n);
  }

  void reserve(size_type n) {
    if (n > capacity())
      reallocate(n, size());
  }

  // Removes all elements from this vector (calls their destructors).
  // Doesn't release memory.
  void clear() {
    shrink(0);
    assert(invariant());
  }

  // EA STL concept.
  // Resets container to an initialized, unallocated state.
  // Safe only for value types with trivial destructor.
  void reset() {
    TStorage::reset();
    assert(invariant());
  }

  // Extension: allows to limit amount of allocated memory.
  void set_capacity(size_type newCapacity) {
    reallocate(newCapacity, size());
  }

  size_type index_of(const T& item, size_type index = 0) const {
    assert(index >= 0 && index < size());
    for ( ; index < size(); ++index)
      if (m_begin[index] == item)
        return index;
    return npos;
  }

  iterator find(const T& item) {
    iterator itEnd = end();
    for (iterator it = begin(); it != itEnd; ++it)
      if (*it == item)
        return it;
    return itEnd;
  }

  const allocator_type& get_allocator() const  { return m_allocator; }
  void set_allocator(const allocator_type& allocator) {
    m_allocator = allocator;
  }

  bool validate_iterator(const_iterator it) const {
    return it >= begin() && it <= end();
  }
  size_type get_high_watermark() const {
    return TStorage::get_high_watermark();
  }

private:
  size_type compute_new_capacity(size_type newMinCapacity) const {
    const size_type c = capacity();
    return (newMinCapacity > c * 2 ? newMinCapacity : (c == 0 ? kInitialCapacity : c * 2));
  }
  inline void grow() {
    assert(m_end == m_capacityEnd);  // size == capacity!
    const size_type c = capacity();
    reallocate((m_capacityEnd == 0 ? kInitialCapacity : c * 2), c);
  }
  INLINE void shrink(size_type newSize) {
    assert(newSize <= size());
    const size_type toShrink = size() - newSize;
    q::destruct_n(m_begin + newSize, toShrink);
    m_end = m_begin + newSize;
  }

  // The following two methods are only to get better cache behavior.
  // We do not really need 'move' here if copying one-by-one, only for memcpy/memmove (on some platforms, see
  // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.kui0002a/c51_memcpy.htm for example).

  INLINE void move_down_1(iterator it, int_to_type<true> itt) {
    internal::move(it + 1, m_end, it, itt);
  }
  INLINE void move_down_1(iterator it, int_to_type<false> itt) {
    internal::copy(it + 1, m_end, it, itt);
  }
  INLINE void move_down(iterator it_start, iterator it_result, int_to_type<true> itt) {
    assert(it_start > it_result);
    internal::move(it_start, m_end, it_result, itt);
  }
  INLINE void move_down(iterator it_start, iterator it_result, int_to_type<false> itt) {
    assert(it_start > it_result);
    internal::copy(it_start, m_end, it_result, itt);
  }
};
} /* namespace q */
