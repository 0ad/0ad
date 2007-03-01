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
	@file FCDEffectStandard.h
	This file contains the FCDEffectStandard class.
*/

#ifndef _FCD_MATERIAL_STANDARD_H_
#define _FCD_MATERIAL_STANDARD_H_

#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_
#ifndef _FCD_EFFECT_PROFILE_H_
#include "FCDocument/FCDEffectProfile.h"
#endif // _FCD_EFFECT_PROFILE_H_

class FCDocument;
class FCDEffect;
class FCDEffectParameter;
class FCDTexture;
class FCDEffectParameterList;

typedef fm::pvector<FCDTexture> FCDTextureList; /**< A dynamically-sized array of texture objects. */
typedef FUObjectList<FCDTexture> FCDTextureTrackList; /**< A dynamically-sized array of tracked texture objects. */
typedef FUObjectContainer<FCDTexture> FCDTextureContainer; /**< A dynamically-sized containment array for texture objects. */

/**
	A COMMON profile effect description.
	
	The COMMON effect profile holds the information necessary
	to render your polygon sets using the well-defined lighting models.

	COLLADA supports four lighting models: constant, Lambert, Phong and Blinn.

	@ingroup FCDEffect
*/
class FCOLLADA_EXPORT FCDEffectStandard : public FCDEffectProfile
{
public:
	/** The list of the lighting models supported by the COMMON profile of COLLADA. */
	enum LightingType
	{
		/** The constant lighting model.
			This lighting model uses the emissive color everywhere, without
			any complex lighting calculations. It also uses the translucency
			factor and the translucency color, by multiplying them together
			and applying them to your standard alpha channel according to the
			final lighting color.*/
		CONSTANT, 

		/** The Lambert lighting model.
			This lighting model improves on the constant lighting model by
			using the dot-product between the normalized light vectors and the
			polygon normals to determine how much light should affect each polygon.
			This value is multiplied to the diffuse color and (1 + the ambient color). */
		LAMBERT,

		/** The Phong lighting model.
			This lighting model improves on the Lambert lighting model by
			calculating how much light is reflected by the polygons into the viewer's eye.
			For this calculation, the shininess, the specular color and the reflectivity is used. */
		PHONG,

		/** The Blinn lighting model.
			This lighting model improves on the Lambert lighting model by
			calculating how much light is reflected by the polygons into the viewer's eye.
			For this calculation, the shininess, the specular color and the reflectivity is used. */
		BLINN,

		/** Not a valid lighting model. */
		UNKNOWN
	};

	/** The list of transparency modes supported by the COMMON profile of COLLADA. */
	enum TransparencyMode
	{
		/** Takes the transparency information from the color's alpha channel, where the
			value 1.0 is opaque. */
		A_ONE,

		/** Takes the transparency information from the color's red, green, and blue channels,
			where the value 0.0 is opaque, with each channel modulated independently. */
		RGB_ZERO
	};

private:
	DeclareObjectType(FCDEffectProfile);
	LightingType type;
	FCDTextureContainer* textureBuckets;

	// Common material parameter: Emission
	FMVector4 emissionColor;
	float emissionFactor; // Max-specific
	bool isEmissionFactor; // Max-specific
	
	// Common material parameter: Reflectivity
	bool isReflective; // Maya-specific (bug #218 - 2006-11-16)
	FMVector4 reflectivityColor;
	float reflectivityFactor;
	float indexOfRefraction;

	// Common material parameter: Translucency
	FMVector4 translucencyColor;
	float translucencyFactor;
	TransparencyMode transparencyMode;

	// Lambert material parameters
	FMVector4 diffuseColor;
	FMVector4 ambientColor;

	// Phong material parameters: Specular
	FMVector4 specularColor;
	float specularFactor; // Max-specific
	float shininess;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffect::AddProfile function
		with the FUDaeProfileType::COMMON parameter.
		@param parent The effect that contains this profile. */
	FCDEffectStandard(FCDEffect* parent);

	/** Destructor. */
	virtual ~FCDEffectStandard();

	/** Retrieves the lighting model to be used for this profile.
		@return The lighting model. */
	inline LightingType GetLightingType() const { return type; }

	/** Sets the lighting model to be used for this profile.
		Note that which parameters are exported depends on the lighting model.
		@param _type The lighting model. */
	inline void SetLightingType(LightingType _type) { type = _type; SetDirtyFlag(); }

	/** Retrieves the profile type for this effect.
		This function is a part of the FCDEffectProfile interface and allows you
		to up-cast an effect profile pointer safely to this class.
		@return The profile type: COMMON. */
	virtual FUDaeProfileType::Type GetType() const { return FUDaeProfileType::COMMON; }

