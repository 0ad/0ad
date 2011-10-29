/* Copyright (C) 2011 Wildfire Games.
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

#include "precompiled.h"

#include "ActorViewer.h"

#include "View.h"

#include "graphics/ColladaManager.h"
#include "graphics/LOSTexture.h"
#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "graphics/ParticleManager.h"
#include "graphics/Patch.h"
#include "graphics/SkeletonAnimManager.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TerritoryTexture.h"
#include "graphics/UnitManager.h"
#include "maths/MathUtil.h"
#include "ps/Font.h"
#include "ps/GameSetup/Config.h"
#include "ps/ProfileViewer.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpUnitMotion.h"
#include "simulation2/components/ICmpVisual.h"

struct ActorViewerImpl : public Scene
{
	NONCOPYABLE(ActorViewerImpl);
public:
	ActorViewerImpl() :
		Entity(INVALID_ENTITY),
		Terrain(),
		ColladaManager(),
		MeshManager(ColladaManager),
		SkeletonAnimManager(ColladaManager),
		UnitManager(),
		Simulation2(&UnitManager, &Terrain),
		ObjectManager(MeshManager, SkeletonAnimManager, Simulation2),
		LOSTexture(Simulation2),
		TerritoryTexture(Simulation2)
	{
		UnitManager.SetObjectManager(ObjectManager);
	}

	entity_id_t Entity;
	CStrW CurrentUnitID;
	CStrW CurrentUnitAnim;
	float CurrentSpeed;
	bool WalkEnabled;
	bool GroundEnabled;
	bool ShadowsEnabled;

	SColor4ub Background;
	
	CTerrain Terrain;

	CColladaManager ColladaManager;
	CMeshManager MeshManager;
	CSkeletonAnimManager SkeletonAnimManager;
	CObjectManager ObjectManager;
	CUnitManager UnitManager;
	CSimulation2 Simulation2;
	CLOSTexture LOSTexture;
	CTerritoryTexture TerritoryTexture;

	// Simplistic implementation of the Scene interface
	virtual void EnumerateObjects(const CFrustum& frustum, SceneCollector* c)
	{
		if (GroundEnabled)
		{
			for (ssize_t pj = 0; pj < Terrain.GetPatchesPerSide(); ++pj)
				for (ssize_t pi = 0; pi < Terrain.GetPatchesPerSide(); ++pi)
					c->Submit(Terrain.GetPatch(pi, pj));
		}

		Simulation2.RenderSubmit(*c, frustum, false);
	}

	virtual CLOSTexture& GetLOSTexture()
	{
		return LOSTexture;
	}

	virtual CTerritoryTexture& GetTerritoryTexture()
	{
		return TerritoryTexture;
	}
};

ActorViewer::ActorViewer()
: m(*new ActorViewerImpl())
{
	m.WalkEnabled = false;
	m.GroundEnabled = true;
	m.ShadowsEnabled = g_Renderer.GetOptionBool(CRenderer::OPT_SHADOWS);
	m.Background = SColor4ub(0, 0, 0, 255);

	// Create a tiny empty piece of terrain, just so we can put shadows
	// on it without having to think too hard
	m.Terrain.Initialize(2, NULL);
	CTerrainTextureEntry* tex = g_TexMan.FindTexture("whiteness");
	if (tex)
	{
		for (ssize_t pi = 0; pi < m.Terrain.GetPatchesPerSide(); ++pi)
		{
			for (ssize_t pj = 0; pj < m.Terrain.GetPatchesPerSide(); ++pj)
			{
				CPatch* patch = m.Terrain.GetPatch(pi, pj);
				for (ssize_t i = 0; i < PATCH_SIZE; ++i)
				{
					for (ssize_t j = 0; j < PATCH_SIZE; ++j)
					{
						CMiniPatch& mp = patch->m_MiniPatches[i][j];
						mp.Tex = tex;
						mp.Priority = 0;
					}
				}
			}
		}
	}
	else
	{
		debug_warn(L"Failed to load whiteness texture");
	}

	// Start the simulation
	m.Simulation2.LoadDefaultScripts();
	m.Simulation2.ResetState();

	// Tell the simulation we've already loaded the terrain
	CmpPtr<ICmpTerrain> cmpTerrain(m.Simulation2, SYSTEM_ENTITY);
	if (!cmpTerrain.null())
		cmpTerrain->ReloadTerrain();

	CmpPtr<ICmpRangeManager> cmpRangeManager(m.Simulation2, SYSTEM_ENTITY);
	if (!cmpRangeManager.null())
		cmpRangeManager->SetLosRevealAll(-1, true);
}

ActorViewer::~ActorViewer()
{
	delete &m;
}

CSimulation2* ActorViewer::GetSimulation2()
{
	return &m.Simulation2;
}

entity_id_t ActorViewer::GetEntity()
{
	return m.Entity;
}

void ActorViewer::UnloadObjects()
{
	m.ObjectManager.UnloadObjects();
}

void ActorViewer::SetActor(const CStrW& name, const CStrW& animation)
{
	bool needsAnimReload = false;

	CStrW id = name;

	// Recreate the entity, if we don't have one or if the new one is different
	if (m.Entity == INVALID_ENTITY || id != m.CurrentUnitID)
	{
		// Delete the old entity (if any)
		if (m.Entity != INVALID_ENTITY)
		{
			m.Simulation2.DestroyEntity(m.Entity);
			m.Simulation2.FlushDestroyedEntities();
			m.Entity = INVALID_ENTITY;
		}

		// If there's no actor to display, return with nothing loaded
		if (id.empty())
			return;

		m.Entity = m.Simulation2.AddEntity(L"preview|" + id);

		if (m.Entity == INVALID_ENTITY)
			return;

		CmpPtr<ICmpPosition> cmpPosition(m.Simulation2, m.Entity);
		if (!cmpPosition.null())
		{
			ssize_t c = CELL_SIZE * m.Terrain.GetPatchesPerSide()*PATCH_SIZE/2;
			cmpPosition->JumpTo(entity_pos_t::FromInt(c), entity_pos_t::FromInt(c));
			cmpPosition->SetYRotation(entity_angle_t::Pi());
		}
		needsAnimReload = true;
	}

	if (animation != m.CurrentUnitAnim)
		needsAnimReload = true;

	if (needsAnimReload)
	{
		CStr anim = animation.ToUTF8().LowerCase();

		// Emulate the typical simulation animation behaviour
		float speed;
		float repeattime = 0.f;
		if (anim == "walk")
		{
			CmpPtr<ICmpUnitMotion> cmpUnitMotion(m.Simulation2, m.Entity);
			if (!cmpUnitMotion.null())
				speed = cmpUnitMotion->GetWalkSpeed().ToFloat();
			else
				speed = 7.f; // typical unit speed

			m.CurrentSpeed = speed;
		}
		else if (anim == "run")
		{
			CmpPtr<ICmpUnitMotion> cmpUnitMotion(m.Simulation2, m.Entity);
			if (!cmpUnitMotion.null())
				speed = cmpUnitMotion->GetRunSpeed().ToFloat();
			else
				speed = 12.f; // typical unit speed

			m.CurrentSpeed = speed;
		}
		else if (anim == "melee")
		{
			speed = 1.f; // speed will be ignored if we have a repeattime
			m.CurrentSpeed = 0.f;

			CStr code = "var cmp = Engine.QueryInterface("+CStr::FromUInt(m.Entity)+", IID_Attack); " +
				"if (cmp) cmp.GetTimers(cmp.GetBestAttack()).repeat; else 0;";
			m.Simulation2.GetScriptInterface().Eval(code.c_str(), repeattime);
		}
		else
		{
			// Play the animation at normal speed, but movement speed is zero
			speed = 1.f;
			m.CurrentSpeed = 0.f;
		}

		CStr sound;
		if (anim == "melee")
			sound = "attack";
		else if (anim == "build")
			sound = "build";
		else if (anim.Find("gather_") == 0)
			sound = anim;

		std::wstring soundgroup;
		if (!sound.empty())
		{
			CStr code = "var cmp = Engine.QueryInterface("+CStr::FromUInt(m.Entity)+", IID_Sound); " +
				"if (cmp) cmp.GetSoundGroup('"+sound+"'); else '';";
			m.Simulation2.GetScriptInterface().Eval(code.c_str(), soundgroup);
		}

		CmpPtr<ICmpVisual> cmpVisual(m.Simulation2, m.Entity);
		if (!cmpVisual.null())
		{
			// TODO: SetEntitySelection(anim)
			cmpVisual->SelectAnimation(anim, false, fixed::FromFloat(speed), soundgroup);
			if (repeattime)
				cmpVisual->SetAnimationSyncRepeat(fixed::FromFloat(repeattime));
		}
	}

	m.CurrentUnitID = id;
	m.CurrentUnitAnim = animation;
}

void ActorViewer::SetBackgroundColour(const SColor4ub& colour)
{
	m.Background = colour;
	m.Terrain.SetBaseColour(colour);
}

void ActorViewer::SetWalkEnabled(bool enabled)    { m.WalkEnabled = enabled; }
void ActorViewer::SetGroundEnabled(bool enabled)  { m.GroundEnabled = enabled; }
void ActorViewer::SetShadowsEnabled(bool enabled) { m.ShadowsEnabled = enabled; }

void ActorViewer::SetStatsEnabled(bool enabled)
{
	if (enabled)
		g_ProfileViewer.ShowTable("renderer");
	else
		g_ProfileViewer.ShowTable("");
}

void ActorViewer::Render()
{
	m.Terrain.MakeDirty(RENDERDATA_UPDATE_COLOR);

	g_Renderer.SetClearColor(m.Background);

	// Disable shadows locally (avoid clobbering global state)
	bool oldShadows = g_Renderer.GetOptionBool(CRenderer::OPT_SHADOWS);
	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWS, m.ShadowsEnabled);

	bool oldSky = g_Renderer.GetSkyManager()->m_RenderSky;
	g_Renderer.GetSkyManager()->m_RenderSky = false;

	bool oldWater = g_Renderer.GetWaterManager()->m_RenderWater;
	g_Renderer.GetWaterManager()->m_RenderWater = false;

	g_Renderer.BeginFrame();

	// Find the centre of the interesting region, in the middle of the patch
	// and half way up the model (assuming there is one)
	CVector3D centre;
	CmpPtr<ICmpVisual> cmpVisual(m.Simulation2, m.Entity);
	if (!cmpVisual.null())
		cmpVisual->GetBounds().GetCentre(centre);
	else
		centre.Y = 0.f;
	centre.X = centre.Z = CELL_SIZE * m.Terrain.GetPatchesPerSide()*PATCH_SIZE/2;

	CCamera camera = View::GetView_Actor()->GetCamera();
	camera.m_Orientation.Translate(centre.X, centre.Y, centre.Z);
	camera.UpdateFrustum();
	
	g_Renderer.SetSceneCamera(camera, camera);

	g_Renderer.RenderScene(m);

	// ....

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_TEXTURE_2D);

	g_ProfileViewer.RenderProfile();

	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	g_Renderer.EndFrame();

	// Restore the old renderer state
	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWS, oldShadows);
	g_Renderer.GetSkyManager()->m_RenderSky = oldSky;
	g_Renderer.GetWaterManager()->m_RenderWater = oldWater;

	ogl_WarnIfError();
}

void ActorViewer::Update(float dt)
{
	m.Simulation2.Update((int)(dt*1000));
	m.Simulation2.Interpolate(dt, 0);
	g_Renderer.GetParticleManager().Interpolate(dt);

	if (m.WalkEnabled && m.CurrentSpeed)
	{
		CmpPtr<ICmpPosition> cmpPosition(m.Simulation2, m.Entity);
		if (!cmpPosition.null())
		{
			// Move the model by speed*dt forwards
			float z = cmpPosition->GetPosition().Z.ToFloat();
			z -= m.CurrentSpeed*dt;
			// Wrap at the edges, so it doesn't run off into the horizon
			ssize_t c = CELL_SIZE * m.Terrain.GetPatchesPerSide()*PATCH_SIZE/2;
			if (z < c - CELL_SIZE*PATCH_SIZE * 0.1f)
				z = c + CELL_SIZE*PATCH_SIZE * 0.1f;
			cmpPosition->JumpTo(cmpPosition->GetPosition().X, entity_pos_t::FromFloat(z));
		}
	}
}
