// standard menu definitions
// don't modify, add personal menus to autoexec.cfg instead

bind ESCAPE "showmenu main"    // it all starts here


newmenu main

menuitem "load map.."       "showmenu maps"
menuitem "singleplayer.."   "showmenu singleplayer"
menuitem "multiplayer.."    "showmenu multiplayer"
menuitem "tweaking.."       "showmenu tweaking"
menuitem "editing.."        "showmenu editing"
menuitem "about.."          "showmenu about"
menuitem "quit"

newmenu about

menuitem "Cube game/engine" " "
menuitem "by Wouter van Oortmerssen" " "
menuitem "http://www.cubeengine.com/" "echo sorry no built-in web browser yet :)"



alias genmapitems [
    alias n (listlen $arg1)
    loop $n [
        at $arg1 $i
        alias mapname $s
        concat map $s
        menuitem $s
        alias next (+ $i 1)
        if (= $next $n) [ alias next 0 ] []
        at $arg1 $next
        alias nextmap $s
        concatword nextmap_ $mapname
        alias $s $nextmap
    ]
]

newmenu maps

genmapitems "metl3 frag ogrosupply powerplant aquae drianmp3 douze kmap5 q3dm2 uf mak1 kmap6 metl2 mak2"
menuitem "more maps (1) .." "showmenu moremaps"
menuitem "more maps (2) .." "showmenu evenmoremaps"
menuitem "more maps (3) .." "showmenu yetmoremaps"

newmenu moremaps

genmapitems "caged wandering oddworld kmap3 hellsgate kmap4 nudist dusk biologie cellar b2k metl1 inkedskin gzdm1 aard3 tartech zippie"

newmenu evenmoremaps

genmapitems "spillway fsession 32 templemount minion1 infertile gdb1 aard1 aard2 kmap2 matador rpgcb01 ludm1 taurus ger1 dragon af"

newmenu yetmoremaps

genmapitems "kmap1 drianmp2 island_pre style hylken5 gib aard1_remix artanis tongues plagiat thearit attacko metalv2 lbase fox darth templeofdespair"

newmenu spmaps

genmapitems "fanatic/revenge dcp_the_core/enter mpsp4 mpsp5 kitchensink ruins rampage ksp1 ksp2 egysp1 kksp1 kartoffel vaterland" 
menuitem "more sp maps.." "showmenu morespmaps"
menuitem "even more sp maps.." "showmenu evenmorespmaps"
menuitem "trickjumping test!" "map tricky"
menuitem "mapmodel demo" "map mapmodel_demo"

newmenu morespmaps

genmapitems "sp_infidel mpsp1 mpsp2 mpsp3 wsg1 wsg2 wsg3 sp_af tut1 camera nsp3 nsp2 mm5remix desert10 cube101"

newmenu evenmorespmaps

genmapitems "cruel01 cruel02 sp1-test tta sp_stalingrad"

menuitem "pigcam episode..." "sp pigcam/pig01"
menuitem "jf sp episode..." "sp jfsp01"

newmenu singleplayer

menuitem "start SP map.." "mode -2; showmenu spmaps"
menuitem "start DMSP map.." "mode -1; showmenu maps"
menuitem "change skill level" "showmenu skill"
menuitem "savegame quicksave"
menuitem "loadgame quicksave"

newmenu skill

menuitem "skill 1"
menuitem "skill 2"
menuitem "skill 3 (default)" "skill 3"
menuitem "skill 4"
menuitem "skill 5"
menuitem "skill 6"
menuitem "skill 7"
menuitem "skill 8"
menuitem "skill 9"
menuitem "skill 10"

newmenu multiplayer

menuitem "server browser.."  "servermenu"
menuitem "vote game mode / map .." "showmenu gamemode"
menuitem "connect localhost"
menuitem "update server list from master server" "updatefrommaster"
menuitem "disconnect"
menuitem "team red"
menuitem "team blue"
menuitem "record tempdemo"
menuitem "demo tempdemo"
menuitem "stop demo play/recording"

newmenu gamemode

menuitem "ffa/default"        "mode 0;  showmenu maps"
menuitem "coop edit"          "mode 1;  showmenu maps"
menuitem "ffa/duel"           "mode 2;  showmenu maps"
menuitem "teamplay"           "mode 3;  showmenu maps"
menuitem "instagib"           "mode 4;  showmenu maps"
menuitem "instagib team"      "mode 5;  showmenu maps"
menuitem "efficiency"         "mode 6;  showmenu maps"
menuitem "efficiency team"    "mode 7;  showmenu maps"
menuitem "insta arena"        "mode 8;  showmenu maps"
menuitem "insta clan arena"   "mode 9;  showmenu maps"
menuitem "tactics arena"      "mode 10; showmenu maps"
menuitem "tactics clan arena" "mode 11; showmenu maps"

newmenu tweaking

menuitem "tweak fov.."                                              "showmenu fov"
menuitem "tweak water subdivision.."                                "showmenu watersubdiv"
menuitem "turn dynamic light/shadows off"                           "dynlight 0"
menuitem "tweak gamma.."                                            "showmenu gamma"

newmenu gamma

menuitem "gamma 50"
menuitem "gamma 60"
menuitem "gamma 70"
menuitem "gamma 80"
menuitem "gamma 90"
menuitem "gamma 100"
menuitem "gamma 110"
menuitem "gamma 120"
menuitem "gamma 130"
menuitem "gamma 140"
menuitem "gamma 150"

newmenu fov

menuitem "fov 120"
menuitem "fov 110"
menuitem "fov 100"
menuitem "fov 90"
menuitem "fov 60"
menuitem "fov 30"

newmenu watersubdiv

