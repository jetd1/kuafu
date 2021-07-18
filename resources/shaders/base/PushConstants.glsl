layout( push_constant ) uniform Constants
{
  vec4 clearColor;

  int frameCount;
  uint sampleRatePerPixel;
  uint maxPathDepth;
  bool useEnvironmentMap;

  bool russianRoulette;
  uint russianRouletteMinBounces;
  bool isNextEventEstimation;
  uint nextEventEstimationMinBounces;

  // @note Do not forget to pad when adding more.
};
