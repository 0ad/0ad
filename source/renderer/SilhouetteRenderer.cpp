/* Copyright (C) 2014 Wildfire Games.
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

#include "SilhouetteRenderer.h"

#include "graphics/Camera.h"
#include "graphics/HFTracer.h"
#include "graphics/Model.h"
#include "graphics/Patch.h"
#include "graphics/ShaderManager.h"
#include "maths/MathUtil.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"

#include <cfloat>

// For debugging
static const bool g_DisablePreciseIntersections = false;

SilhouetteRenderer::SilhouetteRenderer()
{
	m_DebugEnabled = false;
}

void SilhouetteRenderer::AddOccluder(CPatch* patch)
{
	m_SubmittedPatchOccluders.push_back(patch);
}

void SilhouetteRenderer::AddOccluder(CModel* model)
{
	m_SubmittedModelOccluders.push_back(model);
}

void SilhouetteRenderer::AddCaster(CModel* model)
{
	m_SubmittedModelCasters.push_back(model);
}

/*
 * Silhouettes are the solid-coloured versions of units that are rendered when
 * standing behind a building or terrain, so the player won't lose them.
 *
 * The rendering is done in CRenderer::RenderSilhouettes, by rendering the
 * units (silhouette casters) and buildings/terrain (silhouette occluders)
 * in an extra pass using depth and stencil buffers. It's very inefficient to
 * render those objects when they're not actually going to contribute to a
 * silhouette.
 *
 * This class is responsible for finding the subset of casters/occluders
 * that might contribute to a silhouette and will need to be rendered.
 *
 * The algorithm is largely based on sweep-and-prune for detecting intersection
 * along a single axis:
 *
 * First we compute the 2D screen-space bounding box of every occluder, and
 * their minimum distance from the camera. We also compute the screen-space
 * position of each caster (approximating them as points, which is not perfect
 * but almost always good enough).
 *
 * We split each occluder's screen-space bounds into a left ('in') edge and
 * right ('out') edge. We put those edges plus the caster points into a list,
 * and sort by x coordinate.
 *
 * Then we walk through the list, maintaining an active set of occluders.
 * An 'in' edge will add an occluder to the set, an 'out' edge will remove it.
 * When we reach a caster point, the active set contains all the occluders that
 * intersect it in x. We do a quick test of y and depth coordinates against
 * each occluder in the set. If they pass that test, we do a more precise ray
 * vs bounding box test (for model occluders) or ray vs patch (for terrain
 * occluders) to see if we really need to render that caster and occluder.
 *
 * Performance relies on the active set being quite small. Given the game's
 * typical occluder sizes and camera angles, this works out okay.
 *
 * We have to do precise ray/patch intersection tests for terrain, because
 * if we just used the patch's bounding box, pretty much every unit would
 * be seen as intersecting the patch it's standing on.
 *
 * We store screen-space coordinates as 14-bit integers (0..16383) because
 * that lets us pack and sort the edge/point list efficiently.
 */

static const int MAX_COORD = 16384;

struct Occluder
{
	CRenderableObject* renderable;
	bool isPatch;
	u16 x0, y0, x1, y1;
	float z;
	bool rendered;
};

struct Caster
{
	CModel* model;
	u16 x, y;
	float z;
	bool rendered;
};

enum { EDGE_IN, EDGE_OUT, POINT };

// Entry is essentially:
//   struct Entry {
//     u16 id; // index into occluders array
//     u16 type : 2;
//     u16 x : 14;
//  };
// where x is in the most significant bits, so that sorting as a uint32_t
// is the same as sorting by x. To avoid worrying about endianness and the
// compiler's ability to handle bitfields efficiently, we use uint32_t instead
// of the actual struct.

typedef uint32_t Entry;

static Entry EntryCreate(int type, u16 id, u16 x) { return (x << 18) | (type << 16) | id; }
static int EntryGetId(Entry e) { return e & 0xffff; }
static int EntryGetType(Entry e) { return (e >> 16) & 3; }

struct ActiveList
{
	std::vector<u16> m_Ids;

	void Add(u16 id)
	{
		m_Ids.push_back(id);
	}

