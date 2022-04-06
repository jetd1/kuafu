layout ( constant_id = 0 ) const uint MAX_POINT_LIGHTS = 32;
layout ( constant_id = 1 ) const uint MAX_ACTIVE_LIGHTS = 8;

layout( binding = 3, set = 1 ) readonly uniform DirectionalLightProperties
{
  vec4 direction;       // vec3 d + float softness
  vec4 rgbs;            // vec3 rgb + float strength
}
dlight;


layout( binding = 4, set = 1 ) readonly uniform PointLightsProperties
{
  vec4 posr[MAX_POINT_LIGHTS];       // vec3 pos + float radius
  vec4 rgbs[MAX_POINT_LIGHTS];       // vec3 rgb + float strength
}
plights;


layout( binding = 5, set = 1 ) readonly uniform ActiveLightsProperties
{
  mat4   viewMat[MAX_ACTIVE_LIGHTS];         // mat44 viewMat
  mat4   projMat[MAX_ACTIVE_LIGHTS];         // mat33 projMat
  vec4   front[MAX_ACTIVE_LIGHTS];           // vec3 front + bool in_use?
  vec4   rgbs[MAX_ACTIVE_LIGHTS];            // vec3 color + float strength
  vec4   position[MAX_ACTIVE_LIGHTS];
  vec4   sftp[MAX_ACTIVE_LIGHTS];            // softness, fov, texID, padding

//  float  softness[MAX_ACTIVE_LIGHTS];
//  float  fov[MAX_ACTIVE_LIGHTS];
//  uint   texID[MAX_ACTIVE_LIGHTS];
//  uint   padding[MAX_ACTIVE_LIGHTS];
}
alights;
