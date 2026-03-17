#ifndef _MATH_RAY_HPP
#define _MATH_RAY_HPP

#include "Misc/libdesc.hpp"
#include "Misc/hash.hpp"

LIB_NAMESPACE_START

namespace math
{

template < class ValueType, class HashType = hash::DefaultHashType >
struct Ray
{
	typedef Vector3< ValueType, HashType > VectorType;
	typedef Ray< ValueType, HashType > this_type;

	VectorType origin;
	VectorType direction;

	template < class V, class H >
	Ray() {}

	template < class V, class H >
	Ray(const Vector3< V,H >& _origin, Vector3< V,H > _direction) : origin(_origin), direction(_direction) {}

	std::pair<float, float> distance(const this_type& ray2) const
	{
		VectorType p2  = origin			+ direction;
		VectorType p4  = ray2.origin	+ ray2.direction;
		VectorType d21 = origin			- p2;
		VectorType d34 = p4				- ray2.origin;
		VectorType d13 = ray2.origin	- origin;

		float a =  d21.dotp(d21);
		float b =  d21.dotp(d34);
		float c =  d34.dotp(d34);
		float d = -d13.dotp(d21);
		float e = -d13.dotp(d34);

		float u = (d*c-e*b)/(c*a-b*b);
		return std::pair<float, float>(u, (e - b * u) / c);
	}
};

typedef Ray<float> Rayf;
typedef Ray<double> Rayd;

}

LIB_NAMESPACE_END

#endif // _MATH_RAY_HPP
