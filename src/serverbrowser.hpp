/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - serverbrowser.hpp -> exposes routines to list servers
 -------------------------------------------------------------------------*/
#pragma once
namespace q {
namespace browser {

void addserver(const char *servername);
const char *getservername(int n);
void refreshservers(void);
void writeservercfg(void);

} /* namespace browser */
} /* namespace q */

