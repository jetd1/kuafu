#include "Random.glsl"

// @Nvidia vk_ray_tracing_KHR tutorial
// Return the tangent and binormal from the incoming normal
// transform +Z to tangent space
vec3 transformLocalToWorld( vec3 direction, vec3 normal )
{
  vec3 tangent;
  vec3 bitangent;

  if ( abs( normal.x ) > abs( normal.y ) )
  {
    tangent = vec3( normal.z, 0, -normal.x ) / sqrt( normal.x * normal.x + normal.z * normal.z );
  }
  else
  {
    tangent = vec3( 0, -normal.z, normal.y ) / sqrt( normal.y * normal.y + normal.z * normal.z );
  }

  bitangent = cross( normal, tangent );

  return direction.x * tangent + direction.y * bitangent + direction.z * normal;
}

vec3 cosineHemisphereSampling( inout uint seed, in vec3 normal )
{
  // cosine distributed sampling
  float u0 = rnd( seed );
  float u1 = rnd( seed );
  float sq = sqrt( 1.0 - u1 );

  vec3 direction = vec3( cos( 2 * M_PI * u0 ) * sq, sin( 2 * M_PI * u0 ) * sq, sqrt( u1 ) );

  return transformLocalToWorld( direction, normal );
}

// From Nvidia's vk_mini_path_tracer and ultimately from Peter Shirley's "Ray Tracing in one Weekend"
// Randomly sampling in hemisphere
// Generates a random point on a sphere of radius 1 centered at the normal. Uses random_unit_vector function
vec3 cosineHemisphereSampling2( inout uint seed, inout float pdf, in vec3 normal )
{
  // PDF of cosine distributed hemisphere sampling
  pdf = 1.0 / M_PI;

  // cosine distributed sampling
  float theta = 2.0 * M_PI * rnd( seed ); // Random in [0, 2pi]
  float u     = 2.0 * rnd( seed ) - 1.0;  // Random in [-1, 1]
  float r     = sqrt( 1.0 - u * u );

  vec3 direction = normal + vec3( r * cos( theta ), r * sin( theta ), u );
  return normalize( direction );
}

// from RayTracingGems p. 240
// Requires no transforming but lacks precision for the grazing case
vec3 cosineHemisphereSampling3( inout uint seed, inout float pdf, in vec3 normal )
{
  float a   = 1.0 - 2.0 * rnd( seed );
  float b   = sqrt( 1.0 - a * a );
  float phi = 2.0 * M_PI * rnd( seed );

  vec3 dir;
  dir.x = normal.x + b * cos( phi );
  dir.y = normal.y + b * sin( phi );
  dir.z = normal.z + a;

  pdf = a / M_PI;

  return dir;
}

vec3 uniformSphereSampling( inout uint seed )
{
  vec3 inUnitSphere = vec3( 0.0 );
  do
  {
    // x,y, and z range from -1 to 1
    // if point is outside the sphere -> reject it and try again
    inUnitSphere = vec3( rnd( seed ), rnd( seed ), rnd( seed ) ) * 2.0 - 1.0;
  } while ( dot( inUnitSphere, inUnitSphere ) >= 1.0 );

  return inUnitSphere;
}

// A rejection method ( "Ray Tracing in one Weekend" p. 22)
vec3 uniformHemisphereSampling( inout uint seed, in vec3 normal )
{
  vec3 inUnitSphere = uniformSphereSampling( seed );

  // In the same hemisphere as the normal
  if ( dot( inUnitSphere, normal ) > 0.0 )
  {
    return inUnitSphere;
  }
  // Flip normal
  else
  {
    return -inUnitSphere;
  }
}

// Not working
// from RayTracingGems p. 241
vec3 coneSampling( inout uint seed, inout float pdf, in vec3 normal, float roughness )
{
  float cosThetaMax = roughness;

  pdf = 1.0 / ( 2.0 * M_PI * ( 1.0 - cos( cosThetaMax ) ) );

  float u0 = rnd( seed );
  float u1 = rnd( seed );

  float cosTheta = ( 1.0 - u0 ) + u0 * cosThetaMax;
  float sinTheta = sqrt( 1.0 - cosTheta * cosTheta );
  float phi      = u1 * 2.0 * M_PI;
  vec3 dir;
  dir.x = cos( phi ) * sinTheta;
  dir.y = sin( phi ) * sinTheta;
  dir.z = cosTheta;

  return transformLocalToWorld( dir, normal );
}

vec2 diskSampling( inout uint seed )
{
  vec2 p;

  do
  {
    p = 2.0 * vec2( rnd( seed ), rnd( seed ) ) - 1.0;
  } while ( dot( p, p ) >= 1.0 );

  return p;
}
