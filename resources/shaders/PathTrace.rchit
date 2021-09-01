#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "base/Camera.glsl"
#include "base/DirectionalLight.glsl"
#include "base/Geometry.glsl"
#include "base/PushConstants.glsl"
#include "base/Ray.glsl"
#include "base/Sampling.glsl"
#include "base/Shading.glsl"

hitAttributeEXT vec3 attribs;

layout( location = 0 ) rayPayloadInEXT RayPayLoad ray;
layout( location = 1 ) rayPayloadEXT bool isShadowed;

layout( binding = 0, set = 0 ) uniform accelerationStructureEXT topLevelAS;

layout( binding = 1, set = 1 ) readonly buffer GeometryInstances
{
  GeometryInstance i[];
}
geometryInstances;

layout( binding = 0, set = 2 ) readonly buffer Vertices
{
  vec4 v[];
}
vertices[];

layout( binding = 1, set = 2 ) readonly buffer Indices
{
  uint i[];
}
indices[];

layout( binding = 2, set = 2 ) readonly buffer MatIndices
{
  uint i[];
}
matIndices[];

layout( binding = 3, set = 2 ) uniform sampler2D textures[];

layout( binding = 4, set = 2 ) readonly buffer Materials
{
  Material m[];
}
materials;

Vertex unpackVertex( uint index, uint geometryIndex )
{
  vec4 d0 = vertices[nonuniformEXT( geometryIndex )].v[3 * index + 0];
  vec4 d1 = vertices[nonuniformEXT( geometryIndex )].v[3 * index + 1];
  vec4 d2 = vertices[nonuniformEXT( geometryIndex )].v[3 * index + 2];

  Vertex v;
  v.pos      = d0.xyz;
  v.normal   = vec3( d0.w, d1.x, d1.y );
  v.color    = vec3( d1.z, d1.w, d2.x );
  v.texCoord = vec2( d2.y, d2.z );
  return v;
}

Material getShadingData( inout vec3 localNormal, inout vec3 worldNormal, inout vec3 worldPosition, inout vec2 uv )
{
  // Access the instance in the array when TLAS was built and get its geometry index
  uint geometryIndex = geometryInstances.i[gl_InstanceID].geometryIndex;

  // Use geometry index and current primitive ID to access indices
  ivec3 ind = ivec3( indices[nonuniformEXT( geometryIndex )].i[3 * gl_PrimitiveID + 0],   //
                     indices[nonuniformEXT( geometryIndex )].i[3 * gl_PrimitiveID + 1],   //
                     indices[nonuniformEXT( geometryIndex )].i[3 * gl_PrimitiveID + 2] ); //

  // Retrieve vertices using the indices from above
  Vertex v0 = unpackVertex( ind.x, geometryIndex );
  Vertex v1 = unpackVertex( ind.y, geometryIndex );
  Vertex v2 = unpackVertex( ind.z, geometryIndex );

  const vec3 barycentrics = vec3( 1.0 - attribs.x - attribs.y, attribs.x, attribs.y );

  // Computing the normal at hit position
  localNormal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;

  // Transforming the normal to world space
  worldNormal = normalize( vec3( localNormal * gl_WorldToObjectEXT ) );

  // Intersection position in world space
  worldPosition = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

  // Texture coordinate
  uv = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

  // Retrieve material
  uint matIndex = matIndices[nonuniformEXT( geometryIndex )].i[gl_PrimitiveID];
  return materials.m[matIndex];
}

void nextEventEstimation( inout vec3 emission, in Material mat, in vec3 base_bsdf, in vec3 worldPos, in vec3 normal )
{
  // trace shadow ray
  // Tracing shadow ray only if the light is visible from the surface
  // https://computergraphics.stackexchange.com/questions/7929/how-does-a-path-tracer-with-next-event-estimation-work
  if ( isNextEventEstimation && emission == vec3( 0.0 ) )
  {
    vec3 lightDirection = -dlight.direction.xyz;
    float cosTheta = dot( normal, lightDirection );
    if ( cosTheta > 0.0 )
    {
      float tMin = 0.001;
      uint flags = gl_RayFlagsTerminateOnFirstHitEXT
                 | gl_RayFlagsOpaqueEXT
                 | gl_RayFlagsSkipClosestHitShaderEXT;
      isShadowed = true;

      float lightDistance = 100000.0;

      traceRayEXT( topLevelAS,      // acceleration structure
                   flags,           // rayFlags
                   0xFF,            // cullMask
                   0,               // sbtRecordOffset
                   0,               // sbtRecordStride
                   1,               // missIndex
                   worldPos,        // ray origin
                   tMin,            // ray min range
                   lightDirection,  // ray direction
                   lightDistance,   // ray max range
                   1                // payload (location = 1)
      );

      if ( isShadowed ) { }
      else {
        vec3 weight = vec3( 0.0 );
        // Lambertian reflection
        if ( mat.illum == 0 ) {
          weight = vec3(cosTheta);
        } else if ( mat.illum == 1 ) {
          // Flip the normal if ray is transmitted
          vec3 n = gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT ? -normal : normal;
          float cosine = mat.ior * cosTheta;

          vec3  refracted    = refract( lightDirection, n, mat.ior );
          float reflectProb = refracted != vec3( 0.0 ) ? Schlick( cosine, mat.ior ) : 1.0;

          weight = vec3(cosTheta) * reflectProb;
//          weight *= 0;

        } else {
          weight = vec3(cosTheta);
//          weight *= 0;
        }

        ray.shadow_color = dlight.rgbs.xyz * dlight.rgbs.w * weight;
      }
    }
  }
}

