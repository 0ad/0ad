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

#include "ICmpSelectable.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "maths/Ease.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "maths/Vector2D.h"
#include "renderer/Scene.h"
#include "renderer/Renderer.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpFootprint.h"
#include "simulation2/components/ICmpVisual.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/system/Component.h"

// Minimum alpha value for always visible overlays [0 fully transparent, 1 fully opaque]
static const float MIN_ALPHA_ALWAYS_VISIBLE = 0.65f;
// Minimum alpha value for other overlays
static const float MIN_ALPHA_UNSELECTED = 0.0f;
// Desaturation value for unselected, always visible overlays (0.33 = 33% desaturated or 66% of original saturation)
static const float RGB_DESATURATION = 0.333333f;

class CCmpSelectable : public ICmpSelectable
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Interpolate);
		componentManager.SubscribeToMessageType(MT_RenderSubmit);
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeToMessageType(MT_PositionChanged);
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
		componentManager.SubscribeToMessageType(MT_WaterChanged);
		// TODO: it'd be nice if we didn't get these messages except in the rare
		// cases where we're actually drawing a selection highlight
	}

	DEFAULT_COMPONENT_ALLOCATOR(Selectable)

	CCmpSelectable()
		: m_DebugBoundingBoxOverlay(NULL), m_DebugSelectionBoxOverlay(NULL), 
		  m_BuildingOverlay(NULL), m_UnitOverlay(NULL),
		  m_FadeBaselineAlpha(0.f), m_FadeDeltaAlpha(0.f), m_FadeProgress(0.f),
		  m_Selected(false), m_Cached(false), m_Visible(false)
	{
		m_Color = CColor(0, 0, 0, m_FadeBaselineAlpha);
	}

	~CCmpSelectable()
	{
		delete m_DebugBoundingBoxOverlay;
		delete m_DebugSelectionBoxOverlay;
		delete m_BuildingOverlay;
		delete m_UnitOverlay;
	}

	static std::string GetSchema()
	{
		return
			"<a:help>Allows this entity to be selected by the player.</a:help>"
			"<a:example/>"
			"<optional>"
				"<element name='EditorOnly' a:help='If this element is present, the entity is only selectable in Atlas'>"
					"<empty/>"
				"</element>"
			"</optional>"
			"<element name='Overlay' a:help='Specifies the type of overlay to be displayed when this entity is selected'>"
				"<optional>"
					"<element name='AlwaysVisible' a:help='If this element is present, the selection overlay will always be visible (with transparency and desaturation)'>"
						"<empty/>"
					"</element>"
				"</optional>"
				"<choice>"
					"<element name='Texture' a:help='Displays a texture underneath the entity.'>"
						"<element name='MainTexture' a:help='Texture to display underneath the entity. Filepath relative to art/textures/selection/.'><text/></element>"
						"<element name='MainTextureMask' a:help='Mask texture that controls where to apply player color. Filepath relative to art/textures/selection/.'><text/></element>"
					"</element>"
					"<element name='Outline' a:help='Traces the outline of the entity with a line texture.'>"
						"<element name='LineTexture' a:help='Texture to apply to the line. Filepath relative to art/textures/selection/.'><text/></element>"
						"<element name='LineTextureMask' a:help='Texture that controls where to apply player color. Filepath relative to art/textures/selection/.'><text/></element>"
						"<element name='LineThickness' a:help='Thickness of the line, in world units.'><ref name='positiveDecimal'/></element>"
					"</element>"
				"</choice>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		m_EditorOnly = paramNode.GetChild("EditorOnly").IsOk();

		// Certain special units always have their selection overlay shown
		m_AlwaysVisible = paramNode.GetChild("Overlay").GetChild("AlwaysVisible").IsOk();
		if (m_AlwaysVisible)
		{
			m_AlphaMin = MIN_ALPHA_ALWAYS_VISIBLE;
			m_Color.a = m_AlphaMin;
		}
		else
			m_AlphaMin = MIN_ALPHA_UNSELECTED;

		const CParamNode& textureNode = paramNode.GetChild("Overlay").GetChild("Texture");
		const CParamNode& outlineNode = paramNode.GetChild("Overlay").GetChild("Outline");

		const char* textureBasePath = "art/textures/selection/";

		// Save some memory by using interned file paths in these descriptors (almost all actors and
		// entities have this component, and many use the same textures).
		if (textureNode.IsOk())
		{
			// textured quad mode (dynamic, for units)
			m_OverlayDescriptor.m_Type = ICmpSelectable::DYNAMIC_QUAD;
			m_OverlayDescriptor.m_QuadTexture = CStrIntern(textureBasePath + textureNode.GetChild("MainTexture").ToUTF8());
			m_OverlayDescriptor.m_QuadTextureMask = CStrIntern(textureBasePath + textureNode.GetChild("MainTextureMask").ToUTF8());
		}
		else if (outlineNode.IsOk())
		{
			// textured outline mode (static, for buildings)
			m_OverlayDescriptor.m_Type = ICmpSelectable::STATIC_OUTLINE;
			m_OverlayDescriptor.m_LineTexture = CStrIntern(textureBasePath + outlineNode.GetChild("LineTexture").ToUTF8());
			m_OverlayDescriptor.m_LineTextureMask = CStrIntern(textureBasePath + outlineNode.GetChild("LineTextureMask").ToUTF8());
			m_OverlayDescriptor.m_LineThickness = outlineNode.GetChild("LineThickness").ToFloat();
		}
	}

	virtual void Deinit() { }

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// Nothing to do here (the overlay object is not worth saving, it'll get
		// reconstructed by the GUI soon enough, I think)
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		// Need to call Init to reload the template properties
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global));

	virtual void SetSelectionHighlight(CColor color, bool selected)
	{
		m_Selected = selected;
		m_Color.r = color.r;
		m_Color.g = color.g;
		m_Color.b = color.b;

		// Always-visible overlays will be desaturated if their parent unit is deselected.
		if (m_AlwaysVisible && !selected)
		{
			float max;

			// Reduce saturation by one-third, the quick-and-dirty way.
			if (m_Color.r > m_Color.b)
				max = (m_Color.r > m_Color.g) ? m_Color.r : m_Color.g;
			else
				max = (m_Color.b > m_Color.g) ? m_Color.b : m_Color.g;

			m_Color.r += (max - m_Color.r) * RGB_DESATURATION;
			m_Color.g += (max - m_Color.g) * RGB_DESATURATION;
			m_Color.b += (max - m_Color.b) * RGB_DESATURATION;
		}

		SetSelectionHighlightAlpha(color.a);
	}

	virtual void SetSelectionHighlightAlpha(float alpha)
	{
		alpha = std::max(m_AlphaMin, alpha);

		// set up fading from the current value (as the baseline) to the target value
		m_FadeBaselineAlpha = m_Color.a;
		m_FadeDeltaAlpha = alpha - m_FadeBaselineAlpha;
		m_FadeProgress = 0.f;
	}

	virtual void SetVisibility(bool visible)
	{
		m_Visible = visible;
	}

	virtual bool IsEditorOnly()
	{
		return m_EditorOnly;
	}

	void RenderSubmit(SceneCollector& collector);

	/**
	 * Called from RenderSubmit if using a static outline; responsible for ensuring that the static overlay 
	 * is up-to-date before it is rendered. Has no effect unless the static overlay is explicitly marked as
	 * invalid first (see InvalidateStaticOverlay).
	 */
	void UpdateStaticOverlay();

	/**
	 * Called from the interpolation handler; responsible for ensuring the dynamic overlay (provided we're
	 * using one) is up-to-date and ready to be submitted to the next rendering run.
	 */
	void UpdateDynamicOverlay(float frameOffset);

	/// Explicitly invalidates the static overlay.
	void InvalidateStaticOverlay();