	void Remove(u16 id)
	{
		ssize_t sz = m_Ids.size();
		for (ssize_t i = sz-1; i >= 0; --i)
		{
			if (m_Ids[i] == id)
			{
				m_Ids[i] = m_Ids[sz-1];
				m_Ids.pop_back();
				return;
			}
		}
		debug_warn(L"Failed to find id");
	}
};

static void ComputeScreenBounds(Occluder& occluder, const CBoundingBoxAligned& bounds, CMatrix3D& proj)
{
	int x0 = INT_MAX, y0 = INT_MAX, x1 = INT_MIN, y1 = INT_MIN;
	float z0 = FLT_MAX;
	for (size_t ix = 0; ix <= 1; ix++)
	{
		for (size_t iy = 0; iy <= 1; iy++)
		{
			for (size_t iz = 0; iz <= 1; iz++)
			{
				CVector4D vec(bounds[ix].X, bounds[iy].Y, bounds[iz].Z, 1.0f);
				CVector4D svec = proj.Transform(vec);
				x0 = std::min(x0, MAX_COORD/2 + (int)(MAX_COORD/2 * svec.X / svec.W));
				y0 = std::min(y0, MAX_COORD/2 + (int)(MAX_COORD/2 * svec.Y / svec.W));
				x1 = std::max(x1, MAX_COORD/2 + (int)(MAX_COORD/2 * svec.X / svec.W));
				y1 = std::max(y1, MAX_COORD/2 + (int)(MAX_COORD/2 * svec.Y / svec.W));
				z0 = std::min(z0, svec.Z / svec.W);
			}
		}
	}
	// TODO: there must be a quicker way to do this than to test every vertex,
	// given the symmetry of the bounding box

	occluder.x0 = clamp(x0, 0, MAX_COORD-1);
	occluder.y0 = clamp(y0, 0, MAX_COORD-1);
	occluder.x1 = clamp(x1, 0, MAX_COORD-1);
	occluder.y1 = clamp(y1, 0, MAX_COORD-1);
	occluder.z = z0;
}

static void ComputeScreenPos(Caster& caster, const CVector3D& pos, CMatrix3D& proj)
{
	CVector4D vec(pos.X, pos.Y, pos.Z, 1.0f);
	CVector4D svec = proj.Transform(vec);
	int x = MAX_COORD/2 + (int)(MAX_COORD/2 * svec.X / svec.W);
	int y = MAX_COORD/2 + (int)(MAX_COORD/2 * svec.Y / svec.W);
	float z = svec.Z / svec.W;

	caster.x = clamp(x, 0, MAX_COORD-1);
	caster.y = clamp(y, 0, MAX_COORD-1);
	caster.z = z;
}