	/** Retrieves the list of textures belonging to a specific channel.
		@param bucket A texture channel index. This index should match one
			of the values in the FUDaeTextureChannel enum.
		@return The list of textures for this channel. */
	FCDTextureContainer& GetTextureBucket(uint32 bucket);
	const FCDTextureContainer& GetTextureBucket(uint32 bucket) const;

	/** Retrieves the number of textures belonging to a specific channel.
		@param bucket A texture channel index. This index should match one
			of the values in the FUDaeTextureChannel enum.
		@return The number of textures in that channel. */
	size_t GetTextureCount(uint32 bucket) const { FUAssert(bucket < FUDaeTextureChannel::COUNT, return 0); return textureBuckets[bucket].size(); }

	/** Retrieves a texture
		@param bucket A texture channel index. This index should match one
			of the values in the FUDaeTextureChannel enum.
		@param index The index of a texture within this channel.
		@return The texture. This pointer will be NULL if either the bucket or the index is out-of-bounds. */
	inline FCDTexture* GetTexture(uint32 bucket, size_t index) { FUAssert(index < GetTextureCount(bucket), return NULL); return textureBuckets[bucket].at(index); }
	inline const FCDTexture* GetTexture(uint32 bucket, size_t index) const { FUAssert(index < GetTextureCount(bucket), return NULL); return textureBuckets[bucket].at(index); } /**< See above. */

	/** Adds a texture to a specific channel.
		@param bucket A texture channel index. This index should match one
			of the values in the FUDaeTextureChannel enum.
		@return The new texture. This pointer will be NULL if the bucket is out-of-bounds. */
	FCDTexture* AddTexture(uint32 bucket);

	/** Releases a texture contained within this effect profile.
		@param texture The texture to release. */
	void ReleaseTexture(FCDTexture* texture);

	/** Retrieves the base translucency color.
		This value must be multiplied with the translucency factor
		to get the real translucency color. Use the RGB channels
		in case of a RGB_ZERO transparency mode, and the A channel in case
		of a A_ONE transparency mode
		This value is used in all lighting models.
		@return The base translucency color. */
	inline FMVector4& GetTranslucencyColor() { return translucencyColor; }
	inline const FMVector4& GetTranslucencyColor() const { return translucencyColor; }

	/** Sets the base translucency color.
		@param color The base translucency color including the alpha channel. */
	inline void SetTranslucencyColor(const FMVector4& color) { translucencyColor = color; SetDirtyFlag(); }

	/** Retrieves the translucency factor.
		This value must be multiplied with the translucency color
		to get the real translucency color.
		This value is used in all lighting models.
		@return The translucency factor. */
	inline float& GetTranslucencyFactor() { return translucencyFactor; }
	inline const float& GetTranslucencyFactor() const { return translucencyFactor; } /**< See above. */

	/** Sets the translucency factor.
		@param factor The translucency factor. */
	inline void SetTranslucencyFactor(float factor) { translucencyFactor = factor; SetDirtyFlag(); }

	/** Retrieves the transparency mode.
		@return The transparency mode. */
	inline TransparencyMode GetTransparencyMode() const {return transparencyMode; }

	/** Sets the transparency mode.
		@param mode The transparency mode. */
	inline void SetTransparencyMode(TransparencyMode mode) {transparencyMode = mode; SetDirtyFlag(); }

	/** Retrieves the flat opacity.
		This is a calculated value and will not take into consideration any animations
		that affect either the base translucency color or the translucency factor.
		This value can be used in all lighting models.
		@return The flat opacity. */
	float GetOpacity() const;

	/** Retrieves the base emission/self-illumination color.
		This value must be multiplied with the emission factor to get the real emission color.
		This value is used in all lighting models.
		@return The base emission color. */
	inline FMVector4& GetEmissionColor() { return emissionColor; }
	inline const FMVector4& GetEmissionColor() const { return emissionColor; } /**< See above. */

	/** Sets the base emission/self-illumination color.
		@param color The base emission color. */
	inline void SetEmissionColor(const FMVector4& color) { emissionColor = color; SetDirtyFlag(); }

    /** Retrieves the emission/self-illumination factor.
		This value must be multiplied with the base emission color to get the real emission color.
		@return The emission factor. */
	inline float& GetEmissionFactor() { return emissionFactor; }
	inline const float& GetEmissionFactor() const { return emissionFactor; } /**< See above. */

