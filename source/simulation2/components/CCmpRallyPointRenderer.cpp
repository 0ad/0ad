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
#include "ICmpRallyPointRenderer.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpFootprint.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpVisual.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/system/Component.h"

#include "ps/CLogger.h"
#include "graphics/Overlay.h"
#include "graphics/TextureManager.h"
#include "renderer/Renderer.h"

struct SVisibilitySegment
{
	bool m_Visible;
	size_t m_StartIndex;
	size_t m_EndIndex; // inclusive

	SVisibilitySegment(bool visible, size_t startIndex, size_t endIndex) 
		: m_Visible(visible), m_StartIndex(startIndex), m_EndIndex(endIndex)
	{}

	bool operator==(const SVisibilitySegment& other) const
	{
		return (m_Visible == other.m_Visible && m_StartIndex == other.m_StartIndex && m_EndIndex == other.m_EndIndex);
	}

	bool operator!=(const SVisibilitySegment& other) const
	{
		return !(*this == other);
	}

	bool IsSinglePoint()
	{
		return (m_StartIndex == m_EndIndex);
	}
};

class CCmpRallyPointRenderer : public ICmpRallyPointRenderer
{
	// import some types for less verbosity
	typedef ICmpPathfinder::Path Path;
	typedef ICmpPathfinder::Goal Goal;
	typedef ICmpPathfinder::Waypoint Waypoint;
	typedef ICmpRangeManager::CLosQuerier CLosQuerier;
	typedef SOverlayTexturedLine::LineCapType LineCapType;

public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeToMessageType(MT_TurnStart);
		componentManager.SubscribeToMessageType(MT_Destroy);
		componentManager.SubscribeToMessageType(MT_PositionChanged);
	}

	DEFAULT_COMPONENT_ALLOCATOR(RallyPointRenderer)

protected:

	/// Display position of the rally points. Note that this are merely the display positions; they not necessarily the same as the 
	/// actual positions used in the simulation at any given time. In particular, we need this separate copy to support
	/// instantaneously rendering the rally point markers/lines when the user sets one in-game (instead of waiting until the 
	/// network-synchronization code sets it on the RallyPoint component, which might take up to half a second).
	std::vector<CFixedVector2D> m_RallyPoints;
	/// Full path to the rally points as returned by the pathfinder, with some post-processing applied to reduce zig/zagging.
	std::vector<std::vector<CVector2D> > m_Path;
	/// Visibility segments of the rally point paths; splits the path into SoD/non-SoD segments.
	std::deque<std::deque<SVisibilitySegment> > m_VisibilitySegments;

	bool m_Displayed; ///< Should we render the rally points and the path lines? (set from JS when e.g. the unit is selected/deselected)
	bool m_SmoothPath; ///< Smooth the path before rendering?

	std::vector<entity_id_t> m_MarkerEntityIds; ///< Entity IDs of the rally point markers.
	size_t m_LastMarkerCount;
	player_id_t m_LastOwner; ///< Last seen owner of this entity (used to keep track of ownership changes).
	std::wstring m_MarkerTemplate;  ///< Template name of the rally point markers.

	/// Marker connector line settings (loaded from XML)
	float m_LineThickness;
	CColor m_LineColor;
	CColor m_LineDashColor;
	LineCapType m_LineStartCapType;
	LineCapType m_LineEndCapType;
	std::wstring m_LineTexturePath;
	std::wstring m_LineTextureMaskPath;
	std::string m_LinePassabilityClass; ///< Pathfinder passability class to use for computing the (long-range) marker line path.
	std::string m_LineCostClass; ///< Pathfinder cost class to use for computing the (long-range) marker line path.

	CTexturePtr m_Texture;
	CTexturePtr m_TextureMask;

	/// Textured overlay lines to be used for rendering the marker line. There can be multiple because we may need to render 
	/// dashes for segments that are inside the SoD.
	std::vector<std::vector<SOverlayTexturedLine> > m_TexturedOverlayLines;

	/// Draw little overlay circles to indicate where the exact path points are?
	bool m_EnableDebugNodeOverlay;
	std::vector<std::vector<SOverlayLine> > m_DebugNodeOverlays;

