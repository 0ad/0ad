#ifndef MESSAGEPASSER_H__
#define MESSAGEPASSER_H__

namespace AtlasMessage
{
	
template <typename T> class MessagePasser
{
public:
	virtual void Add(T*)=0;
	virtual T* Retrieve()=0;
};

struct mCommand;
struct mInput;
extern MessagePasser<mCommand>* g_MessagePasser_Command;
extern MessagePasser<mInput>*   g_MessagePasser_Input;

#define POST_COMMAND(type) AtlasMessage::g_MessagePasser_Command->Add(new AtlasMessage::m##type)
#define POST_INPUT(type)   AtlasMessage::g_MessagePasser_Input -> Add(new AtlasMessage::m##type)

}

#endif // MESSAGEPASSER_H__
