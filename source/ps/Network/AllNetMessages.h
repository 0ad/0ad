#ifndef _AllNetMessages_H
#define _AllNetMessages_H

#include "types.h"
#include "CStr.h"

enum NetMessageType
{
	/*
		All Message Types should be put here. Never change the order of this
		list.
		First, all negative types are only for internal/local use and may never
		be sent over the network.
	*/
	/**
	 * A special message that contains a PS_RESULT code, used for delivery of
	 * OOB error status messages from a CMessageSocket
	 */
	NMT_ERROR=-1,
	/**
	 * An invalid message type, representing an uninitialized message.
	 */
	NMT_NONE=0,
	
	/* Beware, the list will contain bogus messages when under development ;-) */
	NMT_Aloha,
	NMT_Sayonara,

	/**
	 * One higher than the highest value of any message type
	 */
	NMT_LAST // Always put this last in the list
};

#endif // #ifndef _AllNetMessage_H

#ifdef CREATING_NMT

#define ALLNETMSGS_DONT_CREATE_NMTS

START_NMTS()

START_NMT_CLASS(AlohaMessage, NMT_Aloha)
//	NMT_FIELD_INT(m_AlohaCode, u64, 8)
	NMT_FIELD(CStr, m_AlohaCode)
END_NMT_CLASS()

START_NMT_CLASS(SayonaraMessage, NMT_Sayonara)
//	NMT_FIELD_INT(m_SayonaraCode, u64, 8)
	NMT_FIELD(CStr, m_SayonaraCode)
END_NMT_CLASS()

END_NMTS()

#else
#ifndef ALLNETMSGS_DONT_CREATE_NMTS

#ifdef ALLNETMSGS_IMPLEMENT
#define NMT_CREATOR_IMPLEMENT
#endif

#define NMT_CREATE_HEADER_NAME "AllNetMessages.h"
#include "NMTCreator.h"

#endif // #ifndef ALLNETMSGS_DONT_CREATE_NMTS
#endif // #ifdef CREATING_NMT