public:

	static std::string GetSchema()
	{
		return
			"<a:help>Displays a rally point marker where created units will gather when spawned</a:help>"
			"<a:example>"
				"<MarkerTemplate>special/rallypoint</MarkerTemplate>"
				"<LineThickness>0.75</LineThickness>"
				"<LineStartCap>round</LineStartCap>"
				"<LineEndCap>square</LineEndCap>"
				"<LineColour r='20' g='128' b='240'></LineColour>"
				"<LineDashColour r='158' g='11' b='15'></LineDashColour>"
				"<LineCostClass>default</LineCostClass>"
				"<LinePassabilityClass>default</LinePassabilityClass>"
			"</a:example>"
			"<element name='MarkerTemplate' a:help='Template name for the rally point marker entity (typically a waypoint flag actor)'>"
				"<text/>"
			"</element>"
			"<element name='LineTexture' a:help='Texture file to use for the rally point line'>"
				"<text />"
			"</element>"
			"<element name='LineTextureMask' a:help='Texture mask to indicate where overlay colors are to be applied (see LineColour and LineDashColour)'>"
				"<text />"
			"</element>"
			"<element name='LineThickness' a:help='Thickness of the marker line connecting the entity to the rally point marker'>"
				"<data type='decimal'/>"
			"</element>"
			"<element name='LineColour'>"
				"<attribute name='r'>"
					"<data type='integer'><param name='minInclusive'>0</param><param name='maxInclusive'>255</param></data>"
				"</attribute>"
				"<attribute name='g'>"
					"<data type='integer'><param name='minInclusive'>0</param><param name='maxInclusive'>255</param></data>"
				"</attribute>"
				"<attribute name='b'>"
					"<data type='integer'><param name='minInclusive'>0</param><param name='maxInclusive'>255</param></data>"
				"</attribute>"
			"</element>"
			"<element name='LineDashColour'>"
				"<attribute name='r'>"
					"<data type='integer'><param name='minInclusive'>0</param><param name='maxInclusive'>255</param></data>"
				"</attribute>"
				"<attribute name='g'>"
					"<data type='integer'><param name='minInclusive'>0</param><param name='maxInclusive'>255</param></data>"
				"</attribute>"
				"<attribute name='b'>"
					"<data type='integer'><param name='minInclusive'>0</param><param name='maxInclusive'>255</param></data>"
				"</attribute>"
			"</element>"
			"<element name='LineStartCap'>"
				"<choice>"
					"<value a:help='Abrupt line ending; line endings are not closed'>flat</value>"
					"<value a:help='Semi-circular line end cap'>round</value>"
					"<value a:help='Sharp, pointy line end cap'>sharp</value>"
					"<value a:help='Square line end cap'>square</value>"
				"</choice>"
			"</element>"
			"<element name='LineEndCap'>"
				"<choice>"
					"<value a:help='Abrupt line ending; line endings are not closed'>flat</value>"
					"<value a:help='Semi-circular line end cap'>round</value>"
					"<value a:help='Sharp, pointy line end cap'>sharp</value>"
					"<value a:help='Square line end cap'>square</value>"
				"</choice>"
			"</element>"
			"<element name='LinePassabilityClass' a:help='The pathfinder passability class to use for computing the rally point marker line path'>"
				"<text />"
			"</element>"
			"<element name='LineCostClass' a:help='The pathfinder cost class to use for computing the rally point marker line path'>"
				"<text />"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode);

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// do NOT serialize anything; this is a rendering-only component, it does not and should not affect simulation state
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_RenderSubmit:
			{
				PROFILE3("RallyPoint::RenderSubmit");
				if (m_Displayed && IsSet())
				{
					const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
					RenderSubmit(msgData.collector);
				}
			}
			break;
		case MT_OwnershipChanged:
			{
				UpdateMarkers(); // update marker variation to new player's civilization
			}
			break;
		case MT_TurnStart:
			{
				UpdateOverlayLines(); // check for changes to the SoD and update the overlay lines accordingly
			}
			break;
		case MT_Destroy:
			{
				for (std::vector<entity_id_t>::iterator it = m_MarkerEntityIds.begin(); it < m_MarkerEntityIds.end(); ++it)
				{
					if (*it != INVALID_ENTITY)
					{
						GetSimContext().GetComponentManager().DestroyComponentsSoon(*it);
						*it = INVALID_ENTITY;
					}
				}
			}
			break;
		case MT_PositionChanged:
			{
				// Unlikely to happen in-game, but can occur in atlas
				// Just recompute the path from the entity to the first rally point
				RecomputeRallyPointPath_wrapper(0);
			}
			break;
		}
	}

	/*
	 * Must be called whenever m_Displayed or the size of m_RallyPoints change,
	 * to determine whether we need to respond to render messages.
	 */
	void UpdateMessageSubscriptions()
	{
		bool needRender = m_Displayed && IsSet();
		GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_RenderSubmit, this, needRender);
	}

	virtual void AddPosition_wrapper(CFixedVector2D pos)
	{
		AddPosition(pos, false);
	}

	virtual void SetPosition(CFixedVector2D pos)
	{
		if (!(m_RallyPoints.size() == 1 && m_RallyPoints.front() == pos))
		{
			m_RallyPoints.clear();
			AddPosition(pos, true);
			// Don't need to UpdateMessageSubscriptions here since AddPosition already calls it
		}
	}

	virtual void UpdatePosition(u32 rallyPointId, CFixedVector2D pos)
	{
		if (rallyPointId >= m_RallyPoints.size())
			return;

		m_RallyPoints[rallyPointId] = pos;

		UpdateMarkers();

		// Compute a new path for the current, and if existing the next rally point
		RecomputeRallyPointPath_wrapper(rallyPointId);
		if (rallyPointId+1 < m_RallyPoints.size())
			RecomputeRallyPointPath_wrapper(rallyPointId+1);
	}

	virtual void SetDisplayed(bool displayed)
	{
		if (m_Displayed != displayed)
		{
			m_Displayed = displayed;

			// move the markers out of oblivion and back into the real world, or vice-versa
			UpdateMarkers();

			// Check for changes to the SoD and update the overlay lines accordingly. We need to do this here because this method
			// only takes effect when the display flag is active; we need to pick up changes to the SoD that might have occurred 
			// while this rally point was not being displayed.
			UpdateOverlayLines();

			UpdateMessageSubscriptions();
		}
	}

	virtual void Reset()
	{
		m_RallyPoints.clear();
		RecomputeAllRallyPointPaths();
		UpdateMessageSubscriptions();
	}

	/**
	 * Returns true if at least one display rally point is set; i.e., if we have a point to render our marker/line at.
	 */
	bool IsSet()
	{
		return !m_RallyPoints.empty();
	}

