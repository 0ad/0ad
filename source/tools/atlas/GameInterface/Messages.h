#ifndef MESSAGES_H__
#define MESSAGES_H__

#include "MessagePasser.h"

namespace AtlasMessage
{

struct IMessage
{
	virtual const char* GetType() const = 0;
	virtual ~IMessage() {}
};

#define DEFINE(t) struct m##t : public IMessage { const char* GetType() const { return #t; }

//////////////////////////////////////////////////////////////////////////

DEFINE(CommandString)
mCommandString(const std::string& name_) : name(name_) {}
const std::string name;
};

//////////////////////////////////////////////////////////////////////////

DEFINE(SetContext)
mSetContext(void* /* HDC */ hdc_, void* /* HGLRC */ hglrc_) : hdc(hdc_), hglrc(hglrc_) {};
void* hdc;
void* hglrc;
};

DEFINE(ResizeScreen)
mResizeScreen(int width_, int height_) : width(width_), height(height_) {}
int width, height;
};

//////////////////////////////////////////////////////////////////////////

DEFINE(GenerateMap)
mGenerateMap(int size_) : size(size_) {}
int size; // size in number of patches
};

//////////////////////////////////////////////////////////////////////////

#undef DEFINE

}

#endif // MESSAGES_H__