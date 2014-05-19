-- standard menu definitions
-- don't modify, add personal menus to autoexec.cfg instead
bind('ESCAPE', 'showmenu("main")') -- it all starts here

--[[ main ]]--
newmenu('main')
menuitem('load map..','showmenu("maps")')
menuitem('singleplayer..','showmenu("singleplayer")')
menuitem('multiplayer..','showmenu("multiplayer")')
menuitem('tweaking..','showmenu("tweaking")')
menuitem('editing..','showmenu("editing")')
menuitem('about..','showmenu("about")')
menuitem('quit', 'quit(nil)')

--[[ about ]]--
newmenu('about')
menuitem('mini.q game/engine','')
menuitem('by shadowofq','')

--[[ maps ]]--
local function genmapitems(...)
   for _,v in ipairs{...} do
     menuitem(v, '') -- add next map here!
   end
end
newmenu('maps')
genmapitems('frag','ogrosupply','powerplant','aquae','drianmp3','douze','kmap5','q3dm2','uf','mak1','kmap6','metl2','mak2')
menuitem('more maps (1) ..', 'showmenu("moremaps")')
menuitem('more maps (2) ..', 'showmenu("evenmoremaps")')
menuitem('more maps (3) ..', 'showmenu("yetmoremaps")')

--[[ moremaps ]]--
newmenu('moremaps')
genmapitems('caged', 'wandering', 'oddworld', 'kmap3', 'hellsgate', 'kmap4', 'nudist', 'dusk', 'biologie', 'cellar', 'b2k', 'metl1', 'inkedskin', 'gzdm1', 'aard3', 'tartech', 'zippie')

--[[ evenmoremaps ]]--
newmenu('evenmoremaps')
genmapitems('spillway', 'fsession', '32', 'templemount', 'minion1', 'infertile', 'gdb1', 'aard1', 'aard2', 'kmap2', 'matador', 'rpgcb01', 'ludm1', 'taurus', 'ger1', 'dragon', 'af')

--[[ yetmoremaps ]]--
newmenu('yetmoremaps')
genmapitems('kmap1', 'drianmp2', 'island_pre', 'style', 'hylken5', 'gib', 'aard1_remix', 'artanis', 'tongues', 'plagiat', 'thearit', 'attacko', 'metalv2', 'lbase', 'fox', 'darth', 'templeofdespair')

--[[ spmaps ]]--
newmenu('spmaps')
genmapitems('fanatic/revenge', 'dcp_the_core/enter', 'mpsp4', 'mpsp5', 'kitchensink', 'ruins', 'rampage', 'ksp1', 'ksp2', 'egysp1', 'kksp1', 'kartoffel', 'vaterland')

--[[ singleplayer ]]--
newmenu('singleplayer')
menuitem('start SP map..','mode(-2); showmenu("spmaps")')
menuitem('start DMSP map..','mode(-1); showmenu("maps")')
menuitem('change skill level','showmenu("skill")')
menuitem('savegame quicksave', 'savegame("quicksave")')
menuitem('loadgame quicksave', 'loadgame("quicksave")')

--[[ skill ]]--
newmenu('skill')
menuitem('skill 1', 'skill = 1')
menuitem('skill 2', 'skill = 2')
menuitem('skill 3 (default)',  'skill = 3')
menuitem('skill 4', 'skill = 4')
menuitem('skill 5', 'skill = 5')
menuitem('skill 6', 'skill = 6')
menuitem('skill 7', 'skill = 7')
menuitem('skill 8', 'skill = 8')
menuitem('skill 9', 'skill = 9')
menuitem('skill 10', 'skill = 10')

--[[ multiplayer ]]--
newmenu('multiplayer')
menuitem('server browser..', 'servermenu()')
menuitem('vote game mode / map ..', 'showmenu("gamemode")')
menuitem('connect localhost', 'connect("localhost")')
menuitem('update server list from master server', 'updatefrommaster()')
menuitem('disconnect', 'disconnect()')
menuitem('team red', 'team("red")')
menuitem('team blue', 'team("blue")')
menuitem('record tempdemo', 'record("tempdemo")')
menuitem('demo tempdemo', 'demo("tempdemo")')
menuitem('stop demo play/recording', 'stop()')

--[[ gamemode ]]--
newmenu('gamemode')
menuitem('ffa/default',        'mode(0);  showmenu("maps")')
menuitem('coop edit',          'mode(1);  showmenu("maps")')
menuitem('ffa/duel',           'mode(2);  showmenu("maps")')
menuitem('teamplay',           'mode(3);  showmenu("maps")')
menuitem('instagib',           'mode(4);  showmenu("maps")')
menuitem('instagib team',      'mode(5);  showmenu("maps")')
menuitem('efficiency',         'mode(6);  showmenu("maps")')
menuitem('efficiency team',    'mode(7);  showmenu("maps")')
menuitem('insta arena',        'mode(8);  showmenu("maps")')
menuitem('insta clan arena',   'mode(9);  showmenu("maps")')
menuitem('tactics arena',      'mode(10); showmenu("maps")')
menuitem('tactics clan arena', 'mode(11); showmenu("maps")')

--[[ fov ]]--
newmenu('fov')
menuitem('fov 120', 'fov=120')
menuitem('fov 110', 'fov=110')
menuitem('fov 100', 'fov=100')
menuitem('fov 90', 'fov=90')
menuitem('fov 60', 'fov=60')
menuitem('fov 30', 'fov=30')

