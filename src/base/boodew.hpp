/*-------------------------------------------------------------------------
 - boodew - a very simple and small (and slow) language based on strings
 -------------------------------------------------------------------------*/
#include "base/string.hpp"
#include "base/vector.hpp"
#include <functional>

namespace q {
namespace boodew {

struct value {
  enum kind {STR,DOUBLE,BOOL};
  string s;
  double d;
  bool b;
  kind k;
};
inline value stov(const string &s) {return {s,0.0,false,value::STR};}
inline value dtov(double d) {return {"",d,false,value::DOUBLE};}
inline value btov(bool b) {return {"",0.0,b,value::BOOL};}
string vtos(const value &v);
double vtod(const value &v);
bool vtob(const value &v);

// functions and builtins can have a variable number of arguments
typedef const vector<value> &args;
typedef std::function<value(args)> builtin_type;
typedef std::function<value()> cvar_type;

// extract the argument with extra checks
const value &get(args arg, int idx);

// append a new builtin in the (global and shared) boodew context
bool new_builtin(const string&, const builtin_type&);

// helper macros to do in c++ pre-main (oh yeah)
#define BCMDL(N, FN) auto JOIN(builtin,__COUNTER__) = new_builtin(N,FN);
#define BCMDN(N, FN) auto JOIN(builtin,FN) = new_builtin(N,FN);
#define BCMD(N) CMDN(N,#N)

// append a global variable
bool new_cvar(const string&, const cvar_type&, const builtin_type&);

// helper macros to do at pre-main (integer value here)
#define BIVAR(N,MIN,CURR,MAX) int N = CURR;\
auto JOIN(cvar,__COUNTER__) =\
  q::boodew::new_cvar(#N,[](){return q::boodew::dtov(N);}, [](q::boodew::args arg) {\
    const auto x = int(vtod(q::boodew::get(arg,1)));\
    if (x>=MIN && x<=MAX) N=x;\
    else printf("range for %s is (%d,%d)\n",#N,MIN,MAX);\
    return q::boodew::get(arg,1);\
  });
pair<string,bool> exec(const string&);
} /* namespace boodew */
} /* namespace q */

