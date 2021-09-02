struct Material
{
  vec4 diffuse;   // vec3 diffuse  + hasTex
  vec4 emission;  // vec3 emission  + emissionStrength
  float alpha;
  float metallic;
  float specular;
  float roughness;
  float ior;
  float transmission;

  uint texIdx;

  uint padding0;
};

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec3 color;
  vec2 texCoord;
};

struct GeometryInstance
{
  mat4 transform;
  uint geometryIndex;

  uint padding0;
  uint padding1;
  uint padding2;
};