--[[
newmenu teleport
menuitem "newent teleport 1"
menuitem "newent teledest 1"
menuitem "newent teleport 2"
menuitem "newent teledest 2"
menuitem "newent teleport 3"
menuitem "newent teledest 3"
menuitem "newent teleport 4"
menuitem "newent teledest 4"

newmenu mapmodels

menuitem "bridge (height 4)" "newent mapmodel 1 4"

menuitem "tree1"           "newent mapmodel 0"
menuitem "tree2 (huge)"    "newent mapmodel 2"
menuitem "tree3"           "newent mapmodel 3"
menuitem "palmtree"        "newent mapmodel 4"
menuitem "thorns"          "newent mapmodel 5"
menuitem "plant1"          "newent mapmodel 6"
menuitem "plant2"          "newent mapmodel 15"
menuitem "grass (tiny)"    "newent mapmodel 7"
menuitem "ivy (height 10)" "newent mapmodel 8 10"
menuitem "leafs"           "newent mapmodel 15"
menuitem "mushroom"        "newent mapmodel 19"


menuitem "more mapmodels..."   "showmenu moremapmodels"
menuitem "even more mapmodels..." "showmenu evenmoremapmodels"
menuitem "tentus 1..." "showmenu tentus1"
menuitem "tentus 2..." "showmenu tentus2"

newmenu moremapmodels

menuitem "streetlamp (modern)" "newent mapmodel 10"
menuitem "bench (modern)"      "newent mapmodel 11"
menuitem "palette (modern)"    "newent mapmodel 17"

menuitem "barrel (ind)"        "newent mapmodel 9"
menuitem "pipe/valve (ind)"    "newent mapmodel 16"
menuitem "palette (ind)"       "newent mapmodel 17"
menuitem "valve (ind)"         "newent mapmodel 16"
menuitem "vent (ind)"          "newent mapmodel 18"

menuitem "pillar (egypt)"      "newent mapmodel 12"
menuitem "waterbowl (egypt)"   "newent mapmodel 13"

menuitem "jumppad (tech/sf)"    "newent mapmodel 14"
menuitem "biotank (tech/sf)"    "newent mapmodel 20"
menuitem "console (tech/sf)"    "newent mapmodel 22"
menuitem "groundlamp (tech/sf)" "newent mapmodel 21"
menuitem "turret (tech/sf)"     "newent mapmodel 23"

newmenu evenmoremapmodels

menuitem "cask"			"newent mapmodel 24"
menuitem "cart"			"newent mapmodel 25"
menuitem "candle"		"newent mapmodel 26"
menuitem "vase"			"newent mapmodel 27"
menuitem "sack"			"newent mapmodel 28"
menuitem "chandelier"	"newent mapmodel 29"
menuitem "chest"		"newent mapmodel 30"
menuitem "firebowl"		"newent mapmodel 31"
menuitem "smplant"		"newent mapmodel 32"
menuitem "insect"		"newent mapmodel 33"
menuitem "reed"			"newent mapmodel 34"
menuitem "qb2x2"		"newent mapmodel 35"
menuitem "qb4x4"		"newent mapmodel 36"
menuitem "qb8x8"		"newent mapmodel 37"

menuitem "nocamp"		"newent mapmodel 38"
menuitem "strahler"		"newent mapmodel 39"

newmenu tentus1

menuitem "goblet"					"newent mapmodel 40"
menuitem "apple"					"newent mapmodel 41"
menuitem "pear"						"newent mapmodel 42"
menuitem "appleslice"				"newent mapmodel 43"
menuitem "meat"						"newent mapmodel 44"
menuitem "bowl"						"newent mapmodel 45"
menuitem "pieslice"					"newent mapmodel 46"
menuitem "mug"						"newent mapmodel 47"
menuitem "winebottle"				"newent mapmodel 48"
menuitem "pie"						"newent mapmodel 49"
menuitem "books flat"				"newent mapmodel 50"
menuitem "books multi"				"newent mapmodel 51"
menuitem "chain"					"newent mapmodel 52"
menuitem "curvechain"				"newent mapmodel 53"
menuitem "barrel"					"newent mapmodel 54"

newmenu tentus2

menuitem "sidebarrel"				"newent mapmodel 55"
menuitem "pot1"						"newent mapmodel 56"
menuitem "rope"						"newent mapmodel 57"
menuitem "ropelamp"					"newent mapmodel 58"
menuitem "ladder"					"newent mapmodel 59"
menuitem "fattree"					"newent mapmodel 60"
menuitem "moneybag"					"newent mapmodel 61"
menuitem "woodbench"				"newent mapmodel 62"
menuitem "hammer"					"newent mapmodel 63"
menuitem "anvil"					"newent mapmodel 64"
menuitem "spear"					"newent mapmodel 65"
menuitem "key"						"newent mapmodel 66"
menuitem "redshield"				"newent mapmodel 67"
menuitem "greenshield"				"newent mapmodel 68"
menuitem "bombs"					"newent mapmodel 69"
menuitem "                          "newent mapmodel   "

newmenu monster

menuitem "ogro / fireball"      "newent monster 0"
menuitem "rhino / chaingun"      "newent monster 1"
menuitem "ratamahatta / shotgun"      "newent monster 2"
menuitem "slith / rifle" "newent monster 3"
menuitem "bauul / RL"     "newent monster 4"
menuitem "hellpig / bite"       "newent monster 5"
menuitem "knight / iceball" "newent monster 6"
menuitem "goblin / slimeball"   "newent monster 7"
]]--
