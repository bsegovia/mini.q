/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - sstream.hpp -> stringstream very cut down implementation
 -------------------------------------------------------------------------*/
#pragma once
#include "vector.hpp"
#include "string.hpp"

namespace q {
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
} /* namespace q */

