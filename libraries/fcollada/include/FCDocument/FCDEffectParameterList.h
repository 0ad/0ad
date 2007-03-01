/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDEffectParameterList.h
	This file contains the FCDEffectParameterList class.
*/

#ifndef _FCD_EFFECT_PARAMETER_LIST_H_
#define _FCD_EFFECT_PARAMETER_LIST_H_

class FCDEffectParameter;
typedef FUObjectList<FCDEffectParameter> FCDEffectParameterTrackList; /**< A dynamically-allocated list of tracked effect parameters. */

/**
	A searchable list of COLLADA effect parameters.

	This class is based on the STL vector class and adds some
	useful search methods: by reference and by semantic.

	@ingroup FCDEffect
*/
class FCOLLADA_EXPORT FCDEffectParameterList : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	FCDEffectParameterTrackList parameters;
	bool ownParameters;

public:
	/** Constructor.
		All the objects that need a parameter list will create it when necessary.
		You may also create new lists for the retrieval of parameters during a search.
		@param document The COLLADA document that owns this parameter list. This pointer
			can remain NULL unless you expect to create new parameters within this list.
		@param ownParameters Whether this list should release the contained parameters
			during its destruction. */
	FCDEffectParameterList(FCDocument* document = NULL, bool ownParameters = false);

	/** Destructor. */
	virtual ~FCDEffectParameterList();

	/** Retrieves the actual list of effect parameters.
		@return The list of effect parameters. */
	inline FCDEffectParameterTrackList& GetParameters() { return parameters; }
	inline const FCDEffectParameterTrackList& GetParameters() const { return parameters; } /**< See above. */

	/** Retrieves the number of effect parameters within the list.
		@return The number of effect parameters within the list. */
	inline size_t GetParameterCount() const { return parameters.size(); }

	/** Creates a new parameters within this list.
		@param type The effect parameter type.
		@return The new effect parameter. This pointer will be NULL if this list does not own its parameters. */
	FCDEffectParameter* AddParameter(uint32 type);

	/** Adds one parameter to the list.
		@param parameter A parameter. */
	inline void AddParameter(FCDEffectParameter* parameter) { parameters.push_back(parameter); }

	/** Retrieves an iterator to the start of the parameter list.
		@return An iterator to the start of the parameter list. */
	inline FCDEffectParameterTrackList::iterator begin() { return parameters.begin(); }
	inline FCDEffectParameterTrackList::const_iterator begin() const { return parameters.begin(); }

	/** Retrieves an iterator to the end of the parameter list.
		@return An iterator to the end of the parameter list. */
	inline FCDEffectParameterTrackList::iterator end() { return parameters.end(); }
	inline FCDEffectParameterTrackList::const_iterator end() const { return parameters.end(); }

	/** Retrieves the first effect parameter with the given reference.
		For effect parameter generators, the sub-id is used instead of the reference.
		@param reference A reference to match.
		@return The effect parameter that matches the reference. This pointer will be NULL,
			if no parameter matches the reference. */
	inline FCDEffectParameter* FindReference(const char* reference) { return const_cast<FCDEffectParameter*>(const_cast<const FCDEffectParameterList*>(this)->FindReference(reference)); }
	const FCDEffectParameter* FindReference(const char* reference) const; /**< See above. */
	inline FCDEffectParameter* FindReference(const fm::string& reference) { return FindReference(reference.c_str()); } /**< See above. */
	inline const FCDEffectParameter* FindReference(const fm::string& reference) const { return FindReference(reference.c_str()); } /**< See above. */

	/** Retrieves the first effect parameter with the given semantic.
		@param semantic A semantic to match.
		@return The effect parameter that matches the semantic. This pointer will be NULL
			if no parameter matches the semantic. */
	inline FCDEffectParameter* FindSemantic(const char* semantic) { return const_cast<FCDEffectParameter*>(const_cast<const FCDEffectParameterList*>(this)->FindSemantic(semantic)); }
	const FCDEffectParameter* FindSemantic(const char* semantic) const; /**< See above. */
	inline FCDEffectParameter* FindSemantic(const fm::string& semantic) { return FindReference(semantic.c_str()); } /**< See above. */
	inline const FCDEffectParameter* FindSemantic(const fm::string& semantic) const { return FindReference(semantic.c_str()); } /**< See above. */

	/** Retrieves a subset of this parameter list.
		All the effect parameters that match the given reference will be added to the given list.
		For effect parameter generators, the sub-id is used instead of the reference.
		@param reference A reference to match.
		@param list The effect parameter list to fill in with the matched parameters.
			This list is not cleared. */
	void FindReference(const char* reference, FCDEffectParameterList& list);
	inline void FindReference(const fm::string& reference, FCDEffectParameterList& list) { return FindReference(reference.c_str(), list); } /**< See above. */

	/** Retrieves a subset of this parameter list.
		All the effect parameters that match the given semantic will be added to the given list.
		@param semantic A semantic to match.
		@param list The effect parameter list to fill in with the matched parameters.
			This list is not cleared. */
	void FindSemantic(const char* semantic, FCDEffectParameterList& list);
	inline void FindSemantic(const fm::string& semantic, FCDEffectParameterList& list) { return FindReference(semantic.c_str(), list); } /**< See above. */

	/** Retrieves a subset of this parameter list.
		All the effect parameters that match the given parameter type will be added to the given list.
		@param type The parameter type to match.
		@param list The effect parameter list to fill in with the matched parameters.
			This list is not cleared. */
	void FindType(uint32 type, FCDEffectParameterList& list) const;

	/** Creates a full copy of the list of parameters and its content.
		@param clone The cloned parameter list. If this pointer is NULL,
			a new parameter list is created and you will need to
			release this new list.
		@return The cloned parameter list.*/
	FCDEffectParameterList* Clone(FCDEffectParameterList* clone) const;
};

#endif // _FCD_EFFECT_PARAMETER_LIST_H_
