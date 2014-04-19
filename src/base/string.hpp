/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - string.hpp -> exposes string classes and routines
 - mostly taken from https://code.google.com/p/rdestl/
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "allocator.hpp"
#include "hash.hpp"
#include "atomics.hpp"

namespace q {

/*-------------------------------------------------------------------------
 - raw string allocation
 -------------------------------------------------------------------------*/
char *newstring(const char *s, const char *filename, int linenum);
char *newstring(const char *s, size_t sz, const char *filename, int linenum);
char *newstringbuf(const char *s, const char *filename, int linenum);
#define NEWSTRING(...) newstring(__VA_ARGS__, __FILE__, __LINE__)
#define NEWSTRINGBUF(S) newstringbuf(S, __FILE__, __LINE__)

/*-------------------------------------------------------------------------
 - simple string operations
 -------------------------------------------------------------------------*/
static const u32 MAXDEFSTR = 260, WORDWRAP = 80;
char *tokenize(char *s1, const char *s2, char **lasts);
bool strequal(const char *s1, const char *s2);
bool contains(const char *haystack, const char *needle);
INLINE char *strn0cpy(char *d, const char *s, int m) {strncpy(d,s,m); d[m-1]=0; return d;}
INLINE void strcpy_cs(char *d, const char *s) { strn0cpy(d,s,MAXDEFSTR); }
INLINE void strcat_cs(char *d, const char *s) { const auto n = strlen(d); strn0cpy(d+n,s,MAXDEFSTR-n); }
#define ATOI(s) strtol(s, NULL, 0) // supports hexadecimal numbers

/*-------------------------------------------------------------------------
 - easy safe string
 -------------------------------------------------------------------------*/
static const struct fmttype {fmttype(){}} fmt;
struct fixedstring {
  INLINE fixedstring();
  INLINE fixedstring(const char *txt);
  fixedstring(fmttype, const char *txt, ...);
  fixedstring(const char *txt, va_list va);
  void fmt(const char *txt, ...);
  void fmt(const char *txt, va_list va);
  char &operator[] (int idx) { return bytes[idx]; }
  char operator[] (int idx) const { return bytes[idx]; }
  const char *c_str() const { return &bytes[0]; }
  char *c_str() { return &bytes[0]; }
  char bytes[MAXDEFSTR];
};
INLINE fixedstring::fixedstring() { bytes[0] = '\0'; }
INLINE fixedstring::fixedstring(const char *txt) { strcpy_cs(bytes,txt); }
#define vasprintfs(d,last,txt) \
  do {va_list ap; va_start(ap,last); d.fmt(txt,ap); va_end(ap);} while (0)
#define vasprintfsd(d,last,txt) \
  fixedstring d; \
  vasprintfs(d,last,txt);

// more string operations
INLINE void strcpy_s(fixedstring &d, const char *s) { strcpy_cs(d.c_str(),s); }
INLINE void strcat_s(fixedstring &d, const char *s) { strcat_cs(d.c_str(),s); }
INLINE void strfmt_s(fixedstring &d, const char *fmt, va_list v) {
  vsnprintf(d.c_str(), MAXDEFSTR, fmt, v);
  d[MAXDEFSTR-1] = 0;
}

/*-------------------------------------------------------------------------
 - copy-on-write storage
 -------------------------------------------------------------------------*/
struct string_rep {
  void add_ref() { ++refs; }
  bool release() { --refs; return refs <= 0; }
  void init(short new_capacity = 0) {
    refs = 1;
    size = 0;
    capacity = new_capacity;
  }
  atomic refs;
  s16 size;
  s16 capacity;
  static const size_t kMaxCapacity = (1 << (sizeof(short) << 3)) >> 1;
};

template<typename E, class TAllocator>
class cow_string_storage {
public:
  static_assert(sizeof(E) <= 2, "character is too big");
  typedef E value_type;
  typedef int size_type;
  typedef TAllocator allocator_type;
  typedef const value_type *const_iterator;
  static const unsigned long kGranularity = 32;

