layout( binding = 0, set = 1 ) readonly uniform CameraProperties
{
  mat4 view;
  mat4 proj;
  mat4 viewInverse;
  mat4 projInverse;
  vec4 position;
  vec4 viewingDirection;

  vec4 padding1;
  vec4 padding2;
}
cam;
