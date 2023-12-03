/* Copyright (C) 2022 Wildfire Games.
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
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/system/Component.h"

#include <memory>

class CCmpRangeOverlayRenderer final : public ICmpRangeOverlayRenderer
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Deserialized);
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeToMessageType(MT_PlayerColorChanged);
	}

	DEFAULT_COMPONENT_ALLOCATOR(RangeOverlayRenderer)

	CCmpRangeOverlayRenderer() : m_RangeOverlayData()
	{
		m_Color = CColor(0.f, 0.f, 0.f, 1.f);
	}

	~CCmpRangeOverlayRenderer() = default;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	void Init(const CParamNode& UNUSED(paramNode)) override
	{
		m_Enabled = m_LastEnabledState = false;
		UpdateMessageSubscriptions();
	}

	void Deinit() override { }

	void Serialize(ISerializer& UNUSED(serialize)) override
	{
	}

	void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize)) override
	{
		Init(paramNode);
	}

	void ResetRangeOverlays() override
	{
		m_RangeOverlayData.clear();
		UpdateMessageSubscriptions();
		m_Enabled = false;
	}

	void AddRangeOverlay(float radius, const std::string& texture, const std::string& textureMask, float thickness) override
	{
		if (!CRenderer::IsInitialised())
			return;

		SOverlayDescriptor rangeOverlayDescriptor;
		rangeOverlayDescriptor.m_Radius = radius;
		rangeOverlayDescriptor.m_LineTexture = CStrIntern(TEXTURE_BASE_PATH + texture);
		rangeOverlayDescriptor.m_LineTextureMask = CStrIntern(TEXTURE_BASE_PATH + textureMask);
		rangeOverlayDescriptor.m_LineThickness = thickness;

		m_RangeOverlayData.push_back({
			rangeOverlayDescriptor, std::unique_ptr<SOverlayTexturedLine>()
		});
		m_Enabled = true;
		UpdateMessageSubscriptions();
	}

	void HandleMessage(const CMessage& msg, bool UNUSED(global)) override
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);

			for (RangeOverlayData& rangeOverlay : m_RangeOverlayData)
				UpdateRangeOverlay(rangeOverlay, msgData.offset);

			UpdateMessageSubscriptions();

			break;
		}
		case MT_Deserialized:
		case MT_OwnershipChanged:
		case MT_PlayerColorChanged:
		{
			UpdateColor();
			break;
		}
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector, msgData.frustum, msgData.culling);
			break;
		}
		}
	}

private:
	struct RangeOverlayData {
		SOverlayDescriptor descriptor;
		std::unique_ptr<SOverlayTexturedLine> line;
	};

	void UpdateColor() override
	{
		CmpPtr<ICmpOwnership> cmpOwnership(GetEntityHandle());
		if (!cmpOwnership)
			return;

		player_id_t owner = cmpOwnership->GetOwner();
		if (owner == INVALID_PLAYER)
			return;

		CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSystemEntity());
		if (!cmpPlayerManager)
			return;

		CmpPtr<ICmpPlayer> cmpPlayer(GetSimContext(), cmpPlayerManager->GetPlayerByID(owner));
		if (!cmpPlayer)
			return;

		CColor color = cmpPlayer->GetDisplayedColor();
		m_Color = color;
	}

	void UpdateMessageSubscriptions()
	{
		if (m_Enabled == m_LastEnabledState)
			return;

		CComponentManager& cmpManager = GetSimContext().GetComponentManager();
		cmpManager.DynamicSubscriptionNonsync(MT_Interpolate, this, m_Enabled);
		cmpManager.DynamicSubscriptionNonsync(MT_RenderSubmit, this, m_Enabled);
		m_LastEnabledState = m_Enabled;
	}

	void RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling)
	{
		if (!m_RangeOverlayData.size())
			return;

		for (const RangeOverlayData& rangeOverlay : m_RangeOverlayData)
		{
			if (!rangeOverlay.line)
				continue;
			if (culling && !rangeOverlay.line->IsVisibleInFrustum(frustum))
				continue;
			collector.Submit(rangeOverlay.line.get());
		}
	}

	void UpdateRangeOverlay(RangeOverlayData& rangeOverlay, const float frameOffset)
	{
		if (!CRenderer::IsInitialised())
			return;

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return;

		float rotY;
		CVector2D origin;
		cmpPosition->GetInterpolatedPosition2D(frameOffset, origin.X, origin.Y, rotY);

		rangeOverlay.line = std::make_unique<SOverlayTexturedLine>();
		rangeOverlay.line->m_SimContext = &GetSimContext();
		rangeOverlay.line->m_Color = m_Color;
		rangeOverlay.line->CreateOverlayTexture(&rangeOverlay.descriptor);

		SimRender::ConstructTexturedLineCircle(*rangeOverlay.line.get(), origin, rangeOverlay.descriptor.m_Radius);
	}

	bool m_LastEnabledState;
	bool m_Enabled;

	const char* TEXTURE_BASE_PATH = "art/textures/selection/";
	std::vector<RangeOverlayData> m_RangeOverlayData;
	CColor m_Color;
};

REGISTER_COMPONENT_TYPE(RangeOverlayRenderer)
