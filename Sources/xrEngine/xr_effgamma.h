#ifndef __XR_GAMMA_H__
#define __XR_GAMMA_H__

//-----------------------------------------------------------------------------------------------------------
// Gamma control
//-----------------------------------------------------------------------------------------------------------
class ENGINE_API CGammaControl
{
	float fGamma;
	float fBrightness;
	float fContrast;
	fcolor cBalance;

  public:
	CGammaControl() : fGamma(1.f)
	{
		Brightness(1.f);
		Contrast(1.f);
		Balance(1.f, 1.f, 1.f);
	};

	IC void Balance(float _r, float _g, float _b)
	{
		cBalance.set(_r, _g, _b, 1);
	}
	IC void Balance(fcolor& C)
	{
		Balance(C.r, C.g, C.b);
	}

	IC void Gamma(float G)
	{
		fGamma = G;
	}
	IC void Brightness(float B)
	{
		fBrightness = B;
	}
	IC void Contrast(float C)
	{
		fContrast = C;
	}

	void GetIP(float& G, float& B, float& C, fcolor& Balance)
	{
		G = fGamma;
		B = fBrightness;
		C = fContrast;
		Balance.set(cBalance);
	}

	void GenLUT(D3DGAMMARAMP& G);

	void Update();
};

#endif
