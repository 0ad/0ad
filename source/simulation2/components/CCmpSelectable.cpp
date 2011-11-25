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

#include "simulation2/system/Component.h"
#include "ICmpSelectable.h"

#include "ICmpPosition.h"
#include "ICmpFootprint.h"
#include "ICmpVisual.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/helpers/Render.h"

#include "graphics/Overlay.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "renderer/Scene.h"

class CCmpSelectable : public ICmpSelectable
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Interpolate);
		componentManager.SubscribeToMessageType(MT_RenderSubmit);
		// TODO: it'd be nice if we didn't get these messages except in the rare
		// cases where we're actually drawing a selection highlight
	}

	DEFAULT_COMPONENT_ALLOCATOR(Selectable)

	SOverlayLine m_Overlay;
	SOverlayLine* m_DebugBoundingBoxOverlay;
	SOverlayLine* m_DebugSelectionBoxOverlay;

	CCmpSelectable()
		: m_DebugBoundingBoxOverlay(NULL), m_DebugSelectionBoxOverlay(NULL)
	{
		m_Overlay.m_Thickness = 2;
		m_Overlay.m_Color = CColor(0, 0, 0, 0);
	}

	~CCmpSelectable(){
		delete m_DebugBoundingBoxOverlay;
		delete m_DebugSelectionBoxOverlay;
	}

	static std::string GetSchema()
	{
		return
			"<a:help>Allows this entity to be selected by the player.</a:help>"
			"<a:example/>"
			"<empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// Nothing to do here (the overlay object is not worth saving, it'll get
		// reconstructed by the GUI soon enough, I think)
	}

	virtual void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& UNUSED(deserialize))
	{
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			if (m_Overlay.m_Color.a > 0)
			{
				float offset = static_cast<const CMessageInterpolate&> (msg).offset;
				ConstructShape(offset);
			}
			break;
		}
		case MT_RenderSubmit:
		{
			if (m_Overlay.m_Color.a > 0)
			{
				const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
				RenderSubmit(msgData.collector);
			}
			break;
		}
		}
	}

	virtual void SetSelectionHighlight(CColor color)
	{
		m_Overlay.m_Color = color;

		if (color.a == 0 && !m_Overlay.m_Coords.empty())
		{
			// Delete the overlay data to save memory (we don't want hundreds of bytes
			// times thousands of units when the selections are not being rendered any more)
			std::vector<float> empty;
			m_Overlay.m_Coords.swap(empty);
			ENSURE(m_Overlay.m_Coords.capacity() == 0);
		}

		// TODO: it'd be nice to fade smoothly (but quickly) from transparent to solid
	}

	void ConstructShape(float frameOffset)
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null())
			return;

		if (!cmpPosition->IsInWorld())
			return;

		float x, z, rotY;
		cmpPosition->GetInterpolatedPosition2D(frameOffset, x, z, rotY);

		CmpPtr<ICmpFootprint> cmpFootprint(GetSimContext(), GetEntityId());
		if (cmpFootprint.null())
		{
			// Default (this probably shouldn't happen) - just render an arbitrary-sized circle
			SimRender::ConstructCircleOnGround(GetSimContext(), x, z, 2.f, m_Overlay, cmpPosition->IsFloating());
		}
		else
		{
			ICmpFootprint::EShape shape;
			entity_pos_t size0, size1, height;
			cmpFootprint->GetShape(shape, size0, size1, height);

			if (shape == ICmpFootprint::SQUARE)
				SimRender::ConstructSquareOnGround(GetSimContext(), x, z, size0.ToFloat(), size1.ToFloat(), rotY, m_Overlay, cmpPosition->IsFloating());
			else
				SimRender::ConstructCircleOnGround(GetSimContext(), x, z, size0.ToFloat(), m_Overlay, cmpPosition->IsFloating());
		}
	}

	void RenderSubmit(SceneCollector& collector)
	{
		// (This is only called if a > 0)
		collector.Submit(&m_Overlay);

		if (ICmpSelectable::ms_EnableDebugOverlays)
		{
			// allocate debug overlays on-demand
			if (!m_DebugBoundingBoxOverlay) m_DebugBoundingBoxOverlay = new SOverlayLine;
			if (!m_DebugSelectionBoxOverlay) m_DebugSelectionBoxOverlay = new SOverlayLine;

			CmpPtr<ICmpVisual> cmpVisual(GetSimContext(), GetEntityId()); 
			if (!cmpVisual.null()) 
			{
				SimRender::ConstructBoxOutline(cmpVisual->GetBounds(), *m_DebugBoundingBoxOverlay);
				m_DebugBoundingBoxOverlay->m_Thickness = 2; 
				m_DebugBoundingBoxOverlay->m_Color = CColor(1.f, 0.f, 0.f, 1.f);

				SimRender::ConstructBoxOutline(cmpVisual->GetSelectionBox(), *m_DebugSelectionBoxOverlay);
				m_DebugSelectionBoxOverlay->m_Thickness = 2;
				m_DebugSelectionBoxOverlay->m_Color = CColor(0.f, 1.f, 0.f, 1.f);

				collector.Submit(m_DebugBoundingBoxOverlay);
				collector.Submit(m_DebugSelectionBoxOverlay);
			}
		}
		else
		{
			// reclaim debug overlay line memory when no longer debugging (and make sure to set to zero after deletion)
			if (m_DebugBoundingBoxOverlay) SAFE_DELETE(m_DebugBoundingBoxOverlay);
			if (m_DebugSelectionBoxOverlay) SAFE_DELETE(m_DebugSelectionBoxOverlay);
		}
	}
};

REGISTER_COMPONENT_TYPE(Selectable)
