//***********************************************************
//
// Name:		Matrix3D.Cpp
// Last Update:	31/1/02
// Author:		Poya Manouchehri
//
// Description: A Matrix class used for holding and 
//				manipulating transformation info.
//
//***********************************************************

#include "Matrix3D.H"

CMatrix3D::CMatrix3D ()
{
	SetIdentity ();
}

//Matrix multiplication
CMatrix3D CMatrix3D::operator * (CMatrix3D &matrix)
{
	CMatrix3D Temp;

	Temp._11 = _11*matrix._11 +
			   _12*matrix._21 +
			   _13*matrix._31 +
			   _14*matrix._41;

	Temp._12 = _11*matrix._12 +
			   _12*matrix._22 +
			   _13*matrix._32 +
			   _14*matrix._42;

	Temp._13 = _11*matrix._13 +
			   _12*matrix._23 +
			   _13*matrix._33 +
			   _14*matrix._43;

	Temp._14 = _11*matrix._14 +
			   _12*matrix._24 +
			   _13*matrix._34 +
			   _14*matrix._44;

	Temp._21 = _21*matrix._11 +
			   _22*matrix._21 +
			   _23*matrix._31 +
			   _24*matrix._41;

	Temp._22 = _21*matrix._12 +
			   _22*matrix._22 +
			   _23*matrix._32 +
			   _24*matrix._42;
	
	Temp._23 = _21*matrix._13 +
			   _22*matrix._23 +
			   _23*matrix._33 +
			   _24*matrix._43;

	Temp._24 = _21*matrix._14 +
			   _22*matrix._24 +
			   _23*matrix._34 +
			   _24*matrix._44;

	Temp._31 = _31*matrix._11 +
			   _32*matrix._21 +
			   _33*matrix._31 +
			   _34*matrix._41;

	Temp._32 = _31*matrix._12 +
			   _32*matrix._22 +
			   _33*matrix._32 +
			   _34*matrix._42;

	Temp._33 = _31*matrix._13 +
			   _32*matrix._23 +
			   _33*matrix._33 +
			   _34*matrix._43;

	Temp._34 = _31*matrix._14 +
			   _32*matrix._24 +
			   _33*matrix._34 +
			   _34*matrix._44;

	Temp._41 = _41*matrix._11 +
			   _42*matrix._21 +
			   _43*matrix._31 +
			   _44*matrix._41;

	Temp._42 = _41*matrix._12 +
			   _42*matrix._22 +
			   _43*matrix._32 +
			   _44*matrix._42;

	Temp._43 = _41*matrix._13 +
			   _42*matrix._23 +
			   _43*matrix._33 +
			   _44*matrix._43;

	Temp._44 = _41*matrix._14 +
			   _42*matrix._24 +
			   _43*matrix._34 +
			   _44*matrix._44;

	return Temp;
}

//Matrix multiplication/assignment
CMatrix3D &CMatrix3D::operator *= (CMatrix3D &matrix)
{
	CMatrix3D &Temp = (*this) * matrix;

	return Temp;
}

