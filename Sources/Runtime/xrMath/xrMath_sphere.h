#pragma once

#include "xrMath_common.h"
#include "xrMath_types.h"
#include "xrMath_vector3.h"
#include <list>
#include <cmath>

// -----------------------------------------------------------------------------
// Sphere template
// -----------------------------------------------------------------------------
template <class T> struct template_sphere
{
	template_vector3<T> P;
	T R;

  public:
	IC void set(const template_vector3<T>& _P, T _R)
	{
		P.set(_P);
		R = _R;
	}
	IC void set(const template_sphere<T>& S)
	{
		P.set(S.P);
		R = S.R;
	}
	IC void identity()
	{
		P.set(0, 0, 0);
		R = 1;
	}

	ICF BOOL intersect(const template_vector3<T>& S, const template_vector3<T>& D) const
	{
		template_vector3<T> Q;
		Q.sub(P, S);

		T c = Q.magnitude();
		T v = Q.dotproduct(D);
		T d = R * R - (c * c - v * v);
		return (d > 0);
	}
	ICF BOOL intersect(const template_sphere<T>& S) const
	{
		T SumR = R + S.R;
		return P.distance_to_sqr(S.P) < SumR * SumR;
	}

	enum ERP_Result
	{
		rpNone = 0,
		rpOriginInside = 1,
		rpOriginOutside = 2,
		fcv_forcedword = u32(-1)
	};

	// Ray-sphere intersection
	ICF ERP_Result intersect(const template_vector3<T>& S, const template_vector3<T>& D, T range, int& quantity,
							 T afT[2]) const
	{
		template_vector3<T> kDiff;
		kDiff.sub(S, P);
		T fA = range * range;
		T fB = kDiff.dotproduct(D) * range;
		T fC = kDiff.square_magnitude() - R * R;
		ERP_Result result = rpNone;

		T fDiscr = fB * fB - fA * fC;
		if (fDiscr < (T)0.0)
		{
			quantity = 0;
		}
		else if (fDiscr > (T)0.0)
		{
			T fRoot = std::sqrt(fDiscr);
			T fInvA = ((T)1.0) / fA;
			afT[0] = range * (-fB - fRoot) * fInvA;
			afT[1] = range * (-fB + fRoot) * fInvA;
			if (afT[0] >= (T)0.0)
			{
				quantity = 2;
				result = rpOriginOutside;
			}
			else if (afT[1] >= (T)0.0)
			{
				quantity = 1;
				afT[0] = afT[1];
				result = rpOriginInside;
			}
			else
				quantity = 0;
		}
		else
		{
			afT[0] = range * (-fB / fA);
			if (afT[0] >= (T)0.0)
			{
				quantity = 1;
				result = rpOriginOutside;
			}
			else
				quantity = 0;
		}
		return result;
	}

	ICF typename template_sphere<T>::ERP_Result intersect_full(const template_vector3<T>& start,
															   const template_vector3<T>& dir, T& dist) const
	{
		int quantity;
		float afT[2];
		typename template_sphere<T>::ERP_Result result = intersect(start, dir, dist, quantity, afT);

		if (result == template_sphere<T>::rpOriginInside ||
			((result == template_sphere<T>::rpOriginOutside) && (afT[0] < dist)))
		{
			switch (result)
			{
			case template_sphere<T>::rpOriginInside:
				dist = afT[0] < dist ? afT[0] : dist;
				break;
			case template_sphere<T>::rpOriginOutside:
				dist = afT[0];
				break;
			}
		}
		return result;
	}

	ICF typename template_sphere<T>::ERP_Result intersect(const template_vector3<T>& start,
														  const template_vector3<T>& dir, T& dist) const
	{
		int quantity;
		T afT[2];
		typename template_sphere<T>::ERP_Result result = intersect(start, dir, dist, quantity, afT);
		if (rpNone != result)
		{
			if (afT[0] < dist)
			{
				dist = afT[0];
				return result;
			}
		}
		return rpNone;
	}

	ICF typename template_sphere<T>::ERP_Result intersect2(const template_vector3<T>& S, const template_vector3<T>& D,
														   T& range) const
	{
		template_vector3<T> Q;
		Q.sub(P, S);

		T R2 = R * R;
		T c2 = Q.square_magnitude();
		T v = Q.dotproduct(D);
		T d = R2 - (c2 - v * v);

		if (d > 0.f)
		{
			T _range = v - std::sqrt(d);
			if (_range < range)
			{
				range = _range;
				return (c2 < R2) ? rpOriginInside : rpOriginOutside;
			}
		}
		return rpNone;
	}
	IC BOOL contains(const template_vector3<T>& PT) const
	{
		return P.distance_to_sqr(PT) <= (R * R + EPS_S);
	}

	IC BOOL contains(const template_sphere<T>& S) const
	{
		const T RDiff = R - S.R;
		if (RDiff < 0)
			return false;
		return (P.distance_to_sqr(S.P) <= RDiff * RDiff);
	}

	IC T volume() const
	{
		return T(PI_MUL_4 / 3) * (R * R * R);
	}
};

typedef template_sphere<float> Fsphere;

template <class T> BOOL _valid(const template_sphere<T>& s)
{
	return _valid(s.P) && _valid(s.R);
}

// -----------------------------------------------------------------------------
// Minimal enclosing ball (Miniball) implementation
// -----------------------------------------------------------------------------
class Basis
{
  private:
	enum
	{
		d = 3
	} eDimensions;

