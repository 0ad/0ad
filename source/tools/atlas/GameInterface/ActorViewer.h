#ifndef INCLUDED_ACTORVIEWER
#define INCLUDED_ACTORVIEWER

struct ActorViewerImpl;
struct SColor4ub;
class CUnit;
class CStrW;

class ActorViewer : noncopyable
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
};

#endif // INCLUDED_ACTORVIEWER
