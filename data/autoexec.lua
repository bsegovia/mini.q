q.savepos = 1
q.showstats = 1
q.grabmouse = 1
q.soundvol = 100
q.editing = 1
q.bind('L', [[
  q.debug()
  q.linemode = 1
]]);
q.execscript('pos.lua')

