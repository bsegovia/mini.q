/*-------------------------------------------------------------------------
 - boodew - a very simple and small (and slow) language based on strings
 -------------------------------------------------------------------------*/
#include <cstdarg>
#include <iostream>
#include "base/hash_map.hpp"
#include "base/sstream.hpp"
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
string vtos(const value &v) {
      if (v.k==value::STR) return v.s;
 else if (v.k==value::DOUBLE) return to_string(v.d);
 else return v.b?"true":"false";
}
double vtod(const value &v) {
      if (v.k==value::STR) return stod(v.s);
 else if (v.k==value::DOUBLE) return v.d;
 else return v.b?1.0:0.0;
}
bool vtob(const value &v) {
      if (v.k==value::STR) return !(v.s==string("false"));
 else if (v.k==value::DOUBLE) return v.d!=0.0;
 else return v.b;
}
INLINE value verr(const string &s) {return {s,0.0,false,value::ERROR};}
INLINE value vbreak() {return {"",0.0,false,value::BREAK};}
INLINE value vcont() {return {"",0.0,false,value::CONTINUE};}
INLINE bool earlyout(const value &v) {
  return v.k==value::BREAK || v.k==value::CONTINUE || v.k==value::ERROR ||
         (v.k&value::RETURN) != 0;
}
template <typename T> T *unique() { static T internal; return &internal; }
const value &get(args arg, int idx) {
  return arg[arg.size()<=idx ? arg.size()-1 : idx];
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
  return verr(format("unknown identifier %s", key.c_str()));
}

static value ex(const string &s, size_t curr=0);
static pair<value,size_t> expr(const string &s, char c, size_t curr) {
  const char *match = c=='['?"[]@":"()";
  stringstream ss;
  size_t opened = 1, next = curr;
  while (opened) {
    if ((next = s.find_first_of(match,next+1)) == size_t(string::npos))
      return makepair(verr(format("missing %c", c=='['?']':')')),0);
    if (c == '[' && s[next] == '@') {
      ss << s.substr(curr+1, next-curr-1);
      if (s[next+1] == '(') {
        const auto v = expr(s, '(', next+1);
        if (earlyout(v.first)) return v;
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
        if (earlyout(v.first)) return v.first;
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
      if (earlyout(ret)) return ret;
    }
  }
  return ret;
}
static value while_builtin(args arg) {
  value last;
  while (vtob(ex(vtos(get(arg,1))))) {
    last = ex(vtos(get(arg,2)));
    if (last.k == value::BREAK || last.k == value::RETURN) break;
  }
  return last;
}
static value loop_builtin(args arg) {
  value last;
  auto const n = int(vtod(get(arg,2)));
  for (int i = 0; i < n; ++i) {
    new_local(vtos(get(arg,1)), stov(to_string(i)));
    last = ex(vtos(get(arg,3)));
    if (last.k == value::BREAK || last.k == value::RETURN) break;
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
BCMDL("return", [](args arg){auto v=get(arg,1); v.k|=value::RETURN; return v;})
BCMDL("do", [](args arg){auto v=ex(vtos(get(arg,1))); v.k&=~value::RETURN; return v;})
BCMDL("break", [](args arg){return vbreak();})
BCMDL("continue", [](args arg){return vcont();})
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

