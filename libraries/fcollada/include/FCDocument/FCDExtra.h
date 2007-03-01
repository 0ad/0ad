/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDExtra.h
	This file contains the FCDExtra class and its sub-classes:
	FCDENode, FCDETechnique and FCDEAttribute.
*/

#ifndef _FCD_EXTRA_H_
#define _FCD_EXTRA_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDAnimated;
class FCDAnimatedCustom;
class FCDEAttribute;
class FCDETechnique;
class FCDEType;
class FCDENode;

typedef fm::pvector<FCDEAttribute> FCDEAttributeList; /**< A dynamically-sized array of extra tree attributes. */
typedef fm::pvector<FCDENode> FCDENodeList; /**< A dynamically-sized array of extra tree nodes. */
typedef fm::pvector<FCDETechnique> FCDETechniqueList; /**< A dynamically-sized array of extra tree techniques. */
typedef fm::pvector<FCDEType> FCDETypeList; /**< A dynamically-sized array of typed extra nodes. */
typedef FUObjectContainer<FCDEAttribute> FCDEAttributeContainer; /**< A dynamically-sized containment array for extra tree attributes. */
typedef FUObjectContainer<FCDENode> FCDENodeContainer; /**< A dynamically-sized containment array for extra tree nodes. */
typedef FUObjectContainer<FCDETechnique> FCDETechniqueContainer; /**< A dynamically-sized containment array for extra tree techniques. */
typedef FUObjectContainer<FCDEType> FCDETypeContainer; /**< A dynamically-sized containment array for typed extra nodes. */

/**
	A COLLADA extra tree.

	An extra tree contains the user-defined COLLADA information
	contained within \<extra\> elements. For this, the extra tree
	root simply contains a list of techniques. Each technique
	belongs to a different application-specific profile.
*/
class FCOLLADA_EXPORT FCDExtra : public FCDObject
{
private:
	DeclareObjectType(FCDObject);
	FCDETypeContainer types;

public:
	/** Constructor: do not use directly.
		The structures that contain extra trees will create them.
		@param document The COLLADA document that owns the extra tree. */
	FCDExtra(FCDocument* document);

	/** Destructor. */
	virtual ~FCDExtra();

	/** Retrieves the list of types contained by this extra tree.
		@return The list of types. */
	inline FCDETypeContainer& GetTypes() { return types; }
	inline const FCDETypeContainer& GetTypes() const { return types; } /**< See above. */

	/** Retrieves the number of types contained by this extra tree.
		@return The number of types. */
	size_t GetTypeCount() const { return types.size(); }

	/** Retrieves the default extra type.
		The default extra type has an empty typename and is always created by default.
		The default extra type will NOT be exported if it is empty.
		@return The default extra type. */
	inline FCDEType* GetDefaultType() { return const_cast<FCDEType*>(const_cast<const FCDExtra*>(this)->GetDefaultType()); }
	inline const FCDEType* GetDefaultType() const { return FindType(""); }  /**< See above. */

	/** Retrieves a specific type contained by this extra tree.
		@param index The index of the type.
		@return The type. This pointer will be NULL if the index is out-of-bounds. */
	inline FCDEType* GetType(size_t index) { FUAssert(index < types.size(), return NULL); return types.at(index); }
	inline const FCDEType* GetType(size_t index) const { FUAssert(index < types.size(), return NULL); return types.at(index); } /**< See above. */

	/** Adds a new application-specific type to the extra tree.
		If the given application-specific type already exists
		within the extra tree, the old type will be returned.
		@param name The application-specific name.
		@return A type for this application-specific name. */
	FCDEType* AddType(const char* name);
	inline FCDEType* AddType(const fm::string& name) { return AddType(name.c_str()); } /**< See above. */

	/** Retrieves a specific type contained by this extra tree.
		@param name The application-specific name of the type.
		@return The type that matches the name. This pointer may
			be NULL if no type matches the name. */
	inline FCDEType* FindType(const char* name) { return const_cast<FCDEType*>(const_cast<const FCDExtra*>(this)->FindType(name)); }
	const FCDEType* FindType(const char* name) const; /**< See above. */
	inline FCDEType* FindType(const fm::string& name) { return FindType(name.c_str()); } /**< See above. */
	inline const FCDEType* FindType(const fm::string& name) const { return FindType(name.c_str()); } /**< See above. */

	/** [INTERNAL] Clones the extra tree information.
		@param clone The extra tree that will take in this extra tree's information.
			If this pointer is NULL, a new extra tree will be created and you will
			need to release the returned pointer manually.
		@return The clone. */
	FCDExtra* Clone(FCDExtra* clone = NULL) const;