menuitem "subdivision 1  (insane polygon wastage)"  "watersubdiv 1"
menuitem "subdivision 2  (high quality)"            "watersubdiv 2"
menuitem "subdivision 4  (medium quality)"          "watersubdiv 4"
menuitem "subdivision 8  (low quality)"             "watersubdiv 8"
menuitem "subdivision 16 (my 486 can do water too)" "watersubdiv 16"

newmenu editing

menuitem "insert entity at selection corner.."              "showmenu newent"
menuitem "toggle edit mode (key E)"                         "edittoggle"
menuitem "undo last edit action (key U)"                    "undo"
menuitem "copy selection (key C)"                           "copy"
menuitem "paste selection (red vertex = upper left, key V)" "paste"
menuitem "repeat last texture replace"                      "replace"
menuitem "set tag.."                                        "showmenu tags"
menuitem "delete closest entity (key X)"                    "delent"
menuitem "delete all lights"                                "clearents light"
menuitem "recalc light (key R)"                             "recalc"
menuitem "toggle show geometric mipmaps (bigger = better)"  "showmip"
menuitem "arches and slopes.."                              "showmenu arches"
menuitem "map operations.."                                 "showmenu mapop"
menuitem "help: show more editing keys..."                  "showmenu editkeys"

newmenu tags

menuitem "set tag 0 (no tag)" "edittag 0"
menuitem "set tag 1"          "edittag 1"
menuitem "set tag 2"          "edittag 2"
menuitem "set tag 3"          "edittag 3"
menuitem "set tag 4"          "edittag 4"
menuitem "set tag 5"          "edittag 5"
menuitem "set tag 6"          "edittag 6"
menuitem "set tag 7"          "edittag 7"

newmenu mapop

menuitem "save temp map"                      "savemap temp"
menuitem "load temp map"                      "map temp"
menuitem "newmap 64x64 cubes"                 "newmap 6"
menuitem "newmap 128x128 cubes (recommended)" "newmap 7"
menuitem "newmap 256x256 cubes"               "newmap 8"
menuitem "newmap 512x512 cubes"               "newmap 9"
menuitem "newmap 1024x1024 cubes"             "newmap 10"
menuitem "increase mapsize (2x)"              "mapenlarge"
menuitem "make temp fullbright"               "fullbright"
menuitem "toggle occlusion culling"           "toggleocull"
menuitem "set map title/author"               "saycommand /mapmsg"

newmenu newent

menuitem "mapmodels.."      "showmenu mapmodels"
menuitem "white light.."    "showmenu light"
menuitem "coloured light.." "showmenu colour"
menuitem "teleport.."       "showmenu teleport"
menuitem "monster.."        "showmenu monster"
menuitem "trigger.."        "showmenu trigger"
menuitem "newent playerstart"
menuitem "newent shells"
menuitem "newent bullets"
menuitem "newent rockets"
menuitem "newent riflerounds"
menuitem "newent health"
menuitem "newent healthboost"
menuitem "newent greenarmour"
menuitem "newent yellowarmour"
menuitem "newent quaddamage"

newmenu trigger

loop 9 [ concat newent trigger (+ $i 1) 0; menuitem $s ]

newmenu light

menuitem "newent light  4 255"
menuitem "newent light  8 255"
menuitem "newent light 12 255"
menuitem "newent light 16 255"
menuitem "newent light  4 192"
menuitem "newent light  8 192"
menuitem "newent light 12 192"
menuitem "newent light 16 192"

newmenu colour

menuitem "blue"      "showmenu blue"
menuitem "red"       "showmenu red"
menuitem "green"     "showmenu green"
menuitem "yellow"    "showmenu yellow"
menuitem "purple"    "showmenu purple"
menuitem "turquoise" "showmenu turquoise"

alias colourmenu [
  newmenu $arg1
  colourhalf
  colourhalf "" (div $arg2 2) (div $arg3 2) (div $arg4 2)
]

alias colourhalf [
  loop 4 [
    at "4 8 12 16" $i;
    concat newent light $s $arg2 $arg3 $arg4;
    menuitem $s
  ]
]

colourmenu blue      192 192 255
colourmenu red       255 192 192
colourmenu green     192 255 192
colourmenu yellow    255 255 192
colourmenu purple    255 192 255
colourmenu turquoise 192 255 255

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

newmenu arches

menuitem "normal arch"                        "heightfield 2; arch 0"
menuitem "arch with sideways curve (delta 1)" "heightfield 2; arch 1"
menuitem "arch with sideways curve (delta 2)" "heightfield 2; arch 2"
menuitem "arch with sideways curve (delta 3)" "heightfield 2; arch 3"
menuitem "arch with sideways curve (delta 4)" "heightfield 2; arch 4"
menuitem "slope increase 2 left-right"        "heightfield 0; slope 2 0"
menuitem "slope decrease 2 left-right"        "heightfield 0; slope -2 0"
menuitem "slope increase 2 top-bottom"        "heightfield 0; slope 0 2"
menuitem "slope decrease 2 top-bottom"        "heightfield 0; slope 0 -2"
menuitem "[ arches make ceiling heighfield, slopes floor ]"

newmenu editkeys

menuitem "insert/home/pgup/keyp7 browse through floor/wall/ceiling/" ""
menuitem "      upper textures on your current selection,"           ""
menuitem "      delete/end/pgup/keyp4 browse backwards"              ""
menuitem "[ and ] move the currently selected floor up and down"     ""
menuitem "      O and P do the same for the ceiling"                 ""
menuitem "F makes the current selection SOLID, G makes it SPACE"     ""
menuitem "K makes something a corner (slant)"                        ""
menuitem ", and . equalize the floor/ceiling ceiling level"          ""
menuitem "H makes the floor a heightfield, I the ceiling"            ""
menuitem "8/9 change the offset of a vertex in a heightfield"        ""
