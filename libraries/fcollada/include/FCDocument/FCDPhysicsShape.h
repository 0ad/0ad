/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_PHYSICS_SHAPE_H_
#define _FCD_PHYSICS_SHAPE_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_
#ifndef _FCD_PHYSICS_ANALYTICAL_GEOM_H_
#include "FCDocument/FCDPhysicsAnalyticalGeometry.h"
#endif // _FCD_PHYSICS_ANALYTICAL_GEOM_H_
#ifndef _FCD_TRANSFORM_H_
#include "FCDocument/FCDTransform.h" /** @todo Remove this include by moving the FCDTransform::Type enum to FUDaeEnum.h. */
#endif // _FCD_TRANSFORM_H_
#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_

class FCDocument;
class FCDEntityInstance;
class FCDPhysicsRigidBody;
class FCDPhysicsRigidConstraint;
class FCDGeometry;
class FCDGeometryInstance;
class FCDPhysicsAnalyticalGeometry;
class FCDPhysicsMaterial;
class FCDTransform;

typedef FUObjectContainer<FCDTransform> FCDTransformContainer;

class FCOLLADA_EXPORT FCDPhysicsShape : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	bool hollow;
	FUObjectPtr<FCDPhysicsMaterial> physicsMaterial;
	bool ownsPhysicsMaterial;
	
	//one of these two will define the rigid body
	FUObjectPtr<FCDGeometryInstance> geometry;
	FUObjectRef<FCDPhysicsAnalyticalGeometry> analGeom;
	bool ownsGeometryInstance;

	float* mass;
	float* density;
	FCDTransformContainer transforms;
	FCDEntityInstance* instanceMaterialRef;

public:
	FCDPhysicsShape(FCDocument* document);
	virtual ~FCDPhysicsShape();

	// Physics parameters
	float GetMass();
	void SetMass(float _mass);
	float GetDensity();
	void SetDensity(float _density);
	bool IsHollow() const {return hollow;}
	void SetHollow(bool _hollow) {hollow = _hollow;}

	// Geometry
	bool IsAnalyticalGeometry() const { return analGeom != NULL; }
	FCDPhysicsAnalyticalGeometry* GetAnalyticalGeometry() { return analGeom; }
	const FCDPhysicsAnalyticalGeometry* GetAnalyticalGeometry() const { return analGeom; }
	bool IsGeometryInstance() { return geometry != NULL; }
	FCDGeometryInstance* GetGeometryInstance() { return geometry; }
	const FCDGeometryInstance* GetGeometryInstance() const { return geometry; }
	FCDGeometryInstance* CreateGeometryInstance(FCDGeometry* geom, bool createConvexMesh=false);
	FCDPhysicsAnalyticalGeometry* CreateAnalyticalGeometry(FCDPhysicsAnalyticalGeometry::GeomType type);

	// Geometry Transforms
	FCDTransformContainer& GetTransforms() {return transforms;}
	const FCDTransformContainer& GetTransforms() const {return transforms;}
	FCDTransform* AddTransform(FCDTransform::Type type, size_t index = (size_t)-1);

	// Physics Material
	FCDPhysicsMaterial* GetPhysicsMaterial() {return physicsMaterial;}
	const FCDPhysicsMaterial* GetPhysicsMaterial() const {return physicsMaterial;}
	void SetPhysicsMaterial(FCDPhysicsMaterial* physicsMaterial);
	FCDPhysicsMaterial* AddOwnPhysicsMaterial();

	// Create a copy of this shape
	FCDPhysicsShape* Clone(FCDPhysicsShape* clone = NULL) const;

	// Read in the <physics_shape> node of the COLLADA document
	virtual bool LoadFromXML(xmlNode* node);

	// Write out the <physics_shape> node to the COLLADA XML tree
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_PHYSICS_SHAPE_H_
