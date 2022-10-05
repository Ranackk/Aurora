//#pragma once
//
//#include <numeric>
//#include <glm\ext\vector_float4.hpp>
//#include <glm\geometric.hpp>
//
//namespace Math
//{
//	// Bivectors
//	// Bivectors are defined to have one component per plane of the n-dimensional coordinate system.
//	// They describe the parallelogram between two vectors, as well as the rotation from u to v.
//	// 
//	// Carefull: A bivector always is formed by the vedge product of two vectors. Using three vectors would yield a trivector that represents a volume and that handiness of the valume.
//	// Carefull2: They are, in 3D space, confused with vectors as there, both 3 axes, as well as 3 planes of rotation exist, meaning that both vector and bivector have the same number of components.
//	struct Bivector4
//	{
//		float XY = 0;
//		float XZ = 0;
//		float XW = 0;
//		float YZ = 0;
//		float YW = 0;
//		float ZW = 0;
//
//		Bivector4() = default;
//
//		Bivector4(float xy, float xz, float xw, float yz, float yw, float zw)
//		: XY(xy), XZ(xz), XW(xw), YZ(yz), YW(yw), ZW(zw) {}
//
//		Bivector4 operator* (float v)
//		{
//			return {
//				v * XY,
//				v * XZ, 
//				v * XW,
//				v * YZ,
//				v * YW, 
//				v * ZW
//			};
//		}
//		
//	};
//
//	Bivector4 operator* (float v, const Bivector4& b)
//	{
//		return {
//			v * b.XY,
//			v * b.XZ, 
//			v * b.XW,
//			v * b.YZ,
//			v * b.YW, 
//			v * b.ZW
//		};
//	}
//
//	inline Bivector4 Wedge(const glm::vec4& u, const glm::vec4& v)
//	{
//		return {
//			u.x * v.y - u.y * v.x,	// xy
//			u.x * v.z - u.z * v.x,	// xz
//			u.x * v.w - u.w * v.x,	// xw
//			u.y * v.z - u.z * v.y,	// yz
//			u.y * v.w - u.w * v.y,	// yw,
//			u.z * v.w - u.w * v.z	// zw
//		}
//	}
//	
//	// A rotor is the geometric product of a multiplication of vector and vector
//	// It describes a plane and a rotation in that plane.
//	// As the geometric product of u and v is defined to be the sum of u dot v and u wedge v, it consists of a scalar part (u dot v) and a bivector (u wedge v).
//	// Multiplying a rotor with a vector rotates that vector by a rotation of from u to v in the plane spanned by u and v.
//	// Rotors are similar to quaternions in terms of implementation, but as they are derived from the concept of multivector spaces in geometric algebra, they are
//	// more easily expand- and understandable in higher dimensions, comapred to quaternions that stem from an expansion of complex numbers.
//	// Another reason for that is that quaternions are using angle + axist to rotate, which is a case only applicable to 3D space, because usually, a rotation is defined to be angle + plane.
//	// Only in 3D it is possible to have a direct mapping from polane to axis by the corss product, which does not exist in 4D. Thus, rotors as the more general concept are more usefull for general spaces.
//	
//	// Adopted from https://marctenbosch.com/miegakure/data/RotorCode.png & https://marctenbosch.com/quaternions/code.htm
//	class Rotor4
//	{	
//		// Scalar Part ~> The rotation "amount".
//		// Caution: This is only half the rotation, as a rotation operation on a vector looks like r^-1pr, similar to quaternions.
//		float m_A;
//
//		// Bivector Part ~> The rotation "plane"
//		Bivector4 m_B;
//
//		Rotor4() = default;
//		Rotor4(float a, float xy, float xz, float xw, float yz, float yw, float zw) : m_A(a), m_B(xy, xz, xw, yz, yw, zw) {}
//			
//		// Plane has to be normalized!
//		Rotor4(float angleRadians, const Bivector4& bivectorPlane)
//		{
//			m_A = cos(angleRadians / 2.0f);
//			m_B = -sin(angleRadians / 2.0f) * bivectorPlane;
//		}
//
//		Rotor4(const glm::vec4& from, const glm::vec4& to)
//		{
//			m_A = 1.0f + glm::dot(to, from);
//			m_B = Wedge(to, from);
//			Normalize();
//		}
//
//		//////////////////////////////////////////////////////////////////////////
//
//		inline Rotor4 operator* (const Rotor4& other) const
//		{
//			Rotor4 ret;
//
//			ret.m_A	= m_A * other.m_A 
//				- m_B.XY * other.m_B.XY
//				- m_B.XZ * other.m_B.XZ
//				- m_B.XW * other.m_B.XW
//				- m_B.YZ * other.m_B.YZ
//				- m_B.YW * other.m_B.YW
//				- m_B.ZW * other.m_B.ZW;
//
//			ret.m_B.XY = m_B.XY * other.m_A + m_A * other.m_B.XY + m_
//		}
//			
//		//////////////////////////////////////////////////////////////////////////
//		
//		inline float LengthSQ() const
//		{
//			return glm::sqrt(m_A) + glm::sqrt(m_B.XY) + glm::sqrt(m_B.XZ) + glm::sqrt(m_B.XW) + glm::sqrt(m_B.YZ) + glm::sqrt(m_B.YW) + glm::sqrt(m_B.ZW);
//		}
//
//		//////////////////////////////////////////////////////////////////////////
//
//		inline float Length() const
//		{
//			return sqrt( LengthSQ() );
//		}
//		
//		//////////////////////////////////////////////////////////////////////////
//
//		inline void Normalize()
//		{
//			float len = Length();
//			m_A /= len;
//			m_B = m_B * (1.0f / len);
//		}
//
//	};
//}
//
