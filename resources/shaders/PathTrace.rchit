#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "base/Camera.glsl"
#include "base/Light.glsl"
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


// By Jet <i@jetd.me>, 2021.
//
vec3 calcDirectContribution(
    in vec3 L, in vec3 V, in vec3 N, in vec3 lightEmission,
    in float f, in float a2, in vec3 diffuseColor, in vec3 specularColor, in vec3 transmissionColor) {

  //      vec3 weight;
  vec3 weight = vec3(0.);

  if (transmissionColor != vec3(0.0)) {   // TODO: reconcile refrac and fancy spec
    bool isInside = gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT;
    if (isInside) {                       // perfect refration
      // so the light cannot have any direct contribution
      // TODO: change to a better model
      weight = vec3(0);

    } else {                             // try to calc fancy spec (different from the indirect part!)
      float NdotV = dot(N, V);
      vec3 refractedL  = refract(-V, N, 1 / f);
      float reflectProb = refractedL != vec3( 0.0 ) ? Schlick( NdotV, f ) : 1.0;
      //          float reflectProb = 1.0;

      if (rnd(ray.seed) <= reflectProb) {                   // spec!

        vec3  H     = normalize(L + V);
        float NdotL = dot(N, L);
        float NdotH = dot(N, H);
        float HdotV = dot(H, V);
        float NdotV = max(dot(N, V), 1e-6);
        float LdotH = dot(L, H);
        float D     = ggxNormalDistribution(NdotH, a2);
        float G     = GeometricShadowing(NdotL, NdotV, a2);
        vec3  F     = schlickFresnel(transmissionColor, LdotH);

        weight = D * F * G * HdotV / NdotH * NdotV;

      } else {                                              // refrac! no contribution!

        weight = vec3(0);

      }

    }

  } else {
    vec3  H     = normalize(L + V);
    float NdotL = dot(N, L);
    float NdotH = dot(N, H);
    float HdotV = dot(H, V);
    float NdotV = max(dot(N, V), 1e-6);
    float LdotH = dot(L, H);
    float D     = ggxNormalDistribution(NdotH, a2);
    float G     = GeometricShadowing(NdotL, NdotV, a2);
    vec3  F     = schlickFresnel(specularColor, LdotH);

    float diffuseLum   = length(diffuseColor);
    float specularLum  = length(specularColor);

    float probDiffuse  = diffuseLum / (diffuseLum + specularLum); // TODO: improve this

    if (diffuseLum == 0 && specularLum == 0)                      // TODO: improve this, copy the logic in main
      probDiffuse = 0.5;

    vec3 diffuseWeight  = diffuseColor * vec3(NdotL);
    vec3 specularWeight  = D * F * G * HdotV / NdotH * NdotV;

    weight = rnd(ray.seed) < probDiffuse ? diffuseWeight * probDiffuse : specularWeight * (1 - probDiffuse);
  }

  return lightEmission * weight;
}

// By Jet <i@jetd.me>, 2021.
//
vec3 traceShadowRay(in vec3 worldPos, in vec3 L, in vec3 V, in vec3 N, in float maxDist, in vec3 lightEmission,
    in float f, in float a2, in vec3 diffuseColor, in vec3 specularColor, in vec3 transmissionColor) {
  isShadowed = true;
  float NdotL = dot(N, L);
  if (NdotL > 0.0) {
    float tMin = 0.001;

    uint flags = gl_RayFlagsTerminateOnFirstHitEXT
    | gl_RayFlagsOpaqueEXT
    | gl_RayFlagsSkipClosestHitShaderEXT;

    traceRayEXT(topLevelAS, // acceleration structure
                flags, // rayFlags
                0xFF, // cullMask
                0, // sbtRecordOffset
                0, // sbtRecordStride
                1, // missIndex
                worldPos, // ray origin
                tMin, // ray min range
                L, // ray direction
                maxDist, // ray max range
                1// payload (location = 1)
    );
  }
  return isShadowed ? vec3(0) : calcDirectContribution(L, V, N, lightEmission, f, a2, diffuseColor, specularColor, transmissionColor);
}

// By Jet <i@jetd.me>, 2021.
//
vec3 traceDirectionalLight(
    in vec3 worldPos, in vec3 N,
    in float f, in float a2, in vec3 diffuseColor, in vec3 specularColor, in vec3 transmissionColor) {

  vec3 lightEmission = dlight.rgbs.xyz * dlight.rgbs.w;
  if (lightEmission == vec3(0))
    return vec3(0);

  vec3 L = -dlight.direction.xyz;
  vec3 V = normalize(-ray.direction);

  if (dlight.direction.w != 0) {
    vec3 perturb = vec3(rnd(ray.seed), rnd(ray.seed), rnd(ray.seed));
    L = normalize(L + dlight.direction.w * perturb);
  }

  float maxDist = 1e6;

  return traceShadowRay(worldPos, L, V, N, maxDist, lightEmission, f, a2, diffuseColor, specularColor, transmissionColor);
}

