#!/bin/bash
SHADERS=`ls data/shaders/*glsl`

echo "/*-------------------------------------------------------------------------" > $1
echo " - mini.q - a minimalistic multiplayer FPS" >> $1
echo " - shaders.cpp -> stores shaders (do not modify)" >> $1
echo " -------------------------------------------------------------------------*/" >> $1
echo "#include \"shaders.hpp\"" >> $1
echo "namespace q {" >> $1
echo "namespace shaders {" >> $1

for shader in ${SHADERS}
do
  ./scripts/stringify_shader.sh ${shader} >> $1
done

echo "} /* namespace q */" >> $1
echo "} /* namespace shaders */" >> $1
echo "" >> $1

