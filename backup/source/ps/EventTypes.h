/*

  Net Message Creator registration

*/

	#ifdef CREATING_NMT

	//HACKA HACKA HACKA HACKA HACKA HACK...

	#ifdef NMT_CREATOR_PASS_CLASSDEF

/*

  The Event classes;

  Adding an event currently requires four steps.

  One, create the event class within the NMT_CREATOR_PASS_CLASSDEF
    ifdef. The classEvent macro will help.

  Two, assign an message type in NetMessage.h.

  Three, provide a deserializer function mapping in the
    NMT_CREATOR_PASS_REGISTRATION ifdef, below.

  Four, implement the interface ISerializable for the new class in
    Event.cpp.

*/

	#define classEvent( _ev ) \
	struct CEvent##_ev : public CEvent \
	{ \
	public: \
		uint GetSerializedLength(); \
		u8* Serialize( u8* buffer ); \
		static CNetMessage* Deserialize( const u8* buffer, \
								uint length );



/*
	classEvent( TestOnly )
		int16 TheIntegerIveGot;
	};
*/

	#endif

	#ifdef NMT_CREATOR_PASS_REGISTRATION

/*
	{ NMT_Event_IveGotAnInteger, CEventTestOnly::Deserialize },
*/

	#endif

	#endif