private:

	/**
	 * Helper function for AddPosition_wrapper and SetPosition.
	 */
	void AddPosition(CFixedVector2D pos, bool recompute)
	{
		m_RallyPoints.push_back(pos);
		UpdateMarkers();

		if (recompute)
			RecomputeAllRallyPointPaths();
		else
			RecomputeRallyPointPath_wrapper(m_RallyPoints.size()-1);

		UpdateMessageSubscriptions();
	}

	/**
	 * Repositions the rally point markers; moves them outside of the world (ie. hides them), or positions them at the currently
	 * set rally points. Also updates the actor's variation according to the entity's current owning player's civilization.
	 * 
	 * Should be called whenever either the position of a rally point changes (including whether it is set or not), or the display
	 * flag changes, or the ownership of the entity changes.
	 */
	void UpdateMarkers();

	/**
	 * Recomputes all the full paths from this entity to the rally point and from the rally point to the next, and does all the necessary
	 * post-processing to make them prettier.
	 * 
	 * Should be called whenever all rally points' position changes.
	 */
	void RecomputeAllRallyPointPaths();

	/**
	 * Recomputes the full path for m_Path[ @p index], and does all the necessary post-processing to make it prettier.
	 * 
	 * Should be called whenever either the starting position or the rally point's position changes.
	 */
	void RecomputeRallyPointPath_wrapper(size_t index);

	/**
	 * Recomputes the full path from this entity/the previous rally point to the next rally point, and does all the necessary
	 * post-processing to make it prettier. This doesn't check if we have a valid position or if a rally point is set.
	 * 
	 * You shouldn't need to call this method directly.
	 */
	void RecomputeRallyPointPath(size_t index, CmpPtr<ICmpPosition>& cmpPosition, CmpPtr<ICmpFootprint>& cmpFootprint, CmpPtr<ICmpPathfinder> cmpPathfinder);

	/**
	 * Checks for changes to the SoD to the previously saved state, and reconstructs the visibility segments and overlay lines to 
	 * match if necessary. Does nothing if the rally point lines are not currently set to be displayed, or if no rally point is set.
	 */
	void UpdateOverlayLines();

	/**
	 * Sets up all overlay lines for rendering according to the current full path and visibility segments. Splits the line into solid 
	 * and dashed pieces (for the SoD). Should be called whenever the SoD has changed. If no full path is currently set, this method 
	 * does nothing.
	 */
	void ConstructAllOverlayLines();

	/**
	 * Sets up the overlay lines for rendering according to the full path and visibility segments at @p index. Splits the line into 
	 * solid and dashed pieces (for the SoD). Should be called whenever the SoD of the path at @p index has changed. 
	 */
	void ConstructOverlayLines(size_t index);

	/**
	 * Removes points from @p coords that are obstructed by the originating building's footprint, and links up the last point 
	 * nicely to the edge of the building's footprint. Only needed if the pathfinder can possibly return obstructed tile waypoints, 
	 * i.e. when pathfinding is started from an obstructed tile.
	 */
	void FixFootprintWaypoints(std::vector<CVector2D>& coords, CmpPtr<ICmpPosition> cmpPosition, CmpPtr<ICmpFootprint> cmpFootprint);

	/**
	 * Returns a list of indices of waypoints in the current path (m_Path[index]) where the LOS visibility changes, ordered from 
	 * building/previous rally point to rally point. Used to construct the overlay line segments and track changes to the SoD.
	 */
	void GetVisibilitySegments(std::deque<SVisibilitySegment>& out, size_t index);

	/**
	 * Simplifies the path by removing waypoints that lie between two points that are visible from one another. This is primarily 
	 * intended to reduce some unnecessary curviness of the path; the pathfinder returns a mathematically (near-)optimal path, which
	 * will happily curve and bend to reduce costs. Visually, it doesn't make sense for a rally point path to curve and bend when it
	 * could just as well have gone in a straight line; that's why we have this, to make it look more natural.
	 *
	 * @p coords array of path coordinates to simplify
	 * @p maxSegmentLinks if non-zero, indicates the maximum amount of consecutive node-to-node links that can be joined into a
	 *                    single link. If this value is set to e.g. 1, then no reductions will be performed. A value of 3 means that
	 *                    at most 3 consecutive node links will be joined into a single link.
	 * @p floating whether to consider nodes who are under the water level as floating on top of the water
	 */
	void ReduceSegmentsByVisibility(std::vector<CVector2D>& coords, unsigned maxSegmentLinks = 0, bool floating = true);

	/**
	 * Helper function to GetVisibilitySegments, factored out for testing. Merges single-point segments with its neighbouring 
	 * segments. You should not have to call this method directly.
	 */
	static void MergeVisibilitySegments(std::deque<SVisibilitySegment>& segments);

	void RenderSubmit(SceneCollector& collector);
};

REGISTER_COMPONENT_TYPE(RallyPointRenderer)

void CCmpRallyPointRenderer::Init(const CParamNode& paramNode)
{
	m_Displayed = false;
	m_SmoothPath = true;
	m_LastOwner = INVALID_PLAYER;
	m_LastMarkerCount = 0;
	m_EnableDebugNodeOverlay = false;

	// ---------------------------------------------------------------------------------------------
	// load some XML configuration data (schema guarantees that all these nodes are valid)

	m_MarkerTemplate = paramNode.GetChild("MarkerTemplate").ToString();

	const CParamNode& lineColor = paramNode.GetChild("LineColour");
	m_LineColor = CColor(
		lineColor.GetChild("@r").ToInt()/255.f,
		lineColor.GetChild("@g").ToInt()/255.f,
		lineColor.GetChild("@b").ToInt()/255.f,
		1.f
	);

	const CParamNode& lineDashColor = paramNode.GetChild("LineDashColour");
	m_LineDashColor = CColor(
		lineDashColor.GetChild("@r").ToInt()/255.f,
		lineDashColor.GetChild("@g").ToInt()/255.f,
		lineDashColor.GetChild("@b").ToInt()/255.f,
		1.f
	);

	m_LineThickness = paramNode.GetChild("LineThickness").ToFixed().ToFloat();
	m_LineTexturePath = paramNode.GetChild("LineTexture").ToString();
	m_LineTextureMaskPath = paramNode.GetChild("LineTextureMask").ToString();
	m_LineStartCapType = SOverlayTexturedLine::StrToLineCapType(paramNode.GetChild("LineStartCap").ToString());
	m_LineEndCapType = SOverlayTexturedLine::StrToLineCapType(paramNode.GetChild("LineEndCap").ToString());
	m_LineCostClass = paramNode.GetChild("LineCostClass").ToUTF8();
	m_LinePassabilityClass = paramNode.GetChild("LinePassabilityClass").ToUTF8();

	// ---------------------------------------------------------------------------------------------
	// load some textures

	if (CRenderer::IsInitialised())
	{
		CTextureProperties texturePropsBase(m_LineTexturePath);
		texturePropsBase.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE);
		texturePropsBase.SetMaxAnisotropy(4.f);
		m_Texture = g_Renderer.GetTextureManager().CreateTexture(texturePropsBase);

		CTextureProperties texturePropsMask(m_LineTextureMaskPath);
		texturePropsMask.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE);
		texturePropsMask.SetMaxAnisotropy(4.f);
		m_TextureMask = g_Renderer.GetTextureManager().CreateTexture(texturePropsMask);
	}
}

