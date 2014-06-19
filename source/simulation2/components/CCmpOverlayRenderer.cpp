/* Copyright (C) 2012 Wildfire Games.
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
#include "ICmpOverlayRenderer.h"

#include "ICmpPosition.h"
#include "simulation2/MessageTypes.h"

#include "graphics/Overlay.h"
#include "graphics/TextureManager.h"
#include "renderer/Renderer.h"

class CCmpOverlayRenderer : public ICmpOverlayRenderer
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(OverlayRenderer)

	// Currently-enabled set of sprites
	std::vector<SOverlaySprite> m_Sprites;

	// For each entry in m_Sprites, store the offset of the sprite from the unit's position
	// (so we can recompute the sprite position after the unit moves)
	std::vector<CVector3D> m_SpriteOffsets;

	// Whether the sprites should be drawn (only valid between Interpolate and RenderSubmit)
	bool m_Enabled;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// TODO: should we do anything here?
		// or should we expect other components to reinitialise us
		// after deserialization?
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			PROFILE3("OverlayRenderer::Interpolate");
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);
			Interpolate(msgData.deltaSimTime, msgData.offset);
			break;
		}
		case MT_RenderSubmit:
		{
			PROFILE3("OverlayRenderer::RenderSubmit");
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector);
			break;
		}
		}
	}

	/*
	 * Must be called whenever the size of m_Sprites changes,
	 * to determine whether we need to respond to rendering messages.
	 */
	void UpdateMessageSubscriptions()
	{
		bool needRender = !m_Sprites.empty();

		GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_Interpolate, this, needRender);
		GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_RenderSubmit, this, needRender);
	}

	virtual void Reset()
	{
		m_Sprites.clear();
		m_SpriteOffsets.clear();

		UpdateMessageSubscriptions();
	}

	virtual void AddSprite(VfsPath textureName, CFixedVector2D corner0, CFixedVector2D corner1, CFixedVector3D position)
	{
		CTextureProperties textureProps(textureName);

		SOverlaySprite sprite;
		sprite.m_Texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		sprite.m_X0 = corner0.X.ToFloat();
		sprite.m_Y0 = corner0.Y.ToFloat();
		sprite.m_X1 = corner1.X.ToFloat();
		sprite.m_Y1 = corner1.Y.ToFloat();

		m_Sprites.push_back(sprite);
		m_SpriteOffsets.push_back(CVector3D(position));

		UpdateMessageSubscriptions();
	}

	void Interpolate(float UNUSED(frameTime), float frameOffset)
	{
		// Skip all the following computations if we have no sprites
		if (m_Sprites.empty())
		{
			m_Enabled = false;
			return;
		}

		// Disable rendering of the unit if it has no position
		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
		{
			m_Enabled = false;
			return;
		}

		// Find the precise position of the unit
		CMatrix3D transform(cmpPosition->GetInterpolatedTransform(frameOffset));
		CVector3D position(transform.GetTranslation());

		// Move all the sprites to the desired offset relative to the unit
		for (size_t i = 0; i < m_Sprites.size(); ++i)
			m_Sprites[i].m_Position = position + m_SpriteOffsets[i];

		m_Enabled = true;
	}

	void RenderSubmit(SceneCollector& collector)
	{
		if (!m_Enabled)
			return;

		for (size_t i = 0; i < m_Sprites.size(); ++i)
			collector.Submit(&m_Sprites[i]);
	}
};

REGISTER_COMPONENT_TYPE(OverlayRenderer)
