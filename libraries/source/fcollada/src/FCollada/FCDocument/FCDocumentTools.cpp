/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDocumentTools.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDGeometryNURBSSurface.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDTargetedEntity.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDPlaceHolder.h"
#include "FCDocument/FCDSceneNodeIterator.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDAnimationCurveTools.h"
#include "FCDocument/FCDAnimationMultiCurve.h"

//
// FCDocumentTools
// 

namespace FCDocumentTools
{
	class FCDConversionSwapFunctor
	{
	private:
		enum Axis { X, Y, Z, UNKNOWN } current, target;
		Axis ConvertVector(const FMVector3& v)
		{
			if (IsEquivalent(v, FMVector3::XAxis)) return X;
			else if (IsEquivalent(v, FMVector3::YAxis)) return Y;
			else if (IsEquivalent(v, FMVector3::ZAxis)) return Z;
			else return UNKNOWN;
		}

		typedef void (*ConversionFn) (FMVector3& data, int32 sign);
		ConversionFn functor;

	public:
		FCDConversionSwapFunctor(const FMVector3& targetAxis)
			:	current(UNKNOWN), target(UNKNOWN)
		{
			target = ConvertVector(targetAxis);
		};

		inline void SetCurrent(const FMVector3& axis)
		{
			current = ConvertVector(axis);
			PrepareFunctor();
		}

		inline bool HasConversion() { return functor != Identity; }

		static FCDTRotation* GetLastTransformForPivot(FCDSceneNode* node)
		{
			// Verify whether the last transform on this node's stack is a rotation transform.
			if (node->GetTransformCount() > 0)
			{
				FCDTransform* transform = node->GetTransform(node->GetTransformCount() - 1);
				if (transform->HasType(FCDTRotation::GetClassType()))
				{
					FCDAnimated* animated = transform->GetAnimated();
					if (animated != NULL && !animated->HasCurve()) animated = NULL;
					if (animated == NULL) return (FCDTRotation*) transform;
				}
			}
			return NULL;
		}

		static void SmartAddRotationPivot(FCDSceneNode* node, const FMVector3& axis, float angle)
		{
			// Check for an old rotation pivot to remove.
			FCDTRotation* lastRotation = GetLastTransformForPivot(node);
			if (lastRotation != NULL && IsEquivalent(lastRotation->GetAxis(), axis) && IsEquivalent(lastRotation->GetAngle(), -1.0f * angle))
			{
				SAFE_RELEASE(lastRotation);
				lastRotation = GetLastTransformForPivot(node);
			}
			else
			{
				// Add this rotation to pivot the entity instance.
				FCDTRotation* rotate = (FCDTRotation*) node->AddTransform(FCDTransform::ROTATION);
				rotate->SetAxis(axis);
				rotate->SetAngle(angle);
			}
		}

		void SetPivotTransform(FCDSceneNode* node, bool rollOnly)
		{
			if (functor == Identity) return;
			
			
			// Change out of the current up-axis into Y-up.
			if (current == X && !rollOnly)
			{
				SmartAddRotationPivot(node, FMVector3::ZAxis, 90.0f);
			}
			else if (current == Z)
			{
				SmartAddRotationPivot(node, rollOnly ? FMVector3::ZAxis : FMVector3::XAxis, rollOnly ? 90.0f : -90.0f);
			}

			// Change into the target up-axis.
			if (target == X && !rollOnly)
			{
				SmartAddRotationPivot(node, FMVector3::ZAxis, -90.0f);
			}
			else if (target == Z)
			{
				SmartAddRotationPivot(node, rollOnly ? FMVector3::ZAxis : FMVector3::XAxis, rollOnly ? -90.0f : 90.0f);
			}
		}

		void operator() (FMVector3& data, bool isScale = false) { (*functor)(data, (isScale ? 1 : -1)); }

