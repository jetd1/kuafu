struct Material
{
  vec4 diffuse;   // vec3 diffuse  + padding
  vec4 emission;  // vec3 emission  + emissionStrength
  float alpha;
  float metallic;
  float specular;
  float roughness;
  float ior;
  float transmission;

  int diffuseTexIdx;
  int metallicTexIdx;
  int roughnessTexIdx;
  int transmissionTexIdx;

  uint padding0;
  uint padding1;
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