  explicit cow_string_storage(const allocator_type& allocator)
    : m_allocator(allocator)
  { construct_string(0); }
  cow_string_storage(const value_type* str, const allocator_type& allocator)
    : m_allocator(allocator) {
    const int len = strlen(str);
    construct_string(len);
    memcpy(m_data, str, len*sizeof(value_type));
    assert(len < string_rep::kMaxCapacity);
    get_rep()->size = static_cast<short>(len);
    m_data[len] = 0;
  }
  cow_string_storage(const value_type* str, size_type len,
                     const allocator_type& allocator)
  : m_allocator(allocator) {
    construct_string(len);
    memcpy(m_data, str, len*sizeof(value_type));
    assert(len < string_rep::kMaxCapacity);
    get_rep()->size = static_cast<short>(len);
    m_data[len] = 0;
  }
  cow_string_storage(const cow_string_storage& rhs, const allocator_type& allocator)
  : m_data(rhs.m_data), m_allocator(allocator) {
    if (rhs.is_dynamic())
      get_rep()->add_ref();
    else {
      const int len = rhs.length();
      construct_string(len);
      memcpy(m_data, rhs.c_str(), len*sizeof(value_type));
      assert(len < string_rep::kMaxCapacity);
      get_rep()->size = static_cast<short>(len);
      m_data[len] = 0;
    }
  }
  ~cow_string_storage() {
    if (!is_dynamic())
      assert(get_rep()->refs == 1);
    release_string();
  }

  // @note: doesnt copy allocator
  cow_string_storage& operator=(const cow_string_storage& rhs) {
    if (m_data != rhs.m_data) {
      release_string();
      m_data = rhs.m_data;
      get_rep()->add_ref();
    }
    return *this;
  }

  void assign(const value_type* str, size_type len) {
    // do not use with str = str2.c_str()!
    assert(str != m_data);
    release_string();
    construct_string(len);
    memcpy(m_data, str, len*sizeof(value_type));
    get_rep()->size = short(len);
    m_data[len] = 0;
  }

  void append(const value_type* str, size_type len) {
    const size_type prevLen = length();
    const size_type newCapacity = prevLen + len;
    make_unique(newCapacity);
    string_rep* rep = get_rep();
    assert(rep->capacity >= short(newCapacity));
    const size_type newLen = prevLen + len;
    assert(short(newLen) <= rep->capacity);
    memcpy(m_data + prevLen, str, len * sizeof(value_type));
    m_data[newLen] = 0;
    rep->size = short(newLen);
  }

  inline const value_type* c_str() const { return m_data; }
  inline size_type length() const {
    return get_rep()->size;
  }
  const allocator_type& get_allocator() const { return m_allocator; }