	int m, s; // size and number of support vectors
	fvec3 q0;

	float z[d + 1];
	float f[d + 1];
	fvec3 v[d + 1];
	fvec3 a[d + 1];
	fvec3 c[d + 1];
	float sqr_r[d + 1];

	fvec3* current_c; // pointer to some c[j]
	float current_sqr_r;

  public:
	Basis()
	{
		reset();
	}

	const fvec3* center() const
	{
		return current_c;
	}
	float squared_radius() const
	{
		return current_sqr_r;
	}
	int size() const
	{
		return m;
	}
	int support_size() const
	{
		return s;
	}

	float excess(const fvec3& p) const
	{
		float e = -current_sqr_r;
		e += p.distance_to_sqr(*current_c);
		return e;
	}

	void reset()
	{
		m = s = 0;
		c[0].set(0, 0, 0); // misuse c[0] for the empty sphere center
		current_c = c;
		current_sqr_r = -1;
	}

	bool push(const fvec3& p)
	{
		if (m == 0)
		{
			q0 = p;
			c[0] = q0;
			sqr_r[0] = 0;
		}
		else
		{
			int i;
			const float eps = 1e-16f;

			// v[m] = p - q0
			v[m].sub(p, q0);

			// compute a_{m,i}, i < m
			for (i = 1; i < m; ++i)
			{
				a[m][i] = v[i].dotproduct(v[m]);
				a[m][i] *= (2.f / z[i]);
			}

			// update v[m] to Q_m - \bar{Q}_m
			for (i = 1; i < m; ++i)
			{
				v[m].mad(v[m], v[i], -a[m][i]);
			}

			// compute z[m]
			z[m] = 0;
			z[m] += v[m].square_magnitude();
			z[m] *= 2;

			// reject if z[m] too small
			if (z[m] < eps * current_sqr_r)
				return false;

			// update c, sqr_r
			float e = -sqr_r[m - 1];
			e += p.distance_to_sqr(c[m - 1]);

			f[m] = e / z[m];
			c[m].mad(c[m - 1], v[m], f[m]);
			sqr_r[m] = sqr_r[m - 1] + e * f[m] / 2;
		}

		current_c = c + m;
		current_sqr_r = sqr_r[m];
		s = ++m;
		return true;
	}

	void pop()
	{
		--m;
	}
};

class Miniball
{
  public:
	typedef std::list<fvec3> VectorList;
	typedef VectorList::iterator It;
	typedef VectorList::const_iterator Cit;

  private:
	VectorList L;
	Basis B;
	It support_end;

	void mtf_mb(It i)
	{
		support_end = L.begin();
		if (B.size() == 4)
			return;

		for (It k = L.begin(); k != i;)
		{
			It j = k++;
			if (B.excess(*j) > 0)
			{
				if (B.push(*j))
				{
					mtf_mb(j);
					B.pop();
					move_to_front(j);
				}
			}
		}
	}

	void pivot_mb(It i)
	{
		It t = ++L.begin();
		mtf_mb(t);
		float max_e, old_sqr_r = 0;
		do
		{
			It pivot = L.begin();
			max_e = max_excess(t, i, pivot);
			if (max_e > 0)
			{
				t = support_end;
				if (t == pivot)
					++t;
				old_sqr_r = B.squared_radius();
				B.push(*pivot);
				mtf_mb(support_end);
				B.pop();
				move_to_front(pivot);
			}
		} while ((max_e > 0) && (B.squared_radius() > old_sqr_r));
	}

	void move_to_front(It j)
	{
		if (support_end == j)
			support_end++;
		L.splice(L.begin(), L, j);
	}

	float max_excess(It t, It i, It& pivot) const
	{
		const fvec3* pCenter = B.center();
		float sqr_r = B.squared_radius();
		float e, max_e = 0;

		for (It k = t; k != i; ++k)
		{
			const fvec3& point = *k;
			e = -sqr_r;
			e += point.distance_to_sqr(*pCenter);
			if (e > max_e)
			{
				max_e = e;
				pivot = k;
			}
		}
		return max_e;
	}

	float abs(float r) const
	{
		return (r > 0) ? r : (-r);
	}
	float sqr(float r) const
	{
		return r * r;
	}

  public:
	Miniball()
	{
	}
	void check_in(const fvec3& p)
	{
		L.push_back(p);
	}

	void build()
	{
		B.reset();
		support_end = L.begin();
		pivot_mb(L.end());
	}

	fvec3 center() const
	{
		return *((fvec3*)B.center());
	}
	float squared_radius() const
	{
		return B.squared_radius();
	}
	int num_points() const
	{
		return static_cast<int>(L.size());
	}
	Cit points_begin() const
	{
		return L.begin();
	}
	Cit points_end() const
	{
		return L.end();
	}
	int nr_support_gVectors() const
	{
		return B.support_size();
	}
	Cit support_points_begin() const
	{
		return L.begin();
	}
	Cit support_points_end() const
	{
		return support_end;
	}
};

inline void Fsphere_compute(Fsphere& dest, const fvec3* verts, int count)
{
	Miniball mb;

	for (int i = 0; i < count; i++)
		mb.check_in(verts[i]);

	mb.build();

	dest.P.set(mb.center());
	dest.R = std::sqrt(mb.squared_radius());
}