//Sets the identity matrix
void CMatrix3D::SetIdentity ()
{
	_11=1.0f; _12=0.0f; _13=0.0f; _14=0.0f;
	_21=0.0f; _22=1.0f; _23=0.0f; _24=0.0f;
	_31=0.0f; _32=0.0f; _33=1.0f; _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

//Sets the zero matrix
void CMatrix3D::SetZero ()
{
	_11=0.0f; _12=0.0f; _13=0.0f; _14=0.0f;
	_21=0.0f; _22=0.0f; _23=0.0f; _24=0.0f;
	_31=0.0f; _32=0.0f; _33=0.0f; _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=0.0f;
}

//The following clear the matrix and set the 
//rotation of each of the 3 axes

void CMatrix3D::SetXRotation (float angle)
{
	float Cos = cosf (angle);
	float Sin = sinf (angle);
	
	_11=1.0f; _12=0.0f; _13=0.0f; _14=0.0f;
	_21=0.0f; _22=Cos;  _23=-Sin; _24=0.0f;
	_31=0.0f; _32=Sin;  _33=Cos;  _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

void CMatrix3D::SetYRotation (float angle)
{
	float Cos = cosf (angle);
	float Sin = sinf (angle);

	_11=Cos;  _12=0.0f; _13=Sin;  _14=0.0f;
	_21=0.0f; _22=1.0f; _23=0.0f; _24=0.0f;
	_31=-Sin; _32=0.0f; _33=Cos;  _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

void CMatrix3D::SetZRotation (float angle)
{
	float Cos = cosf (angle);
	float Sin = sinf (angle);

	_11=Cos;  _12=-Sin; _13=0.0f; _14=0.0f;
	_21=Sin;  _22=Cos;  _23=0.0f; _24=0.0f;
	_31=0.0f; _32=0.0f; _33=1.0f; _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

//The following apply a rotation to the matrix
//about each of the axes;

void CMatrix3D::RotateX (float angle)
{
	CMatrix3D Temp;
	Temp.SetXRotation (angle);

	(*this) = Temp * (*this);
}

void CMatrix3D::RotateY (float angle)
{
	CMatrix3D Temp;
	Temp.SetYRotation (angle);

	(*this) = Temp * (*this);
}

void CMatrix3D::RotateZ (float angle)
{
	CMatrix3D Temp;
	Temp.SetZRotation (angle);

	(*this) = Temp * (*this);
}

//Sets the translation of the matrix
void CMatrix3D::SetTranslation (float x, float y, float z)
{
	_11=1.0f; _12=0.0f; _13=0.0f; _14=x;
	_21=0.0f; _22=1.0f; _23=0.0f; _24=y;
	_31=0.0f; _32=0.0f; _33=1.0f; _34=z;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

void CMatrix3D::SetTranslation (CVector3D &vector)
{
	SetTranslation (vector.X, vector.Y, vector.Z);	
}

//Applies a translation to the matrix
void CMatrix3D::Translate (float x, float y, float z)
{
	CMatrix3D Temp;
	Temp.SetTranslation (x, y, z);

	(*this) = Temp * (*this);
}

void CMatrix3D::Translate (CVector3D &vector)
{
	Translate (vector.X, vector.Y, vector.Z);
}

CVector3D CMatrix3D::GetTranslation ()
{
	CVector3D Temp;

	Temp.X = _14;
	Temp.Y = _24;
	Temp.Z = _34;

	return Temp;
}

//Clears and sets the scaling of the matrix
void CMatrix3D::SetScaling (float x_scale, float y_scale, float z_scale)
{
	_11=x_scale; _12=0.0f;	  _13=0.0f;	   _14=0.0f;
	_21=0.0f;	 _22=y_scale; _23=0.0f;	   _24=0.0f;
	_31=0.0f;	 _32=0.0f;	  _33=z_scale; _34=0.0f;
	_41=0.0f;	 _42=0.0f;	  _43=0.0f;    _44=1.0f;
}

//Scales the matrix
void CMatrix3D::Scale (float x_scale, float y_scale, float z_scale)
{
	CMatrix3D Temp;
	Temp.SetScaling (x_scale, y_scale, z_scale);

	(*this) = Temp * (*this);
}

//Returns the transpose of the matrix. For orthonormal
//matrices, this is the same is the inverse matrix
CMatrix3D CMatrix3D::GetTranspose ()
{
	CMatrix3D Temp;

	Temp._11 = _11;
	Temp._21 = _12;
	Temp._31 = _13;
	Temp._41 = 0.0f;

	Temp._12 = _21;
	Temp._22 = _22;
	Temp._32 = _23;
	Temp._42 = 0.0f;

	Temp._13 = _31;
	Temp._23 = _32;
	Temp._33 = _33;
	Temp._43 = 0.0f;

	Temp._14 = 0.0f;
	Temp._24 = 0.0f;
	Temp._34 = 0.0f;
	Temp._44 = 1.0f;

	CMatrix3D Trans;
	Trans.SetTranslation (-_14, -_24, -_34);

	Temp = Temp * Trans;

	return Temp;
}

//Get a vector which points to the left of the matrix
CVector3D CMatrix3D::GetLeft ()
{
	CVector3D Temp;

	Temp.X = -_11;
	Temp.Y = -_21;
	Temp.Z = -_31;

	return Temp;
}

//Get a vector which points up from the matrix
CVector3D CMatrix3D::GetUp ()
{
	CVector3D Temp;

	Temp.X = _12;
	Temp.Y = _22;
	Temp.Z = _32;

	return Temp;
}

//Get a vector which points to front of the matrix
CVector3D CMatrix3D::GetIn ()
{
	CVector3D Temp;

	Temp.X = _13;
	Temp.Y = _23;
	Temp.Z = _33;

	return Temp;
}

//Set the matrix from two vectors (Up and In)
void CMatrix3D::SetFromUpIn (CVector3D &up, CVector3D &in, float scale)
{
	CVector3D u = up;
	CVector3D i = in;
	
	CVector3D r;
	
	r = up.Cross (in);

	u.Normalize (); u *= scale;
	i.Normalize (); i *= scale;
	r.Normalize (); r *= scale;

	_11=r.X;  _12=u.X;  _13=i.X;  _14=0.0f;
	_21=r.Y;  _22=u.Y;  _23=i.Y;  _24=0.0f;
	_31=r.Z;  _32=u.Z;	_33=i.Z;  _34=0.0f;
	_41=0.0f; _42=0.0f;	_43=0.0f; _44=1.0f;
}

//Transform a vector by this matrix
CVector3D CMatrix3D::Transform (CVector3D &vector)
{
	CVector3D Temp;

	Temp.X = _11*vector.X +
			 _12*vector.Y +
			 _13*vector.Z +
			 _14;

	Temp.Y = _21*vector.X +
			 _22*vector.Y +
			 _23*vector.Z +
			 _24;

	Temp.Z = _31*vector.X +
			 _32*vector.Y +
			 _33*vector.Z +
			 _34;

	return Temp;
}

//Only rotate (not translate) a vector by this matrix
CVector3D CMatrix3D::Rotate (CVector3D &vector)
{
	CVector3D Temp;

	Temp.X = _11*vector.X +
			 _12*vector.Y +
			 _13*vector.Z;

	Temp.Y = _21*vector.X +
			 _22*vector.Y +
			 _23*vector.Z;

	Temp.Z = _31*vector.X +
			 _32*vector.Y +
			 _33*vector.Z;

	return Temp;
}
