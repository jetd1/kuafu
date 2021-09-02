layout( binding = 3, set = 1 ) readonly uniform DirectionalLightProperties
{
  vec4 direction;       // vec3 d + float softness
  vec4 rgbs;            // vec3 rgb + float strength
}
dlight;