	private:
		void PrepareFunctor()
		{
			if (target == UNKNOWN || current == UNKNOWN || target == current) functor = Identity;
			else
			{
				switch (target)
				{
				case X: functor = (current == Y) ? XtoY : XtoZ; break;
				case Y: functor = (current == X) ? YtoX : YtoZ; break;
				case Z:	functor = (current == X) ? ZtoX : ZtoY; break;
				case UNKNOWN:
				default: FUFail(break);
				}
			}
		}

		static void Identity(FMVector3& UNUSED(data), int32 UNUSED(sign)) {}
		static void XtoY(FMVector3& data, int32 sign) { float t = sign * data.x; data.x = data.y; data.y = t; }
		static void XtoZ(FMVector3& data, int32 UNUSED(sign)) { float t = data.x; data.x = data.y; data.y = data.z; data.z = t; }
		static void YtoX(FMVector3& data, int32 sign) { float t = data.x; data.x = sign * data.y; data.y = t; }
		static void YtoZ(FMVector3& data, int32 sign) { float t = sign * data.y; data.y = data.z; data.z = t; }
		static void ZtoX(FMVector3& data, int32 UNUSED(sign)) { float t = data.z; data.z = data.y; data.y = data.x; data.x = t; }
		static void ZtoY(FMVector3& data, int32 sign) { float t = data.y; data.y = sign * data.z; data.z = t; }
	};

	class FCDConversionUnitFunctor
	{
	private:
		float factor, target;

	public:
		FCDConversionUnitFunctor(float _target)
			:	factor(1.0f), target(_target) {}

		bool HasConversion() { return !IsEquivalent(factor, 1.0f); }
		void SetCurrent(float current) { factor = ((current <= 0.0f) ? 1.0f : (current / target)); }
		float GetConversionFactor() { return factor; }
		void operator() (float& data) { data *= factor; }
		void operator() (FMVector3& data) { data *= factor; }
	};

	typedef fm::pvector<FCDSceneNodeIterator> FCDSceneNodeIteratorList;
	class VisualSceneNodeIterator
	{
	private:
		FCDSceneNodeIteratorList queue;

	public:
		VisualSceneNodeIterator(FCDVisualSceneNodeLibrary* visualSceneLibrary)
		{
			for (size_t i = 0; i < visualSceneLibrary->GetEntityCount(); ++i) 
			{
				// use depth first to make sure node instancing in StandardizeScale works
				queue.push_back(new FCDSceneNodeIterator(visualSceneLibrary->GetEntity(i), FCDSceneNodeIterator::DEPTH_FIRST_PREORDER, true));
			}
		}

		virtual ~VisualSceneNodeIterator()
		{
			while (!queue.empty())
			{
				SAFE_DELETE(queue.back());
				queue.pop_back();
			}
		}

		FCDSceneNode* Next()
		{
			CleanQueue();
			if (queue.empty()) return NULL;

			FCDSceneNode* node = queue.back()->GetNode();
			queue.back()->Next();
			
			return node;
		}

		bool IsDone()
		{
			CleanQueue();
			return queue.empty();
		}
	private:
		void CleanQueue() // makes sure we can get the next node easily
		{
			while (!queue.empty())
			{
				FCDSceneNodeIterator* sceneNodeIterator = queue.back();
				if (!sceneNodeIterator->IsDone()) return;

				SAFE_DELETE(sceneNodeIterator);
				queue.pop_back();
			}
		}
	};

	void GetAssetFunctors(FCDEntity* entity, FCDAsset* libraryAsset, FCDConversionUnitFunctor& lengthFunctor, FCDConversionSwapFunctor& upAxisFunctor)
	{
		FCDAssetList assets; assets.reserve(3);
		entity->GetHierarchicalAssets(assets);
		if (libraryAsset != NULL) assets.push_back(libraryAsset);
		bool hasLength = false;
		bool hasAxis = false;
		for (FCDAssetList::iterator it = assets.begin(); it != assets.end(); ++it)
		{
			if (!hasLength && (*it)->GetHasUnitsFlag()) { hasLength = true; lengthFunctor.SetCurrent((*it)->GetUnitConversionFactor()); }
			if (!hasAxis && (*it)->GetHasUpAxisFlag()) { hasAxis = true; upAxisFunctor.SetCurrent((*it)->GetUpAxis()); }
		}
		if (!hasLength) lengthFunctor.SetCurrent(-1.0f);
		if (!hasAxis) upAxisFunctor.SetCurrent(FMVector3::Origin);
	}

