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
#include "simulation2/MessageTypes.h"

#include "graphics/Overlay.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "renderer/Scene.h"

const size_t SELECTION_CIRCLE_POINTS = 16;

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

	CCmpSelectable()
	{
		m_Overlay.m_Color = CColor(0, 0, 0, 0);
	}

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode))
	{
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// Nothing to do here (the overlay object is not worth saving, it'll get
		// reconstructed by the GUI soon enough, I think)
	}

	virtual void Deserialize(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode), IDeserializer& UNUSED(deserialize))
	{
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			if (m_Overlay.m_Color.a > 0)
			{
				float offset = static_cast<const CMessageInterpolate&> (msg).offset;
				ConstructCircle(context, offset);
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
			debug_assert(m_Overlay.m_Coords.capacity() == 0);
		}

		// TODO: it'd be nice to fade smoothly (but quickly) from transparent to solid
	}

	void ConstructCircle(const CSimContext& context, float frameOffset)
	{
		CmpPtr<ICmpPosition> cmpPosition(context, GetEntityId());
		if (cmpPosition.null())
			return;

		if (!cmpPosition->IsInWorld())
			return;

		CMatrix3D transform = cmpPosition->GetInterpolatedTransform(frameOffset);
		CVector3D pos = transform.GetTranslation();

		float radius = 2.f; // TODO: get this from the unit somehow

		m_Overlay.m_Coords.clear();
		m_Overlay.m_Coords.reserve((SELECTION_CIRCLE_POINTS + 1) * 3);

		for (size_t i = 0; i <= SELECTION_CIRCLE_POINTS; ++i) // use '<=' so it's a closed loop
		{
			float a = i * 2 * M_PI / SELECTION_CIRCLE_POINTS;
			float x = pos.X + radius * sin(a);
			float z = pos.Z + radius * cos(a);
			float y = pos.Y + 0.25f; // TODO: clamp to ground instead
			m_Overlay.m_Coords.push_back(x);
			m_Overlay.m_Coords.push_back(y);
			m_Overlay.m_Coords.push_back(z);
		}
	}

	void RenderSubmit(SceneCollector& collector)
	{
		// (This is only called if a > 0)

		// TODO: maybe should do some frustum culling
		collector.Submit(&m_Overlay);
	}

	SOverlayLine m_Overlay;
};

REGISTER_COMPONENT_TYPE(Selectable)
