// Message structure

#ifndef MESSAGING_INCLUDED
#define MESSAGING_INCLUDED

struct CMessage
{
	enum EMessageType
	{
		EMSG_TICK,
		EMSG_INIT
	} type;
	CMessage( EMessageType _type )
	{
		type = _type;
	}
};

#endif