	inline void ResetAsset(FCDEntity* entity)
	{
		FCDAsset* asset = const_cast<FCDAsset*>(const_cast<const FCDEntity*>(entity)->GetAsset());
		if (asset != NULL)
		{
			asset->ResetHasUnitsFlag();
			asset->ResetHasUpAxisFlag();
		}
	}

	void ConvertAnimationVector3(FCDAnimated* animated1, FCDAnimated* animated2, FCDAnimated* animated3, float* v1, float* v2, float* v3, FCDocument* document, FCDConversionUnitFunctor& lengthFunctor, FCDConversionSwapFunctor& upAxisFunctor, bool convertLength, bool isScale = false)
	{
		FUAssert(animated1 != NULL && animated2 != NULL && animated3 != NULL, return);

		size_t indices[3] = { animated1->FindValue(v1), animated2->FindValue(v2), animated3->FindValue(v3) };
		if (indices[0] == ~(size_t)0 && indices[1] == ~(size_t)0 && indices[2] == ~(size_t)0) return; // no animations on the vector
		FCDAnimated* animateds[3] = { animated1, animated2, animated3 };
		FMVector3 localAxes[3] = { FMVector3::XAxis, FMVector3::YAxis, FMVector3::ZAxis };

		// save the curves and remove them from the animated
		size_t curvesCount[3];
		FCDAnimationCurve** curves[3];
		for (size_t i = 0; i < 3; ++i)
		{
			FCDAnimationCurveTrackList& curvesList = animateds[i]->GetCurves()[indices[i]];
			curvesCount[i] = curvesList.size();
			if (curvesCount[i] > 0)
			{
				curves[i] = new FCDAnimationCurve*[curvesCount[i]];
				for (size_t j = 0; j < curvesCount[i]; j++)
				{
					curves[i][j] = curvesList.at(j);
				}
				animateds[i]->RemoveCurve(indices[i]);
			}
			else curves[i] = NULL;
		}

		// transform the keys' outputs
		for (size_t i = 0; i < 3; ++i)
		{
			for (size_t curvesCounter = 0; curvesCounter < curvesCount[i]; curvesCounter++)
			{
				FCDAnimationCurve* currentCurve = curves[i][curvesCounter];
				FCDAnimation* animation = currentCurve->GetParent()->GetParent();

				FCDConversionUnitFunctor savedLengthFunctor(lengthFunctor);
				FCDConversionSwapFunctor savedUpAxisFunctor(upAxisFunctor);
				GetAssetFunctors(animation, document->GetAnimationLibrary()->GetAsset(false), lengthFunctor, upAxisFunctor);
				
				// convert the up axes and update the factor if we need to negate signs
				FMVector3 savedLocalAxes(localAxes[i]);
				upAxisFunctor(localAxes[i], isScale);

				float factor = 1.0f;
				FUAssert(IsEquivalent(localAxes[i].LengthSquared(), 1.0f), continue;);
				if (IsEquivalent(fabsf(localAxes[i].x), 1.0f))
				{
					animateds[0]->AddCurve(indices[0], currentCurve);
					factor = localAxes[i].x;
				}
				else if (IsEquivalent(fabsf(localAxes[i].y), 1.0f))
				{
					animateds[1]->AddCurve(indices[1], currentCurve);
					factor = localAxes[i].y;
				}
				else if (IsEquivalent(fabsf(localAxes[i].z), 1.0f))
				{
					animateds[2]->AddCurve(indices[2], currentCurve);
					factor = localAxes[i].z;
				}

				// convert the unit lengths
				FCDConversionScaleFunctor functor(factor * (convertLength ? lengthFunctor.GetConversionFactor() : 1.0f));
				currentCurve->ConvertValues(&functor, &functor);

				localAxes[i] = savedLocalAxes;
				lengthFunctor = savedLengthFunctor;
				upAxisFunctor = savedUpAxisFunctor;
			}
		}

		for (size_t i = 0; i < 3; ++i)
		{
			SAFE_DELETE_ARRAY(curves[i]);
		}
	}
	inline void ConvertAnimationVector3(FCDAnimated* animated1, FCDAnimated* animated2, FCDAnimated* animated3, FMVector3& v, FCDocument* document, FCDConversionUnitFunctor& lengthFunctor, 
			FCDConversionSwapFunctor& upAxisFunctor, bool convertLength, bool isScale = false)
	{
		ConvertAnimationVector3(animated1, animated2, animated3, &v.x, &v.y, &v.z, document, lengthFunctor, upAxisFunctor, convertLength, isScale);
	}
	inline void ConvertAnimationVector3(FCDAnimated* animated, float* x, float* y, float* z, FCDocument* document, FCDConversionUnitFunctor& lengthFunctor, 
			FCDConversionSwapFunctor& upAxisFunctor, bool convertLength, bool isScale = false)
	{
		ConvertAnimationVector3(animated, animated, animated, x, y, z, document, lengthFunctor, upAxisFunctor, convertLength, isScale);
	}
	inline void ConvertAnimationVector3(FCDAnimated* animated, FMVector3& v, FCDocument* document, FCDConversionUnitFunctor& lengthFunctor, 
			FCDConversionSwapFunctor& upAxisFunctor, bool convertLength, bool isScale = false)
	{
		ConvertAnimationVector3(animated, &v.x, &v.y, &v.z, document, lengthFunctor, upAxisFunctor, convertLength, isScale);
	}

