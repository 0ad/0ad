//***********************************************************
// Name:		Vector3D.H
// Description: Provides an interface for a vector in R3 and
//				allows vector and scalar operations on it
//
//***********************************************************

#ifndef VECTOR3D_H
#define VECTOR3D_H

class CVector3D
{
	public:
		float X, Y, Z;

	public:
		CVector3D () : X(0.0f), Y(0.0f), Z(0.0f) {}
		CVector3D (float x, float y, float z) : X(x), Y(y), Z(z) {}

		int operator!() const;

		float& operator[](int index) { return *((&X)+index); }
		const float& operator[](int index) const { return *((&X)+index); }

		//vector equality (testing float equality, so please be careful if necessary)
		bool operator== (const CVector3D &vector) const;
		bool operator!= (const CVector3D &vector) const { return !operator==(vector); }

		//vector addition
		CVector3D operator + (const CVector3D &vector) const ;
		//vector addition/assignment
		CVector3D &operator += (const CVector3D &vector);

		//vector subtraction
		CVector3D operator - (const CVector3D &vector) const ;
		//vector subtraction/assignment
		CVector3D &operator -= (const CVector3D &vector);
		
		//scalar multiplication
		CVector3D operator * (float value) const ;
		//scalar multiplication/assignment
		CVector3D& operator *= (float value);

		// negation
		CVector3D operator-() const;

	public:
		void Set (float x, float y, float z);
		void Clear ();

		//Dot product
		float Dot (const CVector3D &vector) const;
		//Cross product
		CVector3D Cross (const CVector3D &vector) const;

		//Returns length of the vector
		float GetLength () const;
		float LengthSquared () const;
		void Normalize ();

		// Returns 3 element array of floats, e.g. for glVertex3fv
		const float* GetFloatArray() const { return &X; }
};

#endif
