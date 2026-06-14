#pragma once

struct SEnumVerticesCallback
{
	virtual void operator()(const fvec3& p) = 0;
};
