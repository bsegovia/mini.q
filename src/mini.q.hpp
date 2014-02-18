/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.hpp -> includes every game header
 -------------------------------------------------------------------------*/
#pragma once
#include "con.hpp"
#include "client.hpp"
#include "demo.hpp"
#include "entities.hpp"
#include "editing.hpp"
#include "game.hpp"
#include "iso.hpp"
#include "math.hpp"
#include "md2.hpp"
#include "menu.hpp"
#include "monster.hpp"
#include "network.hpp"
#include "ogl.hpp"
#include "renderer.hpp"
#include "physics.hpp"
#include "script.hpp"
#include "shaders.hpp"
#include "server.hpp"
#include "serverbrowser.hpp"
#include "sound.hpp"
#include "stl.hpp"
#include "sys.hpp"
#include "task.hpp"
#include "text.hpp"
#include "weapon.hpp"

namespace q {
void start();
void finish();
}

