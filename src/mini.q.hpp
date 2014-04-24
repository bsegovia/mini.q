/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.hpp -> includes every game header
 -------------------------------------------------------------------------*/
#pragma once
#include "client.hpp"
#include "demo.hpp"
#include "entities.hpp"
#include "editing.hpp"
#include "game.hpp"
#include "md2.hpp"
#include "menu.hpp"
#include "monster.hpp"
#include "network.hpp"
#include "ogl.hpp"
#include "renderer.hpp"
#include "physics.hpp"
#include "shaders.hpp"
#include "server.hpp"
#include "serverbrowser.hpp"
#include "sound.hpp"
#include "text.hpp"
#include "weapon.hpp"
#include "world.hpp"
#include "base/console.hpp"
#include "base/math.hpp"
#include "base/script.hpp"
#include "base/sys.hpp"
#include "base/task.hpp"

namespace q {
extern int fov, farplane, gamespeed, minmillis;
void start(int argc, const char *argv[]);
void finish();
void swap();
}