// By Jet <i@jetd.me>, 2021.
//
vec3 tracePointLights(
    in vec3 worldPos, in vec3 N,
    in float f, in float a2, in vec3 diffuseColor, in vec3 specularColor, in vec3 transmissionColor) {

  vec3 ret = vec3(0);

  for (uint i = 0; i < MAX_POINT_LIGHTS; ++i)
    if (plights.rgbs[i].w > 0) {

      float lum = length(plights.rgbs[i].xyz * plights.rgbs[i].w);
      if (lum == 0)
        continue;

      vec3 V = normalize(-ray.direction);

      vec3 lpos = plights.posr[i].xyz;
      if (plights.posr[i].w != 0) {
        vec3 perturb = uniformSphereSampling(ray.seed);
        lpos += plights.posr[i].w * normalize(perturb);    // TODO: this should be incorrect, back surface?
      }

      vec3 L = lpos - worldPos;
//      float d = length(L) + 1e-3;
      float d = length(L);
      L = normalize(L);
      vec3 lightEmission = plights.rgbs[i].xyz * plights.rgbs[i].w / d / d;

      ret += traceShadowRay(worldPos, L, V, N, d, lightEmission, f, a2, diffuseColor, specularColor, transmissionColor);
    }

  return ret;
}

// By Jet <i@jetd.me>, 2021.
//
vec3 traceActiveLights(
  in vec3 worldPos, in vec3 N,
  in float f, in float a2, in vec3 diffuseColor, in vec3 specularColor, in vec3 transmissionColor) {

  vec3 ret = vec3(0);

  for (uint i = 0; i < MAX_ACTIVE_LIGHTS; ++i)
    if (alights.front[i].w > 0) {

      float lum = length(alights.rgbs[i].xyz * alights.rgbs[i].w);
      if (lum == 0)
        continue;

      vec3 V = normalize(-ray.direction);

      float softness = alights.sftp[i].x;
      vec3 lpos = alights.position[i].xyz;

      if (softness != 0) {                    // TODO: this should be incorrect, sample in fov?
        vec3 perturb = uniformSphereSampling(ray.seed);
        lpos += softness * normalize(perturb);
      }

      vec3 L = lpos - worldPos;
//      float d = length(L) + 1e-3;
      float d = length(L);
      L = normalize(L);

      float fov = alights.sftp[i].y;
      vec3 alightDir = normalize(alights.front[i].xyz);
      float halfAngle = clamp(fov, 0, M_PI) / 2;
      float cos_ = dot(alightDir, -L);

      if (cos_ > cos(halfAngle)) {               // TODO: attenuation / softness
        int texID = int(alights.sftp[i].z);
        vec3 color = alights.rgbs[i].xyz;

        if (texID >= 0) {     // load texture *in addition* to base color
          mat4 view = alights.viewMat[i];
          mat4 proj = alights.projMat[i];

          vec4 texCoord = proj * view * vec4(worldPos, 1);
          texCoord /= texCoord.w;
          vec2 uv = texCoord.xy * 0.5 + 0.5;                      // TODO: softness in texture?

          color *= texture(textures[nonuniformEXT(texID)], uv).xyz;
        }

        vec3 lightEmission = color * alights.rgbs[i].w / d / d;
        ret += traceShadowRay(worldPos, L, V, N, d, lightEmission, f, a2, diffuseColor, specularColor, transmissionColor);
      }
    }

  return ret;
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
    if (mat.diffuseTexIdx >= 0)
      baseColor = texture(textures[nonuniformEXT( mat.diffuseTexIdx )], uv).xyz;
    baseColor /= M_PI;

    float metallic;
    if (mat.metallicTexIdx >= 0)
      metallic = texture(textures[nonuniformEXT( mat.metallicTexIdx )], uv).x;
    else
      metallic = mat.metallic;

    float roughness;
    if (mat.roughnessTexIdx >= 0)
      roughness = texture(textures[nonuniformEXT( mat.roughnessTexIdx )], uv).x;
    else
      roughness = mat.roughness;
    float a2 =  roughness * roughness;

    float transmission;
    if (mat.transmissionTexIdx >= 0)
      transmission = texture(textures[nonuniformEXT( mat.transmissionTexIdx )], uv).x;
    else
      transmission = mat.transmission;

    float f = max(mat.ior, 1e-5);
    float diffuse_weight = (1.0 - clamp(metallic, 0.0, 1.0)) * (1.0 - clamp(transmission, 0.0, 1.0));
    float final_transmission = clamp(transmission, 0.0, 1.0) * (1.0 - clamp(metallic, 0.0, 1.0));
    float specular_weight = (1.0 - final_transmission);

    vec3 weight = vec3(0.);

    vec3 V = normalize(-ray.direction);
    bool isInside = gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT;
    N = isInside ? -N : N;
    vec3 L = vec3(0);

    float NdotV = dot(N, V);

    vec3 diffuseColor      = diffuse_weight * baseColor;
    vec3 specularColor     = specular_weight * (baseColor * metallic
                           + (mat.specular * 0.08 * vec3(1.0)) * (1.0 - metallic));
//    vec3 transmissionColor = baseColor;
    vec3 transmissionColor = transmission * baseColor;
//    vec3 transmissionColor = final_transmission * baseColor;        // TODO: what is transmission color?

    float diffuseLum   = length(diffuseColor);
    float specularLum  = length(specularColor);

    float probDiffuse  = diffuseLum / (diffuseLum + specularLum);   // TODO: improve this

    if (diffuseLum == 0 && specularLum == 0) {                      // TODO: improve this
      if (baseColor == vec3(0)) {
        if (diffuse_weight == 1) {
          probDiffuse = 1.0;
        } else if (diffuse_weight == 0) {
          probDiffuse = 0.0;
        } else {
          probDiffuse = 0.5;
        }
      } else
        probDiffuse = 0.0;
    } else {
      probDiffuse *= specular_weight;    // Prevent undersamping of refrac
    }
//    float probDiffuse  = 1.0;
    bool chooseDiffuse = rnd(ray.seed) < probDiffuse;

    // Diffuse (Lambertian)
    if (chooseDiffuse) {
      L = cosineHemisphereSampling(ray.seed, N);
      float NdotL = clamp(dot(N, L), 0, 1);
      weight = M_PI * diffuseColor * NdotL  / probDiffuse;
    }

    // Specular
    if (!chooseDiffuse) {

      // Do fancy specular
//      if (rnd(ray.seed) < specular_weight) {
      if (final_transmission == 0) {                          // TODO: reconcile refrac and fancy spec
        vec3 H = sampleGGX(ray.seed, a2, N);
        float HdotV = dot(H, V);
        L = 2 * HdotV * H - V;

        float k = a2 / 2;
        float NoV = max(dot(N, V), 1e-7);
        float NoL = dot(N, L);
        float NoH = max(dot(N, H), 1e-7);
        float VoH = max(dot(V, H), 1e-7);

        if(NoL >= 0) {
          float G = GeometricShadowing(NoV, NoL, a2);
          vec3 F = schlickFresnel(specularColor, VoH);
          weight = M_PI * F * G * VoH / (NoH * NoV * (1 - probDiffuse));
        }
        else
          weight = vec3(0);

      } else {   // Do refraction and less fancy reflection

        float ior = isInside ? 1 / f : f;
        float _dot = isInside ? NdotV * ior: NdotV;

        vec3 refractedL  = refract(-V, N, 1 / f);                      // TODO: check if this
//        vec3 refractedL  = refract(-V, N, 1 / ior);                  //  or this is correct
        float reflectProb = refractedL != vec3( 0.0 ) ? Schlick( _dot, f ) : 1.0;
        //        float reflectProb = 0;

        if (rnd(ray.seed) >= reflectProb) {   // perfect refration
          ray.refractive = true;
          L = refractedL;
          weight = M_PI * transmissionColor / (1 - probDiffuse);
        } else {                             // perfect reflection    // TODO: change the to fresnel-based
          L = reflect(-V, N);
          weight = M_PI * transmissionColor / (1 - probDiffuse);
        }
      }
    }

    ray.shadow_color =
        traceDirectionalLight(worldPos, N, f, a2, diffuseColor, specularColor, transmissionColor)
      + tracePointLights(worldPos, N, f, a2, diffuseColor, specularColor, transmissionColor)
      + traceActiveLights(worldPos, N, f, a2, diffuseColor, specularColor, transmissionColor);

    ray.origin    = worldPos;
    ray.direction = L;
    ray.emission  = vec3(0.0);
    ray.weight    = weight;
    ray.albedo    = baseColor;
    ray.normal    = N;
  }
}
