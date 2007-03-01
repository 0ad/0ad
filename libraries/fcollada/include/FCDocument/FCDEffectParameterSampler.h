/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDEffectParameter.h
	This file contains the FCDEffectParameterSampler class.
*/

#ifndef _FCD_EFFECT_PARAMETER_SAMPLER_H_
#define _FCD_EFFECT_PARAMETER_SAMPLER_H_

#ifndef _FCD_EFFECT_PARAMETER_H_
#include "FCDocument/FCDEffectParameter.h"
#endif // _FCD_EFFECT_PARAMETER_H_

class FCDocument;
class FCDEffectPass;
class FCDEffectParameterSurface;

/**
	A COLLADA sampler effect parameter.
	A sampler parameter provides the extra texturing information necessary
	to correctly sample a surface parameter.
	There are four types of samplers supported: 1D, 2D, 3D and cube.

	@ingroup FCDEffect
*/
class FCOLLADA_EXPORT FCDEffectParameterSampler : public FCDEffectParameter
{
public:
	/** The type of sampling to execute. */
	enum SamplerType
	{
		SAMPLER1D, /** 1D sampling. */
		SAMPLER2D, /** 2D sampling. */
		SAMPLER3D, /** 3D sampling. */
		SAMPLERCUBE /** Cube-map sampling. */
	};

private:
	DeclareObjectType(FCDEffectParameter);
	SamplerType samplerType;
	fm::string surfaceSid; // Valid during import only. Necessary for linkage.
	FUObjectPtr<FCDEffectParameterSurface> surface;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterSampler(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterSampler();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: SAMPLER. */
	virtual Type GetType() const { return SAMPLER; }

	/** Retrieves the parameter for the surface to sample.
		@return The surface parameter. This pointer will be NULL if the sampler is
			not yet linked to any surface.. */
	FCDEffectParameterSurface* GetSurface() { return surface; }
	const FCDEffectParameterSurface* GetSurface() const { return surface; } /**< See above. */

	/** Sets the surface parameter for the surface to sample.
		@param surface The surface parameter. This pointer may be NULL
			to unlink the sampler. */
	void SetSurface(FCDEffectParameterSurface* surface);

	/** Retrieves the type of sampling to do.
		@return The sampling type. */
	SamplerType GetSamplerType() const { return samplerType; }

	/** Sets the type of sampling to do.
		@param type The sampling type. */
	void SetSamplerType(SamplerType type) { samplerType = type; SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter );

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Links the sampler with the surface. This is done after
		the whole COLLADA document has been processed once.
		@param parameters The list of parameters available at this abstraction level. */
	virtual void Link(FCDEffectParameterList& parameters);
};

#endif // _FCD_EFFECT_PARAMETER_SAMPLER_H_
