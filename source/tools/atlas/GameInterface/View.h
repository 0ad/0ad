#ifndef VIEW_H__
#define VIEW_H__

class ViewGame;
class ViewActor;

class View
{
public:
	virtual ~View();
	virtual void Update(float frameLength) = 0;
	virtual void Render() = 0;
	virtual CCamera& GetCamera() = 0;
	virtual bool WantsHighFramerate() = 0;

	// These always return a valid (not NULL) object
	static View* GetView_None();
	static ViewGame* GetView_Game();
	static ViewActor* GetView_Actor();

	// Invalidates any View objects previously returned by this class
	static void DestroyViews();
};

//////////////////////////////////////////////////////////////////////////

class ActorViewer;

class ViewGame : public View
{
public:
	ViewGame();
	virtual void Update(float frameLength);
	virtual void Render();
	virtual CCamera& GetCamera();
	virtual bool WantsHighFramerate();

private:
};

class ViewActor : public View
{
public:
	ViewActor();
	~ViewActor();

	virtual void Update(float frameLength);
	virtual void Render();
	virtual CCamera& GetCamera();
	virtual bool WantsHighFramerate();

	void SetSpeedMultiplier(float speed);
	ActorViewer& GetActorViewer();

private:
	float m_SpeedMultiplier;
	CCamera m_Camera;
	ActorViewer* m_ActorViewer;
};


#endif // VIEW_H__
