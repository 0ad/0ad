#ifndef VIEW_H__
#define VIEW_H__

#include "graphics/Camera.h"

#include "Messages.h"

class CUnit;

class ViewGame;
class ViewActor;

class View
{
public:
	virtual ~View();
	virtual void Update(float UNUSED(frameLength)) { };
	virtual void Render() { };
	virtual CCamera& GetCamera() = 0;
	virtual CUnit* GetUnit(AtlasMessage::ObjectID UNUSED(id)) { return NULL; }
	virtual bool WantsHighFramerate() { return false; }

	virtual void SetParam(const std::wstring& name, bool value);
	virtual void SetParam(const std::wstring& name, const AtlasMessage::Colour& value);

	// These always return a valid (not NULL) object
	static View* GetView(int /*eRenderView*/ view);
	static View* GetView_None();
	static ViewGame* GetView_Game();
	static ViewActor* GetView_Actor();

	// Invalidates any View objects previously returned by this class
	static void DestroyViews();
};

//////////////////////////////////////////////////////////////////////////

class ViewNone : public View
{
public:
	virtual CCamera& GetCamera() { return dummyCamera; }
private:
	CCamera dummyCamera;
};

class ViewGame : public View
{
public:
	ViewGame();
	virtual void Update(float frameLength);
	virtual void Render();
	virtual CCamera& GetCamera();
	virtual CUnit* GetUnit(AtlasMessage::ObjectID id);
	virtual bool WantsHighFramerate();

private:
};

class ActorViewer;

class ViewActor : public View
{
public:
	ViewActor();
	~ViewActor();

	virtual void Update(float frameLength);
	virtual void Render();
	virtual CCamera& GetCamera();
	virtual CUnit* GetUnit(AtlasMessage::ObjectID id);
	virtual bool WantsHighFramerate();

	virtual void SetParam(const std::wstring& name, bool value);
	virtual void SetParam(const std::wstring& name, const AtlasMessage::Colour& value);

	void SetSpeedMultiplier(float speed);
	ActorViewer& GetActorViewer();

private:
	float m_SpeedMultiplier;
	CCamera m_Camera;
	ActorViewer* m_ActorViewer;
};

#endif // VIEW_H__