void CCmpRallyPointRenderer::UpdateMarkers()
{
	player_id_t previousOwner = m_LastOwner;
	for (size_t i = 0; i < m_RallyPoints.size(); ++i)
	{
		if (i >= m_MarkerEntityIds.size())
			m_MarkerEntityIds.push_back(INVALID_ENTITY);

		if (m_MarkerEntityIds[i] == INVALID_ENTITY)
		{
			// no marker exists yet, create one first
			CComponentManager& componentMgr = GetSimContext().GetComponentManager();

			// allocate a new entity for the marker
			if (!m_MarkerTemplate.empty())
			{
				m_MarkerEntityIds[i] = componentMgr.AllocateNewLocalEntity();
				if (m_MarkerEntityIds[i] != INVALID_ENTITY)
					m_MarkerEntityIds[i] = componentMgr.AddEntity(m_MarkerTemplate, m_MarkerEntityIds[i]);
			}
		}

		// the marker entity should be valid at this point, otherwise something went wrong trying to allocate it
		if (m_MarkerEntityIds[i] == INVALID_ENTITY)
			LOGERROR(L"Failed to create rally point marker entity");

		CmpPtr<ICmpPosition> markerCmpPosition(GetSimContext(), m_MarkerEntityIds[i]);
		if (markerCmpPosition)
		{
			if (m_Displayed && IsSet())
			{
				markerCmpPosition->JumpTo(m_RallyPoints[i].X, m_RallyPoints[i].Y);
			}
			else
			{
				markerCmpPosition->MoveOutOfWorld(); // hide it
			}
		}

		// set rally point flag selection based on player civilization
		CmpPtr<ICmpOwnership> cmpOwnership(GetEntityHandle());
		if (!cmpOwnership)
			continue;

		player_id_t ownerId = cmpOwnership->GetOwner();
		if (ownerId == INVALID_PLAYER || (ownerId == previousOwner && m_LastMarkerCount >= i))
			continue;

		m_LastOwner = ownerId;
		CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSystemEntity());
		// cmpPlayerManager should not be null as long as this method is called on-demand instead of at Init() time
		// (we can't rely on component initialization order in Init())
		if (!cmpPlayerManager)
			continue;

		CmpPtr<ICmpPlayer> cmpPlayer(GetSimContext(), cmpPlayerManager->GetPlayerByID(ownerId));
		if (!cmpPlayer)
			continue;

		CmpPtr<ICmpVisual> cmpVisualActor(GetSimContext(), m_MarkerEntityIds[i]);
		if (cmpVisualActor)
			cmpVisualActor->SetUnitEntitySelection(CStrW(cmpPlayer->GetCiv()).ToUTF8());
	}
	m_LastMarkerCount = m_RallyPoints.size() - 1;
}

void CCmpRallyPointRenderer::RecomputeAllRallyPointPaths()
{
	m_Path.clear();
	m_VisibilitySegments.clear();
	m_TexturedOverlayLines.clear();

	//// <DEBUG> ///////////////////////////////////////////////
	if (m_EnableDebugNodeOverlay)
		m_DebugNodeOverlays.clear();
	//// </DEBUG> //////////////////////////////////////////////

	if (!IsSet())
		return; // no use computing a path if the rally point isn't set

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return; // no point going on if this entity doesn't have a position or is outside of the world

	CmpPtr<ICmpFootprint> cmpFootprint(GetEntityHandle());
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());

	for (size_t i = 0; i < m_RallyPoints.size(); ++i)
	{
		RecomputeRallyPointPath(i, cmpPosition, cmpFootprint, cmpPathfinder);
	}
}

void CCmpRallyPointRenderer::RecomputeRallyPointPath_wrapper(size_t index)
{
	if (!IsSet())
		return; // no use computing a path if the rally point isn't set

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return; // no point going on if this entity doesn't have a position or is outside of the world

	CmpPtr<ICmpFootprint> cmpFootprint(GetEntityHandle());
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());

	RecomputeRallyPointPath(index, cmpPosition, cmpFootprint, cmpPathfinder);
}

void CCmpRallyPointRenderer::RecomputeRallyPointPath(size_t index, CmpPtr<ICmpPosition>& cmpPosition, CmpPtr<ICmpFootprint>& cmpFootprint, CmpPtr<ICmpPathfinder> cmpPathfinder)
{
	while (index >= m_Path.size())
	{
		std::vector<CVector2D> tmp;
		m_Path.push_back(tmp);
	}
	m_Path[index].clear();

	while (index >= m_VisibilitySegments.size())
	{
		std::deque<SVisibilitySegment> tmp;
		m_VisibilitySegments.push_back(tmp);
	}
	m_VisibilitySegments[index].clear();

	entity_pos_t pathStartX;
	entity_pos_t pathStartY;

	if (index == 0)
	{
		pathStartX = cmpPosition->GetPosition2D().X;
		pathStartY = cmpPosition->GetPosition2D().Y;
	}
	else
	{
		pathStartX = m_RallyPoints[index-1].X;
		pathStartY = m_RallyPoints[index-1].Y;
	}

	// Find a long path to the goal point -- this uses the tile-based pathfinder, which will return a
	// list of waypoints (i.e. a Path) from the building/previous rally point to the goal, where each
	// waypoint is centered at a tile. We'll have to do some post-processing on the path to get it smooth.
	Path path;
	std::vector<Waypoint>& waypoints = path.m_Waypoints;

	Goal goal = { Goal::POINT, m_RallyPoints[index].X, m_RallyPoints[index].Y };
	cmpPathfinder->ComputePath(
		pathStartX,
		pathStartY,
		goal,
		cmpPathfinder->GetPassabilityClass(m_LinePassabilityClass),
		cmpPathfinder->GetCostClass(m_LineCostClass),
		path
	);

	// Check if we got a path back; if not we probably have two markers less than one tile apart.
	if (path.m_Waypoints.size() < 2)
	{
		m_Path[index].push_back(CVector2D(goal.x.ToFloat(), goal.z.ToFloat()));
		m_Path[index].push_back(CVector2D(pathStartX.ToFloat(), pathStartY.ToFloat()));
		return;
	}

	// From here on, we choose to represent the waypoints as CVector2D floats to avoid to have to convert back and forth
	// between fixed-point Waypoint/CFixedVector2D and various other float-based formats used by interpolation and whatnot.
	// Since we'll only be further using these points for rendering purposes, using floats should be fine.

	// Make sure to add the actual goal point as the last point (the long pathfinder only finds paths to the tile closest to the 
	// goal, so we need to complete the last bit from the closest tile to the rally point itself)
	// NOTE: the points are returned in reverse order (from the goal to the start point), so we actually need to insert it at the 
	// front of the coordinate list. Hence, we'll do this first before appending the rest of the fixed waypoints as CVector2Ds.

	Waypoint& lastWaypoint = waypoints.back();
	if (lastWaypoint.x != goal.x || lastWaypoint.z != goal.z)
		m_Path[index].push_back(CVector2D(goal.x.ToFloat(), goal.z.ToFloat()));

	// add the rest of the waypoints
	for (size_t i = 0; i < waypoints.size(); ++i)
		m_Path[index].push_back(CVector2D(waypoints[i].x.ToFloat(), waypoints[i].z.ToFloat()));

	// add the start position
	m_Path[index].push_back(CVector2D(pathStartX.ToFloat(), pathStartY.ToFloat()));

	// -------------------------------------------------------------------------------------------
	// post-processing

	// Linearize the path;
	// Pass through the waypoints, averaging each waypoint with its next one except the last one. Because the path
	// goes from the marker to this entity/the previous flag and we want to keep the point at the marker's exact position,
	// loop backwards through the waypoints so that the marker waypoint is maintained.
	// TODO: see if we can do this at the same time as the waypoint -> coord conversion above
	for(size_t i = m_Path[index].size() - 2; i > 0; --i)
		m_Path[index][i] = (m_Path[index][i] + m_Path[index][i-1]) / 2.0f;

	// if there's a footprint and this path starts from this entity, remove any points returned by the pathfinder that may be on obstructed footprint tiles
	if (index == 0 && cmpFootprint)
		FixFootprintWaypoints(m_Path[index], cmpPosition, cmpFootprint);

	// Eliminate some consecutive waypoints that are visible from eachother. Reduce across a maximum distance of approx. 6 tiles 
	// (prevents segments that are too long to properly stick to the terrain)
	ReduceSegmentsByVisibility(m_Path[index], 6);

	//// <DEBUG> ///////////////////////////////////////////////
	if (m_EnableDebugNodeOverlay)
	{
		while (index >= m_DebugNodeOverlays.size())
		{
			std::vector<SOverlayLine> tmp;
			m_DebugNodeOverlays.push_back(tmp);
		}
		m_DebugNodeOverlays[index].clear();
	}
	if (m_EnableDebugNodeOverlay && m_SmoothPath)
	{
		// Create separate control point overlays so we can differentiate when using smoothing (offset them a little higher from the
		// terrain so we can still see them after the interpolated points are added)
		for (size_t j = 0; j < m_Path[index].size(); ++j)
		{
			SOverlayLine overlayLine;
			overlayLine.m_Color = CColor(1.0f, 0.0f, 0.0f, 1.0f);
			overlayLine.m_Thickness = 2;
			SimRender::ConstructSquareOnGround(GetSimContext(), m_Path[index][j].X, m_Path[index][j].Y, 0.2f, 0.2f, 1.0f, overlayLine, true);
			m_DebugNodeOverlays[index].push_back(overlayLine);
		}
	}
	//// </DEBUG> //////////////////////////////////////////////

	if (m_SmoothPath)
		// The number of points to interpolate goes hand in hand with the maximum amount of node links allowed to be joined together
		// by the visibility reduction. The more node links that can be joined together, the more interpolated points you need to 
		// generate to be able to deal with local terrain height changes.
		SimRender::InterpolatePointsRNS(m_Path[index], false, 0, 8); // no offset, keep line at its exact path

	// -------------------------------------------------------------------------------------------

	// find which point is the last visible point before going into the SoD, so we have a point to compare to on the next turn
	GetVisibilitySegments(m_VisibilitySegments[index], index); 

	// build overlay lines for the new path
	ConstructOverlayLines(index);
}