	/** [INTERNAL] Reads in the extra tree from a given COLLADA XML tree node.
		@param extraNode The COLLADA \<extra\> element XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the entity.*/
	bool LoadFromXML(xmlNode* extraNode);

	/** [INTERNAL] Writes out the extra tree to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the \<extra\> element.
		@return The created element XML tree node. */
	xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Writes out the extra tree techniques to the given COLLADA XML tree node.
		The techniques are written straight into the parent node. This is useful when
		the extra tree represents a technique-switch, rather than an <extra> element.
		@param parentNode The COLLADA XML parent node in which to insert the techniques. */
	void WriteTechniquesToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA typed extra node.

	The 'type' attribute of the extra nodes allow us to bucket techniques
	to allow for different data for the same idea.

	Therefore, a typed extra node contains a type name and a list of techniques.
*/
class FCOLLADA_EXPORT FCDEType : public FCDObject
{
private:
	DeclareObjectType(FCDObject);
	fm::string name;
	FCDETechniqueContainer techniques;

public:
	/** Constructor: do not use directly.
		Use the FCDExtra::AddType function instead.
		@param document The COLLADA document that owns the extra tree.
		@param type The name of the type for this typed extra. */
	FCDEType(FCDocument* document, const char* type);

	/** Destructor. */
	virtual ~FCDEType();

	/** Retrieves the name of the type of the typed extra.
		@return The name of the type. */
	const fm::string& GetName() const { return name; }

	/** Retrieves the list of techniques contained by this extra tree.
		@return The list of techniques. */
	FCDETechniqueContainer& GetTechniques() { return techniques; }
	const FCDETechniqueContainer& GetTechniques() const { return techniques; } /**< See above. */

	/** Retrieves the number of techniques contained by this extra tree.
		@return The number of techniques. */
	size_t GetTechniqueCount() const { return techniques.size(); }

	/** Retrieves a specific technique contained by this extra tree.
		@param index The index of the technique.
		@return The technique. This pointer will be NULL if the
			index is out-of-bounds. */
	FCDETechnique* GetTechnique(size_t index) { FUAssert(index < techniques.size(), return NULL); return techniques.at(index); }
	const FCDETechnique* GetTechnique(size_t index) const { FUAssert(index < techniques.size(), return NULL); return techniques.at(index); } /**< See above. */

	/** Adds a new application-specific profile technique to the extra tree.
		If the given application-specific profile already exists
		within the extra tree, the old technique will be returned.
		@param profile The application-specific profile name.
		@return A technique for this application-specific profile. */
	FCDETechnique* AddTechnique(const char* profile);
	inline FCDETechnique* AddTechnique(const fm::string& profile) { return AddTechnique(profile.c_str()); } /**< See above. */

	/** Retrieves a specific technique contained by this extra tree.
		@param profile The application-specific profile name of the technique.
		@return The technique that matches the profile name. This pointer may
			be NULL if no technique matches the profile name. */
	FCDETechnique* FindTechnique(const char* profile) { return const_cast<FCDETechnique*>(const_cast<const FCDEType*>(this)->FindTechnique(profile)); }
	const FCDETechnique* FindTechnique(const char* profile) const; /**< See above. */
	inline FCDETechnique* FindTechnique(const fm::string& profile) { return FindTechnique(profile.c_str()); } /**< See above. */
	inline const FCDETechnique* FindTechnique(const fm::string& profile) const { return FindTechnique(profile.c_str()); } /**< See above. */

	/** Retrieves the extra tree node that has a given element name.
		This function searches for the extra tree node within all the
		techniques.
		@param name An element name.
		@return The extra tree node that matches the element name. This pointer
			will be NULL if no extra tree node matches the element name. */
	inline FCDENode* FindRootNode(const char* name) { return const_cast<FCDENode*>(const_cast<const FCDEType*>(this)->FindRootNode(name)); }
	const FCDENode* FindRootNode(const char* name) const; /**< See above. */
	inline FCDENode* FindRootNode(const fm::string& name) { return FindRootNode(name.c_str()); } /**< See above. */
	inline const FCDENode* FindRootNode(const fm::string& name) const { return FindRootNode(name.c_str()); } /**< See above. */

	/** [INTERNAL] Clones the extra tree information.
		@param clone The extra tree that will take in this extra tree's information.
			If this pointer is NULL, a new extra tree will be created and you will
			need to release the returned pointer manually.
		@return The clone. */
	FCDEType* Clone(FCDEType* clone = NULL) const;

