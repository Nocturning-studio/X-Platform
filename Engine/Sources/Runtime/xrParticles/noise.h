#ifndef noiseH
#define noiseH

float noise3(const fvec3& vec);
float fractalsum3(const fvec3& v, float freq, int octaves);
float turbulence3(const fvec3& v, float freq, int octaves);

#endif
