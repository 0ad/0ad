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

#ifndef INCLUDED_SILHOUETTERENDERER
#define INCLUDED_SILHOUETTERENDERER

#include "ps/Overlay.h"
#include "graphics/Overlay.h"
#include "maths/BoundingBoxAligned.h"

class CCamera;
class CModel;
class CPatch;
class SceneCollector;

class SilhouetteRenderer
{
public:
	SilhouetteRenderer();

	void AddOccluder(CPatch* patch);
	void AddOccluder(CModel* model);
	void AddCaster(CModel* model);

	void ComputeSubmissions(const CCamera& camera);
	void RenderSubmitOverlays(SceneCollector& collector);
	void RenderSubmitOccluders(SceneCollector& collector);
	void RenderSubmitCasters(SceneCollector& collector);

	void RenderDebugOverlays(const CCamera& camera);

	void EndFrame();

private:
	bool m_DebugEnabled;

	std::vector<CPatch*> m_SubmittedPatchOccluders;
	std::vector<CModel*> m_SubmittedModelOccluders;
	std::vector<CModel*> m_SubmittedModelCasters;

	std::vector<CPatch*> m_VisiblePatchOccluders;
	std::vector<CModel*> m_VisibleModelOccluders;
	std::vector<CModel*> m_VisibleModelCasters;

	struct DebugBounds
	{
		CColor color;
		CBoundingBoxAligned bounds;
	};

	struct DebugRect
	{
		CColor color;
		u16 x0, y0, x1, y1;
	};

	std::vector<DebugBounds> m_DebugBounds;
	std::vector<DebugRect> m_DebugRects;

	std::vector<SOverlaySphere> m_DebugSpheres;
};

#endif // INCLUDED_SILHOUETTERENDERER