	/** [INTERNAL] Reads in the extra tree from a given COLLADA XML tree node.
		@param extraNode The COLLADA \<extra\> element XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the entity.*/
	bool LoadFromXML(xmlNode* extraNode);

	/** [INTERNAL] Writes out the extra tree to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the \<extra\> element.
		@return The created element XML tree node. */
	xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Writes out the extra tree techniques to the given COLLADA XML tree node.
		The techniques are written straight into the parent node. This is useful when
		the extra tree represents a technique-switch, rather than an <extra> element.
		@param parentNode The COLLADA XML parent node in which to insert the techniques. */
	void WriteTechniquesToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA extra tree node.

	The extra tree node is a hierarchical structure that contains child
	extra tree nodes as well as attributes. If the extra tree node is a leaf
	of the tree, it may contain textual content.

	The extra tree node leaf may be animated, if it has the 'sid' attribute.
*/
class FCOLLADA_EXPORT FCDENode : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	fm::string name;
	fstring content;

	FCDENode* parent;
	FCDENodeContainer children;
	FCDEAttributeContainer attributes;

	FUObjectRef<FCDAnimatedCustom> animated;

public:
	/** Constructor: do not use directly.
		Instead, call the FCDENode::AddChild function of the parent within the hierarchy.
		@param document The COLLADA document that owns the extra tree node.
		@param parent The extra tree node that contains this extra tree node. */
	FCDENode(FCDocument* document, FCDENode* parent);

	/** Destructor. */
	virtual ~FCDENode();

	/** Retrieves the name of the extra tree node.
		The name of the extra tree node is the name of the equivalent XML tree node.
		@return The name of the extra tree node. */
	inline const char* GetName() const { return name.c_str(); }

	/** Sets the name of the extra tree node.
		The name of the extra tree node is the name of the equivalent XML tree node.
		@param _name The name of the extra tree node. */
	inline void SetName(const char* _name) { name = _name; SetDirtyFlag(); }
	inline void SetName(const fm::string& _name) { name = _name; SetDirtyFlag(); } /**< See above. */

	/** Retrieves the textual content of the extra tree node.
		This value is only valid for extra tree node that have no children,
		as COLLADA doesn't allow for mixed-content.
		@return The textual content of the extra tree node. */
	const fchar* GetContent() const { return content.c_str(); }

	/** Sets the textual content of the extra tree node.
		This function will release all the child node of this extra tree node,
		as COLLADA doesn't allow for mixed-content.
		@param _content The textual content. */
	void SetContent(const fchar* _content);
	inline void SetContent(const fstring& _content) { return SetContent(_content.c_str()); } /**< See above. */

	/** Retrieves the animated values associated with this extra tree node.
		Extra tree node leaves may be animated. If this extra tree node leaf
		is animated, this animated value will contain the animation curves.
		@return The animated value. */
	FCDAnimatedCustom* GetAnimated() { return animated; }
	const FCDAnimatedCustom* GetAnimated() const { return animated; } /**< See above. */

	/** Retrieves the parent of an extra tree node.
		The hierarchy cannot be changed dynamically. If you to move an extra tree node,
		you will need to clone it manually and release the old extra tree node.
		@return The parent extra tree node within the hierarchy. This pointer
			will be NULL if the extra tree node is a extra tree technique. */
	FCDENode* GetParent() { return parent; }
	const FCDENode* GetParent() const { return parent; } /**< See above. */

	/** Retrieves the children of an extra tree node.
		@return The list of child extra tree nodes. */
	FCDENodeContainer& GetChildNodes() { return children; }
	const FCDENodeContainer& GetChildNodes() const { return children; } /**< See above. */

	/** Retrieves the number of children of an extra tree node.
		@return The number of children. */
	size_t GetChildNodeCount() const { return children.size(); }

	/** Retrieves a specific child extra tree node.
		@param index The index of the child extra tree node.
		@return The child extra tree node. This pointer will be NULL if the index
			is out-of-bounds. */
	FCDENode* GetChildNode(size_t index) { FUAssert(index < children.size(), return NULL); return children.at(index); }
	const FCDENode* GetChildNode(size_t index) const { FUAssert(index < children.size(), return NULL); return children.at(index); } /**< See above. */

	/** Adds a new child extra tree to this extra tree node.
		@see AddParameter
		@return The new child extra tree node. */
	inline FCDENode* AddChildNode() { FCDENode* node = children.Add(GetDocument(), this); SetDirtyFlag(); return node; }

