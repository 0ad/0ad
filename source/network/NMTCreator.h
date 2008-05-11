#include "Serialization.h"
#include <vector>

// If included from within the NMT Creation process, perform a pass
#ifdef CREATING_NMT

#include NMT_CREATE_HEADER_NAME

#undef START_NMTS
#undef END_NMTS
#undef START_NMT_CLASS
#undef START_NMT_CLASS_DERIVED
#undef NMT_FIELD_INT
#undef NMT_FIELD
#undef NMT_START_ARRAY
#undef NMT_END_ARRAY
#undef END_NMT_CLASS

#else
// If not within the creation process, and called with argument, perform the
// creation process with the header specified
#ifdef NMT_CREATE_HEADER_NAME

#ifndef ARRAY_STRUCT_PREFIX
#define ARRAY_STRUCT_PREFIX(_nm) S_##_nm
#endif

#define CREATING_NMT

/*************************************************************************/
// Pass 1, class definition
#define NMT_CREATOR_PASS_CLASSDEF
#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
	START_NMT_CLASS_DERIVED(CNetMessage, _nm, _tp)

/**
 * Start the definition of a network message type.
 *
 * @param _base The name of the base class of the message
 * @param _nm The name of the class
 * @param _tp The NetMessageType associated with the class. It is *not* safe to
 * have several classes with the same value of _tp in the same executable
 */