	/** Sets the emission/self-illumination factor.
		@param factor The emission factor. */
	inline void SetEmissionFactor(float factor) { emissionFactor = factor; SetDirtyFlag(); }

	/** Retrieves whether the emission factor was used, rather than the emission color.
		This value is used in conjunction with 3dsMax, in which the self-illumination color
		and the self-illumination factor are mutually exclusive.
		@return Whether the emission factor is to be used. */
	inline bool IsEmissionFactor() const { return isEmissionFactor; }

	/** Sets whether the emission factor is to be used, rather than the emission color.
		This value is used in conjunction with 3dsMax, in which the self-illumination color
		and the self-illumination factor are mutually exclusive.
		@param useFactor Whether the emission factor should be used. */
	inline void SetIsEmissionFactor(bool useFactor) { isEmissionFactor = useFactor; SetDirtyFlag(); }

	/** Retrieves the diffuse color.
		This value is used in the Lambert lighting model.
		@return The diffuse color. */
	inline FMVector4& GetDiffuseColor() { return diffuseColor; }
	inline const FMVector4& GetDiffuseColor() const { return diffuseColor; } /**< See above. */

	/** Sets the diffuse color.
		@param color The diffuse color. */
	inline void SetDiffuseColor(const FMVector4& color) { diffuseColor = color; SetDirtyFlag(); }

	/** Retrieves the ambient color.
		This value is used in the Lambert lighting model.
		@return The ambient color. */
	inline FMVector4& GetAmbientColor() { return ambientColor; }
	inline const FMVector4& GetAmbientColor() const { return ambientColor; } /**< See above. */

	/** Sets the ambient color.
		@param color The ambient color. */
	inline void SetAmbientColor(const FMVector4& color) { ambientColor = color; SetDirtyFlag(); }

	/** Retrieves the base specular color.
		This value must be multiplied with the specular factor
		to get the real specular color.
		This value is used in the Phong and Blinn lighting models.
		@return The specular color. */
	inline FMVector4& GetSpecularColor() { return specularColor; }
	inline const FMVector4& GetSpecularColor() const { return specularColor; } /**< See above. */

	/** Sets the specular color.
		@param color The specular color. */
	inline void SetSpecularColor(const FMVector4& color) { specularColor = color; SetDirtyFlag(); }

	/** Retrieves the specular factor.
		This value must be multiplied with the base specular color
		to get the real specular color.
		This value is used in the Phong and Blinn lighting models.
		@return The specular factor. */
	inline float& GetSpecularFactor() { return specularFactor; }
	inline const float& GetSpecularFactor() const { return specularFactor; } /**< See above. */

	/** Sets the specular factor.
		@param factor The specular factor. */
	inline void SetSpecularFactor(float factor) { specularFactor = factor; SetDirtyFlag(); }

	/** Retrieves the specular shininess.
		This value represents the exponent to which you must raise
		the dot-product between the view vector and reflected light vectors:
		as such, it is usually a number greater than 1.
		This value is used in the Phong and Blinn lighting models.
		@return The specular shininess. */
	inline float& GetShininess() { return shininess; }
	inline const float& GetShininess() const { return shininess; } /**< See above. */

	/** Sets the specular shininess.
		This value represents the exponent to which you must raise
		the dot-product between the view vector and reflected light vectors:
		as such, it is usually a number greater than 1.
		@param _shininess The specular shininess. */
	inline void SetShininess(float _shininess) { shininess = _shininess; SetDirtyFlag(); }

	/**	Retrieves the reflectivity state.
		@return True if the effect has reflectivity information.*/
	inline bool IsReflective() const { return isReflective; }

	/**	Sets the reflectivity state.
		@param r The effect's new reflectivity state.*/
	inline void SetReflective(bool r) { isReflective = r; }

	/** Retrieves the base reflectivity color.
		This value must be multiplied to the reflectivity factor to
		get the real reflectivity color.
		This value is used in the Phong and Blinn lighting models.
		@return The base reflectivity color. */
    inline FMVector4& GetReflectivityColor() { return reflectivityColor; }
    inline const FMVector4& GetReflectivityColor() const { return reflectivityColor; } /**< See above. */

	/** Sets the base reflectivity color.
		@param color The base reflectivity color. */
	inline void SetReflectivityColor(const FMVector4& color) { reflectivityColor = color; SetDirtyFlag(); }
	
	/** Retrieves the reflectivity factor.
		This value must be multiplied to the base reflectivity color
		to get the real reflectivity color.
		This value is used in the Phong and Blinn lighting models.
		@return The reflectivity factor. */
	inline float& GetReflectivityFactor() { return reflectivityFactor; }
	inline const float& GetReflectivityFactor() const { return reflectivityFactor; } /**< See above. */

