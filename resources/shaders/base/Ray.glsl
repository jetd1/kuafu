struct RayPayLoad
{
  vec3 direction;
  vec3 emission;
  vec3 origin;
  vec3 weight;
  uint seed;
  uint depth;
  bool reflective;
  bool refractive;
};
