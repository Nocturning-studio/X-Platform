#pragma once

template <class T> class template_plane3D
{
  public:
	typedef T TYPE;
	typedef template_plane3D<T> Self;
	typedef Self& SelfRef;
	typedef const Self& SelfCRef;

  public:
	template_vector3<T> n;
	T d;

  public:
	IC SelfRef set(Self& P)
	{
		n.set(P.n);
		d = P.d;
		return *this;
	}
	IC BOOL similar(Self& P, T eps_n = EPS, T eps_d = EPS)
	{
		return (n.similar(P.n, eps_n) && (_abs(d - P.d) < eps_d));
	}
	ICF SelfRef build(const template_vector3<T>& v1, const template_vector3<T>& v2, const template_vector3<T>& v3)
	{
		template_vector3<T> t1, t2;
		n.crossproduct(t1.sub(v1, v2), t2.sub(v1, v3)).normalize();
		d = -n.dotproduct(v1);
		return *this;
	}
	ICF SelfRef build_precise(const template_vector3<T>& v1, const template_vector3<T>& v2, const template_vector3<T>& v3)
	{
		template_vector3<T> t1, t2;
		n.crossproduct(t1.sub(v1, v2), t2.sub(v1, v3));
		exact_normalize(n);
		d = -n.dotproduct(v1);
		return *this;
	}
	ICF SelfRef build(const template_vector3<T>& _p, const template_vector3<T>& _n)
	{
		d = -n.normalize(_n).dotproduct(_p);
		return *this;
	}
	ICF SelfRef build_unit_normal(const template_vector3<T>& _p, const template_vector3<T>& _n)
	{
		d = -n.set(_n).dotproduct(_p);
		return *this;
	}
	IC SelfRef project(template_vector3<T>& pdest, template_vector3<T>& psrc)
	{
		pdest.mad(psrc, n, -classify(psrc));
		return *this;
	}
	ICF T classify(const template_vector3<T>& v) const
	{
		return n.dotproduct(v) + d;
	}
	IC SelfRef normalize()
	{
		T denom = 1.f / n.magnitude();
		n.mul(denom);
		d *= denom;
		return *this;
	}
	IC T distance(const template_vector3<T>& v)
	{
		return _abs(classify(v));
	}
	IC BOOL intersectRayDist(const template_vector3<T>& P, const template_vector3<T>& D, T& dist)
	{
		T numer = classify(P);
		T denom = n.dotproduct(D);

		if (_abs(denom) < EPS_S) // normal is orthogonal to vector3, cant intersect
			return FALSE;

		dist = -(numer / denom);
		return ((dist > 0.f) || fis_zero(dist));
	}
	ICF BOOL intersectRayPoint(const template_vector3<T>& P, const template_vector3<T>& D, template_vector3<T>& dest)
	{
		T numer = classify(P);
		T denom = n.dotproduct(D);

		if (_abs(denom) < EPS_S)
			return FALSE; // normal is orthogonal to vector3, cant intersect
		else
		{
			float dist = -(numer / denom);
			dest.mad(P, D, dist);
			return ((dist > 0.f) || fis_zero(dist));
		}
	}
	IC BOOL intersect(const template_vector3<T>& u, const template_vector3<T>& v, // segment
					  template_vector3<T>& isect)						  // intersection point
	{
		T denom, dist;
		template_vector3<T> t;

		t.sub(v, u);
		denom = n.dotproduct(t);
		if (_abs(denom) < EPS)
			return false; // they are parallel

		dist = -(n.dotproduct(u) + d) / denom;
		if (dist < -EPS || dist > 1 + EPS)
			return false;
		isect.mad(u, t, dist);
		return true;
	}

	IC BOOL intersect_2(const template_vector3<T>& u, const template_vector3<T>& v, // segment
						template_vector3<T>& isect)							// intersection point
	{
		T dist1, dist2;
		template_vector3<T> t;

		dist1 = n.dotproduct(u) + d;
		dist2 = n.dotproduct(v) + d;

		if (dist1 * dist2 < 0.0f)
			return false;

		t.sub(v, u);
		isect.mad(u, t, dist1 / _abs(dist1 - dist2));

		return true;
	}
	IC SelfRef transform(template_matrix4x4<T>& M)
	{
		// rotate the normal
		M.transform_dir(n);
		// slide the offset
		d -= M.c.dotproduct(n);
		return *this;
	}
};

typedef template_plane3D<float> Fplane;
typedef template_plane3D<double> Dplane;

template <class T> BOOL _valid(const template_plane3D<T>& s)
{
	return _valid(s.n) && _valid(s.d);
}
