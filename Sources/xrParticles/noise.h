#ifndef noiseH
#define noiseH

float noise3(const float3& vec);
float fractalsum3(const float3& v, float freq, int octaves);
float turbulence3(const float3& v, float freq, int octaves);

#endif