	/** Sets the reflectivity factor.
		@param factor The reflectivity factor. */
	inline void SetReflectivityFactor(float factor) { reflectivityFactor = factor; SetDirtyFlag(); }

	/** Retrieves the flat reflectivity.
		This is a calculated value and will not take into consideration any animations
		that affect either the base reflectivity color or the reflectivity factor.
		This value can be used in the Phong and Blinn lighting models.
		@return The flat reflectivity. */
	float GetReflectivity() const;

	/** Retrieves the index of refraction.
		The index of refraction defaults to 1.0f.
		@return The index of refraction. */
	inline float& GetIndexOfRefraction() { return indexOfRefraction; }
	inline const float& GetIndexOfRefraction() const { return indexOfRefraction; } /**< See above. */

	/** Sets the index of refraction.
		@param index The new index of refraction. */
	inline void SetIndexOfRefraction(float index) { indexOfRefraction = index; SetDirtyFlag(); }

	/** Retrieves an effect parameter.
		Looks for the effect parameter with the correct semantic, in order to bind or override its value.
		This function searches through the effect profile and the level of abstractions below.
		@param semantic The effect parameter semantic to match.
		@return The effect parameter that matches the semantic. This pointer may be
			NULL if no effect parameter matches the given semantic. */
	virtual FCDEffectParameter* FindParameterBySemantic(const fm::string& semantic);

	/** Retrieves a subset of the effect parameter list.
		Look for effect parameters with the correct semantic.
		This function searches through the effect profile and the level of abstractions below.
		@param semantic The effect parameter semantic to match.
		@param parameters The list of parameters to fill in. This list is not cleared. */
	virtual void FindParametersBySemantic(const fm::string& semantic, FCDEffectParameterList& parameters);

	/** Retrieves a subset of the effect parameter list.
		Look for effect parameters with the correct reference.
		This function searches through the effect profile and the level of abstractions below.
		@param reference The effect parameter reference to match. In the case of effect
			parameter, the reference is replaced by the sub-id.
		@param parameters The list of parameters to fill in. This list is not cleared. */
	virtual void FindParametersByReference(const fm::string& reference, FCDEffectParameterList& parameters);

	/** Clones the COMMON profile effect and its parameters.
		@param clone The cloned profile.
			If this pointer is NULL, a new COMMON profile is created and
			you will need to release this pointer.
		@return The cloned COMMON profile. */
	virtual FCDEffectProfile* Clone(FCDEffectProfile* clone = NULL) const;

	/** [INTERNAL] Flattens the profile.
		Does nothing on the common profile. */
	virtual void Flatten() {}

	/** [INTERNAL] Reads in the \<profile_COMMON\> element from a given COLLADA XML tree node.
		@param baseNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the effect profile.*/
	virtual bool LoadFromXML(xmlNode* baseNode);

	/** [INTERNAL] Writes out the \<profile_COMMON\> element to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the effect profile.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Links the effect profile and its parameters. This is done after
		the whole COLLADA document has been processed once. */
	virtual void Link();

	/** Adds an extra attribute to the given profile
		@param profile The profile in which to insert the attribute.
		@param key The attribute's key
		@param value The attribute's value */
	void AddExtraAttribute( const char *profile, const char *key, const fchar *value );

	/** Get the extra attribute value corresponding to the given profile and key values
		@param profile The profile to look for.
		@param key The attribute key to look for. */
	const fchar* GetExtraAttribute( const char *profile, const char *key ) const;

private:
	xmlNode* WriteColorTextureParameterToXML(xmlNode* parentNode, const char* parameterNodeName, const FMVector4& value, const FCDTextureTrackList& textureBucket) const;
	xmlNode* WriteFloatTextureParameterToXML(xmlNode* parentNode, const char* parameterNodeName, const float& value, const FCDTextureTrackList& textureBucket) const;
	xmlNode* WriteTextureParameterToXML(xmlNode* parentNode, const FCDTextureTrackList& textureBucket) const;

	bool ParseColorTextureParameter(xmlNode* parameterNode, FMVector4& value, FCDTextureContainer& textureBucket);
	bool ParseFloatTextureParameter(xmlNode* parameterNode, float& value, FCDTextureContainer& textureBucket);
	bool ParseSimpleTextureParameter(xmlNode* parameterNode, FCDTextureContainer& textureBucket);

};

#endif //_FCD_MATERIAL_STANDARD_H_