void CCmpRallyPointRenderer::ConstructAllOverlayLines()
{
	m_TexturedOverlayLines.clear();

	for (size_t i = 0; i < m_Path.size(); ++i)
		ConstructOverlayLines(i);
}

void CCmpRallyPointRenderer::ConstructOverlayLines(size_t index)
{
	// We need to create a new SOverlayTexturedLine every time we want to change the coordinates after having passed it to the 
	// renderer, because it does some fancy vertex buffering thing and caches them internally instead of recomputing them on every 
	// pass (which is only sensible).
	while (index >= m_TexturedOverlayLines.size())
	{
		std::vector<SOverlayTexturedLine> tmp;
		m_TexturedOverlayLines.push_back(tmp);
	}
	m_TexturedOverlayLines[index].clear();

	if (m_Path[index].size() < 2)
		return;

	CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
	LineCapType dashesLineCapType = SOverlayTexturedLine::LINECAP_ROUND; // line caps to use for the dashed segments (and any other segment's edges that border it)

	for (std::deque<SVisibilitySegment>::const_iterator it = m_VisibilitySegments[index].begin(); it != m_VisibilitySegments[index].end(); ++it)
	{
		const SVisibilitySegment& segment = (*it);

		if (segment.m_Visible)
		{
			// does this segment border on the building or rally point flag on either side?
			bool bordersBuilding = (segment.m_EndIndex == m_Path[index].size() - 1);
			bool bordersFlag = (segment.m_StartIndex == 0);

			// construct solid textured overlay line along a subset of the full path points from startPointIdx to endPointIdx
			SOverlayTexturedLine overlayLine;
			overlayLine.m_Thickness = m_LineThickness;
			overlayLine.m_SimContext = &GetSimContext();
			overlayLine.m_TextureBase = m_Texture;
			overlayLine.m_TextureMask = m_TextureMask;
			overlayLine.m_Color = m_LineColor;
			overlayLine.m_Closed = false;
			// we should take care to only use m_LineXCap for the actual end points at the building and the rally point; any intermediate
			// end points (i.e., that border a dashed segment) should have the dashed cap
			// the path line is actually in reverse order as well, so let's swap out the start and end caps
			overlayLine.m_StartCapType = (bordersFlag ? m_LineEndCapType : dashesLineCapType);
			overlayLine.m_EndCapType = (bordersBuilding ? m_LineStartCapType : dashesLineCapType);
			overlayLine.m_AlwaysVisible = true;

			// push overlay line coordinates
			ENSURE(segment.m_EndIndex > segment.m_StartIndex);
			for (size_t j = segment.m_StartIndex; j <= segment.m_EndIndex; ++j) // end index is inclusive here
			{
				overlayLine.m_Coords.push_back(m_Path[index][j].X);
				overlayLine.m_Coords.push_back(m_Path[index][j].Y);
			}

			m_TexturedOverlayLines[index].push_back(overlayLine);
		}
		else
		{
			// construct dashed line from startPointIdx to endPointIdx; add textured overlay lines for it to the render list
			std::vector<CVector2D> straightLine;
			straightLine.push_back(m_Path[index][segment.m_StartIndex]);
			straightLine.push_back(m_Path[index][segment.m_EndIndex]);

			// We always want to the dashed line to end at either point with a full dash (i.e. not a cleared space), so that the dashed
			// area is visually obvious. This requires some calculations to see what size we should make the dashes and clears for them
			// to fit exactly.

			float maxDashSize = 3.f;
			float maxClearSize = 3.f;
			
			float dashSize = maxDashSize;
			float clearSize = maxClearSize;
			float pairDashRatio = (dashSize / (dashSize + clearSize)); // ratio of the dash's length to a (dash + clear) pair's length

			float distance = (m_Path[index][segment.m_StartIndex] - m_Path[index][segment.m_EndIndex]).Length(); // straight-line distance between the points

			// See how many pairs (dash + clear) of unmodified size can fit into the distance. Then check the remaining distance; if it's not exactly
			// a dash size's worth (which it probably won't be), then adjust the dash/clear sizes slightly so that it is.
			int numFitUnmodified = floor(distance/(dashSize + clearSize));
			float remainderDistance = distance - (numFitUnmodified * (dashSize + clearSize));

			// Now we want to make remainderDistance equal exactly one dash size (i.e. maxDashSize) by scaling dashSize and clearSize slightly. 
			// We have (remainderDistance - maxDashSize) of space to distribute over numFitUnmodified instances of (dashSize + clearSize) to make
			// it fit, so each (dashSize + clearSize) pair needs to adjust its length by (remainderDistance - maxDashSize)/numFitUnmodified 
			// (which will be positive or negative accordingly). This number can then be distributed further proportionally among the dash's 
			// length and the clear's length.

			// we always want to have at least one dash/clear pair (i.e., "|===|   |===|"); also, we need to avoid division by zero below.
			numFitUnmodified = std::max(1, numFitUnmodified);

			float pairwiseLengthDifference = (remainderDistance - maxDashSize)/numFitUnmodified; // can be either positive or negative
			dashSize += pairDashRatio * pairwiseLengthDifference;
			clearSize += (1 - pairDashRatio) * pairwiseLengthDifference;

			// ------------------------------------------------------------------------------------------------

			SDashedLine dashedLine;
			SimRender::ConstructDashedLine(straightLine, dashedLine, dashSize, clearSize);

			// build overlay lines for dashes
			size_t numDashes = dashedLine.m_StartIndices.size();
			for (size_t i=0; i < numDashes; i++)
			{
				SOverlayTexturedLine dashOverlay;

				dashOverlay.m_Thickness = m_LineThickness;
				dashOverlay.m_SimContext = &GetSimContext();
				dashOverlay.m_TextureBase = m_Texture;
				dashOverlay.m_TextureMask = m_TextureMask;
				dashOverlay.m_Color = m_LineDashColor;
				dashOverlay.m_Closed = false;
				dashOverlay.m_StartCapType = dashesLineCapType;
				dashOverlay.m_EndCapType = dashesLineCapType;
				dashOverlay.m_AlwaysVisible = true;
				// TODO: maybe adjust the elevation of the dashes to be a little lower, so that it slides underneath the actual path

				size_t dashStartIndex = dashedLine.m_StartIndices[i];
				size_t dashEndIndex = dashedLine.GetEndIndex(i);
				ENSURE(dashEndIndex > dashStartIndex);

				for (size_t n = dashStartIndex; n < dashEndIndex; n++)
				{
					dashOverlay.m_Coords.push_back(dashedLine.m_Points[n].X);
					dashOverlay.m_Coords.push_back(dashedLine.m_Points[n].Y);
				}

				m_TexturedOverlayLines[index].push_back(dashOverlay);
			}
			
		}
	}

	//// <DEBUG> //////////////////////////////////////////////
	if (m_EnableDebugNodeOverlay)
	{
		while (index >= m_DebugNodeOverlays.size())
		{
			std::vector<SOverlayLine> tmp;
			m_DebugNodeOverlays.push_back(tmp);
		}
		for (size_t j = 0; j < m_Path[index].size(); ++j)
		{
			SOverlayLine overlayLine;
			overlayLine.m_Color = CColor(1.0f, 1.0f, 1.0f, 1.0f);
			overlayLine.m_Thickness = 1;
			SimRender::ConstructCircleOnGround(GetSimContext(), m_Path[index][j].X, m_Path[index][j].Y, 0.075f, overlayLine, true);
			m_DebugNodeOverlays[index].push_back(overlayLine);
		}
	}
	//// </DEBUG> //////////////////////////////////////////////
}