private:
	SOverlayDescriptor m_OverlayDescriptor;
	SOverlayTexturedLine* m_BuildingOverlay;
	SOverlayQuad* m_UnitOverlay;

	SOverlayLine* m_DebugBoundingBoxOverlay;
	SOverlayLine* m_DebugSelectionBoxOverlay;

	// Whether the selectable will be rendered.
	bool m_Visible;
	// Whether the entity is only selectable in Atlas editor
	bool m_EditorOnly;
	// Whether the selection overlay is always visible
	bool m_AlwaysVisible;
	/// Whether the parent entity is selected (caches GUI's selection state).
	bool m_Selected;
	/// Current selection overlay color. Alpha component is subject to fading.
	CColor m_Color;
	/// Whether the selectable's player colour has been cached for rendering.
	bool m_Cached;
	/// Minimum value for current selection overlay alpha.
	float m_AlphaMin;
	/// Baseline alpha value to start fading from. Constant during a single fade.
	float m_FadeBaselineAlpha;
	/// Delta between target and baseline alpha. Constant during a single fade. Can be positive or negative.
	float m_FadeDeltaAlpha;
	/// Linear time progress of the fade, between 0 and m_FadeDuration.
	float m_FadeProgress;

	/// Total duration of a single fade, in seconds. Assumed constant for now; feel free to change this into
	/// a member variable if you need to adjust it per component.
	static const double FADE_DURATION;
};

