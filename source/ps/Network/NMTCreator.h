#include "Serialization.h"

// If included from within the NMT Creation process, perform a pass
#ifdef CREATING_NMT

#include NMT_CREATE_HEADER_NAME

#undef START_NMTS
#undef END_NMTS
#undef START_NMT_CLASS
#undef NMT_FIELD_INT
#undef NMT_FIELD
#undef END_NMT_CLASS

#else
// If not within the creation process, and called with argument, perform the
// creation process with the header specified
#ifdef NMT_CREATE_HEADER_NAME

#define CREATING_NMT

/*************************************************************************/
// Pass 1, class definition
#define NMT_CREATOR_PASS_CLASSDEF
#define START_NMTS()
#define END_NMTS()

/**
 * Start the definition of a network message type.
 *
 * @param _nm The name of the class
 * @param _tp The NetMessageType associated with the class. IT is *not* safe to
 * have several classes with the same value of _tp in the same executable
 */
#define START_NMT_CLASS(_nm, _tp) \
CNetMessage *Deserialize##_nm(const u8 *, uint); \
struct _nm: public CNetMessage \
{ \
	_nm(): CNetMessage(_tp) {} \
	virtual uint GetSerializedLength() const; \
	virtual void Serialize(u8 *buffer) const;

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

#define END_NMT_CLASS() };

#include "NMTCreator.h"
#undef NMT_CREATOR_PASS_CLASSDEF

#ifdef NMT_CREATOR_IMPLEMENT

/*************************************************************************/
// Pass 2, GetSerializedLength
#define NMT_CREATOR_PASS_GETLENGTH
#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
uint _nm::GetSerializedLength() const \
{ \
	uint ret=0;

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	ret += _netsz;

#define NMT_FIELD(_tp, _nm) \
	ret += _nm.GetSerializedLength();

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
void _nm::Serialize(u8 *buffer) const \
{ \
	/*printf("In " #_nm "::Serialize()\n");*/ \
	u8 *pos=buffer; \

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	Serialize_int_##_netsz(pos, _nm); \

#define NMT_FIELD(_tp, _nm) \
	pos=_nm.Serialize(pos);

#define END_NMT_CLASS() }

#include "NMTCreator.h"

#undef NMT_CREATOR_PASS_SERIALIZE

/*************************************************************************/
// Pass 4, Deserialize

#define NMT_CREATOR_PASS_DESERIALIZE

#define START_NMTS()
#define END_NMTS()

#define BAIL_DESERIALIZER STMT( delete ret; return NULL; )

#define START_NMT_CLASS(_nm, _tp) \
CNetMessage *Deserialize##_nm(const u8 *buffer, uint length) \
{ \
	/*printf("In Deserialize" #_nm "\n"); */\
	_nm *ret=new _nm(); \
	const u8 *pos=buffer; \
	const u8 *end=buffer+length; \

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	if (pos+_netsz >= end) BAIL_DESERIALIZER; \
	Deserialize_int_##_netsz(pos, (ret->_nm)); \
	/*printf("\t" #_nm " == 0x%x\n", ret->_nm);*/

#define NMT_FIELD(_tp, _nm) \
	if ((pos=ret->_nm.Deserialize(pos, end)) == NULL) BAIL_DESERIALIZER;

#define END_NMT_CLASS() \
	return ret; \
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
	{ _tp, Deserialize##_nm },

#define NMT_FIELD_INT(_nm, _hosttp, _netsz)

#define NMT_FIELD(_tp, _nm)

#define END_NMT_CLASS()

#include "NMTCreator.h"

#undef NMT_CREATOR_PASS_REGISTRATION

#endif // #ifdef NMT_CREATOR_IMPLEMENT

/*************************************************************************/
// Cleanup
#undef NMT_CREATE_HEADER_NAME
#undef NMT_CREATOR_IMPLEMENT
#undef CREATING_NMT

#endif // #ifdef NMT_CREATE_HEADER_NAME
#endif // #ifndef CREATING_NMT