  value_type* reserve(size_type capacity_hint) {
    make_unique(capacity_hint);
    return m_data;
  }
  void clear() { resize(0); }
  void resize(size_type size) {
    string_rep* rep = get_rep();
    make_unique(size);
    rep->size = (short)size;
    m_data[size] = 0;
  }

protected:
  bool invariant() const {
    assert(m_data);
    string_rep* rep = get_rep();
    assert(rep->refs >= 1);
    assert(rep->size <= rep->capacity);
    if (length() != 0)
      assert(m_data[length()] == 0);
    return true;
  }
  void make_unique(size_type capacity_hint) {
    string_rep* rep = get_rep();
    assert(rep->refs >= 1);

    if (capacity_hint != 0) {
      ++capacity_hint;
      capacity_hint = (capacity_hint+kGranularity-1) & ~(kGranularity-1);
      if (capacity_hint < kGranularity)
        capacity_hint = kGranularity;
    }
    assert(capacity_hint < string_rep::kMaxCapacity);
    // reallocate string only if we truly need to make it unique
    // (it's shared) or if our current buffer is too small.
    if (rep->refs > 1 || short(capacity_hint) > rep->capacity) {
      if (capacity_hint > 0) {
        const size_type toAlloc = sizeof(string_rep) + sizeof(value_type)*capacity_hint;
        void* newMem = m_allocator.allocate(toAlloc);
        string_rep* newRep = reinterpret_cast<string_rep*>(newMem);
        newRep->init(short(capacity_hint));
        value_type* newData = reinterpret_cast<value_type*>(newRep + 1);
        memcpy(newData, m_data, rep->size*sizeof(value_type));
        newRep->size = rep->size;
        newData[rep->size] = 0;
        release_string();
        m_data = newData;
      } else {
        release_string();
        string_rep* rep = reinterpret_cast<string_rep*>(m_buffer);
        rep->init();
        m_data = reinterpret_cast<value_type*>(rep + 1);
        *m_data = 0;
      }
    }
  }
  INLINE E* get_data() { return m_data; }

private:
  INLINE string_rep *get_rep() const {
    return reinterpret_cast<string_rep*>(m_data) - 1;
  }
  void construct_string(size_type capacity) {
    if (capacity != 0) {
      ++capacity;
      capacity = (capacity+kGranularity-1) & ~(kGranularity-1);
      if (capacity < kGranularity)
        capacity = kGranularity;
      assert(capacity < string_rep::kMaxCapacity);

      const size_type toAlloc = sizeof(string_rep) + sizeof(value_type)*capacity;
      void* mem = m_allocator.allocate(toAlloc);
      string_rep* rep = reinterpret_cast<string_rep*>(mem);
      rep->init(static_cast<short>(capacity));
      m_data = reinterpret_cast<value_type*>(rep + 1);
    } else { // empty string, no allocation needed. Use our internal buffer.
      string_rep* rep = reinterpret_cast<string_rep*>(m_buffer);
      rep->init();
      m_data = reinterpret_cast<value_type*>(rep + 1);
    }
    *m_data = 0;
  }
  void release_string() {
    string_rep* rep = get_rep();
    if (rep->release() && rep->capacity != 0) {
      // make sure it was a dynamically allocated data.
      assert(is_dynamic());
      m_allocator.deallocate(rep, rep->capacity);
    }
  }
  bool is_dynamic() const {
    return get_rep() != reinterpret_cast<const string_rep*>(&m_buffer[0]);
  }

  E *m_data;
  // @note: hack-ish. sizeof(string_rep) bytes for string_rep, than place for
  // terminating character (up to 2-bytes!)
  char m_buffer[sizeof(string_rep)+2];
  TAllocator m_allocator;
};

/*-------------------------------------------------------------------------
 - stl-like string implementation
 -------------------------------------------------------------------------*/
template<typename E,
         class TAllocator = q::allocator,
         typename TStorage = q::cow_string_storage<E, TAllocator>>
class basic_string : private TStorage {
public:
  typedef typename TStorage::value_type value_type;
  typedef typename TStorage::size_type size_type;
  typedef typename TStorage::const_iterator const_iterator;
  typedef typename TStorage::allocator_type allocator_type;

  //For find
  static const size_type npos = size_type(-1);

  explicit basic_string(const allocator_type& allocator = allocator_type())
  : TStorage(allocator)
  {}
  basic_string(const value_type* str,
               const allocator_type& allocator = allocator_type())
  : TStorage(str, allocator) {}
  basic_string(const value_type* str, size_type len,
               const allocator_type& allocator = allocator_type())
  : TStorage(str, len, allocator) {}
  basic_string(const basic_string& str,
               const allocator_type& allocator = allocator_type())
  : TStorage(str, allocator) {}
  ~basic_string() {}

  size_type capacity() const { return TStorage::capacity(); }

  // No operator returning ref for the time being. It's dangerous with COW.
  value_type operator[](size_type i) const {
    assert(i < length());
    assert(invariant());
    return c_str()[i];
  }

  basic_string& operator=(const basic_string& rhs) {
    assert(rhs.invariant());
    if (this != &rhs)
      TStorage::operator=((TStorage&)rhs);
    assert(invariant());
    return *this;
  }
  basic_string& operator=(const value_type* str) {
    return assign(str);
  }