const double CCmpSelectable::FADE_DURATION = 0.3;

void CCmpSelectable::HandleMessage(const CMessage& msg, bool UNUSED(global))
{
	switch (msg.GetType())
	{
	case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);

			if (m_FadeDeltaAlpha != 0.f)
			{
				m_FadeProgress += msgData.deltaRealTime;
				if (m_FadeProgress >= FADE_DURATION)
				{
					const float targetAlpha = m_FadeBaselineAlpha + m_FadeDeltaAlpha;

					// stop the fade
					m_Color.a = targetAlpha;
					m_FadeBaselineAlpha = targetAlpha;
					m_FadeDeltaAlpha = 0.f;
					m_FadeProgress = FADE_DURATION; // will need to be reset to start the next fade again
				}
				else
				{
					m_Color.a = Ease::QuartOut(m_FadeProgress, m_FadeBaselineAlpha, m_FadeDeltaAlpha, FADE_DURATION);
				}
			}

			// update dynamic overlay only when visible
			if (m_Color.a > 0)
				UpdateDynamicOverlay(msgData.offset);

			break;
		}
	case MT_OwnershipChanged: 
		{
			const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);

			// don't update color if there's no new owner (e.g. the unit died)
			if (msgData.to == INVALID_PLAYER)
				break;

			// update the selection highlight color
			CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSystemEntity());
			if (!cmpPlayerManager)
				break;

			CmpPtr<ICmpPlayer> cmpPlayer(GetSimContext(), cmpPlayerManager->GetPlayerByID(msgData.to));
			if (!cmpPlayer)
				break;

			// Update the highlight color, while keeping the current alpha target value intact
			// (i.e. baseline + delta), so that any ongoing fades simply continue with the new color.
			CColor color = cmpPlayer->GetColour();
			SetSelectionHighlight(CColor(color.r, color.g, color.b, m_FadeBaselineAlpha + m_FadeDeltaAlpha), m_Selected);

			InvalidateStaticOverlay();
			break;
		}
	case MT_PositionChanged:
	case MT_TerrainChanged:
	case MT_WaterChanged:
		{
			InvalidateStaticOverlay();
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

void CCmpSelectable::InvalidateStaticOverlay()
{
	SAFE_DELETE(m_BuildingOverlay);
}

void CCmpSelectable::UpdateStaticOverlay()
{
	// Static overlays are allocated once and not updated until they are explicitly deleted again
	// (see InvalidateStaticOverlay). Since they are expected to change rarely (if ever) during
	// normal gameplay, this saves us doing all the work below on each frame.
	
	if (m_BuildingOverlay || m_OverlayDescriptor.m_Type != STATIC_OUTLINE)
		return;
	
	if (!CRenderer::IsInitialised())
		return;

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	CmpPtr<ICmpFootprint> cmpFootprint(GetEntityHandle());
	if (!cmpFootprint || !cmpPosition || !cmpPosition->IsInWorld())
		return;

	CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
	if (!cmpTerrain)
		return; // should never happen

	// grab position/footprint data
	CFixedVector2D position = cmpPosition->GetPosition2D();
	CFixedVector3D rotation = cmpPosition->GetRotation();

	ICmpFootprint::EShape fpShape;
	entity_pos_t fpSize0_fixed, fpSize1_fixed, fpHeight_fixed;
	cmpFootprint->GetShape(fpShape, fpSize0_fixed, fpSize1_fixed, fpHeight_fixed);

	CTextureProperties texturePropsBase(m_OverlayDescriptor.m_LineTexture.c_str()); 
	texturePropsBase.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE); 
	texturePropsBase.SetMaxAnisotropy(4.f);

	CTextureProperties texturePropsMask(m_OverlayDescriptor.m_LineTextureMask.c_str());
	texturePropsMask.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE); 
	texturePropsMask.SetMaxAnisotropy(4.f);

	// -------------------------------------------------------------------------------------

	m_BuildingOverlay = new SOverlayTexturedLine;
	m_BuildingOverlay->m_AlwaysVisible = false;
	m_BuildingOverlay->m_Closed = true;
	m_BuildingOverlay->m_SimContext = &GetSimContext();
	m_BuildingOverlay->m_Thickness = m_OverlayDescriptor.m_LineThickness;
	m_BuildingOverlay->m_TextureBase = g_Renderer.GetTextureManager().CreateTexture(texturePropsBase);
	m_BuildingOverlay->m_TextureMask = g_Renderer.GetTextureManager().CreateTexture(texturePropsMask);

	CVector2D origin(position.X.ToFloat(), position.Y.ToFloat());

	switch (fpShape)
	{
	case ICmpFootprint::SQUARE:
		{
			float s = sinf(-rotation.Y.ToFloat());
			float c = cosf(-rotation.Y.ToFloat());
			CVector2D unitX(c, s);
			CVector2D unitZ(-s, c);

			// add half the line thickness to the radius so that we get an 'outside' stroke of the footprint shape
			const float halfSizeX = fpSize0_fixed.ToFloat()/2.f + m_BuildingOverlay->m_Thickness/2.f;
			const float halfSizeZ = fpSize1_fixed.ToFloat()/2.f + m_BuildingOverlay->m_Thickness/2.f;

			std::vector<CVector2D> points;
			points.push_back(CVector2D(origin + unitX *  halfSizeX    + unitZ *(-halfSizeZ)));
			points.push_back(CVector2D(origin + unitX *(-halfSizeX)   + unitZ *(-halfSizeZ)));
			points.push_back(CVector2D(origin + unitX *(-halfSizeX)   + unitZ *  halfSizeZ));
			points.push_back(CVector2D(origin + unitX *  halfSizeX    + unitZ *  halfSizeZ));

			SimRender::SubdividePoints(points, TERRAIN_TILE_SIZE/3.f, m_BuildingOverlay->m_Closed);
			m_BuildingOverlay->PushCoords(points);
		}
		break;
	case ICmpFootprint::CIRCLE:
		{
			const float radius = fpSize0_fixed.ToFloat() + m_BuildingOverlay->m_Thickness/3.f;
			if (radius > 0) // prevent catastrophic failure
			{
				float stepAngle;
				unsigned numSteps;
				SimRender::AngularStepFromChordLen(TERRAIN_TILE_SIZE/3.f, radius, stepAngle, numSteps);

				for (unsigned i = 0; i < numSteps; i++) // '<' is sufficient because the line is closed automatically
				{
					float angle = i * stepAngle;
					float px = origin.X + radius * sinf(angle);
					float pz = origin.Y + radius * cosf(angle);

					m_BuildingOverlay->PushCoords(px, pz);
				}
			}
		}
		break;
	}

	ENSURE(m_BuildingOverlay);
}

