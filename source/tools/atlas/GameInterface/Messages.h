namespace AtlasMessage
{

struct IMessage
{
	virtual int GetType() = 0;
	virtual ~IMessage() {}
};

enum {
	CommandString,
	SetContext,
	ResizeScreen,
};

struct mCommandString : public IMessage
{
	mCommandString(const std::string& n) : name(n) {};
	const std::string name;
	virtual int GetType() { return CommandString; }
};

struct mSetContext : public IMessage
{
	mSetContext(void* /* HDC */ dc, void* /* HGLRC */ cx) : hdc(dc), hglrc(cx) {};
	void* hdc;
	void* hglrc;
	virtual int GetType() { return SetContext; }
};

struct mResizeScreen : public IMessage
{
	mResizeScreen(int w, int h) : width(w), height(h) {}
	int width, height;
	virtual int GetType() { return ResizeScreen; }
};

}
