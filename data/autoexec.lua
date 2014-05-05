q.savepos = 1
q.showstats = 1
q.grabmouse = 0
q.soundvol = 100
q.editing = 1
q.bind('L', [[
  q.playerpos(21, 3, 11)
  q.playerypr(247, 9, 0)
  q.debug()
  q.linemode = 1
]]);
q.execscript('pos.lua')