  basic_string& assign(const value_type* str, size_type len) {
    TStorage::assign(str, len);
    assert(invariant());
    return *this;
  }
  basic_string& assign(const value_type* str) {
    return assign(str, strlen(str));
  }

  basic_string substr(size_type begin, size_type end) const {
    assert(end >= begin && end <= length() && begin >= 0);
    return basic_string(c_str() + begin, end - begin);
  }
  basic_string substr(size_type begin) const {
    return substr(begin, length());
  }

  void append(const value_type* str, size_type len) {
    if (!str || len == 0 || *str == 0)
      return;
    TStorage::append(str, len);
  }
  void append(const basic_string& str) { append(str.c_str(), str.length()); }
  void append(const value_type* str) { append(str, strlen(str)); }
  void append(const value_type ch) { append(&ch, 1); }
  basic_string& operator+=(const basic_string& rhs) {
    append(rhs);
    return *this;
  }

  int compare(const value_type* str) const {
    const size_type thisLen = length();
    const size_type strLen = strlen(str);
    if (thisLen < strLen)
      return -1;
    if (thisLen > strLen)
      return 1;
    return strcmp(c_str(), str);
  }
  int compare(const basic_string& rhs) const {
    const size_type thisLen = length();
    const size_type rhsLen = rhs.length();
    if (thisLen < rhsLen) return -1;
    if (thisLen > rhsLen) return 1;
    return strcmp(c_str(), rhs.c_str());
  }

  // @note: do NOT const_cast!
  const value_type* c_str() const {
    assert(invariant());
    return TStorage::c_str();
  }
  const_iterator begin() const { assert(invariant()); return c_str(); }
  const_iterator end() const   { assert(invariant()); return c_str() + length(); }

  size_type length() const { return TStorage::length(); }
  bool empty() const  { return length() == 0; }

  const allocator_type& get_allocator() const  { return TStorage::get_allocator; }

  value_type* reserve(size_type capacity_hint) {
    return TStorage::reserve(capacity_hint);
  }
  void clear() { TStorage::clear(); }
  void resize(size_type size) { TStorage::resize(size); }
  void make_lower() {
    const size_type len = length();
    TStorage::make_unique(len);
    static const int chDelta = 'a' - 'A';
    E* data = TStorage::get_data();
    for (size_type i = 0; i < len; ++i) {
      if (data[i] < 'a')
        data[i] += chDelta;
    }
  }
  void make_upper() {
    const size_type len = length();
    TStorage::make_unique(len);
    static const int chDelta = 'a' - 'A';
    E* data = TStorage::get_data();
    for (size_type i = 0; i < len; ++i)
      if (data[i] > 'Z')
        data[i] -= chDelta;
  }

  size_type find_index_of(const value_type ch) const {
    size_type retIndex(basic_string::npos);
    const E* ptr = c_str();
    size_type currentIndex(0);
    while (*ptr != value_type(0)) {
      if (*ptr == ch) {
        retIndex = currentIndex;
        break;
      }
      ++ptr;
      ++currentIndex;
    }
    return retIndex;
  }
  size_type find_index_of_last(const value_type ch) const {
    size_type retIndex(basic_string::npos);
    const value_type* ptr = c_str();
    size_type currentIndex(0);
    while (*ptr != value_type(0)) {
      if (*ptr == ch)
        retIndex = currentIndex;
      ++ptr;
      ++currentIndex;
    }
    return retIndex;
  }

  size_type find(const value_type* needle) const {
    const value_type* s(c_str());
    size_type si(0);
    while (*s) {
      const value_type* n = needle;
      if (*s == *n) { // first character matches
        // go through the sequence, and make sure while (x) x == n for all of n
        const value_type* x = s;
        size_type match = 0;
        while (*x && *n) {
          if (*n == *x)
            ++match;
          ++n;
          ++x;
        }
        if (match == strlen(needle))
          return si;
      }
      ++s;
      ++si;
    }
    return basic_string::npos;
  }

