namespace AtlasMessage
{
struct IMessage;
	
class MessageHandler
{
public:
	virtual void Add(IMessage*)=0;
	virtual IMessage* Retrieve()=0;
	
	virtual void Query(IMessage&)=0;
	virtual void QueryDone()=0;
};

extern MessageHandler* g_MessageHandler;

}


/*

atlas->game command ("initialise now", "render now")
atlas->game->atlas query ("what is at position (x,y)?")
game->atlas notification ("game ended")  ??

*/
