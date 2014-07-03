/* Copyright (C) 2010 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Provides an interface for a vector in R3 and allows vector and
 * scalar operations on it
 */

#ifndef INCLUDED_VECTOR3D
#define INCLUDED_VECTOR3D

class CFixedVector3D;

class CVector3D
{
	public:
		float X, Y, Z;

	public:
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

	public:
		float Dot (const CVector3D &vector) const
		{
			return ( X * vector.X +
					 Y * vector.Y +
					 Z * vector.Z );
		}

		CVector3D Cross (const CVector3D &vector) const
		{
			CVector3D Temp;
			Temp.X = (Y * vector.Z) - (Z * vector.Y);
			Temp.Y = (Z * vector.X) - (X * vector.Z);
			Temp.Z = (X * vector.Y) - (Y * vector.X);
			return Temp;
		}

		float Length () const;
		float LengthSquared () const;
		void Normalize ();
		CVector3D Normalized () const;

		// Returns 3 element array of floats, e.g. for glVertex3fv
		const float* GetFloatArray() const { return &X; }
};

extern float MaxComponent(const CVector3D& v);

#endif
