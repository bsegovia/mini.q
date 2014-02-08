#!/bin/bash
SHADERS=`ls data/shaders/*glsl`

echo "/*-------------------------------------------------------------------------" > $1
echo " - mini.q - a minimalistic multiplayer FPS" >> $1
echo " - shaders.cxx -> stores shaders (do not modify)" >> $1
echo " -------------------------------------------------------------------------*/" >> $1

for shader in ${SHADERS}
do
  ./scripts/stringify_shader.sh ${shader} >> $1
done
echo "" >> $1

