/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Provides an interface for a vector in R3 and allows vector and
 * scalar operations on it
 */

#ifndef INCLUDED_VECTOR3D
#define INCLUDED_VECTOR3D

#include "ps/containers/Span.h"

class CFixedVector3D;

class CVector3D
{
	public:
		float X, Y, Z;

	public:
		// Returns maximum/minimum possible position stored in the CVector3D.
		static CVector3D Max();
		static CVector3D Min();

		CVector3D() : X(0.0f), Y(0.0f), Z(0.0f) {}
		CVector3D(float x, float y, float z) : X(x), Y(y), Z(z) {}
		CVector3D(const CFixedVector3D& v);

		int operator!() const;

		float& operator[](int index) { return *((&X)+index); }
		const float& operator[](int index) const { return *((&X)+index); }

		// vector equality (testing float equality, so please be careful if necessary)
		bool operator==(const CVector3D &vector) const
		{
			return (X == vector.X && Y == vector.Y && Z == vector.Z);
		}

		bool operator!=(const CVector3D& vector) const
		{
			return !operator==(vector);
		}

		CVector3D operator+(const CVector3D& vector) const
		{
			return CVector3D(X + vector.X, Y + vector.Y, Z + vector.Z);
		}

		CVector3D& operator+=(const CVector3D& vector)
		{
			X += vector.X;
			Y += vector.Y;
			Z += vector.Z;
			return *this;
		}

		CVector3D operator-(const CVector3D& vector) const
		{
			return CVector3D(X - vector.X, Y - vector.Y, Z - vector.Z);
		}

		CVector3D& operator-=(const CVector3D& vector)
		{
			X -= vector.X;
			Y -= vector.Y;
			Z -= vector.Z;
			return *this;
		}

		CVector3D operator*(float value) const
		{
			return CVector3D(X * value, Y * value, Z * value);
		}

		CVector3D& operator*=(float value)
		{
			X *= value;
			Y *= value;
			Z *= value;
			return *this;
		}

		CVector3D operator-() const
		{
			return CVector3D(-X, -Y, -Z);
		}

		float Dot(const CVector3D& vector) const
		{
			return ( X * vector.X +
					 Y * vector.Y +
					 Z * vector.Z );
		}

		CVector3D Cross(const CVector3D& vector) const
		{
			CVector3D temp;
			temp.X = (Y * vector.Z) - (Z * vector.Y);
			temp.Y = (Z * vector.X) - (X * vector.Z);
			temp.Z = (X * vector.Y) - (Y * vector.X);
			return temp;
		}

		float Length() const;
		float LengthSquared() const;
		void Normalize();
		CVector3D Normalized() const;

		// Returns 3 element array of floats, e.g. for vec3 uniforms.
		PS::span<const float> AsFloatArray() const
		{
			// Additional check to prevent a weird compiler has a different
			// alignement for an array and a class members.
			static_assert(
				sizeof(CVector3D) == sizeof(float) * 3u &&
				offsetof(CVector3D, X) == 0 &&
				offsetof(CVector3D, Y) == sizeof(float) &&
				offsetof(CVector3D, Z) == sizeof(float) * 2u,
				"Vector3D should be properly layouted to use AsFloatArray");
			return PS::span<const float>(&X, 3);
		}
};

extern float MaxComponent(const CVector3D& v);

#endif
