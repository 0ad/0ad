#ifndef ACTORVIEWER_H__
#define ACTORVIEWER_H__

struct ActorViewerImpl;
struct SColor4ub;
class CUnit;
class CStrW;

class ActorViewer
{
public:
	ActorViewer();
	~ActorViewer();

	void SetActor(const CStrW& id, const CStrW& animation);
	void UnloadObjects();
	CUnit* GetUnit();
	void SetBackgroundColour(const SColor4ub& colour);
	void SetWalkEnabled(bool enabled);
	void SetGroundEnabled(bool enabled);
	void SetShadowsEnabled(bool enabled);
	void SetStatsEnabled(bool enabled);
	void Render();
	void Update(float dt);
	
	// Returns whether there is a selected actor which has more than one
	// frame of animation
	bool HasAnimation() const;

private:
	ActorViewerImpl& m;

	NO_COPY_CTOR(ActorViewer);
};

#endif // ACTORVIEWER_H__