	void ConvertAnimationFloat(FCDAnimated* animated, float& f, FCDocument* document, FCDConversionUnitFunctor& lengthFunctor, FCDConversionSwapFunctor& upAxisFunctor)
	{
		if (animated != NULL)
		{
			size_t index = animated->FindValue(&f);
			if (index == ~(size_t)0) return; // no animations on the float

			FCDAnimationCurveListList& curvesListList = animated->GetCurves();
			if (index >= curvesListList.size()) return; // no animations on the float
			FCDAnimationCurveTrackList& curves = curvesListList.at(index);

			// transform the keys' outputs
			size_t curvesCount = curves.size();
			for (size_t curvesCounter = 0; curvesCounter < curvesCount; curvesCounter++)
			{
				FCDAnimationCurve* currentCurve = curves[curvesCounter];
				FCDAnimation* animation = currentCurve->GetParent()->GetParent();

				FCDConversionUnitFunctor savedLengthFunctor(lengthFunctor);
				FCDConversionSwapFunctor savedUpAxisFunctor(upAxisFunctor);
				GetAssetFunctors(animation, document->GetAnimationLibrary()->GetAsset(false), lengthFunctor, upAxisFunctor);
				
				// convert the unit lengths
				FCDConversionScaleFunctor functor(lengthFunctor.GetConversionFactor());
				currentCurve->ConvertValues(&functor, &functor);

				lengthFunctor = savedLengthFunctor;
				upAxisFunctor = savedUpAxisFunctor;
			}
		} 
	}

#define CONVERT_FL(parameter, f) { lengthFunctor(f); if (parameter.IsAnimated()) { ConvertAnimationFloat(parameter.GetAnimated(), f, document, lengthFunctor, upAxisFunctor); } }
#define CONVERT_VECT3(parameter, v) { upAxisFunctor(v); if (parameter.IsAnimated()) { ConvertAnimationVector3(parameter.GetAnimated(), v, document, lengthFunctor, upAxisFunctor, false); } }
#define CONVERT_SCALE(parameter, v) { upAxisFunctor(v, true); if (parameter.IsAnimated()) { ConvertAnimationVector3(parameter.GetAnimated(), v, document, lengthFunctor, upAxisFunctor, false, true); } }
#define CONVERT_VECT3L(parameter, v) { upAxisFunctor(v); lengthFunctor(v); if (parameter.IsAnimated()) { ConvertAnimationVector3(parameter.GetAnimated(), v, document, lengthFunctor, upAxisFunctor, true); } }
#define CONVERT_MAT44(parameter, m) { \
	FMMatrix44& _m = (m); \
	/* Rotation/Scale. */ \
	CONVERT_VECT3(parameter, *(FMVector3*) _m[0]); \
	CONVERT_VECT3(parameter, *(FMVector3*) _m[1]); \
	CONVERT_VECT3(parameter, *(FMVector3*) _m[2]); \
	for (int i = 0; i < 3; ++i) { \
		FMVector3 v = FMVector3(_m[0][i], _m[1][i], _m[2][i]); \
		upAxisFunctor(v); _m[0][i] = v.x; _m[1][i] = v.y; _m[2][i] = v.z; \
		/* Handle the animation carefully... */ \
		if (parameter.IsAnimated()) { \
			ConvertAnimationVector3(parameter.GetAnimated(), &_m[0][i], &_m[1][i], &_m[2][i], document, lengthFunctor, upAxisFunctor, false); } } \
	/* Translation.. */ \
	CONVERT_VECT3(parameter, *(FMVector3*) _m[3]); \
	CONVERT_FL(parameter, _m[3][0]); CONVERT_FL(parameter, _m[3][1]); CONVERT_FL(parameter, _m[3][2]); }

