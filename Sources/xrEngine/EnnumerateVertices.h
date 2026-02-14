#pragma once

struct SEnumVerticesCallback
{
	virtual void operator()(const float3& p) = 0;
};
