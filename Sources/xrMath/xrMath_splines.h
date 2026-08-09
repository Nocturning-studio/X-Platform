#pragma once

// Сплайны (math implementation)
inline void spline1(float t, fvec3* p, fvec3* ret)
{
	float t2 = t * t;
	float t3 = t2 * t;
	float m[4];

	ret->x = 0.0f;
	ret->y = 0.0f;
	ret->z = 0.0f;
	m[0] = (0.5f * ((-1.0f * t3) + (2.0f * t2) + (-1.0f * t)));
	m[1] = (0.5f * ((3.0f * t3) + (-5.0f * t2) + (0.0f * t) + 2.0f));
	m[2] = (0.5f * ((-3.0f * t3) + (4.0f * t2) + (1.0f * t)));
	m[3] = (0.5f * ((1.0f * t3) + (-1.0f * t2) + (0.0f * t)));

	for (int i = 0; i < 4; i++)
	{
		ret->x += p[i].x * m[i];
		ret->y += p[i].y * m[i];
		ret->z += p[i].z * m[i];
	}
}

inline void spline2(float t, fvec3* p, fvec3* ret)
{
	float s = 1.0f - t;
	float t2 = t * t;
	float t3 = t2 * t;
	float m[4];

	m[0] = s * s * s;
	m[1] = 3.0f * t3 - 6.0f * t2 + 4.0f;
	m[2] = -3.0f * t3 + 3.0f * t2 + 3.0f * t + 1;
	m[3] = t3;

	ret->x = (p[0].x * m[0] + p[1].x * m[1] + p[2].x * m[2] + p[3].x * m[3]) / 6.0f;
	ret->y = (p[0].y * m[0] + p[1].y * m[1] + p[2].y * m[2] + p[3].y * m[3]) / 6.0f;
	ret->z = (p[0].z * m[0] + p[1].z * m[1] + p[2].z * m[2] + p[3].z * m[3]) / 6.0f;
}

#define beta1 1.0f
#define beta2 0.8f

inline void spline3(float t, fvec3* p, fvec3* ret)
{
	float s = 1.0f - t;
	float t2 = t * t;
	float t3 = t2 * t;
	float b12 = beta1 * beta2;
	float b13 = b12 * beta1;
	float delta = 2.0f - b13 + 4.0f * b12 + 4.0f * beta1 + beta2 + 2.0f;
	float d = 1.0f / delta;
	float b0 = 2.0f * b13 * d * s * s * s;
	float b3 = 2.0f * t3 * d;
	float b1 = d * (2 * b13 * t * (t2 - 3 * t + 3) + 2 * b12 * (t3 - 3 * t2 + 2) + 2 * beta1 * (t3 - 3 * t + 2) +
					beta2 * (2 * t3 - 3 * t2 + 1));
	float b2 = d * (2 * b12 * t2 * (-t + 3) + 2 * beta1 * t * (-t2 + 3) + beta2 * t2 * (-2 * t + 3) + 2 * (-t3 + 1));

	ret->x = p[0].x * b0 + p[1].x * b1 + p[2].x * b2 + p[3].x * b3;
	ret->y = p[0].y * b0 + p[1].y * b1 + p[2].y * b2 + p[3].y * b3;
	ret->z = p[0].z * b0 + p[1].z * b1 + p[2].z * b2 + p[3].z * b3;
}
