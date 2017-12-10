/* Copyright (C) 2017 Wildfire Games.
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

#include "ICmpRangeOverlayRenderer.h"

#include "graphics/Overlay.h"
#include "graphics/TextureManager.h"
#include "renderer/Renderer.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/system/Component.h"

class CCmpRangeOverlayRenderer : public ICmpRangeOverlayRenderer
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
	}

	DEFAULT_COMPONENT_ALLOCATOR(RangeOverlayRenderer)

	CCmpRangeOverlayRenderer() : m_RangeOverlayData()
	{
		m_Color = CColor(0, 0, 0, 0);
	}

	~CCmpRangeOverlayRenderer()
	{
		for (const RangeOverlayData& rangeOverlay : m_RangeOverlayData)
			delete rangeOverlay.second;
	}

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_EnabledInterpolate = false;
		m_EnabledRenderSubmit = false;
		m_Enabled = false;
		UpdateMessageSubscriptions();
	}

	virtual void Deinit() { }

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	void ResetRangeOverlays()
	{
		for (const RangeOverlayData& rangeOverlay : m_RangeOverlayData)
			delete rangeOverlay.second;
		m_RangeOverlayData.clear();
		UpdateMessageSubscriptions();
		m_Enabled = false;
	}

	virtual void AddRangeOverlay(float radius, const std::string& texture, const std::string& textureMask, float thickness)
	{
		if (!CRenderer::IsInitialised())
			return;

		SOverlayDescriptor rangeOverlayDescriptor;
		SOverlayTexturedLine* rangeOverlay = nullptr;
		rangeOverlayDescriptor.m_Radius = radius;
		rangeOverlayDescriptor.m_LineTexture = CStrIntern(TEXTUREBASEPATH + texture);
		rangeOverlayDescriptor.m_LineTextureMask = CStrIntern(TEXTUREBASEPATH + textureMask);
		rangeOverlayDescriptor.m_LineThickness = thickness;

		m_RangeOverlayData.push_back({ rangeOverlayDescriptor, rangeOverlay });
		m_Enabled = true;
		UpdateMessageSubscriptions();
	}

	void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);

			for (RangeOverlayData& rangeOverlay : m_RangeOverlayData)
			{
				delete rangeOverlay.second;
				rangeOverlay.second = new SOverlayTexturedLine;
				UpdateRangeOverlay(&rangeOverlay.first, *rangeOverlay.second, msgData.offset);
			}

			UpdateMessageSubscriptions();

			break;
		}
		case MT_OwnershipChanged:
		{
			const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);
			if (msgData.to == INVALID_PLAYER)
				break;

			CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSystemEntity());
			if (!cmpPlayerManager)
				break;

			CmpPtr<ICmpPlayer> cmpPlayer(GetSimContext(), cmpPlayerManager->GetPlayerByID(msgData.to));
			if (!cmpPlayer)
				break;

			CColor color = cmpPlayer->GetColor();
			m_Color = color;
			break;
		}
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector);
			break;
		}
		}
	}

	void UpdateMessageSubscriptions()
	{
		bool needInterpolate = false;
		bool needRenderSubmit = false;

		if (m_Enabled)
		{
			needInterpolate = true;
			needRenderSubmit = true;
		}

		if (needInterpolate != m_EnabledInterpolate)
		{
			GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_Interpolate, this, needInterpolate);
			m_EnabledInterpolate = needInterpolate;
			m_Enabled = needInterpolate;
		}

		if (needRenderSubmit != m_EnabledRenderSubmit)
		{
			GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_RenderSubmit, this, needRenderSubmit);
			m_EnabledRenderSubmit = needRenderSubmit;
			m_Enabled = needRenderSubmit;
		}
	}

	void RenderSubmit(SceneCollector& collector)
	{
		if (!m_RangeOverlayData.size())
			return;

		for (const RangeOverlayData& rangeOverlay : m_RangeOverlayData)
			if (rangeOverlay.second)
				collector.Submit(rangeOverlay.second);
	}

private:
	void UpdateRangeOverlay(const SOverlayDescriptor* overlayDescriptor, SOverlayTexturedLine& overlay, float frameOffset)
	{
		if (!CRenderer::IsInitialised())
			return;

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return;

		float rotY;
		CVector2D origin;
		cmpPosition->GetInterpolatedPosition2D(frameOffset, origin.X, origin.Y, rotY);

		overlay.m_SimContext = &GetSimContext();
		overlay.m_Color = m_Color;
		overlay.CreateOverlayTexture(overlayDescriptor);

		SimRender::ConstructTexturedLineCircle(overlay, origin, overlayDescriptor->m_Radius);
	}

	bool m_EnabledInterpolate;
	bool m_EnabledRenderSubmit;
	bool m_Enabled;

	const char* TEXTUREBASEPATH = "art/textures/selection/";
	typedef std::pair<SOverlayDescriptor, SOverlayTexturedLine*> RangeOverlayData;
	std::vector<RangeOverlayData> m_RangeOverlayData;
	CColor m_Color;
};

REGISTER_COMPONENT_TYPE(RangeOverlayRenderer)