	/** Adds a new, named, child extra tree to this extra tree node.
		@see AddParameter
		@param name The name of the child node.
		@return The new child extra tree node. */
	inline FCDENode* AddChildNode(const char* name) { FCDENode* node = children.Add(GetDocument(), this); node->SetName(name); SetDirtyFlag(); return node; }
	inline FCDENode* AddChildNode(const fm::string& name) { FCDENode* node = children.Add(GetDocument(), this); node->SetName(name.c_str()); SetDirtyFlag(); return node; }

	/** Retrieves the child extra tree node with the given name.
		@param name A name.
		@return The child extra tree node that matches the given name.
			This pointer will be NULL if no child extra tree node matches
			the given name. */
	inline FCDENode* FindChildNode(const char* name) { return const_cast<FCDENode*>(const_cast<const FCDENode*>(this)->FindChildNode(name)); }
	const FCDENode* FindChildNode(const char* name) const; /**< See above. */
	inline FCDENode* FindChildNode(const fm::string& name) { return FindChildNode(name.c_str()); } /**< See above. */
	inline const FCDENode* FindChildNode(const fm::string& name) const { return FindChildNode(name.c_str()); } /**< See above. */

	/** Retrieves the child extra tree nodes with the given name.
		@param name A name.
		@param nodes A list of nodes to fill in with the nodes
			that match a given name. */
	void FindChildrenNodes(const char* name, FCDENodeList& nodes) const;
	inline void FindChildrenNodes(const fm::string& name, FCDENodeList& nodes) const { FindChildrenNodes(name.c_str(), nodes); } /**< See above. */

	/** Retrieves the child extra tree node with the given name.
		A parameter has no child nodes and is described as: \<X\>value\</X\>.
		The first child extra tree node where the name matches 'X' will be returned.
		@param name The parameter name.
		@return The first child extra tree node holding the wanted parameter within the hierarchy.
			This pointer will be NULL to indicate that no parameter matches the given name. */
	const FCDENode* FindParameter(const char* name) const;
	inline FCDENode* FindParameter(const char* name) { return const_cast<FCDENode*>(const_cast<const FCDENode*>(this)->FindParameter(name)); } /**< See above. */

	/** Retrieves a list of all the parameters contained within the hierarchy.
		A parameter has no child nodes and is described as: \<X\>value\</X\>.
		Using this function, The parameter would be returned with the name 'X'.
		@param nodes The list of parameters to fill in. This list is not emptied by the function.
		@param names The list of names of the parameters. This list is not emptied by the function. */
	void FindParameters(FCDENodeList& nodes, StringList& names);

	/** Retrieves the list of attributes for this extra tree node.
		@return The list of attributes. */
	FCDEAttributeContainer& GetAttributes() { return attributes; }
	const FCDEAttributeContainer& GetAttributes() const { return attributes; } /**< See above. */

	/** Retrieves the number of attributes for this extra tree node.
		@return The number of attributes. */
	size_t GetAttributeCount() const { return attributes.size(); }

	/** Retrieves a specific attribute of this extra tree node.
		@param index The index.
		@return The attribute at this index. This pointer will be NULL
			if the index is out-of-bounds. */
	FCDEAttribute* GetAttribute(size_t index) { FUAssert(index < attributes.size(), return NULL); return attributes.at(index); }
	const FCDEAttribute* GetAttribute(size_t index) const { FUAssert(index < attributes.size(), return NULL); return attributes.at(index); } /**< See above. */

	/** Adds a new attribute to this extra tree node.
		If an attribute with the same name already exists, this function simply
		assigns the new value to the existing attribute and returns the existing attribute.
		@param _name The name of the attribute.
		@param _value The value of the attribute.
		@return The new attribute. */
	FCDEAttribute* AddAttribute(const char* _name, const fchar* _value);
	inline FCDEAttribute* AddAttribute(const fm::string& _name, const fchar* _value) { return AddAttribute(_name.c_str(), _value); } /**< See above. */
	inline FCDEAttribute* AddAttribute(const char* _name, const fstring& _value) { return AddAttribute(_name, _value.c_str()); } /**< See above. */
	inline FCDEAttribute* AddAttribute(const fm::string& _name, const fstring& _value) { return AddAttribute(_name.c_str(), _value.c_str()); } /**< See above. */
	template <typename T> inline FCDEAttribute* AddAttribute(const char* _name, const T& _value) { return AddAttribute(_name, TO_FSTRING(_value)); } /**< See above. */
	template <typename T> inline FCDEAttribute* AddAttribute(const fm::string& _name, const T& _value) { return AddAttribute(_name.c_str(), TO_FSTRING(_value)); } /**< See above. */

