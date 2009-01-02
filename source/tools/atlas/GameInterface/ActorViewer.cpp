#include "precompiled.h"

#include "ActorViewer.h"

#include "View.h"

#include "graphics/ColladaManager.h"
#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "graphics/Patch.h"
#include "graphics/SkeletonAnim.h"
#include "graphics/SkeletonAnimDef.h"
#include "graphics/SkeletonAnimManager.h"
#include "graphics/Terrain.h"
#include "graphics/TextureEntry.h"
#include "graphics/TextureManager.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "maths/MathUtil.h"
#include "ps/Font.h"
#include "ps/GameSetup/Config.h"
#include "ps/ProfileViewer.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"
#include "renderer/SkyManager.h"

struct ActorViewerImpl : public Scene, noncopyable
{
	ActorViewerImpl()
		: Unit(NULL), ColladaManager(), MeshManager(ColladaManager), SkeletonAnimManager(ColladaManager),
		ObjectManager(MeshManager, SkeletonAnimManager)
	{
	}

	CUnit* Unit;
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

	// Simplistic implementation of the Scene interface
	void EnumerateObjects(const CFrustum& UNUSED(frustum), SceneCollector* c)
	{
		if (GroundEnabled)
			c->Submit(Terrain.GetPatch(0, 0));

		if (Unit)
			c->SubmitRecursive(Unit->GetModel());
	}
};

ActorViewer::ActorViewer()
: m(*new ActorViewerImpl())
{
	m.Unit = NULL;
	m.WalkEnabled = false;
	m.Background = SColor4ub(255, 255, 255, 255);

	// Set up the renderer
	g_TexMan.LoadTerrainTextures();
	g_Renderer.LoadAlphaMaps();
	g_Renderer.GetSkyManager()->m_RenderSky = false;
	// (TODO: should these be unloaded properly some time? and what should
	// happen if we want the actor viewer and scenario editor loaded at
	// the same time?)

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
}

ActorViewer::~ActorViewer()
{
	delete m.Unit;
	delete &m;
}

CUnit* ActorViewer::GetUnit()
{
	return m.Unit;
}

void ActorViewer::UnloadObjects()
{
	m.ObjectManager.UnloadObjects();
}

void ActorViewer::SetActor(const CStrW& id, const CStrW& animation)
{
	bool needsAnimReload = false;

	if (! m.Unit || id != m.CurrentUnitID)
	{
		delete m.Unit;
		m.Unit = NULL;

		// If there's no actor to display, return with nothing loaded
		if (id.empty())
			return;

		m.Unit = CUnit::Create((CStr)id, NULL, std::set<CStr>(), m.ObjectManager);

		if (! m.Unit)
			return;

		float angle = PI;
		CMatrix3D mat;
		mat.SetYRotation(angle + PI);
		mat.Translate(CELL_SIZE * PATCH_SIZE/2, 0.f, CELL_SIZE * PATCH_SIZE/2);
		m.Unit->GetModel()->SetTransform(mat);
		m.Unit->GetModel()->ValidatePosition();

		needsAnimReload = true;
	}

	if (animation != m.CurrentUnitAnim)
		needsAnimReload = true;

	if (needsAnimReload)
	{
		CStr anim = ((CStr)animation).LowerCase();

		float speed;
		// TODO: this is just copied from template_unit.xml and isn't the
		// same for all units. But we don't know anything about entities here,
		// so what to do?
		if (anim == "walk")
			speed = 7.f;
		else if (anim == "run")
			speed = 12.f;
		else
			speed = 0.f;
		m.CurrentSpeed = speed;

		m.Unit->SetEntitySelection(anim);
		m.Unit->SetRandomAnimation(anim, false, speed);
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

	g_Renderer.BeginFrame();

	// Find the centre of the interesting region, in the middle of the patch
	// and half way up the model (assuming there is one)
	CVector3D centre;
	if (m.Unit)
		m.Unit->GetModel()->GetBounds().GetCentre(centre);
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

	CFont font("console");
	font.Bind();

	g_ProfileViewer.RenderProfile();

	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	g_Renderer.EndFrame();

	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWS, oldShadows);

	ogl_WarnIfError();
}

void ActorViewer::Update(float dt)
{
	if (m.Unit)
	{
		m.Unit->GetModel()->Update(dt);

		CMatrix3D mat = m.Unit->GetModel()->GetTransform();

		if (m.WalkEnabled && m.CurrentSpeed)
		{
			// Move the model by speed*dt forwards
			float z = mat.GetTranslation().Z;
			z -= m.CurrentSpeed*dt;
			// Wrap at the edges, so it doesn't run off into the horizon
			if (z < CELL_SIZE*PATCH_SIZE * 0.4f)
				z = CELL_SIZE*PATCH_SIZE * 0.6f;
			mat.Translate(0.f, 0.f, z - mat.GetTranslation().Z);
		}

		m.Unit->GetModel()->SetTransform(mat);
		m.Unit->GetModel()->ValidatePosition();
	}
}

bool ActorViewer::HasAnimation() const
{
	if (m.Unit &&
		m.Unit->GetModel()->GetAnimation() &&
		m.Unit->GetModel()->GetAnimation()->m_AnimDef &&
		m.Unit->GetModel()->GetAnimation()->m_AnimDef->GetNumFrames() > 1)
		return true;

	if (m.Unit && m.WalkEnabled && m.CurrentSpeed)
		return true;

	return false;
}
