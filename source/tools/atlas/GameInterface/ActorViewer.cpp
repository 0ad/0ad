/* Copyright (C) 2010 Wildfire Games.
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
#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "graphics/Patch.h"
#include "graphics/SkeletonAnimManager.h"
#include "graphics/Terrain.h"
#include "graphics/TextureEntry.h"
#include "graphics/TextureManager.h"
#include "graphics/UnitManager.h"
#include "maths/MathUtil.h"
#include "ps/Font.h"
#include "ps/GameSetup/Config.h"
#include "ps/ProfileViewer.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"
#include "renderer/SkyManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpVisual.h"

struct ActorViewerImpl : public Scene
{
	NONCOPYABLE(ActorViewerImpl);
public:
	ActorViewerImpl()
		: Entity(INVALID_ENTITY), Terrain(), ColladaManager(), MeshManager(ColladaManager), SkeletonAnimManager(ColladaManager),
		ObjectManager(MeshManager, SkeletonAnimManager), UnitManager(), Simulation2(&UnitManager, &Terrain)
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

	// Simplistic implementation of the Scene interface
	void EnumerateObjects(const CFrustum& frustum, SceneCollector* c)
	{
		if (GroundEnabled)
			c->Submit(Terrain.GetPatch(0, 0));

		Simulation2.RenderSubmit(*c, frustum, false);
	}
};

ActorViewer::ActorViewer()
: m(*new ActorViewerImpl())
{
	m.WalkEnabled = false;
	m.GroundEnabled = true;
	m.ShadowsEnabled = g_Renderer.GetOptionBool(CRenderer::OPT_SHADOWS);
	m.Background = SColor4ub(255, 255, 255, 255);

	// Create a tiny empty piece of terrain, just so we can put shadows
	// on it without having to think too hard
	m.Terrain.Initialize(1, NULL);
	CTextureEntry* tex = g_TexMan.FindTexture("whiteness");
	if (tex)
	{
		CPatch* patch = m.Terrain.GetPatch(0, 0);
		for (ssize_t i = 0; i < PATCH_SIZE; ++i)
		{
			for (ssize_t j = 0; j < PATCH_SIZE; ++j)
			{
				CMiniPatch& mp = patch->m_MiniPatches[i][j];
				mp.Tex1 = tex->GetHandle();
				mp.Tex1Priority = 0;
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
			cmpPosition->JumpTo(entity_pos_t::FromInt(CELL_SIZE*PATCH_SIZE/2), entity_pos_t::FromInt(CELL_SIZE*PATCH_SIZE/2));
			cmpPosition->SetYRotation(entity_angle_t::FromFloat((float)M_PI));
		}
		needsAnimReload = true;
	}

	if (animation != m.CurrentUnitAnim)
		needsAnimReload = true;

	if (needsAnimReload)
	{
		CStr anim = CStr(animation).LowerCase();

		float speed;
		// TODO: this is just copied from template_unit.xml and isn't the
		// same for all units. We ought to get it from the entity definition
		// (if there is one)
		if (anim == "walk")
			speed = 7.f;
		else if (anim == "run")
			speed = 12.f;
		else
			speed = 0.f;
		m.CurrentSpeed = speed;

		CmpPtr<ICmpVisual> cmpVisual(m.Simulation2, m.Entity);
		if (!cmpVisual.null())
		{
			// TODO: SetEntitySelection(anim)
			cmpVisual->SelectAnimation(anim, false, speed);
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

	g_Renderer.BeginFrame();

	// Find the centre of the interesting region, in the middle of the patch
	// and half way up the model (assuming there is one)
	CVector3D centre;
	CmpPtr<ICmpVisual> cmpVisual(m.Simulation2, m.Entity);
	if (!cmpVisual.null())
		cmpVisual->GetBounds().GetCentre(centre);
	else
		centre.Y = 0.f;
	centre.X = centre.Z = CELL_SIZE * PATCH_SIZE/2;

	CCamera camera = View::GetView_Actor()->GetCamera();
	camera.m_Orientation.Translate(centre.X, centre.Y, centre.Z);
	camera.UpdateFrustum();
	
	g_Renderer.SetSceneCamera(camera, camera);

	g_Renderer.RenderScene(&m);

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

	CFont font(L"console");
	font.Bind();

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

	ogl_WarnIfError();
}

void ActorViewer::Update(float dt)
{
	m.Simulation2.Update(dt);
	m.Simulation2.Interpolate(dt);

	if (m.WalkEnabled && m.CurrentSpeed)
	{
		CmpPtr<ICmpPosition> cmpPosition(m.Simulation2, m.Entity);
		if (!cmpPosition.null())
		{
			// Move the model by speed*dt forwards
			float z = cmpPosition->GetPosition().Z.ToFloat();
			z -= m.CurrentSpeed*dt;
			// Wrap at the edges, so it doesn't run off into the horizon
			if (z < CELL_SIZE*PATCH_SIZE * 0.4f)
				z = CELL_SIZE*PATCH_SIZE * 0.6f;
			cmpPosition->JumpTo(cmpPosition->GetPosition().X, entity_pos_t::FromFloat(z));
		}
	}
}
