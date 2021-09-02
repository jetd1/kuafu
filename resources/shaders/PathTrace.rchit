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

// TODO: rewrite this using callable shader
void traceShadowRay( in Material mat, in vec3 diffuseColor, in vec3 specularColor, in vec3 worldPos, in vec3 N )
{
  // trace shadow ray
  // Tracing shadow ray only if the light is visible from the surface
  vec3 L = -dlight.direction.xyz;
  vec3 V = normalize(-ray.direction);
  ray.shadow_color = vec3(0);

  if (dlight.direction.w != 0) {
    vec3 perturb = vec3(rnd(ray.seed), rnd(ray.seed), rnd(ray.seed));
    L = normalize(L + dlight.direction.w * perturb);
  }

  float NdotL = dot(N, L);
  if (NdotL > 0.0) {
    float tMin = 0.001;
    float lightDistance = 100000.0;

    uint flags = gl_RayFlagsTerminateOnFirstHitEXT
    | gl_RayFlagsOpaqueEXT
    | gl_RayFlagsSkipClosestHitShaderEXT;
    isShadowed = true;

    traceRayEXT(topLevelAS, // acceleration structure
                flags, // rayFlags
                0xFF, // cullMask
                0, // sbtRecordOffset
                0, // sbtRecordStride
                1, // missIndex
                worldPos, // ray origin
                tMin, // ray min range
                L, // ray direction
                lightDistance, // ray max range
                1// payload (location = 1)
    );

    if (!isShadowed) {

      float a2    = mat.roughness * mat.roughness;
      vec3  H     = normalize(L + V);
      float NdotH = dot(N, H);
      float HdotV = dot(H, V);
      float NdotV = max(dot(N, V), 1e-6);
      float LdotH = dot(L, H);
      float D     = ggxNormalDistribution(NdotH, a2);
      float G     = GeometricShadowing(NdotL, NdotV, a2);
      vec3  F     = schlickFresnel(specularColor, LdotH);

      float diffuseLum   = length(diffuseColor);
      float specularLum  = length(specularColor);

//      float probDiffuse  = diffuseLum / (diffuseLum + specularLum);// TODO: improve this
      float probDiffuse  = 1.0;

      vec3 diffuseWeight  = diffuseColor * vec3(NdotL);
      vec3 specularWeight  = D * F * G * HdotV / NdotH * NdotV;

      vec3 weight = rnd(ray.seed) < probDiffuse ?
                    diffuseWeight * probDiffuse
                  : specularWeight * (1 - probDiffuse);

      ray.shadow_color = dlight.rgbs.xyz * dlight.rgbs.w * weight;
    }
  }
}

// By Jet <i@jetd.me>, 2021.
// Implemented according to blender's PrincipledBSDF
// https://github.com/blender/blender/blob/master/intern/cycles/kernel/shaders/node_principled_bsdf.osl
void main( )
{
  vec3 localNormal, N, worldPos;
  vec2 uv;
  Material mat = getShadingData(localNormal, N, worldPos, uv);

  // Stop recursion if a emissive object is hit.
  // TODO: change this behavior
  vec3 emission = mat.emission.rgb * mat.emission.w;
  if (emission != vec3(0.)) {

    ray.depth     = maxPathDepth + 1;
    ray.emission  = mat.emission.xyz * mat.emission.w;

  } else {

    vec3 baseColor = mat.diffuse.xyz;
    if (mat.diffuse.w != 0)
      baseColor = texture(textures[nonuniformEXT( mat.texIdx )], uv).xyz;
    baseColor /= M_PI;

    float f = max(mat.ior, 1e-5);
    float diffuse_weight = (1.0 - clamp(mat.metallic, 0.0, 1.0)) * (1.0 - clamp(mat.transmission, 0.0, 1.0));
    float final_transmission = clamp(mat.transmission, 0.0, 1.0) * (1.0 - clamp(mat.metallic, 0.0, 1.0));
    float specular_weight = (1.0 - final_transmission);
    float a2 =  mat.roughness * mat.roughness;

    vec3 bsdf = baseColor;
    float pdf = 0;
    vec3 weight = vec3(0.);

    vec3 V = normalize(-ray.direction);
    bool isInside = gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT;
    N = isInside ? -N : N;
    vec3 L = vec3(0);

    float NdotV = dot(N, V);

    vec3 diffuseColor  = diffuse_weight * baseColor;
    vec3 specularColor = specular_weight * (baseColor * mat.metallic
                       + (mat.specular * 0.08 * vec3(1.0)) * (1.0 - mat.metallic));

    float diffuseLum   = length(diffuseColor);
    float specularLum  = length(specularColor);

//    if (diffuseLum == 0 && specularLum == 0) {
//
//    }

//    float probDiffuse  = diffuseLum / (diffuseLum + specularLum);   // TODO: improve this
    float probDiffuse  = 1.0;   // TODO: improve this
    bool chooseDiffuse = rnd(ray.seed) < probDiffuse;

    // Diffuse (Lambertian)
    if (chooseDiffuse) {
      L = cosineHemisphereSampling(ray.seed, pdf, N);
      float NdotL = clamp(dot(N, L), 0, 1);
      weight = diffuseColor * NdotL * M_PI  / probDiffuse;
    }

    // Specular
    if (!chooseDiffuse) {
      bool refracted = false;

//      bool tryTransmit = rnd(ray.seed) < final_transmission;
      bool tryTransmit = false;

      // Transmission ?
      if (tryTransmit) {
        // Flip the normal if ray is transmitted
        vec3 _N = gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT ? -N : N;

        float cosine = NdotV > 0 ? mat.ior * NdotV : -NdotV;
        float ior    = NdotV > 0 ? mat.ior : 1 / mat.ior;

        vec3 refractedL   = refract( V, _N, ior );
        float reflectProb = refractedL != vec3( 0.0 ) ? Schlick( cosine, mat.ior ) : 1.0;

        if (rnd(ray.seed) >= reflectProb) {
          L  = refractedL;
          ray.refractive = true;
          refracted = true;
          float NdotL = dot(_N, L);

          // TODO: the following is incorrect, check
//          bsdf        = specularColor * NdotL;
//          bsdf        = specularColor;
//          bsdf        = baseColor * NdotL;
          bsdf        = baseColor;
          pdf         = 1 / M_PI;
          weight = bsdf / pdf;
        }
      }

      if (!refracted) {
        vec3 H = sampleGGX(ray.seed, a2, N);
        float HdotV = dot(H, V);
        L = 2 * HdotV * H - V;

        float k = a2 / 2;
        float NoV = max(dot(N, V), 1e-7);
        float NoL = dot(N, L);
        float NoH = max(dot(N, H), 1e-7);
        float VoH = max(dot(V, H), 1e-7);

        if(NoL >= 0) {
          ray.reflective = true;
          float G = GeometricShadowing(NoV, NoL, a2);
          vec3 F = schlickFresnel(specularColor, VoH);
          weight = M_PI * F * G * VoH / (NoH * NoV * (1 - probDiffuse));
//          weight = M_PI * specularColor;
        }
        else {
//          weight = specularColor;
          weight = vec3(0);
        }
      }
    }

    traceShadowRay(mat, diffuseColor, specularColor, worldPos, N);

    ray.origin    = worldPos;
    ray.direction = L;
    ray.emission  = vec3(0.0);
    ray.weight    = weight;
  }
}
