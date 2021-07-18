// Randomly sampling in hemisphere ( Peter Shirley's "Ray Tracing in one Weekend" )
float Schlick( const float cosine, const float ior )
{
  float r0 = ( 1.0 - ior ) / ( 1.0 + ior );
  r0 *= r0;
  return r0 + ( 1.0 - r0 ) * pow( 1.0 - cosine, 5.0 );
}
