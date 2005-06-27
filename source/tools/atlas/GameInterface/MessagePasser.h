#ifndef MESSAGEPASSER_H__
#define MESSAGEPASSER_H__

namespace AtlasMessage
{
struct IMessage;
	
class MessagePasser
{
public:
	virtual void Add(IMessage*)=0;
	virtual IMessage* Retrieve()=0;
	
	virtual void Query(IMessage&)=0;
	virtual void QueryDone()=0;
};

extern MessagePasser* g_MessagePasser;

#define ADD_MESSAGE(type) AtlasMessage::g_MessagePasser->Add(new AtlasMessage::m##type)

}


/*

atlas->game command ("initialise now", "render now")
atlas->game->atlas query ("what is at position (x,y)?")
game->atlas notification ("game ended")  ??

*/

#endif // MESSAGEPASSER_H__