void SilhouetteRenderer::ComputeSubmissions(const CCamera& camera)
{
	PROFILE3("compute silhouettes");

	m_DebugBounds.clear();
	m_DebugRects.clear();
	m_DebugSpheres.clear();

	m_VisiblePatchOccluders.clear();
	m_VisibleModelOccluders.clear();
	m_VisibleModelCasters.clear();

	std::vector<Occluder> occluders;
	std::vector<Caster> casters;
	std::vector<Entry> entries;

	occluders.reserve(m_SubmittedModelOccluders.size() + m_SubmittedPatchOccluders.size());
	casters.reserve(m_SubmittedModelCasters.size());
	entries.reserve((m_SubmittedModelOccluders.size() + m_SubmittedPatchOccluders.size()) * 2 + m_SubmittedModelCasters.size());

	CMatrix3D proj = camera.GetViewProjection();

	// Bump the positions of unit casters upwards a bit, so they're not always
	// detected as intersecting the terrain they're standing on
	CVector3D posOffset(0.0f, 0.1f, 0.0f);

#if 0
	// For debugging ray-patch intersections - casts a ton of rays and draws
	// a sphere where they intersect
	extern int g_xres, g_yres;
	for (int y = 0; y < g_yres; y += 8)
	{
		for (int x = 0; x < g_xres; x += 8)
		{
			SOverlaySphere sphere;
			sphere.m_Color = CColor(1, 0, 0, 1);
			sphere.m_Radius = 0.25f;
			sphere.m_Center = camera.GetWorldCoordinates(x, y, false);

			CVector3D origin, dir;
			camera.BuildCameraRay(x, y, origin, dir);

			for (size_t i = 0; i < m_SubmittedPatchOccluders.size(); ++i)
			{
				CPatch* occluder = m_SubmittedPatchOccluders[i];
				if (CHFTracer::PatchRayIntersect(occluder, origin, dir, &sphere.m_Center))
					sphere.m_Color = CColor(0, 0, 1, 1);
			}
			m_DebugSpheres.push_back(sphere);
		}
	}
#endif

	{
		PROFILE("compute bounds");

		for (size_t i = 0; i < m_SubmittedModelOccluders.size(); ++i)
		{
			CModel* occluder = m_SubmittedModelOccluders[i];

			Occluder d;
			d.renderable = occluder;
			d.isPatch = false;
			d.rendered = false;
			ComputeScreenBounds(d, occluder->GetWorldBounds(), proj);

			// Skip zero-sized occluders, so we don't need to worry about EDGE_OUT
			// getting sorted before EDGE_IN
			if (d.x0 == d.x1 || d.y0 == d.y1)
				continue;

			size_t id = occluders.size();
			occluders.push_back(d);

			entries.push_back(EntryCreate(EDGE_IN, id, d.x0));
			entries.push_back(EntryCreate(EDGE_OUT, id, d.x1));
		}

		for (size_t i = 0; i < m_SubmittedPatchOccluders.size(); ++i)
		{
			CPatch* occluder = m_SubmittedPatchOccluders[i];

			Occluder d;
			d.renderable = occluder;
			d.isPatch = true;
			d.rendered = false;
			ComputeScreenBounds(d, occluder->GetWorldBounds(), proj);

			// Skip zero-sized occluders
			if (d.x0 == d.x1 || d.y0 == d.y1)
				continue;

			size_t id = occluders.size();
			occluders.push_back(d);

			entries.push_back(EntryCreate(EDGE_IN, id, d.x0));
			entries.push_back(EntryCreate(EDGE_OUT, id, d.x1));
		}

		for (size_t i = 0; i < m_SubmittedModelCasters.size(); ++i)
		{
			CModel* model = m_SubmittedModelCasters[i];
			CVector3D pos = model->GetTransform().GetTranslation() + posOffset;

			Caster d;
			d.model = model;
			d.rendered = false;
			ComputeScreenPos(d, pos, proj);

			size_t id = casters.size();
			casters.push_back(d);

			entries.push_back(EntryCreate(POINT, id, d.x));
		}
	}

	// Make sure the u16 id didn't overflow
	ENSURE(occluders.size() < 65536 && casters.size() < 65536);

	{
		PROFILE("sorting");
		std::sort(entries.begin(), entries.end());
	}

	{
		PROFILE("sweeping");

		ActiveList active;
		CVector3D cameraPos = camera.GetOrientation().GetTranslation();

		for (size_t i = 0; i < entries.size(); ++i)
		{
			Entry e = entries[i];
			int type = EntryGetType(e);
			u16 id = EntryGetId(e);
			if (type == EDGE_IN)
				active.Add(id);
			else if (type == EDGE_OUT)
				active.Remove(id);
			else
			{
				Caster& caster = casters[id];
				for (size_t j = 0; j < active.m_Ids.size(); ++j)
				{
					Occluder& occluder = occluders[active.m_Ids[j]];

					if (caster.y < occluder.y0 || caster.y > occluder.y1)
						continue;

					if (caster.z < occluder.z)
						continue;

					// No point checking further if both are already being rendered
					if (caster.rendered && occluder.rendered)
						continue;

					if (!g_DisablePreciseIntersections)
					{
						CVector3D pos = caster.model->GetTransform().GetTranslation() + posOffset;
						if (occluder.isPatch)
						{
							CPatch* patch = static_cast<CPatch*>(occluder.renderable);
							if (!CHFTracer::PatchRayIntersect(patch, pos, cameraPos - pos, NULL))
								continue;
						}
						else
						{
							float tmin, tmax;
							if (!occluder.renderable->GetWorldBounds().RayIntersect(pos, cameraPos - pos, tmin, tmax))
								continue;
						}
					}

					caster.rendered = true;
					occluder.rendered = true;
				}
			}
		}
	}

	if (m_DebugEnabled)
	{
		for (size_t i = 0; i < occluders.size(); ++i)
		{
			DebugRect r;
			r.color = occluders[i].rendered ? CColor(1.0f, 1.0f, 0.0f, 1.0f) : CColor(0.2f, 0.2f, 0.0f, 1.0f);
			r.x0 = occluders[i].x0;
			r.y0 = occluders[i].y0;
			r.x1 = occluders[i].x1;
			r.y1 = occluders[i].y1;
			m_DebugRects.push_back(r);

			DebugBounds b;
			b.color = r.color;
			b.bounds = occluders[i].renderable->GetWorldBounds();
			m_DebugBounds.push_back(b);
		}
	}

	for (size_t i = 0; i < occluders.size(); ++i)
	{
		if (occluders[i].rendered)
		{
			if (occluders[i].isPatch)
				m_VisiblePatchOccluders.push_back(static_cast<CPatch*>(occluders[i].renderable));
			else
				m_VisibleModelOccluders.push_back(static_cast<CModel*>(occluders[i].renderable));
		}
	}

	for (size_t i = 0; i < casters.size(); ++i)
		if (casters[i].rendered)
			m_VisibleModelCasters.push_back(casters[i].model);
}

