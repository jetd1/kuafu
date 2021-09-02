#version 460
#extension GL_EXT_ray_tracing : require

#include "base/Ray.glsl"
#include "base/PushConstants.glsl"

layout( location = 0 ) rayPayloadInEXT RayPayLoad ray;


layout( binding = 2, set = 1 ) uniform samplerCube environmentMap;

void main( )
{
  vec3 dir = ray.direction;
  dir.x *= -1.0; // mirror

  if ( ray.depth == 0 )     // view ray
    if ( useEnvironmentMap )
      ray.emission = texture( environmentMap, dir ).xyz * 0.8F;    // TODO: kuafu_urgent: make sense?
    else
      ray.emission = clearColor.xyz * clearColor.w;

  else                     // bounce ray
      if ( useEnvironmentMap )
        ray.emission = texture( environmentMap, dir ).xyz;
      else
        ray.emission = clearColor.xyz * clearColor.w;

  // End the path
  ray.depth = maxPathDepth + 1;
}