void main( )
{
  vec3 localNormal, normal, worldPos;
  vec2 uv;
  Material mat = getShadingData( localNormal, normal, worldPos, uv );

  // Colors
  vec3 reflectance = vec3( 0.0 );
  vec3 emission    = vec3( 0.0 ); // emittance / emissiveFactor

  emission = mat.emission.xyz;

  // Stop recursion if a emissive object is hit.
  if ( emission != vec3( 0.0 ) )
  {
    ray.depth = maxPathDepth + 1;
  }

  // Retrieve material textures and colors
  reflectance = mat.diffuse.xyz;
  if ( mat.diffuse.w != -1.0 )
  {
    reflectance *= texture( textures[nonuniformEXT( int( mat.diffuse.w ) )], uv ).xyz;
  }

  // Pick a random direction from here and keep going.
  vec3 rayOrigin = worldPos;
  vec3 nextDirection;

  // Probability of the new ray (PDF)
  float pdf;
  float cosTheta = 1.0;

  // BSDF (Divide by Pi to ensure energy conversation)
  // @todo path regularization: blur the bsdf for indirect rays (raytracinggems p.251)
  vec3 base_bsdf = reflectance / M_PI;
  vec3 new_bsdf = base_bsdf;

  // Metallic reflection
  if ( mat.illum == 2 )
  {
    vec3 reflectDir = reflect( ray.direction, normal );
    nextDirection   = reflectDir + mat.roughness * cosineHemisphereSampling( ray.seed, pdf, normal );
    ray.reflective  = true;
  }
  // @todo Resulting background color is inverted for any 2D surface
  // Dielectric reflection ( Peter Shirley's "Ray Tracing in one Weekend" Chapter 9 )
  else if ( mat.illum == 1 )
  {
    // Flip the normal if ray is transmitted
    vec3 temp = gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT ? -normal : normal;

    float dot_   = dot( ray.direction, normal );
    float cosine = dot_ > 0 ? mat.ior * dot_ : -dot_;
    float ior    = dot_ > 0 ? mat.ior : 1 / mat.ior;

    vec3 refracted    = refract( ray.direction, temp, ior );
    float reflectProb = refracted != vec3( 0.0 ) ? Schlick( cosine, mat.ior ) : 1.0;

    if ( rnd( ray.seed ) < reflectProb )
    {
      nextDirection = reflect( ray.direction, normal ) + mat.roughness * cosineHemisphereSampling( ray.seed, pdf, normal );
      ray.reflective  = true;
    }
    else
    {
      nextDirection  = refracted;
      ray.refractive = true;
    }
  }
  // Lambertian reflection
  else if ( mat.illum == 0 )
  {
    nextDirection = cosineHemisphereSampling( ray.seed, pdf, normal );
    cosTheta      = dot( nextDirection, normal ); // The steeper the incident direction to the surface is the more important the sample gets
    new_bsdf *= cosTheta;
  }

  // Add specular highlight
  if ( mat.emission.w > 0.0 && mat.illum != 0 )
  {
    vec3 reflectDir = reflect( ray.direction, normal );
    float spec      = pow( max( dot( normalize( cam.position.xyz - worldPos ), reflectDir ), 0.0 ), mat.emission.w );
    if ( spec > 0.0 )
    {
      new_bsdf = ( reflectance + clamp( spec, 0, 1 ) ) / M_PI;
    }
  }

  //pdf = 1 / ( 1.5 * M_PI ); // the smaller the higher the contribution

  nextEventEstimation( emission, mat, base_bsdf, worldPos, normal );

  ray.origin    = rayOrigin;
  ray.direction = nextDirection;
  ray.emission  = emission;
  ray.weight    = new_bsdf / pdf; // divide reflectance by PDF
}
