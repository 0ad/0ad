/* Copyright (C) 2009 Wildfire Games.
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

class SimState;

class ViewGame : public View
{
public:
	ViewGame();
	virtual ~ViewGame();
	virtual void Update(float frameLength);
	virtual void Render();
	virtual CCamera& GetCamera();
	virtual CUnit* GetUnit(AtlasMessage::ObjectID id);
	virtual bool WantsHighFramerate();

	void SetSpeedMultiplier(float speed);
	void SaveState(const std::wstring& label, bool onlyEntities);
	void RestoreState(const std::wstring& label);

private:
	float m_SpeedMultiplier;
	std::map<std::wstring, SimState*> m_SavedStates;
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

#endif // INCLUDED_VIEW