#define CONVERT_MAT44_NONANIMATABLE(m) { \
	FMMatrix44& _m = (m); \
	/* Rotation/Scale. */ \
	upAxisFunctor(*(FMVector3*) _m[0]); \
	upAxisFunctor(*(FMVector3*) _m[1]); \
	upAxisFunctor(*(FMVector3*) _m[2]); \
	for (int i = 0; i < 3; ++i) { \
		FMVector3 v(_m[0][i], _m[1][i], _m[2][i]); \
		upAxisFunctor(v); _m[0][i] = v.x; _m[1][i] = v.y; _m[2][i] = v.z; } \
	/* Translation.. */ \
	upAxisFunctor(*(FMVector3*) _m[3]); \
	lengthFunctor(_m[3][0]); lengthFunctor(_m[3][1]); lengthFunctor(_m[3][2]); }

	void StandardizeUpAxisAndLength(FCDocument* document, const FMVector3& upAxis, float unitInMeters, bool handleTargets)
	{
		if (document == NULL) return;

        // Figure out the wanted up_axis and unit values, if they are not provided.
        FCDAsset* baseAsset = document->GetAsset();
        if (IsEquivalent(unitInMeters, 0.0f) || unitInMeters < 0.0f)
        {
            if (baseAsset->GetHasUnitsFlag()) unitInMeters = baseAsset->GetUnitConversionFactor();
            else unitInMeters = 1.0f; // COLLADA specification default.
        }
		FMVector3 _upAxis = upAxis;
        if (!IsEquivalent(upAxis, FMVector3::Origin)) _upAxis = upAxis;
        else
        {
            if (baseAsset->GetHasUpAxisFlag()) _upAxis = baseAsset->GetUpAxis();
            else _upAxis = FMVector3::YAxis; // COLLADA specification default.
        }

		// Setup the modification functors.
		FCDConversionUnitFunctor lengthFunctor(unitInMeters);
        FCDConversionSwapFunctor upAxisFunctor(_upAxis);

		FCDocumentList documentQueue;
		documentQueue.push_back(document);

		// documentQueue grows while we are accessing!
		for (size_t documentCounter = 0; documentCounter < documentQueue.size(); documentCounter++)
		{
			FCDocument* document = documentQueue.at(documentCounter);
			FCDExternalReferenceManager* referenceManager = document->GetExternalReferenceManager();
			size_t placeHolderCount = referenceManager->GetPlaceHolderCount();
			for (size_t i = 0; i < placeHolderCount; ++i)
			{
				FCDPlaceHolder* placeHolder = referenceManager->GetPlaceHolder(i);

				if (!placeHolder->IsTargetLoaded())
				{
					if (FCollada::GetDereferenceFlag())
					{
						placeHolder->LoadTarget();
						if (!placeHolder->IsTargetLoaded()) continue;
					}
					else
					{
						// we don't want to load the external reference
						continue;
					}
				}
				FUAssert(placeHolder->IsTargetLoaded(), continue);

				FCDocument* targetDocument = placeHolder->GetTarget();
				if (documentQueue.find(targetDocument) == documentQueue.end())
				{
					documentQueue.push_back(targetDocument);
				}
			}
		}

		// Iterate over all the documents, modifying their elements
		while (!documentQueue.empty())
		{
			FCDocument* document = documentQueue.back(); documentQueue.pop_back();

			// Iterate over the scene graph, modifying the transforms.
			VisualSceneNodeIterator visualSceneNodeIt(document->GetVisualSceneLibrary());
			while (!visualSceneNodeIt.IsDone())
			{
				FCDSceneNode* node = visualSceneNodeIt.Next();
				GetAssetFunctors(node, document->GetVisualSceneLibrary()->GetAsset(false), lengthFunctor, upAxisFunctor);
				if (lengthFunctor.HasConversion() || upAxisFunctor.HasConversion())
				{
					size_t transformCount = node->GetTransformCount();
					for (size_t t = 0; t < transformCount; ++t)
					{
						FCDTransform* transform = node->GetTransform(t);
						if (transform->HasType(FCDTTranslation::GetClassType())) CONVERT_VECT3L(((FCDTTranslation*) transform)->GetTranslation(), ((FCDTTranslation*) transform)->GetTranslation())
						else if (transform->HasType(FCDTRotation::GetClassType())) CONVERT_VECT3(((FCDTRotation*) transform)->GetAngleAxis(), ((FCDTRotation*) transform)->GetAxis())
						else if (transform->HasType(FCDTScale::GetClassType())) CONVERT_SCALE(((FCDTScale*) transform)->GetScale(), ((FCDTScale*) transform)->GetScale())
						else if (transform->HasType(FCDTSkew::GetClassType())) { CONVERT_VECT3(((FCDTSkew*) transform)->GetSkew(), ((FCDTSkew*) transform)->GetRotateAxis()); CONVERT_VECT3(((FCDTSkew*) transform)->GetSkew(), ((FCDTSkew*) transform)->GetAroundAxis()); }
						else if (transform->HasType(FCDTLookAt::GetClassType())) { CONVERT_VECT3L(((FCDTLookAt*) transform)->GetLookAt(), ((FCDTLookAt*) transform)->GetPosition()); CONVERT_VECT3L(((FCDTLookAt*) transform)->GetLookAt(), ((FCDTLookAt*) transform)->GetTarget()); CONVERT_VECT3(((FCDTLookAt*) transform)->GetLookAt(), ((FCDTLookAt*) transform)->GetUp()); }
						else if (transform->HasType(FCDTMatrix::GetClassType())) { CONVERT_MAT44(((FCDTMatrix*) transform)->GetTransform(), ((FCDTMatrix*) transform)->GetTransform()); }
					}
				}
	
				if (upAxisFunctor.HasConversion())
				{
					// Unfortunately, some of the entity types do not survive very well on up-axis changes.
					// Check for cameras, lights, emitters, force fields
					fm::pvector<FCDEntityInstance> toRemove;
					size_t nodeInstanceCount = node->GetInstanceCount();
					for (size_t n = 0; n < nodeInstanceCount; ++n)
					{
						FCDEntityInstance* nodeInstance = node->GetInstance(n);
						if (nodeInstance->GetEntityType() == FCDEntity::CAMERA || nodeInstance->GetEntityType() == FCDEntity::LIGHT
							|| nodeInstance->GetEntityType() == FCDEntity::FORCE_FIELD || nodeInstance->GetEntityType() == FCDEntity::EMITTER)
						{
							// Targeted entities should still point in the correct direction: do only roll up-axis changes.
							bool rollOnly = false;
							if (handleTargets && nodeInstance->GetEntity() != NULL && nodeInstance->GetEntity()->HasType(FCDTargetedEntity::GetClassType()))
							{
								FCDTargetedEntity* targetedEntity = (FCDTargetedEntity*) nodeInstance->GetEntity();
								rollOnly = targetedEntity->HasTarget(); // Don't pivot fully.
							}

							// Do not create a pivot node unless necessary.
							FCDSceneNode* pivotedNode = node;
							if (node->GetChildrenCount() == 0 && node->GetInstanceCount() == 1)
							{
								pivotedNode = node;
							}
							else
							{
								// Create a pivot node.
								pivotedNode = node->AddChildNode();
								pivotedNode->SetName(node->GetName() + FC("_pivot"));
								pivotedNode->SetDaeId(node->GetDaeId() + "_pivot");

								// Need to clone the instance and remove the original one, then.
								FCDEntityInstance* clone = pivotedNode->AddInstance(nodeInstance->GetEntityType());
								nodeInstance->Clone(clone);
								toRemove.push_back(nodeInstance);
							}

							// Do the 'pivoting'.
							upAxisFunctor.SetPivotTransform(pivotedNode, rollOnly);
						}
					}
					CLEAR_POINTER_VECTOR(toRemove);
				}
	
				ResetAsset(node);
			}
	
			// Iterate over the skin controllers: need to convert the bind-poses.
			size_t controllerCount = document->GetControllerLibrary()->GetEntityCount();
			for (size_t i = 0; i < controllerCount; ++i)
			{
				FCDController* controller = document->GetControllerLibrary()->GetEntity(i);
				if (controller->IsSkin())
				{
					GetAssetFunctors(controller, document->GetControllerLibrary()->GetAsset(false), lengthFunctor, upAxisFunctor);
					if (lengthFunctor.HasConversion() || upAxisFunctor.HasConversion())
					{
						FCDSkinController* skin = controller->GetSkinController();
						FMMatrix44 mx = skin->GetBindShapeTransform();
						CONVERT_MAT44_NONANIMATABLE(mx);
						skin->SetBindShapeTransform(mx);
						size_t jointCount = skin->GetJointCount();
						for (size_t j = 0; j < jointCount; ++j)
						{
							FMMatrix44 bindPose = skin->GetJoint(j)->GetBindPoseInverse().Inverted();
							CONVERT_MAT44_NONANIMATABLE(bindPose);
							skin->GetJoint(j)->SetBindPoseInverse(bindPose.Inverted());
						}
					}
					ResetAsset(controller);
				}
			}
	
			// Iterate over the geometries. Depending on the type, convert the control points, the normals and all other 3D data.
			size_t geometryCount = document->GetGeometryLibrary()->GetEntityCount();
			for (size_t i = 0; i < geometryCount; ++i)
			{
				FCDGeometry* geometry = document->GetGeometryLibrary()->GetEntity(i);
				GetAssetFunctors(geometry, document->GetGeometryLibrary()->GetAsset(false), lengthFunctor, upAxisFunctor);
				if (lengthFunctor.HasConversion() || upAxisFunctor.HasConversion())
				{
					if (geometry->IsMesh())
					{
						// Iterate over the sources. Convert the source data depending on the stride and the type.
						FCDGeometryMesh* mesh = geometry->GetMesh();
						size_t sourceCount = mesh->GetSourceCount();
						for (size_t s = 0; s < sourceCount; ++s)
						{
							FCDGeometrySource* source = mesh->GetSource(s);
							uint32 stride = source->GetStride();
							size_t count = source->GetValueCount();
							if (count == 0) continue;
		
							float* ptr = source->GetData();
							switch (source->GetType())
							{
							case FUDaeGeometryInput::POSITION:
								if (upAxisFunctor.HasConversion() && stride >= 3)
								{
									for (uint32 j = 0; j < count; ++j)
									{
										FMVector3& v = *(FMVector3*) (ptr + j * stride);
										upAxisFunctor(v);
										if (source->GetSourceData().IsAnimated(stride * j) || source->GetSourceData().IsAnimated(stride * j + 1) || source->GetSourceData().IsAnimated(stride * j + 2))
										{
											ConvertAnimationVector3(source->GetSourceData().GetAnimated(stride * j), source->GetSourceData().GetAnimated(stride * j + 1), source->GetSourceData().GetAnimated(stride * j + 2), v, document, lengthFunctor, upAxisFunctor, false);
										}
									}
								}
								if (lengthFunctor.HasConversion())
								{
									for (uint32 j = 0; j < count * stride; ++j)
									{
										lengthFunctor(ptr[j]);
										if (source->GetSourceData().IsAnimated(j))
										{
											ConvertAnimationFloat(source->GetSourceData().GetAnimated(j), ptr[j], document, lengthFunctor, upAxisFunctor);
										}
									}
								}
								break;
		
							case FUDaeGeometryInput::NORMAL:
							case FUDaeGeometryInput::GEOTANGENT:
							case FUDaeGeometryInput::GEOBINORMAL:
							case FUDaeGeometryInput::TEXTANGENT:
							case FUDaeGeometryInput::TEXBINORMAL:
								if (upAxisFunctor.HasConversion() && stride >= 3)
								{
									for (uint32 j = 0; j < count; ++j)
									{
										FMVector3& v = *(FMVector3*) (ptr + j * stride);
										upAxisFunctor(v);
										if (source->GetSourceData().IsAnimated(j) || source->GetSourceData().IsAnimated(j + 1) || source->GetSourceData().IsAnimated(j + 2))
										{
											ConvertAnimationVector3(source->GetSourceData().GetAnimated(j), source->GetSourceData().GetAnimated(j+1), source->GetSourceData().GetAnimated(j+2), v, document, lengthFunctor, upAxisFunctor, false);
										}
									}
								}
								break;
							case FUDaeGeometryInput::TEXCOORD:
							case FUDaeGeometryInput::UV:
							case FUDaeGeometryInput::EXTRA:
							case FUDaeGeometryInput::UNKNOWN:
							case FUDaeGeometryInput::COLOR:
							case FUDaeGeometryInput::POINT_SIZE:
							case FUDaeGeometryInput::POINT_ROTATION:
							case FUDaeGeometryInput::VERTEX:
							default:
								break; // No work to do on these data types.
							}
						}
					}
					else if (geometry->IsSpline())
					{
						FCDGeometrySpline* spline = geometry->GetSpline();
						size_t elementCount = spline->GetSplineCount();
						for (size_t j = 0; j < elementCount; ++j)
						{
							FCDSpline* element = spline->GetSpline(j);
							for (FMVector3List::iterator it2 = element->GetCVs().begin(); it2 != element->GetCVs().end(); ++it2)
							{
								// 3D positions
								upAxisFunctor(*it2); lengthFunctor(*it2);
							}
						}
					}
				}
	
				ResetAsset(geometry);
			}

			// Iterate over the cameras: need to convert the far/near clip planes.
			size_t cameraCount = document->GetCameraLibrary()->GetEntityCount();
			for (size_t i = 0; i < cameraCount; ++i)
			{
				FCDCamera* camera = document->GetCameraLibrary()->GetEntity(i);
				GetAssetFunctors(camera, document->GetCameraLibrary()->GetAsset(false), lengthFunctor, upAxisFunctor);
				if (lengthFunctor.HasConversion())
				{
					CONVERT_FL(camera->GetFarZ(), camera->GetFarZ());
					CONVERT_FL(camera->GetNearZ(), camera->GetNearZ());
				}
				ResetAsset(camera);
			}

			// Iterate over the animations resetting their assets since its been taken care of.
			FCDAnimationLibrary* library = document->GetAnimationLibrary();
			size_t animationCount = library->GetEntityCount();
			for (size_t i = 0; i < animationCount; ++i)
			{
				ResetAsset(library->GetEntity(i));
			}

			document->GetAsset()->SetUnitConversionFactor(unitInMeters);
			document->GetAsset()->SetUpAxis(_upAxis);
		}
	}

#undef CONVERT_FL
#undef CONVERT_VECT3
#undef CONVERT_SCALE
#undef CONVERT_VECT3L
#undef CONVERT_MAT44
				
};