  size_type rfind(const value_type* needle) const {
    const value_type* s(c_str() + length());
    size_type si(length()+1);

    // find the last index of the first char in needle
    // searching from end->start for obvious reasons
    while (--si >= 0) {
      // if the first character matches, run our check
      if (*s-- == *needle) {
        // go through the sequence, and make sure while (x) x == n for all of n
        const value_type* x = c_str() + si;
        const value_type* n = needle;
        size_type match = 0;
        while (*x && *n) {
          if (*n == *x)
            ++match;
          ++n;
          ++x;
        }
        if (match == strlen(needle))
          return si;
      }
    }
    return basic_string::npos;
  }

private:
  bool invariant() const { return TStorage::invariant(); }
};

template<typename E, class TStorage, class TAllocator>
basic_string<E, TStorage, TAllocator>
operator+(const basic_string<E, TStorage, TAllocator>& lhs,
          const basic_string<E, TStorage, TAllocator>& rhs) {
  basic_string<E, TStorage, TAllocator> dst(lhs);
  dst.append(rhs);
  return dst;
}

template<typename E, class TStorage, class TAllocator>
bool operator==(const basic_string<E, TStorage, TAllocator>& lhs,
                const basic_string<E, TStorage, TAllocator>& rhs) {
  return lhs.compare(rhs) == 0;
}

template<typename E, class TStorage, class TAllocator>
bool operator!=(const basic_string<E, TStorage, TAllocator>& lhs,
                const basic_string<E, TStorage, TAllocator>& rhs) {
  return !(lhs == rhs);
}

template<typename E, class TStorage, class TAllocator>
bool operator<(const basic_string<E, TStorage, TAllocator>& lhs,
               const basic_string<E, TStorage, TAllocator>& rhs) {
  return lhs.compare(rhs) < 0;
}

template<typename E, class TStorage, class TAllocator>
bool operator>(const basic_string<E, TStorage, TAllocator>& lhs,
               const basic_string<E, TStorage, TAllocator>& rhs) {
  return lhs.compare(rhs) > 0;
}

/*-------------------------------------------------------------------------
 - std::string like string
 -------------------------------------------------------------------------*/
typedef basic_string<char> string;
template<typename E, class TAllocator, typename TStorage>
struct hash<basic_string<E, TAllocator, TStorage> > {
  hash_value_t operator()(const basic_string<E, TAllocator, TStorage>& x) const {
    return murmurhash2(x.c_str(), x.length());
  }
};

/*-------------------------------------------------------------------------
 - hash_map/set/map compatible raw string wrapper
 -------------------------------------------------------------------------*/
struct string_ref {
  INLINE string_ref() {bytes=NULL;}
  INLINE string_ref(const char *str) {bytes=const_cast<char*>(str);}
  INLINE string_ref(char *str) {bytes=str;}
  INLINE string_ref(const string_ref &other) : bytes(other.bytes) {}
  INLINE string_ref &operator= (const string_ref &other) {
    bytes = other.bytes;
    return *this;
  }
  INLINE char *c_str()             {assert(NULL!=bytes);return bytes;}
  INLINE const char *c_str() const {assert(NULL!=bytes);return bytes;}
  char *bytes;
};
INLINE bool operator== (const string_ref &x, const string_ref &y) {
  return x.c_str() == y.c_str() || strcmp(x.c_str(), y.c_str()) == 0;
}
INLINE bool operator!= (const string_ref &x, const string_ref &y) {
  return !(x==y);
}
INLINE bool operator< (const string_ref &x, const string_ref &y) {
  return strcmp(x.c_str(), y.c_str());
}
template<>
struct hash<string_ref> {
  INLINE hash_value_t operator()(const string_ref &str) const {
    return murmurhash2(str.c_str(), strlen(str.c_str()));
  }
};
} /* namespace q */