void CCmpSelectable::UpdateDynamicOverlay(float frameOffset)
{
	// Dynamic overlay lines are allocated once and never deleted. Since they are expected to change frequently,
	// they are assumed dirty on every call to this function, and we should therefore use this function more
	// thoughtfully than calling it right before every frame render.
	
	if (m_OverlayDescriptor.m_Type != DYNAMIC_QUAD)
		return;

	if (!CRenderer::IsInitialised())
		return;

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	CmpPtr<ICmpFootprint> cmpFootprint(GetEntityHandle());
	if (!cmpFootprint || !cmpPosition || !cmpPosition->IsInWorld())
		return;

	float rotY;
	CVector2D position;
	cmpPosition->GetInterpolatedPosition2D(frameOffset, position.X, position.Y, rotY);

	CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());
	CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
	ENSURE(cmpWaterManager && cmpTerrain);

	CTerrain* terrain = cmpTerrain->GetCTerrain();
	ENSURE(terrain);

	ICmpFootprint::EShape fpShape;
	entity_pos_t fpSize0_fixed, fpSize1_fixed, fpHeight_fixed;
	cmpFootprint->GetShape(fpShape, fpSize0_fixed, fpSize1_fixed, fpHeight_fixed);

	// ---------------------------------------------------------------------------------

	if (!m_UnitOverlay)
	{
		m_UnitOverlay = new SOverlayQuad;

		// Assuming we don't need the capability of swapping textures on-demand.
		CTextureProperties texturePropsBase(m_OverlayDescriptor.m_QuadTexture.c_str()); 
		texturePropsBase.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE); 
		texturePropsBase.SetMaxAnisotropy(4.f);

		CTextureProperties texturePropsMask(m_OverlayDescriptor.m_QuadTextureMask.c_str());
		texturePropsMask.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE); 
		texturePropsMask.SetMaxAnisotropy(4.f);

		m_UnitOverlay->m_Texture = g_Renderer.GetTextureManager().CreateTexture(texturePropsBase);
		m_UnitOverlay->m_TextureMask = g_Renderer.GetTextureManager().CreateTexture(texturePropsMask);
	}

	m_UnitOverlay->m_Color = m_Color;

	// TODO: some code duplication here :< would be nice to factor out getting the corner points of an 
	// entity based on its footprint sizes (and regardless of whether it's a circle or a square)

	float s = sinf(-rotY);
	float c = cosf(-rotY);
	CVector2D unitX(c, s);
	CVector2D unitZ(-s, c);

	float halfSizeX = fpSize0_fixed.ToFloat();
	float halfSizeZ = fpSize1_fixed.ToFloat();
	if (fpShape == ICmpFootprint::SQUARE)
	{
		halfSizeX /= 2.0f;
		halfSizeZ /= 2.0f;
	}

	std::vector<CVector2D> points;
	points.push_back(CVector2D(position + unitX *(-halfSizeX)   + unitZ *  halfSizeZ));  // top left
	points.push_back(CVector2D(position + unitX *(-halfSizeX)   + unitZ *(-halfSizeZ))); // bottom left
	points.push_back(CVector2D(position + unitX *  halfSizeX    + unitZ *(-halfSizeZ))); // bottom right
	points.push_back(CVector2D(position + unitX *  halfSizeX    + unitZ *  halfSizeZ));  // top right

	for (int i=0; i < 4; i++)
	{
		float quadY = std::max(
			terrain->GetExactGroundLevel(points[i].X, points[i].Y),
			cmpWaterManager->GetExactWaterLevel(points[i].X, points[i].Y)
		);

		m_UnitOverlay->m_Corners[i] = CVector3D(points[i].X, quadY, points[i].Y);
	}
}

