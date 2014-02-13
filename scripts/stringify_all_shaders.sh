#!/bin/bash
SHADERS=`ls data/shaders/*glsl`

##########################################################################
# emit the glsl source code for all shaders
##########################################################################
echo "/*-------------------------------------------------------------------------" > $1
echo " - mini.q - a minimalistic multiplayer FPS" >> $1
echo " - shaders.cxx -> stores shaders (do not modify)" >> $1
echo " -------------------------------------------------------------------------*/" >> $1

for shader in ${SHADERS}
do
  ./scripts/stringify_shader.sh ${shader} >> $1
done
echo "" >> $1

##########################################################################
# emit the shader declarations
##########################################################################
echo "/*-------------------------------------------------------------------------" > $2
echo " - mini.q - a minimalistic multiplayer FPS" >> $2
echo " - shaders.hxx -> declare all shaders (do not modify)" >> $2
echo " -------------------------------------------------------------------------*/" >> $2
echo "#pragma once" >> $2
echo "namespace q {" >> $2
echo "namespace shaders {" >> $2

for shader in ${SHADERS}
do
  file=$(basename $shader)
  echo $file | sed 's/\(.*\)\.glsl/extern const char \1[];/' >> $2
done
echo "} /* namespace shaders */" >> $2
echo "} /* namespace q */" >> $2
echo "" >> $2

##########################################################################
# emit the extra makefile dependencies
##########################################################################
echo "SHADERS=\\" > $3
for shader in ${SHADERS}
do
  file=$(basename $shader)
  echo "$shader\\" >> $3
done
echo "" >> $3

