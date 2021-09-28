struct RayPayLoad
{
  vec3 direction;
  vec3 albedo;
  vec3 normal;
  vec3 emission;
  vec3 origin;
  vec3 weight;
  uint seed;
  uint depth;
  uint type;     // 0 == view, 1 == shadow, 2 == bounce
  vec3 shadow_color;
  bool refractive;
};
