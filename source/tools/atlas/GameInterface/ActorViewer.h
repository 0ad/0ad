#ifndef ACTORVIEWER_H__
#define ACTORVIEWER_H__

struct ActorViewerImpl;

class ActorViewer
{
public:
	ActorViewer();
	~ActorViewer();

	void SetActor(const CStrW& id, const CStrW& animation);
	void Render();
	void Update(float dt);

private:
	ActorViewerImpl& m;

	NO_COPY_CTOR(ActorViewer);
};

#endif // ACTORVIEWER_H__
