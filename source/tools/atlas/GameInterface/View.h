/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_VIEW
#define INCLUDED_VIEW

#include <map>

#include "graphics/Camera.h"

#include "Messages.h"
#include "simulation2/system/Entity.h"

class CUnit;
class CSimulation2;

class AtlasViewGame;
class AtlasViewActor;

/**
 * Superclass for all Atlas game views.
 */
class AtlasView
{
public:
	virtual ~AtlasView();
	virtual void Update(float UNUSED(realFrameLength)) { };
	virtual void Render() { };
	virtual void DrawOverlays() { };
	virtual CCamera& GetCamera() = 0;
	virtual CSimulation2* GetSimulation2() { return NULL; }
	virtual entity_id_t GetEntityId(AtlasMessage::ObjectID obj) { return (entity_id_t)obj; }
	virtual bool WantsHighFramerate() { return false; }
	virtual void SetEnabled(bool UNUSED(enabled)) {}

	virtual void SetParam(const std::wstring& name, bool value);
	virtual void SetParam(const std::wstring& name, int value);
	virtual void SetParam(const std::wstring& name, const AtlasMessage::Color& value);
	virtual void SetParam(const std::wstring& name, const std::wstring& value);

	// These always return a valid (not NULL) object
	static AtlasView* GetView(int /*eRenderView*/ view);
	static AtlasView* GetView_None();
	static AtlasViewGame* GetView_Game();
	static AtlasViewActor* GetView_Actor();

	// Invalidates any AtlasView objects previously returned by this class
	static void DestroyViews();
};

//////////////////////////////////////////////////////////////////////////

class AtlasViewNone : public AtlasView
{
public:
	virtual CCamera& GetCamera() { return dummyCamera; }
private:
	CCamera dummyCamera;
};

//////////////////////////////////////////////////////////////////////////

class SimState;

/**
 * Main editor/game view of Atlas. Editing the world/scenario and simulation testing happens here.
 */
class AtlasViewGame : public AtlasView
{
public:
	AtlasViewGame();
	virtual ~AtlasViewGame();
	virtual void Update(float realFrameLength);
	virtual void Render();
	virtual void DrawOverlays();
	virtual CCamera& GetCamera();
	virtual CSimulation2* GetSimulation2();
	virtual bool WantsHighFramerate();

	virtual void SetParam(const std::wstring& name, bool value);
	virtual void SetParam(const std::wstring& name, const std::wstring& value);

	void SetSpeedMultiplier(float speedMultiplier);
	void SetTesting(bool testing);
	void SaveState(const std::wstring& label);
	void RestoreState(const std::wstring& label);
	std::wstring DumpState(bool binary);
	void SetBandbox(bool visible, float x0, float y0, float x1, float y1);

private:
	float m_SpeedMultiplier;
	bool m_IsTesting;
	std::map<std::wstring, SimState*> m_SavedStates;
	std::string m_DisplayPassability;

	typedef struct SBandboxVertex
	{
		SBandboxVertex(float x, float y, u8 r, u8 g, u8 b, u8 a) : x(x), y(y), r(r), g(g), b(b), a(a) {}
		u8 r, g, b, a;
		float x, y;
	} SBandboxVertex;

	std::vector<SBandboxVertex> m_BandboxArray;
};

//////////////////////////////////////////////////////////////////////////

class ActorViewer;

/**
 * Actor Viewer window in Atlas. Dedicated view for examining a single actor/entity and its variations,
 * animations, etc. in more detail.
 */
class AtlasViewActor : public AtlasView
{
public:
	AtlasViewActor();
	~AtlasViewActor();

	virtual void Update(float realFrameLength);
	virtual void Render();
	virtual CCamera& GetCamera();
	virtual CSimulation2* GetSimulation2();
	virtual entity_id_t GetEntityId(AtlasMessage::ObjectID obj);
	virtual bool WantsHighFramerate();
	virtual void SetEnabled(bool enabled);

	virtual void SetParam(const std::wstring& name, bool value);
	virtual void SetParam(const std::wstring& name, int value);
	virtual void SetParam(const std::wstring& name, const AtlasMessage::Color& value);

	void SetSpeedMultiplier(float speedMultiplier);
	ActorViewer& GetActorViewer();

private:
	float m_SpeedMultiplier;
	CCamera m_Camera;
	ActorViewer* m_ActorViewer;
};

#endif // INCLUDED_VIEW
