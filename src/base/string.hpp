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
INLINE void strcat_cs(char *d, const char *s) { const auto n = int(strlen(d)); strn0cpy(d+n,s,MAXDEFSTR-n); }
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
 - simple string storage
 -------------------------------------------------------------------------*/
template<typename E, class TAllocator>
class string_storage
{
public:
  typedef E value_type;
  typedef int size_type;
  typedef TAllocator allocator_type;
  typedef const value_type *const_iterator;
  static const unsigned long kGranularity = 32;

  explicit string_storage(const allocator_type& allocator)
    : m_length(0), m_allocator(allocator) {
    m_data = construct_string(0, m_capacity);
  }
  string_storage(const value_type* str, const allocator_type& allocator)
    : m_allocator(allocator)
  {
    const auto len = int(strlen(str));
    m_data = construct_string(len, m_capacity);
    memcpy(m_data, str, len*sizeof(value_type));
    m_length = len;
    m_data[len] = 0;
  }
  string_storage(const value_type* str, size_type len,
    const allocator_type& allocator)
  : m_allocator(allocator)
  {
    m_data = construct_string(len, m_capacity);
    memcpy(m_data, str, len*sizeof(value_type));
    m_length = len;
    m_data[len] = 0;
  }
  string_storage(const string_storage& rhs, const allocator_type& allocator)
  : m_data(0),
    m_capacity(0),
    m_length(0),
    m_allocator(allocator) { assign(rhs.c_str(), rhs.size()); }
  ~string_storage() { release_string(); }

  // @note: doesnt copy allocator
  string_storage& operator=(const string_storage& rhs) {
    if (m_data != rhs.c_str())
      assign(rhs.c_str(), rhs.size());
    return *this;
  }

  void assign(const value_type* str, size_type len) {
    // Do not use with str = str.c_str()!
    assert(str != m_data);
    if (m_capacity <= len + 1)
    {
      release_string();
      m_data = construct_string(len, m_capacity);
    }
    if (NULL != str) memcpy(m_data, str, len*sizeof(value_type));
    m_length = len;
    m_data[len] = 0;
  }

  void append(const value_type* str, size_type len) {
    const size_type prevLen = size();
    const size_type newLen = prevLen + len;
    if (m_capacity <= newLen + 1) {
      size_type newCapacity;
      value_type* newData = construct_string(newLen, newCapacity);
      memcpy(newData, m_data, prevLen * sizeof(value_type));
      release_string();
            m_data = newData;
      m_capacity = newCapacity;
    }
    memcpy(m_data + prevLen, str, len * sizeof(value_type));
    m_data[newLen] = 0;
    m_length = newLen;
    assert(invariant());
  }

  inline const value_type* c_str() const { return m_data; }
  inline size_type size() const { return m_length; }
  inline size_type capacity() const { return m_capacity; } 
  void clear() {
    release_string();
    m_data = construct_string(0, m_capacity);
    m_length = 0;
  }
  const allocator_type& get_allocator() const  { return m_allocator; }
  void make_unique(size_type) {}
  value_type* get_data() { return m_data; }

protected:
  bool invariant() const {
    assert(m_data);
    assert(m_length <= m_capacity);
    if (size() != 0)
      assert(m_data[size()] == 0);
    return true;
  }
private:
  value_type* construct_string(size_type capacity, size_type& out_capacity) {
    value_type* data(0);
    if (capacity != 0) {
      capacity = (capacity+kGranularity-1) & ~(kGranularity-1);
      if (capacity < size_type(kGranularity))
        capacity = kGranularity;

      const size_type toAlloc = sizeof(value_type)*(capacity + 1);
      void* mem = m_allocator.allocate(toAlloc);
      data = static_cast<value_type*>(mem);
    } else  // empty string, no allocation needed. Use our internal buffer.
      data = &m_end_of_data;

    out_capacity = capacity;
    *data = 0;
    return data;
  }
  void release_string() {
    if (m_capacity != 0) {
      assert(m_data != &m_end_of_data);
      m_allocator.deallocate(m_data, m_capacity);
    }
  }

  E* m_data;
  E m_end_of_data;
  size_type m_capacity;
  size_type m_length;
  TAllocator m_allocator;
};

/*-------------------------------------------------------------------------
 - stl-like string implementation
 -------------------------------------------------------------------------*/
template<typename E,
         class TAllocator = q::allocator,
         typename TStorage = q::string_storage<E, TAllocator>>
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
    assert(i <= size());
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

  basic_string substr(size_type begin, size_type len) const {
    assert(begin+len <= size() && begin >= 0);
    return basic_string(c_str() + begin, len);
  }
  basic_string substr(size_type begin) const {
    return substr(begin, size());
  }

  void allocate(int sz) { assign(NULL, sz); }
  void append(const value_type* str, size_type len) {
    if (!str || len == 0 || *str == 0)
      return;
    TStorage::append(str, len);
  }
  void append(const basic_string& str) { append(str.c_str(), str.size()); }
  void append(const value_type* str) { append(str, int(strlen(str))); }
  void append(const value_type ch) { append(&ch, 1); }
  basic_string& operator+=(const basic_string& rhs) {
    append(rhs);
    return *this;
  }

  int compare(const value_type* str) const {
    const size_type thisLen = size();
    const size_type strLen = strlen(str);
    if (thisLen < strLen)
      return -1;
    if (thisLen > strLen)
      return 1;
    return strcmp(c_str(), str);
  }
  int compare(const basic_string& rhs) const {
    const size_type thisLen = size();
    const size_type rhsLen = rhs.size();
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
  const_iterator end() const   { assert(invariant()); return c_str() + size(); }

  size_type size() const { return TStorage::size(); }
  bool empty() const  { return size() == 0; }

  const allocator_type& get_allocator() const  { return TStorage::get_allocator; }

  value_type* reserve(size_type capacity_hint) {
    return TStorage::reserve(capacity_hint);
  }
  void clear() { TStorage::clear(); }
  void make_lower() {
    const size_type len = size();
    TStorage::make_unique(len);
    static const int chDelta = 'a' - 'A';
    E* data = TStorage::get_data();
    for (size_type i = 0; i < len; ++i) {
      if (data[i] < 'a')
        data[i] += chDelta;
    }
  }
  void make_upper() {
    const size_type len = size();
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

  size_type find_first_of(const value_type *ch, size_type from) const {
    const auto sz = size();
    if (from >= sz) return basic_string::npos;
    const auto match = size_type(strcspn(c_str()+from, ch));
    return match == sz-from ? basic_string::npos : from+match;
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
    const value_type* s(c_str() + size());
    size_type si(size()+1);

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
    return murmurhash2(x.c_str(), x.size());
  }
};
string format(const char *fmt, ...);
string to_string(int i);
string to_string(double d);
double stod(const string &str);
int stoi(const string &str);

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
  return strcmp(x.c_str(), y.c_str()) != 0;
}
template<>
struct hash<string_ref> {
  INLINE hash_value_t operator()(const string_ref &str) const {
    return murmurhash2(str.c_str(), int(strlen(str.c_str())));
  }
};
} /* namespace q */

