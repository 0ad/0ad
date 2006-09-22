#include "precompiled.h"

#include "ActorViewer.h"

#include "View.h"

#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "graphics/TextureEntry.h"
#include "graphics/TextureManager.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "maths/MathUtil.h"
#include "ps/GameSetup/Config.h"
#include "renderer/Renderer.h"
#include "renderer/SkyManager.h"

struct ActorViewerImpl
{
	CUnit* Unit;
	CStrW CurrentUnitID;
	CStrW CurrentUnitAnim;
	float CurrentSpeed;
	
	CTerrain Terrain;
};

ActorViewer::ActorViewer()
: m(*new ActorViewerImpl())
{
	m.Unit = NULL;

	// Set up the renderer
	g_TexMan.LoadTerrainTextures();
	g_Renderer.LoadAlphaMaps();
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
		for (int i = 0; i < PATCH_SIZE; ++i)
		{
			for (int j = 0; j < PATCH_SIZE; ++j)
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

		m.Unit = CUnit::Create((CStr)id, NULL, std::set<CStr>());

		if (! m.Unit)
			return;

		float angle = PI;
		float s = sin(angle);
		float c = cos(angle);
		CMatrix3D mat;
		mat._11 = -c;   mat._12 = 0.0f; mat._13 = -s;   mat._14 = CELL_SIZE * PATCH_SIZE/2;
		mat._21 = 0.0f; mat._22 = 1.0f; mat._23 = 0.0f; mat._24 = 0.0f;
		mat._31 = s;    mat._32 = 0.0f; mat._33 = -c;   mat._34 = CELL_SIZE * PATCH_SIZE/2;
		mat._41 = 0.0f; mat._42 = 0.0f; mat._43 = 0.0f; mat._44 = 1.0f;
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

// TODO: don't have this duplicated from GameView
void SubmitModelRecursive(CModel* model)
{
	g_Renderer.Submit(model);

	const std::vector<CModel::Prop>& props = model->GetProps();
	for (size_t i = 0; i < props.size(); ++i)
		SubmitModelRecursive(props[i].m_Model);
}

void ActorViewer::Render()
{
	m.Terrain.MakeDirty(RENDERDATA_UPDATE_COLOR);

	g_Renderer.SetClearColor(0xFFFFFFFFu);

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

	g_Renderer.Submit(m.Terrain.GetPatch(0, 0));

	if (m.Unit)
		SubmitModelRecursive(m.Unit->GetModel());

	g_Renderer.FlushFrame();

	g_Renderer.EndFrame();

	oglCheck();
}

void ActorViewer::Update(float dt)
{
	if (m.Unit)
	{
		m.Unit->GetModel()->Update(dt);

		// Move the model by speed*dt forwards
		CMatrix3D mat = m.Unit->GetModel()->GetTransform();
		float z = mat.GetTranslation().Z;
		z -= m.CurrentSpeed*dt;
		// Wrap at the edges, so it doesn't run off into the horizon
		if (z < CELL_SIZE*PATCH_SIZE * 0.4f)
			z = CELL_SIZE*PATCH_SIZE * 0.6f;

		mat.Translate(0.f, 0.f, z - mat.GetTranslation().Z);
		m.Unit->GetModel()->SetTransform(mat);
		m.Unit->GetModel()->ValidatePosition();
	}
}
