/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDEffectProfileFX.h
	This file declares the FCDEffectProfileFX class.
*/

#ifndef _FCD_EFFECT_PROFILE_FX_H_
#define _FCD_EFFECT_PROFILE_FX_H_

#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_
#ifndef _FCD_EFFECT_PROFILE_H_
#include "FCDocument/FCDEffectProfile.h"
#endif // _FCD_EFFECT_PROFILE_H_

class FCDocument;
class FCDEffect;
class FCDEffectCode;
class FCDEffectParameter;
class FCDEffectParameterSurface;
class FCDEffectTechnique;
class FCDEffectParameterList;

typedef fm::pvector<FCDEffectTechnique> FCDEffectTechniqueList; /**< A dynamically-sized array of effect techniques. */
typedef fm::pvector<FCDEffectCode> FCDEffectCodeList; /**< A dynamically-sized array of effect code inclusion. */
typedef FUObjectContainer<FCDEffectTechnique> FCDEffectTechniqueContainer; /**< A dynamically-sized containment array for effect techniques. */
typedef FUObjectContainer<FCDEffectCode> FCDEffectCodeContainer; /**< A dynamically-sized containment array for effect code inclusion. */

/**
	A general effect profile description.

	The general effect profile contains all the information necessary
	to implement the advanced effect profiles, such as CG, HLSL, GLSL and GLES.
	Since these effect profiles contains extremely similar information, they
	use the same description structure. For the COMMON profile,
	see the FCDEffectStandard class.

	You should use the GetType function to figure out which profile this structure
	addresses. You can then retrieve one or many of the FCDEffectTechnique objects
	that describe how to render for this profile. You may want to check the
	FCDEffectMaterialTechniqueHint objects at the FCDMaterial level, in order to
	determine which technique(s) to use for your platform. At the profile
	level of abstraction, parameters may be generated within the FCDEffectParamterList.

	@ingroup FCDEffect
*/
class FCOLLADA_EXPORT FCDEffectProfileFX : public FCDEffectProfile
{
private:
	DeclareObjectType(FCDEffectProfile);
	FUDaeProfileType::Type type;
	fstring platform;

	FCDEffectCodeContainer codes;
	FCDEffectTechniqueContainer techniques;

public:
	/** Constructor: do not use directly. Instead, use the FCDEffect::AddProfile function.
		@param parent The effect which contains this profile.
		@param type The type of profile. */
	FCDEffectProfileFX(FCDEffect* parent, FUDaeProfileType::Type type);

	/** Destructor. */
	virtual ~FCDEffectProfileFX();

	/** Retrieves the profile type for this effect.
		This function is a part of the FCDEffectProfile interface and allows you
		to up-cast an effect profile pointer safely to this class.
		@return The profile type. This should never be the value: 'COMMON',
			but all other profiles currently derive from this class. */
	virtual FUDaeProfileType::Type GetType() const { return type; }

	/** Retrieves the name of the platform in which to use the effect profile.
		This parameter is very optional.
		@return The platform name. */
	const fstring& GetPlatform() const { return platform; }

	/** Sets the name of the platform in which to use the effect profile.
		This parameter is very optional.
		@param _platform The platform name. */
	void SetPlatform(const fstring& _platform) { platform = _platform; SetDirtyFlag(); }

	/** Retrieves the list of techniques contained within this effect profile.
		You may want to check the FCDEffectMaterialTechniqueHint objects at the FCDMaterial level,
		in order to determine which technique(s) to use for your platform.
		@return The list of inner techniques. */
	FCDEffectTechniqueContainer& GetTechniqueList() { return techniques; }
	const FCDEffectTechniqueContainer& GetTechniqueList() const { return techniques; } /**< See above. */

	/** Retrieves the number of techniques contained within this effect profile.
		@return The number of inner techniques. */
	size_t GetTechniqueCount() const { return techniques.size(); }

