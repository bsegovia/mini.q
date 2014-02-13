vec3 diffuse(vec3 pos, vec3 nor, vec3 lpos, vec3 lpow) {
  vec3 ldir = lpos-pos;
  float llen2 = dot(ldir,ldir);
  return lpow * max(dot(nor,ldir),0.0) / (llen2*llen2);
}

