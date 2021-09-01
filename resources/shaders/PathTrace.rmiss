#version 460
#extension GL_EXT_ray_tracing : require

#include "base/Ray.glsl"
#include "base/DirectionalLight.glsl"

layout( location = 0 ) rayPayloadInEXT RayPayLoad ray;

layout( push_constant ) uniform Constants
{
  vec4 clearColor;
  int frameCount;
  uint sampleRatePerPixel;
  uint maxPathDepth;
  bool useEnvironmentMap;

  uint padding0;
  uint padding1;
  uint padding2;
  uint padding3;
};

layout( binding = 2, set = 1 ) uniform samplerCube environmentMap;

void main( )
{
  vec3 dir = ray.direction;
  dir.x *= -1.0; // mirror



  if ( ray.type == 0 )     // view ray
  {
    if ( useEnvironmentMap )
    {
      ray.emission = texture( environmentMap, dir ).xyz * 0.8F;  // TODO: kuafu_urgent: make sense?
    }
    else
    {
      ray.emission = clearColor.xyz * clearColor.w;
    }
  }
  else if (ray.type == 1)  // shadow ray
  {
    ray.emission = dlight.rgbs.xyz * dlight.rgbs.w;
  }
  else                     // bounce ray
  {
    if ( ray.reflective )
    {
      ray.reflective = false;
      if ( useEnvironmentMap )
      {
        ray.emission = texture( environmentMap, dir ).xyz;
      }
      else
      {
        ray.emission = clearColor.xyz * clearColor.w;
      }
    }
    else
    {
      // small contribution from environment
      if ( useEnvironmentMap )
      {
        ray.emission = texture( environmentMap, dir ).xyz * 0.01;
      }
      else
      {
        ray.emission = clearColor.xyz * clearColor.w * 0.01;
      }
    }
  }

  // End the path
  ray.depth = maxPathDepth + 1;
}