void CCmpRallyPointRenderer::UpdateOverlayLines()
{
	// We should only do this if the rally point is currently being displayed and set inside the world, otherwise it's a massive 
	// waste of time to calculate all this stuff (this method is called every turn)
	if (!m_Displayed || !IsSet())
		return;

	// see if there have been any changes to the SoD by grabbing the visibility edge points and comparing them to the previous ones
	std::deque<std::deque<SVisibilitySegment> > newVisibilitySegments;
	for (size_t i = 0; i < m_Path.size(); ++i)
	{
		std::deque<SVisibilitySegment> tmp;
		newVisibilitySegments.push_back(tmp);
		GetVisibilitySegments(newVisibilitySegments[i], i);
	}

	// Check if the full path changed, then reconstruct all overlay lines, otherwise check if a segment changed and update that.
	if (m_VisibilitySegments.size() != newVisibilitySegments.size())
	{
		m_VisibilitySegments = newVisibilitySegments; // save the new visibility segments to compare against next time
		ConstructAllOverlayLines();
	}
	else
	{
		for (size_t i = 0; i < m_VisibilitySegments.size(); ++i)
		{
			if (m_VisibilitySegments[i] != newVisibilitySegments[i])
			{
				// The visibility segments have changed, reconstruct the overlay lines to match. NOTE: The path itself doesn't
				// change, only the overlay lines we construct from it.
				m_VisibilitySegments[i] = newVisibilitySegments[i]; // save the new visibility segments to compare against next time
				ConstructOverlayLines(i);
			}
		}
	}
}