void CCmpSelectable::RenderSubmit(SceneCollector& collector)
{
	// don't render selection overlay if it's not gonna be visible
	if (m_Visible && m_Color.a > 0)
	{
		if (!m_Cached)
		{
			// Default to white if there's no owner (e.g. decorative, editor-only actors)
			CColor color = CColor(1.0, 1.0, 1.0, 1.0);
			CmpPtr<ICmpOwnership> cmpOwnership(GetEntityHandle());
			if (cmpOwnership)
			{
				player_id_t owner = cmpOwnership->GetOwner();
				if (owner == INVALID_PLAYER)
					return;

				// Try to initialize m_Color to the owning player's colour.
				CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSystemEntity());
				if (!cmpPlayerManager)
					return;

				CmpPtr<ICmpPlayer> cmpPlayer(GetSimContext(), cmpPlayerManager->GetPlayerByID(owner));
				if (!cmpPlayer)
					return;

				color = cmpPlayer->GetColour();
			}
			color.a = m_FadeBaselineAlpha + m_FadeDeltaAlpha;

			SetSelectionHighlight(color, m_Selected);
			m_Cached = true;
		}

		switch (m_OverlayDescriptor.m_Type)
		{
			case STATIC_OUTLINE:
				{
					UpdateStaticOverlay();
					m_BuildingOverlay->m_Color = m_Color; // done separately so alpha changes don't require a full update call
					collector.Submit(m_BuildingOverlay);
				}
				break;
			case DYNAMIC_QUAD:
				{
					if (m_UnitOverlay)
						collector.Submit(m_UnitOverlay);
				}
				break;
			default:
				break;
		}
	}

	// Render bounding box debug overlays if we have a positive target alpha value. This ensures
	// that the debug overlays respond immediately to deselection without delay from fading out.
	if (m_FadeBaselineAlpha + m_FadeDeltaAlpha > 0)
	{
		if (ICmpSelectable::ms_EnableDebugOverlays)
		{
			// allocate debug overlays on-demand
			if (!m_DebugBoundingBoxOverlay) m_DebugBoundingBoxOverlay = new SOverlayLine;
			if (!m_DebugSelectionBoxOverlay) m_DebugSelectionBoxOverlay = new SOverlayLine;

			CmpPtr<ICmpVisual> cmpVisual(GetEntityHandle());
			if (cmpVisual) 
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
}

REGISTER_COMPONENT_TYPE(Selectable)