#define START_NMT_CLASS_DERIVED(_base, _nm, _tp) \
CNetMessage *Deserialize##_nm(const u8 *, size_t); \
class _nm: public _base \
{ \
protected: \
	_nm(ENetMessageType type): _base(type) {}\
	\
	/* This one is for subclasses that want to use the base class' string */ \
	/* converters to get SubMessage { <parent fields>, ... } */ \
	CStr GetStringRaw() const;\
public: \
	_nm(): _base(_tp) {} \
	virtual size_t GetSerializedLength() const; \
	virtual u8 *Serialize(u8 *buffer) const; \
	virtual const u8 *Deserialize(const u8 *pos, const u8 *end); \
	virtual CStr GetString() const; \
	virtual CNetMessage *Copy() const \
	{ \
		return new _nm(*this); \
	} \
	inline operator CStr () const \
	{ return GetString(); }

/**
 * Add an integer field to the message type.
 *
 * @param _nm The name of the field
 * @param _hosttp The local type of the field (the data type used in the field
 * definition)
 * @param _netsz The number of bytes that should be serialized. If the variable
 * has a value larger than the maximum value of the specified network size,
 * higher order bytes will be discarded.
 */		
#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	_hosttp _nm;

/**
 * Add a generic field to the message type. The data type must be a class
 * implementing the ISerializable interface
 *
 * @param _tp The local data type of the field
 * @param _nm The name of the field
 * @see ISerializable
 */
#define NMT_FIELD(_tp, _nm) \
	_tp _nm;

#define NMT_START_ARRAY(_nm) \
	struct ARRAY_STRUCT_PREFIX(_nm); \
	std::vector <ARRAY_STRUCT_PREFIX(_nm)> _nm; \
	struct ARRAY_STRUCT_PREFIX(_nm) {

#define NMT_END_ARRAY() \
	};

#define END_NMT_CLASS() };

#include "NMTCreator.h"
#undef NMT_CREATOR_PASS_CLASSDEF

#ifdef NMT_CREATOR_IMPLEMENT

#include "StringConverters.h"

/*************************************************************************/
// Pass 2, GetSerializedLength
#define NMT_CREATOR_PASS_GETLENGTH
#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
	START_NMT_CLASS_DERIVED(CNetMessage, _nm, _tp)
#define START_NMT_CLASS_DERIVED(_base, _nm, _tp) \
size_t _nm::GetSerializedLength() const \
{ \
	size_t ret=_base::GetSerializedLength(); \
	const _nm *thiz=this;\
	UNUSED2(thiz);	// preempt any "unused" warning

#define NMT_START_ARRAY(_nm) \
	std::vector <ARRAY_STRUCT_PREFIX(_nm)>::const_iterator it=_nm.begin(); \
	while (it != _nm.end()) \
	{ \
		const ARRAY_STRUCT_PREFIX(_nm) *thiz=&*it;\
		UNUSED2(thiz);	// preempt any "unused" warning

#define NMT_END_ARRAY() \
		++it; \
	}

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	ret += _netsz;

#define NMT_FIELD(_tp, _nm) \
	ret += thiz->_nm.GetSerializedLength();

#define END_NMT_CLASS() \
	return ret; \
};

#include "NMTCreator.h"
#undef NMT_CREATOR_PASS_GETLENGTH

/*************************************************************************/
// Pass 3, Serialize

#define NMT_CREATOR_PASS_SERIALIZE

#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
	START_NMT_CLASS_DERIVED(CNetMessage, _nm, _tp)
#define START_NMT_CLASS_DERIVED(_base, _nm, _tp) \
u8 *_nm::Serialize(u8 *buffer) const \
{ \
	/*printf("In " #_nm "::Serialize()\n");*/ \
	u8 *pos=_base::Serialize(buffer); \
	const _nm *thiz=this;\
	UNUSED2(thiz);	// preempt any "unused" warning

#define NMT_START_ARRAY(_nm) \
	std::vector <ARRAY_STRUCT_PREFIX(_nm)>::const_iterator it=_nm.begin(); \
	while (it != _nm.end()) \
	{ \
		const ARRAY_STRUCT_PREFIX(_nm) *thiz=&*it;\
		UNUSED2(thiz);	// preempt any "unused" warning

#define NMT_END_ARRAY() \
		++it; \
	}

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	Serialize_int_##_netsz(pos, thiz->_nm); \

#define NMT_FIELD(_tp, _nm) \
	pos=thiz->_nm.Serialize(pos);

#define END_NMT_CLASS() \
	return pos; \
}

#include "NMTCreator.h"

#undef NMT_CREATOR_PASS_SERIALIZE

/*************************************************************************/
// Pass 4, Deserialize

#define NMT_CREATOR_PASS_DESERIALIZE

#define START_NMTS()
#define END_NMTS()

#define BAIL_DESERIALIZER return NULL

#define START_NMT_CLASS(_nm, _tp) \
	START_NMT_CLASS_DERIVED(CNetMessage, _nm, _tp)
#define START_NMT_CLASS_DERIVED(_base, _nm, _tp) \
CNetMessage *Deserialize##_nm(const u8 *buffer, size_t length) \
{ \
	_nm *ret=new _nm(); \
	if (ret->Deserialize(buffer, buffer+length)) \
		return ret; \
	else \
	{ delete ret; return NULL; } \
} \
const u8 *_nm::Deserialize(const u8 *pos, const u8 *end) \
{ \
	pos=_base::Deserialize(pos, end); \
	_nm *thiz=this; \
	/*printf("In Deserialize" #_nm "\n"); */\
	UNUSED2(thiz);	// preempt any "unused" warning


#define NMT_START_ARRAY(_nm) \
	while (pos < end) \
	{ \
		ARRAY_STRUCT_PREFIX(_nm) *thiz=&*_nm.insert(_nm.end(), ARRAY_STRUCT_PREFIX(_nm)());\
		UNUSED2(thiz);	// preempt any "unused" warning

#define NMT_END_ARRAY() \
	}

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	if (pos+_netsz > end) BAIL_DESERIALIZER; \
	Deserialize_int_##_netsz(pos, thiz->_nm); \
	/*printf("\t" #_nm " == 0x%x\n", thiz->_nm);*/

#define NMT_FIELD(_tp, _nm) \
	if ((pos=thiz->_nm.Deserialize(pos, end)) == NULL) BAIL_DESERIALIZER;

#define END_NMT_CLASS() \
	return pos; \
}

#include "NMTCreator.h"

#undef BAIL_DESERIALIZER

#undef NMT_CREATOR_PASS_DESERIALIZE

/*************************************************************************/
// Pass 5, Deserializer Registration

#define NMT_CREATOR_PASS_REGISTRATION

#define START_NMTS() SNetMessageDeserializerRegistration g_DeserializerRegistrations[] = {
#define END_NMTS() { NMT_NONE, NULL } };

#define START_NMT_CLASS(_nm, _tp) \
	START_NMT_CLASS_DERIVED(CNetMessage, _nm, _tp)
#define START_NMT_CLASS_DERIVED(_base, _nm, _tp) \
	{ _tp, Deserialize##_nm },

#define NMT_START_ARRAY(_nm)

#define NMT_END_ARRAY()

#define NMT_FIELD_INT(_nm, _hosttp, _netsz)

#define NMT_FIELD(_tp, _nm)

#define END_NMT_CLASS()

#include "NMTCreator.h"

#undef NMT_CREATOR_PASS_REGISTRATION

/*************************************************************************/
// Pass 6, String Representation

#define _T(s) s
// PT: I'm not sure whether this is correct - it appears that this code
// relied on CStr.h leaving _T defined in ASCII mode, so I'm not sure
// why the macro is actually used at all...

#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
CStr _nm::GetString() const \
{ \
	CStr ret=#_nm _T(" { "); \
	return ret + GetStringRaw() + _T(" }"); \
} \
CStr _nm::GetStringRaw() const \
{ \
	CStr ret; \
	const _nm *thiz=this;\
	UNUSED2(thiz);	// preempt any "unused" warning

#define START_NMT_CLASS_DERIVED(_base, _nm, _tp) \
CStr _nm::GetString() const \
{ \
	CStr ret=#_nm _T(" { "); \
	return ret + GetStringRaw() + _T(" }"); \
} \
CStr _nm::GetStringRaw() const \
{ \
	CStr ret=_base::GetStringRaw() + _T(", "); \
	const _nm *thiz=this;\
	UNUSED2(thiz);	// preempt any "unused" warning

#define NMT_START_ARRAY(_nm) \
	ret+=#_nm _T(": { "); \
	std::vector < ARRAY_STRUCT_PREFIX(_nm) >::const_iterator it=_nm.begin(); \
	while (it != _nm.end()) \
	{ \
		ret+=_T(" { "); \
		const ARRAY_STRUCT_PREFIX(_nm) *thiz=&*it;\
		UNUSED2(thiz);	// preempt any "unused" warning

#define NMT_END_ARRAY() \
		++it; \
		ret=ret.substr(0, ret.length()-2)+_T(" }, "); \
	} \
	ret=ret.substr(0, ret.length()-2)+_T(" }, ");

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	ret += #_nm _T(": "); \
	ret += NetMessageStringConvert(thiz->_nm); \
	ret += _T(", ");

#define NMT_FIELD(_tp, _nm) \
	ret += #_nm _T(": "); \
	ret += NetMessageStringConvert(thiz->_nm); \
	ret += _T(", ");

#define END_NMT_CLASS() \
	return ret.substr(0, ret.length()-2); \
}

#include "NMTCreator.h"

#endif // #ifdef NMT_CREATOR_IMPLEMENT

/*************************************************************************/
// Cleanup
#undef NMT_CREATE_HEADER_NAME
#undef NMT_CREATOR_IMPLEMENT
#undef CREATING_NMT

#endif // #ifdef NMT_CREATE_HEADER_NAME
#endif // #ifndef CREATING_NMT
