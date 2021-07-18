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