void SilhouetteRenderer::RenderSubmitOverlays(SceneCollector& collector)
{
	for (size_t i = 0; i < m_DebugSpheres.size(); i++)
		collector.Submit(&m_DebugSpheres[i]);
}

void SilhouetteRenderer::RenderSubmitOccluders(SceneCollector& collector)
{
	for (size_t i = 0; i < m_VisiblePatchOccluders.size(); ++i)
		collector.Submit(m_VisiblePatchOccluders[i]);

	for (size_t i = 0; i < m_VisibleModelOccluders.size(); ++i)
		collector.SubmitNonRecursive(m_VisibleModelOccluders[i]);
}

void SilhouetteRenderer::RenderSubmitCasters(SceneCollector& collector)
{
	for (size_t i = 0; i < m_VisibleModelCasters.size(); ++i)
		collector.SubmitNonRecursive(m_VisibleModelCasters[i]);
}

void SilhouetteRenderer::RenderDebugOverlays(const CCamera& camera)
{
	CShaderTechniquePtr shaderTech = g_Renderer.GetShaderManager().LoadEffect(str_gui_solid);
	shaderTech->BeginPass();
	CShaderProgramPtr shader = shaderTech->GetShader();

	glDepthMask(0);
	glDisable(GL_CULL_FACE);

	shader->Uniform(str_transform, camera.GetViewProjection());

	for (size_t i = 0; i < m_DebugBounds.size(); ++i)
	{
		shader->Uniform(str_color, m_DebugBounds[i].color);
		m_DebugBounds[i].bounds.RenderOutline(shader);
	}

	CMatrix3D m;
	m.SetIdentity();
	m.Scale(1.0f, -1.f, 1.0f);
	m.Translate(0.0f, (float)g_yres, -1000.0f);

	CMatrix3D proj;
	proj.SetOrtho(0.f, MAX_COORD, 0.f, MAX_COORD, -1.f, 1000.f);
	m = proj * m;

	shader->Uniform(str_transform, proj);

	for (size_t i = 0; i < m_DebugRects.size(); ++i)
	{
		const DebugRect& r = m_DebugRects[i];
		shader->Uniform(str_color, r.color);
		u16 verts[] = {
			r.x0, r.y0,
			r.x1, r.y0,
			r.x1, r.y1,
			r.x0, r.y1,
			r.x0, r.y0,
		};
		shader->VertexPointer(2, GL_SHORT, 0, verts);
		glDrawArrays(GL_LINE_STRIP, 0, 5);
	}

	shaderTech->EndPass();

	glEnable(GL_CULL_FACE);
	glDepthMask(1);
}

void SilhouetteRenderer::EndFrame()
{
	m_SubmittedPatchOccluders.clear();
	m_SubmittedModelOccluders.clear();
	m_SubmittedModelCasters.clear();
}
