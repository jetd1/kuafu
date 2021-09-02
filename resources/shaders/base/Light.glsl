layout ( constant_id = 0 ) const uint MAX_POINT_LIGHTS = 4;

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