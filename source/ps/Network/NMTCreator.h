#include "Serialization.h"

// If included from within the NMT Creation process, perform a pass
#ifdef CREATING_NMT

#include NMT_CREATE_HEADER_NAME

#undef START_NMTS
#undef END_NMTS
#undef START_NMT_CLASS
#undef NMT_FIELD_INT
#undef END_NMT_CLASS

#else
// If not within the creation process, and called with argument, perform the
// creation process with the header specified
#ifdef NMT_CREATE_HEADER_NAME

#define CREATING_NMT

/*************************************************************************/
// Pass 1, class definition
#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
CNetMessage *Deserialize##_nm(u8 *, uint); \
struct _nm: public CNetMessage \
{ \
	_nm(): CNetMessage(_tp) {} \
	virtual uint GetSerializedLength() const; \
	virtual void Serialize(u8 *buffer) const;

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	_hosttp _nm;

#define END_NMT_CLASS() };

#include "NMTCreator.h"

#ifdef NMT_CREATOR_IMPLEMENT

/*************************************************************************/
// Pass 2, GetSerializedLength
#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
uint _nm::GetSerializedLength() const \
{ \
	uint ret=0;

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	ret += _netsz;

#define END_NMT_CLASS() \
	return ret; \
};

#include "NMTCreator.h"

/*************************************************************************/
// Pass 3, Serialize
#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
void _nm::Serialize(u8 *buffer) const \
{ \
	printf("In " #_nm "::Serialize()\n"); \
	u8 *pos=buffer; \

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	pos=SerializeInt<_hosttp, _netsz>(pos, _nm); \

#define END_NMT_CLASS() }

#include "NMTCreator.h"

/*************************************************************************/
// Pass 4, Deserialize
#define START_NMTS()
#define END_NMTS()

#define START_NMT_CLASS(_nm, _tp) \
CNetMessage *Deserialize##_nm(u8 *buffer, uint length) \
{ \
	printf("In Deserialize" #_nm "\n"); \
	_nm *ret=new _nm(); \
	u8 *pos=buffer; \
	u8 *end=buffer+length; \

#define NMT_FIELD_INT(_nm, _hosttp, _netsz) \
	ret->_nm=DeserializeInt<_hosttp, _netsz>(&pos); \
	printf("\t" #_nm " == 0x%x\n", ret->_nm);

#define END_NMT_CLASS() \
	return ret; \
}

#include "NMTCreator.h"

/*************************************************************************/
// Pass 5, Deserializer Registration
#define START_NMTS() SNetMessageDeserializerRegistration g_DeserializerRegistrations[] = {
#define END_NMTS() { NMT_NONE, NULL } };

#define START_NMT_CLASS(_nm, _tp) \
	{ _tp, Deserialize##_nm },

#define NMT_FIELD_INT(_nm, _hosttp, _netsz)

#define END_NMT_CLASS()

#include "NMTCreator.h"

#endif // #ifdef NMT_CREATOR_IMPLEMENT

/*************************************************************************/
// Cleanup
#undef NMT_CREATE_HEADER_NAME
#undef NMT_CREATOR_IMPLEMENT
#undef CREATING_NMT

#endif // #ifdef NMT_CREATE_HEADER_NAME
#endif // #ifndef CREATING_NMT