void CCmpRallyPointRenderer::FixFootprintWaypoints(std::vector<CVector2D>& coords, CmpPtr<ICmpPosition> cmpPosition, CmpPtr<ICmpFootprint> cmpFootprint)
{
	ENSURE(cmpPosition);
	ENSURE(cmpFootprint);

	// -----------------------------------------------------------------------------------------------------
	// TODO: nasty fixed/float conversions everywhere

	// grab the shape and dimensions of the footprint
	entity_pos_t footprintSize0, footprintSize1, footprintHeight;
	ICmpFootprint::EShape footprintShape;
	cmpFootprint->GetShape(footprintShape, footprintSize0, footprintSize1, footprintHeight);

	// grab the center of the footprint
	CFixedVector2D center = cmpPosition->GetPosition2D();

	// -----------------------------------------------------------------------------------------------------

	switch (footprintShape)
	{
	case ICmpFootprint::SQUARE:
		{
			// in this case, footprintSize0 and 1 indicate the size along the X and Z axes, respectively.

			// the building's footprint could be rotated any which way, so let's get the rotation around the Y axis
			// and the rotated unit vectors in the X/Z plane of the shape's footprint
			// (the Footprint itself holds only the outline, the Position holds the orientation)

			fixed s, c; // sine and cosine of the Y axis rotation angle (aka the yaw)
			fixed a = cmpPosition->GetRotation().Y;
			sincos_approx(a, s, c);
			CFixedVector2D u(c, -s); // unit vector along the rotated X axis
			CFixedVector2D v(s, c); // unit vector along the rotated Z axis
			CFixedVector2D halfSize(footprintSize0/2, footprintSize1/2);

			// starting from the start position, check if any points are within the footprint of the building
			// (this is possible if the pathfinder was started from a point located within the footprint)
			for(int i = (int)(coords.size() - 1); i >= 0; i--)
			{
				const CVector2D& wp = coords[i];
				if (Geometry::PointIsInSquare(CFixedVector2D(fixed::FromFloat(wp.X), fixed::FromFloat(wp.Y)) - center, u, v, halfSize))
				{
					coords.erase(coords.begin() + i);
				}
				else
				{
					break; // point no longer inside footprint, from this point on neither will any of the following be
				}
			}

			// add a point right on the edge of the footprint (nearest to the last waypoint) so that it links up nicely with the rest of the path
			CFixedVector2D lastWaypoint(fixed::FromFloat(coords.back().X), fixed::FromFloat(coords.back().Y));
			CFixedVector2D footprintEdgePoint = Geometry::NearestPointOnSquare(lastWaypoint - center, u, v, halfSize); // relative to the shape origin (center)
			CVector2D footprintEdge((center.X + footprintEdgePoint.X).ToFloat(), (center.Y + footprintEdgePoint.Y).ToFloat());
			coords.push_back(footprintEdge);

		}
		break;
	case ICmpFootprint::CIRCLE:
		{
			// in this case, both footprintSize0 and 1 indicate the circle's radius

			for(int i = (int)(coords.size() - 1); i >= 0; i--)
			{
				const CVector2D& wp = coords[i];
				fixed pointDistance = (CFixedVector2D(fixed::FromFloat(wp.X), fixed::FromFloat(wp.Y)) - center).Length();
				if (pointDistance <= footprintSize0)
				{
					coords.erase(coords.begin() + i);
				}
				else
				{
					break; // point no longer inside footprint, from this point on neither will any of the following be
				}
			}

			// add a point right on the edge of the footprint so that it links up nicely with the rest of the path
			CVector2D centerVec2D(center.X.ToFloat(), center.Y.ToFloat());
			CVector2D centerToLast(coords.back() - centerVec2D);
			coords.push_back(centerVec2D + (centerToLast.Normalized() * footprintSize0.ToFloat()));
		}
		break;
	}
}

void CCmpRallyPointRenderer::ReduceSegmentsByVisibility(std::vector<CVector2D>& coords, unsigned maxSegmentLinks, bool floating)
{
	CmpPtr<ICmpPathfinder> cmpPathFinder(GetSystemEntity());
	CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
	CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());
	ENSURE(cmpPathFinder && cmpTerrain && cmpWaterManager);

	if (coords.size() < 3)
		return;

	// The basic idea is this: starting from a base node, keep checking each individual point along the path to see if there's a visible
	// line between it and the base point. If so, keep going, otherwise, make the last visible point the new base node and start the same
	// process from there on until the entire line is checked. The output is the array of base nodes.

	std::vector<CVector2D> newCoords;
	StationaryOnlyObstructionFilter obstructionFilter;
	entity_pos_t lineRadius = fixed::FromFloat(m_LineThickness);
	ICmpPathfinder::pass_class_t passabilityClass = cmpPathFinder->GetPassabilityClass(m_LinePassabilityClass);

	newCoords.push_back(coords[0]); // save the first base node

	size_t baseNodeIdx = 0;
	size_t curNodeIdx = 1;

	float baseNodeY;
	entity_pos_t baseNodeX;
	entity_pos_t baseNodeZ;

	// set initial base node coords
	baseNodeX = fixed::FromFloat(coords[baseNodeIdx].X);
	baseNodeZ = fixed::FromFloat(coords[baseNodeIdx].Y);
	baseNodeY = cmpTerrain->GetExactGroundLevel(coords[baseNodeIdx].X, coords[baseNodeIdx].Y);
	if (floating)
		baseNodeY = std::max(baseNodeY, cmpWaterManager->GetExactWaterLevel(coords[baseNodeIdx].X, coords[baseNodeIdx].Y));

	while (curNodeIdx < coords.size())
	{
		ENSURE(curNodeIdx > baseNodeIdx); // this needs to be true at all times, otherwise we're checking visibility between a point and itself

		entity_pos_t curNodeX = fixed::FromFloat(coords[curNodeIdx].X);
		entity_pos_t curNodeZ = fixed::FromFloat(coords[curNodeIdx].Y);
		float curNodeY = cmpTerrain->GetExactGroundLevel(coords[curNodeIdx].X, coords[curNodeIdx].Y);
		if (floating)
			curNodeY = std::max(curNodeY, cmpWaterManager->GetExactWaterLevel(coords[curNodeIdx].X, coords[curNodeIdx].Y));

		// find out whether curNode is visible from baseNode (careful; this is in 2D only; terrain height differences are ignored!)
		bool curNodeVisible = cmpPathFinder->CheckMovement(obstructionFilter, baseNodeX, baseNodeZ, curNodeX, curNodeZ, lineRadius, passabilityClass);

		// since height differences are ignored by CheckMovement, let's call two points visible from one another only if they're at 
		// roughly the same terrain elevation
		curNodeVisible = curNodeVisible && (fabsf(curNodeY - baseNodeY) < 3.f); // TODO: this could probably use some tuning
		if (maxSegmentLinks > 0)
			// max. amount of node-to-node links to be eliminated (unsigned subtraction is valid because curNodeIdx is always > baseNodeIdx)
			curNodeVisible = curNodeVisible && ((curNodeIdx - baseNodeIdx) <= maxSegmentLinks);

		if (!curNodeVisible)
		{
			// current node is not visible from the base node, so the previous one was the last visible point from baseNode and should
			// hence become the new base node for further iterations.

			// if curNodeIdx is adjacent to the current baseNode (which is possible due to steep height differences, e.g. hills), then
			// we should take care not to stay stuck at the current base node
			if (curNodeIdx > baseNodeIdx + 1)
			{
				baseNodeIdx = curNodeIdx - 1;
			}
			else
			{
				// curNodeIdx == baseNodeIdx + 1
				baseNodeIdx = curNodeIdx;
				curNodeIdx++; // move the next candidate node one forward so that we don't test a point against itself in the next iteration
			}

			newCoords.push_back(coords[baseNodeIdx]); // add new base node to output list

			// update base node coordinates
			baseNodeX = fixed::FromFloat(coords[baseNodeIdx].X);
			baseNodeZ = fixed::FromFloat(coords[baseNodeIdx].Y);
			baseNodeY = cmpTerrain->GetExactGroundLevel(coords[baseNodeIdx].X, coords[baseNodeIdx].Y);
			if (floating)
				baseNodeY = std::max(baseNodeY, cmpWaterManager->GetExactWaterLevel(coords[baseNodeIdx].X, coords[baseNodeIdx].Y));
		}

		curNodeIdx++;
	}

	// we always need to add the last point back to the array; if e.g. all the points up to the last one are all visible from the current
	// base node, then the loop above just ends and no endpoint is ever added to the list.
	ENSURE(curNodeIdx == coords.size());
	newCoords.push_back(coords[coords.size() - 1]);

	coords.swap(newCoords);
}

