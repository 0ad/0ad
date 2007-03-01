/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDEffectProfile.h
	This file contains the FCDEffectProfile abstract class.
*/

#ifndef _FCD_EFFECT_PROFILE_H_
#define _FCD_EFFECT_PROFILE_H_

#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_
#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDocument;
class FCDEffect;
class FCDEffectParameterList;

/**
	The base for a COLLADA effect profile.

	COLLADA has multiple effect profiles: CG, HLSL, GLSL, GLES and the COMMON profile.
	For each profile, there is a class which implements this abstract class.
	This abstract class solely holds the parent effect and allows access to the
	profile type.

	@see FCDEffectProfileFX FCDEffectStandard
	
	@ingroup FCDEffect
*/
class FCOLLADA_EXPORT FCDEffectProfile : public FCDObject
{
private:
	DeclareObjectType(FCDObject);
	FCDEffect* parent;
	FCDEffectParameterList* parameters;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffect::AddProfile function.
		@param parent The effect which contains this profile. */
	FCDEffectProfile(FCDEffect* parent);

	/** Destructor. */
	virtual ~FCDEffectProfile();

	/** Retrieves the profile type for this effect.
		This function allows you to up-cast the pointer safely to a more specific
		effect profile class.
		@return The profile type. */
	virtual FUDaeProfileType::Type GetType() const = 0;

	/** Retrieves the parent effect.
		This is the effect which contains this profile.
		@return The parent effect. This pointer will never be NULL. */
	FCDEffect* GetParent() { return parent; }
	const FCDEffect* GetParent() const { return parent; } /**< See above. */

	/** [INTERNAL] Retrieves the COLLADA id of the parent effect.
		This function is useful when reporting errors and warnings.
		@return The COLLADA id of the parent effect. */
	const fm::string& GetDaeId() const;

	/** Retrieves the list of effect parameters contained within the effect profile.
		At this level of abstraction, there should be only effect parameter generators.
		@return The list of effect parameters. */
	FCDEffectParameterList* GetParameters() { return parameters; }
	const FCDEffectParameterList* GetParameters() const { return parameters; } /**< See above. */

	/** [INTERNAL] Inserts an existing parameter into the list of effect parameters
		at this abstraction level. This function is used during the flattening of a material.
		@param parameter The effect parameter to insert. */
	void AddParameter(FCDEffectParameter* parameter);

	/** Retrieves an effect parameter.
		Looks for the effect parameter with the correct reference.
		@param reference The effect parameter reference to match.
		@return The first effect parameter that matches the given reference. */
	virtual const FCDEffectParameter* FindParameter(const char* reference) const;
	inline const FCDEffectParameter* FindParameterByReference(const char* reference) const { return FindParameter(reference); } /**< See above. */
	inline const FCDEffectParameter* FindParameterByReference(const fm::string& reference) const { return FindParameter(reference.c_str()); } /**< See above. */
	inline FCDEffectParameter* FindParameterByReference(const char* reference) { return const_cast<FCDEffectParameter*>(const_cast<const FCDEffectProfile*>(this)->FindParameter(reference)); } /**< See above. */
	inline FCDEffectParameter* FindParameterByReference(const fm::string& reference) { return const_cast<FCDEffectParameter*>(const_cast<const FCDEffectProfile*>(this)->FindParameter(reference.c_str())); } /**< See above. */

	/** Retrieves an effect parameter.
		Looks for the effect parameter with the correct semantic, in order to bind or override its value.
		This function searches through the effect profile and the level of abstractions below.
		@param semantic The effect parameter semantic to match.
		@return The first effect parameter that matches the semantic.
			This pointer will be NULL if no effect parameter matches the given semantic. */
	virtual FCDEffectParameter* FindParameterBySemantic(const fm::string& semantic);

	/** Retrieves a subset of the effect parameter list.
		Look for the effect parameter generators with the correct semantic.
		This function searches through the effect profile and the level of abstractions below.
		@param semantic The effect parameter semantic to match.
		@param parameters The list of parameters to fill in. This list is not cleared. */
	virtual void FindParametersBySemantic(const fm::string& semantic, FCDEffectParameterList& parameters);

	/** Retrieves a subset of the effect parameter list.
		Look for the effect parameter generators with the correct reference.
		This function searches through the effect profile and the level of abstractions below.
		@param reference The effect parameter reference to match. In the case of effect
			parameter generators, the reference is replaced by the sub-id.
		@param parameters The list of parameters to fill in. This list is not cleared. */
	virtual void FindParametersByReference(const fm::string& reference, FCDEffectParameterList& parameters);

	/** Clones the profile effect and its parameters.
		@param clone The cloned profile.
			If this pointer is NULL, a new profile is created and
			you will need to release this new profile.
		@return The cloned profile. This pointer will be NULL if the
			abstract class' cloning function is used without a given clone. */
	virtual FCDEffectProfile* Clone(FCDEffectProfile* clone = NULL) const;

	/** [INTERNAL] Flattens this effect profile, pushing all the effect parameter overrides
		into the effect parameter generators and moving all the parameters to the 
		effect technique level of abstraction. To flatten the material, use the
		FCDMaterialInstance::FlattenMaterial function. */
	virtual void Flatten() = 0;

	/** [INTERNAL] Reads in the effect profile from a given COLLADA XML tree node.
		@param profileNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the effect profile.*/
	virtual bool LoadFromXML(xmlNode* profileNode);

	/** [INTERNAL] Writes out the effect profile to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the effect profile.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Links the effect profile and its parameters. This is done after
		the whole COLLADA document has been processed once. */
	virtual void Link();
};

#endif // _FCD_EFFECT_PROFILE_H_
