/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - stl.hpp -> exposes standard library routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "math.hpp"

namespace q {

// get the power larger or equal than x
INLINE u32 nextpowerof2(u32 x) {
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}
INLINE bool ispoweroftwo(u32 x) { return ((x&(x-1))==0); }
INLINE u32 ilog2(u32 x) {
  u32 l = 0;
  while (x >>= 1) ++l;
  return l;
}

// fast 32 bits murmur hash and its generic version
u32 murmurhash2(const void *key, int len, u32 seed = 0xffffffffu);
template <typename T> INLINE u32 murmurhash2(const T &x) { return murmurhash2(&x, sizeof(T)); }

// more string operations
INLINE char *strn0cpy(char *d, const char *s, int m) {strncpy(d,s,m); d[m-1]=0; return d;}

// easy safe string
static const u32 MAXDEFSTR = 260, WORDWRAP = 80;
typedef char string[MAXDEFSTR];
INLINE void strcpy_cs(char *d, const char *s) { strn0cpy(d,s,MAXDEFSTR); }
INLINE void strcat_cs(char *d, const char *s) { size_t n = strlen(d); strn0cpy(d+n,s,MAXDEFSTR-n); }
INLINE void strcpy_s(string &d, const char *s) { strcpy_cs(d,s); }
INLINE void strcat_s(string &d, const char *s) { strcat_cs(d,s); }
INLINE void strfmt_s(string &d, const char *fmt, va_list v) {
  vsnprintf(d, MAXDEFSTR, fmt, v);
  d[MAXDEFSTR-1] = 0;
}
char *newstring(const char *s, const char *filename, int linenum);
char *newstring(const char *s, size_t sz, const char *filename, int linenum);
char *newstringbuf(const char *s, const char *filename, int linenum);
#define NEWSTRING(...) newstring(__VA_ARGS__, __FILE__, __LINE__)
#define NEWSTRINGBUF(S) newstringbuf(S, __FILE__, __LINE__)

char *tokenize(char *s1, const char *s2, char **lasts);
bool strequal(const char *s1, const char *s2);
bool contains(const char *haystack, const char *needle);

// format string boilerplate
struct sprintfmt_s {
  INLINE sprintfmt_s(string &str) : d(str) {}
  void operator()(const char *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    vsnprintf(d, MAXDEFSTR, fmt, v);
    va_end(v);
    d[MAXDEFSTR-1] = 0;
  }
  static INLINE sprintfmt_s make(string &s) { return sprintfmt_s(s); }
  string &d;
};
#define sprintf_s(d) sprintfmt_s::make(d)
#define sprintf_sd(d) string d; sprintf_s(d)
#define sprintf_sdlv(d,last,fmt) string d; {va_list ap; va_start(ap,last); strfmt_s(d,fmt,ap); va_end(ap);}
#define sprintf_sdv(d,fmt) sprintf_sdlv(d,fmt,fmt)
#define ATOI(s) strtol(s, NULL, 0) // supports hexadecimal numbers

// global variable with proper constructor
#define GLOBAL(TYPE, NAME) static TYPE &NAME() {static TYPE var; return var;}

template <typename T> void swap(T &x, T &y) { T tmp = x; x = y; y = tmp; }

// makes a class non-copyable
class noncopyable {
protected:
  INLINE noncopyable() {}
  INLINE ~noncopyable() {}
private:
  INLINE noncopyable(const noncopyable&) {}
  INLINE noncopyable& operator= (const noncopyable&) {return *this;}
};

/*-------------------------------------------------------------------------
 - fast growing pool allocation
 -------------------------------------------------------------------------*/
template <typename T> class growingpool {
public:
  growingpool(void) : current(NEW(growingpoolelem, 1)) {}
  ~growingpool(void) { SAFE_DEL(current); }
  T *allocate(void) {
    if (current->allocated == current->maxelemnum) {
      growingpoolelem *elem = NEW(growingpoolelem, 2*current->maxelemnum);
      elem->next = current;
      current = elem;
    }
    T *data = current->data + current->allocated++;
    return data;
  }
private:
  // chunk of elements to allocate
  struct growingpoolelem {
    growingpoolelem(size_t elemnum) {
      data = NEWAE(T, elemnum);
      next = NULL;
      maxelemnum = elemnum;
      allocated = 0;
    }
    ~growingpoolelem(void) {
      SAFE_DELA(data);
      SAFE_DEL(next);
    }
    T *data;
    growingpoolelem *next;
    size_t allocated, maxelemnum;
  };
  growingpoolelem *current;
};

// very simple fixed size circular buffer that wraps around
template <typename T, u32 max_n> struct DEFAULT_ALIGNED ringbuffer {
  INLINE ringbuffer() : first(0x7fffffff), n(0) {}
  INLINE T &back()  {return items[(first+n-1)%max_n];}
  INLINE T &front() {return items[first%max_n];}
  INLINE void popfront() {if (n!=0) {first++;--n;}}
  INLINE void popback()  {if (n!=0) {--n;}}
  INLINE T &addback() {
    if (n==max_n) popfront(); ++n;
    return items[(first+n-1)%max_n];
  }
  INLINE T &addfront() {
    if (n==max_n) popback(); ++n;
    return items[--first%max_n];
  }
  INLINE T &operator[](u32 idx) { return items[(first+idx)%max_n]; }
  INLINE u32 size() const {return n;}
  INLINE bool empty() const {return n==0;}
  INLINE bool full() const {return n==max_n;}
  T items[max_n];
  u32 first, n;
};

// very simple fixed size linear allocator
template <u32 size, u32 align=sizeof(int)>
struct DEFAULT_ALIGNED linear_allocator {
  INLINE linear_allocator() : head(data) {}
  INLINE char *newstring(const char *s, u32 l=~0u) {
    if (l==~0u) l=strlen(s);
    auto d = strncpy(alloc(l+1),s,l); d[l]=0;
    return d;
  }
  INLINE void rewind() { head=data; }
  INLINE char *alloc(int sz) {
    if (head+ALIGN(sz,align)>data+size) sys::fatal("out-of-memory");
    head += ALIGN(sz,align);
    return head-sz;
  }
  char *head, data[size];
};

/*-------------------------------------------------------------------------
 - pair
 -------------------------------------------------------------------------*/
template <typename T, typename U> struct pair {
  INLINE pair() {}
  INLINE pair(T t, U u) : first(t), second(u) {}
  T first; U second;
};
template <typename T, typename U>
INLINE bool operator==(const pair<T,U> &x0, const pair<T,U> &x1) {
  return x0.first == x1.first && x0.second == x1.second;
}
template <typename T, typename U>
INLINE pair<T,U> makepair(const T &t, const U &u){return pair<T,U>(t,u);}

/*-------------------------------------------------------------------------
 - very simple fixed size hash table (linear probing)
 -------------------------------------------------------------------------*/
template <typename T, u32 max_n=1024> struct hashtable : noncopyable {
  hashtable() : n(0) {}
  INLINE pair<const char*, T> *begin() { return items; }
  INLINE pair<const char*, T> *end() { return items+n; }
  INLINE void remove(pair<const char*, T> *it) {
    assert(it >= begin() && it < end());
    *it = items[(n--)-1];
  }
  T *access(const char *key, const T *value = NULL) {
    u32 h = 5381, i = 0;
	for (u32 k; (k = key[i]) != 0; ++i) h = ((h << 5) + h) ^ k;
	for (i = 0; i<n; ++i) { if (h == hashes[i] && !strcmp(key, items[i].first)) break; }
    if (value != NULL) {
      if (i>=max_n) sys::fatal("out-of-memory");
      items[i] = makepair(key,*value);
      hashes[i] = h;
      n = u32(i+1)>n?u32(i+1):n;
    }
    return i==n ? NULL : &items[i].second;
  }
  pair<const char*, T> items[max_n];
  u32 hashes[max_n], n;
};

/*-------------------------------------------------------------------------
 - simple resizeable vector
 -------------------------------------------------------------------------*/
template <class T> struct vector : noncopyable {
  T *buf;
  int alen, ulen;
  INLINE vector(int len = 0) {
    if (len) buf = (T*) MALLOC(len*sizeof(T)); else buf = NULL;
    loopi(len) sys::callctor<T>(buf+i);
    alen = ulen = len;
  }
  INLINE ~vector() { setsize(0); FREE(buf); }
  INLINE void destroy() { vector<T>().moveto(*this); }
  INLINE T &add(const T &x) {
    const T copy(x);
    if (ulen==alen) realloc();
    sys::callctor<T>(buf+ulen, copy);
    return buf[ulen++];
  }
  INLINE T &add() {
    if (ulen==alen) realloc();
    sys::callctor<T>(buf+ulen);
    return buf[ulen++];
  }
  pair<T*,u32> move() {
    const auto dst = makepair(buf,u32(ulen));
    alen = ulen = 0;
    buf = NULL;
    return dst;
  }
  INLINE void moveto(vector<T> &other) {
    other.~vector<T>();
    other.alen = alen; alen = 0;
    other.ulen = ulen; ulen = 0;
    other.buf = buf; buf = NULL;
  }
  INLINE void memset(u8 v) {::memset(buf, v, sizeof(T)*ulen);}
  T *begin() { return buf; }
  T *end() { return buf+ulen; }
  const T *begin() const { return buf; }
  const T *end() const { return buf+ulen; }
  INLINE T &pop() { return buf[--ulen]; }
  INLINE T &last() { return buf[ulen-1]; }
  INLINE bool empty() { return ulen==0; }
  INLINE int length() const { return ulen; }
  INLINE int size() const { return ulen*sizeof(T); }
  INLINE bool empty() const { return size()==0; }
  INLINE const T &operator[](int i) const { assert(i>=0 && i<ulen); return buf[i]; }
  INLINE T &operator[](int i) { assert(i>=0 && i<ulen); return buf[i]; }
  INLINE T *getbuf() { return buf; }
  INLINE void prealloc(int len) {
    if (len > alen) {
      alen = nextpowerof2(len);
      buf = (T*)REALLOC(buf, alen*sizeof(T));
    }
  }
  INLINE void realloc() {
    alen = 2*alen>1?2*alen:1;
    buf = (T*)REALLOC(buf, alen*sizeof(T));
  }

  void setsize(int i) {
    if (i<ulen)
      for(; ulen>i; --ulen) buf[ulen-1].~T();
    else if (i>ulen) {
      prealloc(i);
      for(; i>ulen; ++ulen) sys::callctor<T>(buf+ulen);
    }
  }

  T remove(int i) {
    T e = buf[i];
    for(int p = i+1; p<ulen; p++) buf[p-1] = buf[p];
    ulen--;
    return e;
  }

  T removeunordered(int i) {
    T e = buf[i];
    ulen--;
    if(ulen>0) buf[i] = buf[ulen];
    return e;
  }

  void removeobj(const T &o) {loopi(ulen) if(buf[i]==o) remove(i--);}
  T &insert(int i, const T &e) {
    add(T());
    for(int p = ulen-1; p>i; p--) buf[p] = buf[p-1];
    buf[i] = e;
    return buf[i];
  }

  void sort(void *cf) {
    qsort(buf, ulen, sizeof(T), (int (CDECL *)(const void *,const void *))cf);
  }

  static INLINE int heapparent(int i) { return (i - 1) >> 1; }
  static INLINE int heapchild(int i) { return (i << 1) + 1; }

  void buildheap() { for(int i = ulen/2; i >= 0; i--) downheap(i); }

  int upheap(int i) {
    auto score = buf[i];
    while (i > 0) {
      int pi = heapparent(i);
      if (!(score < buf[pi])) break;
      swap(buf[i], buf[pi]);
      i = pi;
    }
    return i;
  }

  T &addheap(const T &x) {
    add(x);
    return buf[upheap(ulen-1)];
  }

  int downheap(int i) {
    auto score = buf[i];
    for (;;) {
      int ci = heapchild(i);
      if (ci >= ulen) break;
      auto cscore = buf[ci];
      if (cscore < score) {
        if (ci+1 < ulen && buf[ci+1] < cscore) {
          swap(buf[ci+1], buf[i]);
          i = ci+1;
        } else {
          swap(buf[ci], buf[i]);
          i = ci;
        }
      } else if (ci+1 < ulen && buf[ci+1] < score) {
        swap(buf[ci+1], buf[i]);
        i = ci+1;
      } else
        break;
    }
    return i;
  }

  T removeheap() {
    T e = removeunordered(0);
    if (ulen) downheap(0);
    return e;
  }
};
typedef vector<char *> cvector;
typedef vector<int> ivector;

/*-------------------------------------------------------------------------
 - simple intrusive list
 -------------------------------------------------------------------------*/
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
    GBE_ASSERT(m_node);
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

/*-------------------------------------------------------------------------
 - atomics structure
 -------------------------------------------------------------------------*/
struct atomic : noncopyable {
public:
  INLINE atomic(void) {}
  INLINE atomic(s32 data) : data(data) {}
  INLINE atomic& operator =(const s32 input) { data = input; return *this; }
  INLINE operator s32(void) const { return data; }
  INLINE friend void storerelease(atomic &value, s32 x) { storerelease(&value.data, x); }
  INLINE friend s32 operator+=   (atomic &value, s32 input) { return atomic_add(&value.data, input) + input; }
  INLINE friend s32 operator++   (atomic &value) { return atomic_add(&value.data,  1) + 1; }
  INLINE friend s32 operator--   (atomic &value) { return atomic_add(&value.data, -1) - 1; }
  INLINE friend s32 operator++   (atomic &value, s32) { return atomic_add(&value.data,  1); }
  INLINE friend s32 operator--   (atomic &value, s32) { return atomic_add(&value.data, -1); }
  INLINE friend s32 cmpxchg      (atomic &value, s32 v, s32 c) { return atomic_cmpxchg(&value.data,v,c); }
private:
  volatile s32 data;
};

/*-------------------------------------------------------------------------
 - reference counted object
 -------------------------------------------------------------------------*/
template<typename T> struct ref {
  INLINE ref(void) : ptr(NULL) {}
  INLINE ref(const ref &input) : ptr(input.ptr) { if (ptr) ptr->acquire(); }
  INLINE ref(T* const input) : ptr(input) { if (ptr) ptr->acquire(); }
  INLINE ~ref(void) { if (ptr) ptr->release();  }
  INLINE operator bool(void) const       { return ptr != NULL; }
  INLINE operator T* (void)  const       { return ptr; }
  INLINE const T& operator* (void) const { return *ptr; }
  INLINE const T* operator->(void) const { return  ptr;}
  INLINE T& operator* (void) {return *ptr;}
  INLINE T* operator->(void) {return  ptr;}
  INLINE ref &operator= (const ref &input);
  INLINE ref &operator= (niltype);
  template<typename U> INLINE ref<U> cast(void) { return ref<U>((U*)(ptr)); }
  template<typename U> INLINE const ref<U> cast(void) const { return ref<U>((U*)(ptr)); }
  T* ptr;
};

struct refcount {
  INLINE refcount(void) : refcounter(0) {}
  virtual ~refcount(void) {}
  INLINE void acquire(void) { refcounter++; }
  INLINE void release(void) {
    if (--refcounter == 0) {
      auto ptr = this;
      DEL(ptr);
    }
  }
  atomic refcounter;
};

template <typename T>
INLINE ref<T> &ref<T>::operator= (const ref<T> &input) {
  if (input.ptr) input.ptr->acquire();
  if (ptr) ptr->release();
  *(T**)&ptr = input.ptr;
  return *this;
}
template <typename T>
INLINE ref<T> &ref<T>::operator= (niltype) {
  if (ptr) ptr->release();
  *(T**)&ptr = NULL;
  return *this;
}

/*-------------------------------------------------------------------------
 - sorting
 -------------------------------------------------------------------------*/
template<class T> INLINE bool compareless(const T &x, const T &y) { return x < y; }
template<class T, class F>
INLINE void insertionsort(T *start, T *end, F fun) {
  for(T *i = start+1; i < end; i++) {
    if(fun(*i, i[-1])) {
      T tmp = *i;
      *i = i[-1];
      T *j = i-1;
      for(; j > start && fun(tmp, j[-1]); --j)
        *j = j[-1];
      *j = tmp;
    }
  }
}

template<class T, class F>
INLINE void insertionsort(T *buf, int n, F fun) {
  insertionsort(buf, buf+n, fun);
}

template<class T>
INLINE void insertionsort(T *buf, int n) {
  insertionsort(buf, buf+n, compareless<T>);
}

template<class T, class F> void quicksort(T *start, T *end, F fun) {
  while(end-start > 10) {
    T *mid = &start[(end-start)/2], *i = start+1, *j = end-2, pivot;
    if(fun(*start, *mid)) { // start < mid 
      if(fun(end[-1], *start)) { pivot = *start; *start = end[-1]; end[-1] = *mid; } // end < start < mid 
      else if(fun(end[-1], *mid)) { pivot = end[-1]; end[-1] = *mid; } // start <= end < mid 
      else { pivot = *mid; } // start < mid <= end
    }
    else if(fun(*start, end[-1])) { pivot = *start; *start = *mid; } // mid <= start < end 
    else if(fun(*mid, end[-1])) { pivot = end[-1]; end[-1] = *start; *start = *mid; } //  mid < end <= start 
    else { pivot = *mid; swap(*start, end[-1]); }  // end <= mid <= start 
    *mid = end[-2];
    do {
      while(fun(*i, pivot)) if(++i >= j) goto partitioned;
      while(fun(pivot, *--j)) if(i >= j) goto partitioned;
      swap(*i, *j);
    } while(++i < j);
partitioned:
    end[-2] = *i;
    *i = pivot;

    if(i-start < end-(i+1)) {
      quicksort(start, i, fun);
      start = i+1;
    } else {
      quicksort(i+1, end, fun);
      end = i;
    }
  }

  insertionsort(start, end, fun);
}

template<class T, class F> INLINE void quicksort(T *buf, int n, F fun) {
  quicksort(buf, buf+n, fun);
}

template<class T> INLINE void quicksort(T *buf, int n) {
  quicksort(buf, buf+n, compareless<T>);
}
} /* namespace q */

