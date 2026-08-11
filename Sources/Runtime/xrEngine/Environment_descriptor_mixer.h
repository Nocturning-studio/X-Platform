#pragma once

class ENGINE_API CEnvDescriptorMixer : public CEnvDescriptor
{
  public:
	ref_texture sky_texture_0;
	ref_texture sky_texture_1;
	ref_texture sky_irradiance_0;
	ref_texture sky_irradiance_1;
	ref_texture clouds_texture_0;
	ref_texture clouds_texture_1;
	ref_texture lut_texture_0;
	ref_texture lut_texture_1;

	float weight;

  public:
	CEnvDescriptorMixer(shared_str const& identifier);
	void lerp(CEnvironment* parent, CEnvDescriptor& A, CEnvDescriptor& B, float f, CEnvModifier& M, float m_power);
	void clear();
	void destroy();
};