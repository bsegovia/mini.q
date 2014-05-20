/*-------------------------------------------------------------------------
 - boodew - a very simple and small (and slow) language based on strings
 -------------------------------------------------------------------------*/
#include <cstdarg>
#include <iostream>
#include "base/hash_map.hpp"
#include "base/console.hpp"
#include "boodew.hpp"

namespace q {
namespace boodew {

struct boodew_exception : std::exception {
  boodew_exception(string str) : str(str) {}
  ~boodew_exception() throw() {}
  const char *what() const throw() {return str.c_str();}
  string str;
};

struct stringstream {
  stringstream &operator<< (const string &str) {
    auto const sz = str.size();
    loopi(sz) buf.push_back(str[i]);
    return *this;
  }
  stringstream &operator<< (char c) {
    buf.push_back(c);
    return *this;
  }
  string str() const { return string(&buf[0], buf.size()); }
  vector<char> buf;
};

#define C(RET,NAME,S,D,B) RET NAME(const value &v) {\
      if (v.k==value::STR) return S;\
 else if (v.k==value::DOUBLE) return D;\
 else return B;\
}
C(string, vtos, v.s, to_string(v.d), v.b?"true":"false")
C(double, vtod, stod(v.s), v.d, v.b?1.0:0.0)
C(bool, vtob, !(v.s==string("false")), v.d!=0.0, v.b)
#undef C

template <typename T> T *unique() { static T internal; return &internal; }
const value &get(args arg, int idx) {
  if (arg.size() <= idx) throw boodew_exception("argument is missing");
  return arg[idx];
}

// builtins and global variables boiler plate
typedef hash_map<string, std::function<value(args)>> builtinmap;
bool new_builtin(const string &n, const builtin_type &fn) {
  return unique<builtinmap>()->insert(makepair(n,fn)), true;
}
typedef hash_map<string, std::function<value()>> cvar_map;
bool new_cvar(const string &n, const cvar_type &f0, const builtin_type &f1) {
  return unique<cvar_map>()->insert(makepair(n,f0)), new_builtin(n,f1);
}

// local variables use dynamic scope
typedef vector<hash_map<string,value>> stack;
static value new_local(const string &name, const value &v) {
  const auto s = unique<stack>();
  if (s->size() == 0) s->push_back(hash_map<string,value>());
  s->back()[name] = v;
  return v;
}
struct scope {
  scope() {unique<stack>()->push_back(hash_map<string,value>());}
  ~scope() {unique<stack>()->pop_back();}
};

static value getvar(args arg) {
  const auto key = vtos(get(arg,1));
  const auto sz = unique<stack>()->size();
  for (auto i = sz-1; i >= 0; --i) {
    const auto &frame = (*unique<stack>())[i];
    const auto local = frame.find(key);
    if (local != frame.end()) return local->second;
  }
  const auto it = unique<cvar_map>()->find(key);
  if (it != unique<cvar_map>()->end()) return it->second();
  throw boodew_exception(format("unknown identifier %s", key.c_str()));
}

static value ex(const string &s, size_t curr=0);
static pair<value,size_t> expr(const string &s, char c, size_t curr) {
  const char *match = c=='['?"[]@":"()";
  stringstream ss;
  size_t opened = 1, next = curr;
  while (opened) {
    if ((next = s.find_first_of(match,next+1)) == size_t(string::npos))
      throw boodew_exception(format("missing %c", c=='['?']':')'));
    if (c == '[' && s[next] == '@') {
      ss << s.substr(curr+1, next-curr-1);
      if (s[next+1] == '(') {
        const auto v = expr(s, '(', next+1);
        ss << vtos(v.first);
        if (s[v.second]!=']'||opened!=1) ss << s[v.second];
        curr = v.second;
        next = curr > 0 ? curr-1 : 0;
      } else
        do ss << s[curr = ++next]; while (s[next] == '@');
    } else
      opened += s[next] == c ? +1 : -1;
  }
  if (next>curr) ss << s.substr(curr+1, next-curr-1);
  return makepair((c=='[' ? stov(ss.str()) : ex(ss.str())), next+1);
}
static value ex(const string &s, size_t curr) {
  value ret, id;
  bool running = true;
  while (running) {
    vector<value> tok;
    for (;;) {
      const auto next = s.find_first_of("; \r\t\n[(", curr);
      const auto last = next == string::npos ? s.size() : next;
      const auto len = last-curr;
      const auto c = s[last];
      if (len!=0) tok.push_back(stov(s.substr(curr,len)));
      if (c == '(' || c == '[') {
        const auto v = expr(s, c, curr);
        tok.push_back(v.first);
        curr = v.second;
      } else if (c == ';' || c == '\n') {curr=last+1; break;}
      else if (c == '\0') {curr=last; running=false; break;}
      else curr=last+1;
    }

    if (tok.size() == 0) return btov(false);
    auto const it = unique<builtinmap>()->find(vtos(tok[0]));
    if (it!=unique<builtinmap>()->end()) // try to call a builtin
      ret = it->second(tok);
    else if (tok.size() == 1 && vtos(tok[0]) == s) return stov(s); // literals
    else { // function call
      scope frame;
      auto const sz = tok.size();
      for (int i = 1; i < sz; ++i) new_local(to_string(i-1),tok[i]);
      ret = ex(vtos(tok[0]));
    }
  }
  return ret;
}
static value while_builtin(args arg) {
  value last;
  while (vtob(ex(vtos(get(arg,1))))) try {return last=ex(vtos(get(arg,2))); }
    catch (bool b) { if (b) break; else continue; }
  return last;
}
static value loop_builtin(args arg) {
  value last;
  auto const n = int(vtod(get(arg,2)));
  for (int i = 0; i < n; ++i) {
    try {
      new_local(vtos(get(arg,1)), stov(to_string(i)));
      last = ex(vtos(get(arg,3)));
    } catch (bool b) { if (b) break; else continue; }
  }
  return last;
}
static value if_builtin(args arg) {
  return vtob(get(arg,1)) ? ex(vtos(get(arg,2))):
    (arg.size()>3?ex(vtos(get(arg,3))):btov(false));
}
#define O(S)BCMDL(#S,[](args arg){return dtov(vtod(get(arg,1)) S vtod(get(arg,2)));})
O(+) O(-) O(/) O(*)
#undef O
#define O(S)BCMDL(#S,[](args arg){return btov(vtod(get(arg,1)) S vtod(get(arg,2)));})
O(==) O(!=) O(<) O(>) O(<=) O(>=)
#undef O
BCMDL("int",[](args arg){return stov(to_string(int(vtod(get(arg,1)))));})
BCMDL("var",[](args arg){return new_local(vtos(get(arg,1)),arg.size()<3?btov(false):get(arg,2));})
BCMDL("#", [](args){return btov(false);})
BCMDL("..", [](args arg){return stov(vtos(get(arg,1))+vtos(get(arg,2)));})
BCMDL("echo", [](args arg){printf(vtos(get(arg,1)).c_str());return get(arg,1);})
BCMDL("^", [](args arg){return get(arg,1);})
BCMDL("return", [](args arg)->value {throw get(arg,1);})
BCMDL("do", [](args arg){try {return ex(vtos(get(arg,1)));} catch (value v) {return v;}})
BCMDL("break", [](args arg)->value {throw true;})
BCMDL("continue", [](args arg)->value {throw false;})
BCMDN("while", while_builtin)
BCMDN("loop", loop_builtin)
BCMDN("?", if_builtin)
BCMDN("$", getvar)
pair<string,bool> exec(const string &s) {
  try {ex(s); return makepair(string(),true);}
  catch (const boodew_exception &e) {return makepair(string(e.what()),false);}
}
} /* namespace boodew */
} /* namespace q */