	/** Retrieve the attribute of this extra tree node with the given name.
		Attribute names are unique within an extra tree node.
		@param name The attribute name.
		@return The attribute that matches the name. This pointer will be NULL if
			there is no attribute with the given name. */
	inline FCDEAttribute* FindAttribute(const char* name) { return const_cast<FCDEAttribute*>(const_cast<const FCDENode*>(this)->FindAttribute(name)); }
	const FCDEAttribute* FindAttribute(const char* name) const; /**< See above. */

	/** Retrieves the value of an attribute on this extra tree node.
		Attributes names are unique within an extra tree node.
		@param name The attribute name.
		@return The value attached to the attribute with the given name. This value
			will be the empty string when no attribute exists with the given name. */
	const fstring& ReadAttribute(const char* name) const;

	/** Adds a parameter as the child node.
		A parameter is the simplest child node possible:
		with a name and a value, represented as the node's content.
		@see AddChildNode
		@param name The parameter name.
		@param value The parameter value. */
	FCDENode* AddParameter(const char* name, const fchar* value);
	inline FCDENode* AddParameter(const fm::string& name, const fchar* value) { return AddParameter(name.c_str(), value); } /**< See above. */
	inline FCDENode* AddParameter(const char* name, const fstring& value) { return AddParameter(name, value.c_str()); } /**< See above. */
	inline FCDENode* AddParameter(const fm::string& name, const fstring& value) { return AddParameter(name.c_str(), value.c_str()); } /**< See above. */
	template <class T>
	inline FCDENode* AddParameter(const char* name, const T& value) { return AddParameter(name, TO_FSTRING(value)); } /**< See above. */
	template <class T>
	inline FCDENode* AddParameter(const fm::string& name, const T& value) { return AddParameter(name.c_str(), TO_FSTRING(value)); } /**< See above. */

	/** Clones the extra tree node.
		@param clone The extra tree node that will receive the clone information.
			This pointer cannot be NULL.
		@return The clone. You will need to release the returned pointer manually. */
	virtual FCDENode* Clone(FCDENode* clone) const;

	/** [INTERNAL] Reads in the extra tree node from a given COLLADA XML tree node.
		@param customNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the extra tree node.*/
	virtual bool LoadFromXML(xmlNode* customNode);

	/** [INTERNAL] Writes out the extra tree node to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the extra tree node.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

protected:
	/** [INTERNAL] Reads in the children nodes of the extra tree node.
		Used by the FCDETechnique class exclusively.
		@param customNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the extra tree node.*/
	bool ReadChildrenFromXML(xmlNode* customNode);

	/** [INTERNAL] Writes out the children nodes of extra tree node.
		Used by the FCDETechnique class exclusively.
		@param customNode The COLLADA XML node for the extra tree node. */
	void WriteChildrenToXML(xmlNode* customNode) const;
};

/**
	A COLLADA extra tree technique.

	For convenience, this extra tree technique is based on top of the FCDENode class.
	An extra tree technique is the root of the extra tree specific to 
	the profile of an application.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDETechnique : public FCDENode
{
private:
	DeclareObjectType(FCDENode);

	fm::string profile;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEType::AddTechnique function.
		@param document The COLLADA document that owns the technique.
		@param profile The application-specific profile name. */
	FCDETechnique(FCDocument* document, const char* profile);

	/** Destructor. */
	virtual ~FCDETechnique();

	/** Retrieves the name of the application-specific profile of the technique.
		@return The name of the application-specific profile. */
	const char* GetProfile() const { return profile.c_str(); }

	/** Clones the extra tree node.
		@param clone The extra tree node that will receive the clone information.
			If this pointer is NULL, a new extra tree technique will be created and you will
			need to release the returned pointer manually.
		@return The clone. */
	virtual FCDENode* Clone(FCDENode* clone) const;

	/** [INTERNAL] Reads in the extra tree technique from a given COLLADA XML tree node.
		@param techniqueNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the extra tree technique.*/
	virtual bool LoadFromXML(xmlNode* techniqueNode);

	/** [INTERNAL] Writes out the extra tree technique to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the extra tree technique.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	An extra tree attribute.
	Contains a name and a value string.
*/
class FCDEAttribute : public FUObject
{
private:
	DeclareObjectType(FUObject);

public:
	fm::string name; /**< The attribute name. Must be provided. */
	fstring value; /**< The attribute value. Is optional. */

public:
	/** Default constructor.
		The name and the value string will be blank. */
	FCDEAttribute() {}

	/** Constructor.
		Sets the attribute name and the attribute value appropriately.
		@param _name The attribute name.
		@param _value The attribute value. */
	FCDEAttribute(const char* _name, const fchar* _value);
};

#endif // _FCD_EXTRA_H_