void CCmpRallyPointRenderer::GetVisibilitySegments(std::deque<SVisibilitySegment>& out, size_t index)
{
	out.clear();

	if (m_Path[index].size() < 2)
		return;

	CmpPtr<ICmpRangeManager> cmpRangeMgr(GetSystemEntity());

	player_id_t currentPlayer = GetSimContext().GetCurrentDisplayedPlayer();
	CLosQuerier losQuerier(cmpRangeMgr->GetLosQuerier(currentPlayer));

	// go through the path node list, comparing each node's visibility with the previous one. If it changes, end the current segment and start
	// a new one at the next point.

	bool lastVisible = losQuerier.IsExplored(
		(fixed::FromFloat(m_Path[index][0].X) / (int) TERRAIN_TILE_SIZE).ToInt_RoundToNearest(),
		(fixed::FromFloat(m_Path[index][0].Y) / (int) TERRAIN_TILE_SIZE).ToInt_RoundToNearest()
	);
	size_t curSegmentStartIndex = 0; // starting node index of the current segment

	for (size_t k = 1; k < m_Path[index].size(); ++k)
	{
		// grab tile indices for this coord
		int i = (fixed::FromFloat(m_Path[index][k].X) / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();
		int j = (fixed::FromFloat(m_Path[index][k].Y) / (int)TERRAIN_TILE_SIZE).ToInt_RoundToNearest();

		bool nodeVisible = losQuerier.IsExplored(i, j);
		if (nodeVisible != lastVisible)
		{
			// visibility changed; write out the segment that was just completed and get ready for the new one
			out.push_back(SVisibilitySegment(lastVisible, curSegmentStartIndex, k - 1));

			//curSegmentStartIndex = k; // new segment starts here
			curSegmentStartIndex = k - 1;
			lastVisible = nodeVisible;
		}

	}

	// terminate the last segment
	out.push_back(SVisibilitySegment(lastVisible, curSegmentStartIndex, m_Path[index].size() - 1));

	MergeVisibilitySegments(out);
}

void CCmpRallyPointRenderer::MergeVisibilitySegments(std::deque<SVisibilitySegment>& segments)
{
	// Scan for single-point segments; if they are inbetween two other segments, delete them and merge the surrounding segments.
	// If they're at either end of the path, include them in their bordering segment (but only if those bordering segments aren't 
	// themselves single-point segments, because then we would want those to get absorbed by its surrounding ones first).

	// first scan for absorptions of single-point surrounded segments (i.e. excluding edge segments)
	size_t numSegments = segments.size();

	// WARNING: FOR LOOP TRICKERY AHEAD!
	for (size_t i = 1; i < numSegments - 1;)
	{
		SVisibilitySegment& segment = segments[i];
		if (segment.IsSinglePoint())
		{
			// since the segments' visibility alternates, the surrounding ones should have the same visibility
			ENSURE(segments[i-1].m_Visible == segments[i+1].m_Visible);

			segments[i-1].m_EndIndex = segments[i+1].m_EndIndex; // make previous segment span all the way across to the next
			segments.erase(segments.begin() + i); // erase this segment ...
			segments.erase(segments.begin() + i); // and the next (we removed [i], so [i+1] is now at position [i])
			numSegments -= 2; // we removed 2 segments, so update the loop condition
			// in the next iteration, i should still point to the segment right after the one that got expanded, which is now
			// at position i; so don't increment i here
		}
		else
		{
			++i;
		}
	}

	ENSURE(numSegments == segments.size());

	// check to see if the first segment needs to be merged with its neighbour
	if (segments.size() >= 2 && segments[0].IsSinglePoint())
	{
		int firstSegmentStartIndex = segments.front().m_StartIndex;
		ENSURE(firstSegmentStartIndex == 0);
		ENSURE(!segments[1].IsSinglePoint()); // at this point, the second segment should never be a single-point segment

		segments.erase(segments.begin());
		segments.front().m_StartIndex = firstSegmentStartIndex;
	}

	// check to see if the last segment needs to be merged with its neighbour
	if (segments.size() >= 2 && segments[segments.size()-1].IsSinglePoint())
	{
		int lastSegmentEndIndex = segments.back().m_EndIndex;
		ENSURE(!segments[segments.size()-2].IsSinglePoint()); // at this point, the second-to-last segment should never be a single-point segment

		segments.erase(segments.end());
		segments.back().m_EndIndex = lastSegmentEndIndex;
	}

	// --------------------------------------------------------------------------------------------------------
	// at this point, every segment should have at least 2 points
	for (size_t i = 0; i < segments.size(); ++i)
	{
		ENSURE(!segments[i].IsSinglePoint());
		ENSURE(segments[i].m_EndIndex > segments[i].m_StartIndex);
	}
}

void CCmpRallyPointRenderer::RenderSubmit(SceneCollector& collector)
{
	// we only get here if the rally point is set and should be displayed
	for (size_t i = 0; i < m_TexturedOverlayLines.size(); ++i)
	{
		for (size_t j = 0; j < m_TexturedOverlayLines[i].size(); ++j)
		{
			if (!m_TexturedOverlayLines[i][j].m_Coords.empty())
				collector.Submit(&m_TexturedOverlayLines[i][j]);
		}
	}

	if (m_EnableDebugNodeOverlay && !m_DebugNodeOverlays.empty())
	{
		for (size_t i = 0; i < m_DebugNodeOverlays.size(); ++i)
			for (size_t j = 0; j < m_DebugNodeOverlays[i].size(); ++j)
				collector.Submit(&m_DebugNodeOverlays[i][j]);
	}
}
