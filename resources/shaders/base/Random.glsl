#define M_PI 3.141592

// @Nvidia vk_ray_tracing_KHR tutorial
// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint tea( uint val0, uint val1 )
{
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0;

  for ( uint n = 0; n < 16; n++ )
  {
    s0 += 0x9e3779b9;
    v0 += ( ( v1 << 4 ) + 0xa341316c ) ^ ( v1 + s0 ) ^ ( ( v1 >> 5 ) + 0xc8013ea4 );
    v1 += ( ( v0 << 4 ) + 0xad90777d ) ^ ( v0 + s0 ) ^ ( ( v0 >> 5 ) + 0x7e95761e );
  }

  return v0;
}

// @Nvidia vk_ray_tracing_KHR tutorial
// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg( inout uint prev )
{
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = ( LCG_A * prev + LCG_C );
  return prev & 0x00FFFFFF;
}

// @Nvidia vk_ray_tracing_KHR tutorial
// Generate a random float in [0, 1) given the previous RNG state
float rnd( inout uint prev )
{
  return ( float( lcg( prev ) ) / float( 0x01000000 ) );
}

// Random functions from Alan Wolfe's excellent tutorials (https://blog.demofox.org/)
uint wang_hash( inout uint seed )
{
  seed = uint( seed ^ uint( 61 ) ) ^ uint( seed >> uint( 16 ) );
  seed *= uint( 9 );
  seed = seed ^ ( seed >> 4 );
  seed *= uint( 0x27d4eb2d );
  seed = seed ^ ( seed >> 15 );
  return seed;
}

float RandomFloat01( inout uint state )
{
  return float( wang_hash( state ) ) / 4294967296.0;
}

// https://github.com/TeamWisp/WispForMaya/blob/df0603a740bb38d7828ebc0954540cd3dc327b57/module/wisp/bin/resources/shaders/rand_util.hlsl
vec3 getPerpendicularVector(vec3 u)
{
  vec3 a = abs(u);
  uint xm = ((a.x - a.y) < 0 && (a.x - a.z) <0) ? 1 : 0;
  uint ym = (a.y - a.z) < 0 ? (1u ^ xm) : 0;
  uint zm = 1u ^ (xm | ym);
  return cross(u, vec3(xm, ym, zm));
}

// Randomly sampling in hemisphere ( Peter Shirley's "Ray Tracing in one Weekend" )
float Schlick( const float cosine, const float ior )
{
  float r0 = ( 1.0 - ior ) / ( 1.0 + ior );
  r0 *= r0;
  return r0 + ( 1.0 - r0 ) * pow( 1.0 - cosine, 5.0 );
}

float ggxNormalDistribution( float NdotH, float a2 )
{
  float d = max(NdotH * NdotH * (a2 - 1) + 1, 1e-6);
  return a2 / (d * d * M_PI);
}

// When using this function to sample, the probability density is:
//      pdf = D * NdotH / (4 * HdotV)
vec3 sampleGGX(inout uint randSeed, float a2, vec3 N)
{
  // Get our uniform random numbers
  vec2 r = vec2(rnd(randSeed), rnd(randSeed));

  // Get an orthonormal basis from the normal
  vec3 B = getPerpendicularVector(N);
  vec3 T = cross(B, N);

  // GGX NDF sampling
  float cosThetaH = sqrt(clamp((1.0 - r.x) / ((a2 - 1.0) * r.x + 1), 0, 1));
  float sinThetaH = sqrt(clamp(1.0 - cosThetaH * cosThetaH, 0, 1));
  float phiH = r.y * M_PI * 2.0f;

  // Get our GGX NDF sample (i.e., the half vector)
  return T * (sinThetaH * cos(phiH)) +
  B * (sinThetaH * sin(phiH)) +
  N * cosThetaH;
}

//float schlickMaskingTerm(float NdotL, float NdotV, float a2)
//{
////  // Karis notes they use alpha / 2 (or roughness^2 / 2)
//  float k = a2 / 2;
//
////  float k = (roughness + 1)*(roughness + 1) / 8;
//
//  // Compute G(v) and G(l).  These equations directly from Schlick 1994
//  //     (Though note, Schlick's notation is cryptic and confusing.)
//  float g_v = NdotV / (NdotV*(1 - k) + k);
//  float g_l = NdotL / (NdotL*(1 - k) + k);
//  return g_v * g_l;
//}

float G1(float dotValue, float a2)
{
  //GGX
  return (2 * dotValue) / (dotValue + sqrt(a2 + (1 - a2) * (dotValue * dotValue)));
}

float GeometricShadowing(float NdotV, float NdotL, float a2)
{
  //Smith with GGX
  return G1(NdotL, a2) * G1(NdotV, a2);
}

//vec3 schlickFresnel(vec3 f0, float lDotH)
//{
//  return f0 + (vec3(1.0f) - f0) * pow(1.0f - lDotH, 5.0f);
//}
vec3 schlickFresnel(vec3 f0, float lDotH)
{
  return f0 + (vec3(1.0f) - f0) * pow(2.0f, (-5.55473 * lDotH - 6.98316) * lDotH);
}

