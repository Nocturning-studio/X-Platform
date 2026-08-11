#pragma once

template <class T> IC template_matrix4x4<T>& template_matrix4x4<T>::rotation(const template_quaternion<T>& Q)
{
	T xx = Q.x * Q.x;
	T yy = Q.y * Q.y;
	T zz = Q.z * Q.z;
	T xy = Q.x * Q.y;
	T xz = Q.x * Q.z;
	T yz = Q.y * Q.z;
	T wx = Q.w * Q.x;
	T wy = Q.w * Q.y;
	T wz = Q.w * Q.z;

	_11 = 1 - 2 * (yy + zz);
	_12 = 2 * (xy - wz);
	_13 = 2 * (xz + wy);
	_14 = 0;
	_21 = 2 * (xy + wz);
	_22 = 1 - 2 * (xx + zz);
	_23 = 2 * (yz - wx);
	_24 = 0;
	_31 = 2 * (xz - wy);
	_32 = 2 * (yz + wx);
	_33 = 1 - 2 * (xx + yy);
	_34 = 0;
	_41 = 0;
	_42 = 0;
	_43 = 0;
	_44 = 1;
	return *this;
}

template <class T> IC template_matrix4x4<T>& template_matrix4x4<T>::mk_transform(const template_quaternion<T>& Q, const Tvector& V)
{
	T xx = Q.x * Q.x;
	T yy = Q.y * Q.y;
	T zz = Q.z * Q.z;
	T xy = Q.x * Q.y;
	T xz = Q.x * Q.z;
	T yz = Q.y * Q.z;
	T wx = Q.w * Q.x;
	T wy = Q.w * Q.y;
	T wz = Q.w * Q.z;

	_11 = 1 - 2 * (yy + zz);
	_12 = 2 * (xy - wz);
	_13 = 2 * (xz + wy);
	_14 = 0;
	_21 = 2 * (xy + wz);
	_22 = 1 - 2 * (xx + zz);
	_23 = 2 * (yz - wx);
	_24 = 0;
	_31 = 2 * (xz - wy);
	_32 = 2 * (yz + wx);
	_33 = 1 - 2 * (xx + yy);
	_34 = 0;
	_41 = V.x;
	_42 = V.y;
	_43 = V.z;
	_44 = 1;
	return *this;
}

#define TRACE_QZERO_TOLERANCE 0.1f
template <class T> IC template_quaternion<T>& template_quaternion<T>::set(const template_matrix4x4<T>& M)
{
	float trace, s;

	trace = M._11 + M._22 + M._33;
	if (trace > 0.0f)
	{
		s = sqrt(trace + 1.0f);
		w = s * 0.5f;
		s = 0.5f / s;

		x = (M._32 - M._23) * s;
		y = (M._13 - M._31) * s;
		z = (M._21 - M._12) * s;
	}
	else
	{
		int biggest;
		enum
		{
			A,
			E,
			I
		};
		if (M._11 > M._22)
		{
			if (M._33 > M._11)
				biggest = I;
			else
				biggest = A;
		}
		else
		{
			if (M._33 > M._11)
				biggest = I;
			else
				biggest = E;
		}

		// in the unusual case the original trace fails to produce a good sqrt, try others...
		switch (biggest)
		{
		case A:
			s = std::sqrt(M._11 - (M._22 + M._33) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				x = s * 0.5f;
				s = 0.5f / s;
				w = (M._32 - M._23) * s;
				y = (M._12 + M._21) * s;
				z = (M._13 + M._31) * s;
				break;
			}
			// I
			s = std::sqrt(M._33 - (M._11 + M._22) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				z = s * 0.5f;
				s = 0.5f / s;
				w = (M._21 - M._12) * s;
				x = (M._31 + M._13) * s;
				y = (M._32 + M._23) * s;
				break;
			}
			// E
			s = std::sqrt(M._22 - (M._33 + M._11) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				y = s * 0.5f;
				s = 0.5f / s;
				w = (M._13 - M._31) * s;
				z = (M._23 + M._32) * s;
				x = (M._21 + M._12) * s;
				break;
			}
			break;
		case E:
			s = std::sqrt(M._22 - (M._33 + M._11) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				y = s * 0.5f;
				s = 0.5f / s;
				w = (M._13 - M._31) * s;
				z = (M._23 + M._32) * s;
				x = (M._21 + M._12) * s;
				break;
			}
			// I
			s = std::sqrt(M._33 - (M._11 + M._22) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				z = s * 0.5f;
				s = 0.5f / s;
				w = (M._21 - M._12) * s;
				x = (M._31 + M._13) * s;
				y = (M._32 + M._23) * s;
				break;
			}
			// A
			s = std::sqrt(M._11 - (M._22 + M._33) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				x = s * 0.5f;
				s = 0.5f / s;
				w = (M._32 - M._23) * s;
				y = (M._12 + M._21) * s;
				z = (M._13 + M._31) * s;
				break;
			}
			break;
		case I:
			s = std::sqrt(M._33 - (M._11 + M._22) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				z = s * 0.5f;
				s = 0.5f / s;
				w = (M._21 - M._12) * s;
				x = (M._31 + M._13) * s;
				y = (M._32 + M._23) * s;
				break;
			}
			// A
			s = std::sqrt(M._11 - (M._22 + M._33) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				x = s * 0.5f;
				s = 0.5f / s;
				w = (M._32 - M._23) * s;
				y = (M._12 + M._21) * s;
				z = (M._13 + M._31) * s;
				break;
			}
			// E
			s = std::sqrt(M._22 - (M._33 + M._11) + 1.0f);
			if (s > TRACE_QZERO_TOLERANCE)
			{
				y = s * 0.5f;
				s = 0.5f / s;
				w = (M._13 - M._31) * s;
				z = (M._23 + M._32) * s;
				x = (M._21 + M._12) * s;
				break;
			}
			break;
		}
	}
	return *this;
}