	/** Retrieves a technique contained within this effect profile.
		You may want to check the FCDEffectMaterialTechniqueHint objects at the FCDMaterial level,
		in order to determine which technique(s) to use for your platform.
		@param index The index of the technique.
		@return The inner technique. This pointer will be NULL if the index is out-of-bounds. */
	FCDEffectTechnique* GetTechnique(size_t index) { FUAssert(index < GetTechniqueCount(), return NULL); return techniques.at(index); }
	const FCDEffectTechnique* GetTechnique(size_t index) const { FUAssert(index < GetTechniqueCount(), return NULL); return techniques.at(index); } /**< See above. */

	/** Adds a new technique to this effect profile.
		@return The new technique object. */
	FCDEffectTechnique* AddTechnique();

	/** Retrieves the list of code inclusions.
		@return The list of code inclusions. */		
	FCDEffectCodeContainer& GetCodeList() { return codes; }
	const FCDEffectCodeContainer& GetCodeList() const { return codes; } /**< See above. */

	/** Retrieves the number of code inclusions contained within the effect profile.
		@return The number of code inclusions. */
	size_t GetCodeCount() const { return codes.size(); }

	/** Retrieves a code inclusion contained within the effect profile.
		@param index The index of the code inclusion.
		@return The code inclusion. This pointer will be NULL if the index is out-of-bounds. */
	FCDEffectCode* GetCode(size_t index) { FUAssert(index < GetCodeCount(), return NULL); return codes.at(index); }
	const FCDEffectCode* GetCode(size_t index) const { FUAssert(index < GetCodeCount(), return NULL); return codes.at(index); } /**< See above. */

	/** Retrieves the code inclusion with the given sub-id.
		@param sid A COLLADA sub-id.
		@return The code inclusion with the given sub-id. This pointer will be NULL,
			if there are no code inclusions that match the given sub-id. */
	FCDEffectCode* FindCode(const fm::string& sid);
	const FCDEffectCode* FindCode(const fm::string& sid) const; /**< See above. */

	/** Adds a new code inclusion to this effect profile.
		@return The new code inclusion. */
	FCDEffectCode* AddCode();

	/** Retrieves an effect parameter.
		Looks for the effect parameter with the correct reference.
		@param reference The effect parameter reference to match.
		@return The first effect parameter that matches the given reference. */
	virtual const FCDEffectParameter* FindParameter(const char* reference) const;
	inline const FCDEffectParameter* FindParameterByReference(const char* reference) const { return FindParameter(reference); } /**< See above. */
	inline const FCDEffectParameter* FindParameterByReference(const fm::string& reference) const { return FindParameter(reference.c_str()); } /**< See above. */
	inline FCDEffectParameter* FindParameterByReference(const char* reference) { return const_cast<FCDEffectParameter*>(const_cast<const FCDEffectProfileFX*>(this)->FindParameter(reference)); } /**< See above. */
	inline FCDEffectParameter* FindParameterByReference(const fm::string& reference) { return const_cast<FCDEffectParameter*>(const_cast<const FCDEffectProfileFX*>(this)->FindParameter(reference.c_str())); } /**< See above. */

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

	/** Clones the full effect profile.
		@param clone The cloned profile.
			If this pointer is NULL, a new profile is created and
			you will need to release this new profile.
		@return The cloned profile. This pointer will never be NULL. */
	virtual FCDEffectProfile* Clone(FCDEffectProfile* clone = NULL) const;

	/** [INTERNAL] Flattens this effect profile. Pushes all the effect parameter overrides
		into the effect parameter generators and moves all the parameters to the 
		effect technique level of abstraction. To flatten the material, use the
		FCDMaterialInstance::FlattenMaterial function. */
	virtual void Flatten();

	/** [INTERNAL] Reads in the effect profile from a given COLLADA XML tree node.
		@param profileNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the effect profile.*/
	virtual bool LoadFromXML(xmlNode* profileNode);

	/** [INTERNAL] Writes out the effect profile to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the material declaration.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode)  const;

	/** [INTERNAL] Links the effect profile and its parameters. This is done after
		the whole COLLADA document has been processed once. */
	virtual void Link();
};

#endif // _FCD_EFFECT_PROFILE_H_
