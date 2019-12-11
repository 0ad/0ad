/* Copyright (C) 2019 Wildfire Games.
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
#include "CCmpRallyPointRenderer.h"

std::string CCmpRallyPointRenderer::GetSchema()
{
	return
		"<a:help>Displays a rally point marker where created units will gather when spawned</a:help>"
		"<a:example>"
			"<MarkerTemplate>special/rallypoint</MarkerTemplate>"
			"<LineThickness>0.75</LineThickness>"
			"<LineStartCap>round</LineStartCap>"
			"<LineEndCap>square</LineEndCap>"
			"<LineDashColor r='158' g='11' b='15'></LineDashColor>"
			"<LinePassabilityClass>default</LinePassabilityClass>"
		"</a:example>"
		"<element name='MarkerTemplate' a:help='Template name for the rally point marker entity (typically a waypoint flag actor)'>"
			"<text/>"
		"</element>"
		"<element name='LineTexture' a:help='Texture file to use for the rally point line'>"
			"<text />"
		"</element>"
		"<element name='LineTextureMask' a:help='Texture mask to indicate where overlay colors are to be applied (see LineColor and LineDashColor)'>"
			"<text />"
		"</element>"
		"<element name='LineThickness' a:help='Thickness of the marker line connecting the entity to the rally point marker'>"
			"<data type='decimal'/>"
		"</element>"
		"<element name='LineDashColor'>"
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
		"</element>";
}

void CCmpRallyPointRenderer::Init(const CParamNode& paramNode)
{
	m_Displayed = false;
	m_SmoothPath = true;
	m_LastOwner = INVALID_PLAYER;
	m_LastMarkerCount = 0;
	m_EnableDebugNodeOverlay = false;

	UpdateLineColor();
	// ---------------------------------------------------------------------------------------------
	// Load some XML configuration data (schema guarantees that all these nodes are valid)

	m_MarkerTemplate = paramNode.GetChild("MarkerTemplate").ToString();
	const CParamNode& lineDashColor = paramNode.GetChild("LineDashColor");
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
	m_LinePassabilityClass = paramNode.GetChild("LinePassabilityClass").ToUTF8();

	// ---------------------------------------------------------------------------------------------
	// Load some textures

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

void CCmpRallyPointRenderer::ClassInit(CComponentManager& componentManager)
{
	componentManager.SubscribeGloballyToMessageType(MT_PlayerColorChanged);
	componentManager.SubscribeToMessageType(MT_OwnershipChanged);
	componentManager.SubscribeToMessageType(MT_TurnStart);
	componentManager.SubscribeToMessageType(MT_Destroy);
	componentManager.SubscribeToMessageType(MT_PositionChanged);
}

void CCmpRallyPointRenderer::Deinit()
{
}

void CCmpRallyPointRenderer::Serialize(ISerializer& UNUSED(serialize))
{
	// Do NOT serialize anything; this is a rendering-only component, it does not and should not affect simulation state
}

void CCmpRallyPointRenderer::Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
{
	Init(paramNode);
	// The dependent components have not been deserialized, so the color is loaded on first SetDisplayed
}

void CCmpRallyPointRenderer::HandleMessage(const CMessage& msg, bool UNUSED(global))
{
	switch (msg.GetType())
	{
	case MT_PlayerColorChanged:
	{
		const CMessagePlayerColorChanged& msgData = static_cast<const CMessagePlayerColorChanged&> (msg);

		CmpPtr<ICmpOwnership> cmpOwnership(GetEntityHandle());
		if (!cmpOwnership || msgData.player != cmpOwnership->GetOwner())
			break;

		UpdateLineColor();
		ConstructAllOverlayLines();
	}
	break;
	case MT_RenderSubmit:
	{
		PROFILE("RallyPoint::RenderSubmit");
		if (m_Displayed && IsSet())
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector, msgData.frustum, msgData.culling);
		}
	}
	break;
	case MT_OwnershipChanged:
	{
		const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);

		// Ignore destroyed entities
		if (msgData.to == INVALID_PLAYER)
			break;
		Reset();
		// Required for both the initial and capturing players color
		UpdateLineColor();

		// Support capturing, even though RallyPoint is typically deleted then
		UpdateMarkers();
		ConstructAllOverlayLines();
	}
	break;
	case MT_TurnStart:
	{
		UpdateOverlayLines(); // Check for changes to the SoD and update the overlay lines accordingly
	}
	break;
	case MT_Destroy:
	{
		Reset();
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

void CCmpRallyPointRenderer::UpdateMessageSubscriptions()
{
	GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_RenderSubmit, this, m_Displayed && IsSet());
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
			// No marker exists yet, create one first
			CComponentManager& componentMgr = GetSimContext().GetComponentManager();

			// Allocate a new entity for the marker
			if (!m_MarkerTemplate.empty())
			{
				m_MarkerEntityIds[i] = componentMgr.AllocateNewLocalEntity();
				if (m_MarkerEntityIds[i] != INVALID_ENTITY)
					m_MarkerEntityIds[i] = componentMgr.AddEntity(m_MarkerTemplate, m_MarkerEntityIds[i]);
			}
		}

		// The marker entity should be valid at this point, otherwise something went wrong trying to allocate it
		if (m_MarkerEntityIds[i] == INVALID_ENTITY)
			LOGERROR("Failed to create rally point marker entity");

		CmpPtr<ICmpPosition> markerCmpPosition(GetSimContext(), m_MarkerEntityIds[i]);
		if (markerCmpPosition)
		{
			if (m_Displayed && IsSet())
			{
				markerCmpPosition->MoveTo(m_RallyPoints[i].X, m_RallyPoints[i].Y);
			}
			else
			{
				markerCmpPosition->MoveOutOfWorld();
			}
		}

		// Set rally point flag selection based on player civilization
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
			cmpVisualActor->SetVariant("civ", CStrW(cmpPlayer->GetCiv()).ToUTF8());
	}
	m_LastMarkerCount = m_RallyPoints.size() - 1;
}

void CCmpRallyPointRenderer::UpdateLineColor()
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

	m_LineColor = cmpPlayer->GetDisplayedColor();
}

void CCmpRallyPointRenderer::RecomputeAllRallyPointPaths()
{
	m_Path.clear();
	m_VisibilitySegments.clear();
	m_TexturedOverlayLines.clear();

	if (m_EnableDebugNodeOverlay)
		m_DebugNodeOverlays.clear();

	// No use computing a path if the rally point isn't set
	if (!IsSet())
		return;

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	// No point going on if this entity doesn't have a position or is outside of the world
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return;

	CmpPtr<ICmpFootprint> cmpFootprint(GetEntityHandle());
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());

	for (size_t i = 0; i < m_RallyPoints.size(); ++i)
	{
		RecomputeRallyPointPath(i, cmpPosition, cmpFootprint, cmpPathfinder);
	}
}

void CCmpRallyPointRenderer::RecomputeRallyPointPath_wrapper(size_t index)
{
	// No use computing a path if the rally point isn't set
	if (!IsSet())
		return;

	// No point going on if this entity doesn't have a position or is outside of the world
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return;

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
		std::vector<SVisibilitySegment> tmp;
		m_VisibilitySegments.push_back(tmp);
	}
	m_VisibilitySegments[index].clear();

	// Find a long path to the goal point -- this uses the tile-based pathfinder, which will return a
	// list of waypoints (i.e. a Path) from the goal to the foundation/previous rally point, where each
	// waypoint is centered at a tile. We'll have to do some post-processing on the path to get it smooth.
	WaypointPath path;
	std::vector<Waypoint>& waypoints = path.m_Waypoints;

	CFixedVector2D start(cmpPosition->GetPosition2D());
	PathGoal goal = { PathGoal::POINT, m_RallyPoints[index].X, m_RallyPoints[index].Y };

	if (index == 0)
		GetClosestsEdgePointFrom(start,m_RallyPoints[index], cmpPosition, cmpFootprint);
	else
	{
		start.X = m_RallyPoints[index-1].X;
		start.Y = m_RallyPoints[index-1].Y;
	}
	cmpPathfinder->ComputePathImmediate(start.X, start.Y, goal, cmpPathfinder->GetPassabilityClass(m_LinePassabilityClass), path);

	// Check if we got a path back; if not we probably have two markers less than one tile apart.
	if (path.m_Waypoints.size() < 2)
	{
		m_Path[index].emplace_back(start.X.ToFloat(), start.Y.ToFloat());
		m_Path[index].emplace_back(m_RallyPoints[index].X.ToFloat(), m_RallyPoints[index].Y.ToFloat());
		return;
	}
	else if (index == 0)
	{
		// Sometimes this ends up not being optimal if you asked for a long path, so improve.
		CFixedVector2D newend(waypoints[waypoints.size()-2].x,waypoints[waypoints.size()-2].z);
		GetClosestsEdgePointFrom(newend,newend, cmpPosition, cmpFootprint);
		waypoints.back().x = newend.X;
		waypoints.back().z = newend.Y;
	}
	else
	{
		// Make sure we actually start at the rallypoint because the pathfinder moves us to a usable tile.
		waypoints.back().x = m_RallyPoints[index-1].X;
		waypoints.back().z = m_RallyPoints[index-1].Y;
	}

	// Pathfinder makes us go to the nearest passable cell which isn't always what we want
	waypoints[0].x = m_RallyPoints[index].X;
	waypoints[0].z = m_RallyPoints[index].Y;

	// From here on, we choose to represent the waypoints as CVector2D floats to avoid to have to convert back and forth
	// between fixed-point Waypoint/CFixedVector2D and various other float-based formats used by interpolation and whatnot.
	// Since we'll only be further using these points for rendering purposes, using floats should be fine.

	for (Waypoint& waypoint : waypoints)
		m_Path[index].emplace_back(waypoint.x.ToFloat(), waypoint.z.ToFloat());

	// Post-processing

	// Linearize the path;
	// Pass through the waypoints, averaging each waypoint with its next one except the last one. Because the path
	// goes from the marker to this entity/the previous flag and we want to keep the point at the marker's exact position,
	// loop backwards through the waypoints so that the marker waypoint is maintained.
	// TODO: see if we can do this at the same time as the waypoint -> coord conversion above
	for(size_t i = m_Path[index].size() - 2; i > 0; --i)
		m_Path[index][i] = (m_Path[index][i] + m_Path[index][i-1]) / 2.0f;

	// Eliminate some consecutive waypoints that are visible from eachother. Reduce across a maximum distance of approx. 6 tiles
	// (prevents segments that are too long to properly stick to the terrain)
	ReduceSegmentsByVisibility(m_Path[index], 6);

	// Debug overlays
	if (m_EnableDebugNodeOverlay)
	{
		while (index >= m_DebugNodeOverlays.size())
			m_DebugNodeOverlays.emplace_back();

		m_DebugNodeOverlays[index].clear();
	}
	if (m_EnableDebugNodeOverlay && m_SmoothPath)
	{
		// Create separate control point overlays so we can differentiate when using smoothing (offset them a little higher from the
		// terrain so we can still see them after the interpolated points are added)
		for (const CVector2D& point : m_Path[index])
		{
			SOverlayLine overlayLine;
			overlayLine.m_Color = CColor(1.0f, 0.0f, 0.0f, 1.0f);
			overlayLine.m_Thickness = 2;
			SimRender::ConstructSquareOnGround(GetSimContext(), point.X, point.Y, 0.2f, 0.2f, 1.0f, overlayLine, true);
			m_DebugNodeOverlays[index].push_back(overlayLine);
		}
	}

	if (m_SmoothPath)
		// The number of points to interpolate goes hand in hand with the maximum amount of node links allowed to be joined together
		// by the visibility reduction. The more node links that can be joined together, the more interpolated points you need to
		// generate to be able to deal with local terrain height changes.
		// no offset, keep line at its exact path
		SimRender::InterpolatePointsRNS(m_Path[index], false, 0, 4);

	// Find which point is the last visible point before going into the SoD, so we have a point to compare to on the next turn
	GetVisibilitySegments(m_VisibilitySegments[index], index);

	// Build overlay lines for the new path
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

	SOverlayTexturedLine::LineCapType dashesLineCapType = SOverlayTexturedLine::LINECAP_ROUND; // line caps to use for the dashed segments (and any other segment's edges that border it)


	for(const SVisibilitySegment& segment : m_VisibilitySegments[index])
	{
		if (segment.m_Visible)
		{
			// Does this segment border on the building or rally point flag on either side?
			bool bordersBuilding = (segment.m_EndIndex == m_Path[index].size() - 1);
			bool bordersFlag = (segment.m_StartIndex == 0);

			// Construct solid textured overlay line along a subset of the full path points from startPointIdx to endPointIdx
			SOverlayTexturedLine overlayLine;
			overlayLine.m_Thickness = m_LineThickness;
			overlayLine.m_SimContext = &GetSimContext();
			overlayLine.m_TextureBase = m_Texture;
			overlayLine.m_TextureMask = m_TextureMask;
			overlayLine.m_Color = m_LineColor;
			overlayLine.m_Closed = false;
			// We should take care to only use m_LineXCap for the actual end points at the building and the rally point; any intermediate
			// end points (i.e., that border a dashed segment) should have the dashed cap
			// the path line is actually in reverse order as well, so let's swap out the start and end caps
			overlayLine.m_StartCapType = (bordersFlag ? m_LineEndCapType : dashesLineCapType);
			overlayLine.m_EndCapType = (bordersBuilding ? m_LineStartCapType : dashesLineCapType);
			overlayLine.m_AlwaysVisible = true;

			// Push overlay line coordinates
			ENSURE(segment.m_EndIndex > segment.m_StartIndex);
			// End index is inclusive here
			for (size_t j = segment.m_StartIndex; j <= segment.m_EndIndex; ++j)
			{
				overlayLine.m_Coords.push_back(m_Path[index][j].X);
				overlayLine.m_Coords.push_back(m_Path[index][j].Y);
			}

			m_TexturedOverlayLines[index].push_back(overlayLine);
		}
		else
		{
			// Construct dashed line from startPointIdx to endPointIdx; add textured overlay lines for it to the render list
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
			// Ratio of the dash's length to a (dash + clear) pair's length
			float pairDashRatio = dashSize / (dashSize + clearSize);

			// Straight-line distance between the points
			float distance = (m_Path[index][segment.m_StartIndex] - m_Path[index][segment.m_EndIndex]).Length();

			// See how many pairs (dash + clear) of unmodified size can fit into the distance. Then check the remaining distance; if it's not exactly
			// a dash size's worth (which it probably won't be), then adjust the dash/clear sizes slightly so that it is.
			int numFitUnmodified = floor(distance/(dashSize + clearSize));
			float remainderDistance = distance - (numFitUnmodified * (dashSize + clearSize));

			// Now we want to make remainderDistance equal exactly one dash size (i.e. maxDashSize) by scaling dashSize and clearSize slightly.
			// We have (remainderDistance - maxDashSize) of space to distribute over numFitUnmodified instances of (dashSize + clearSize) to make
			// it fit, so each (dashSize + clearSize) pair needs to adjust its length by (remainderDistance - maxDashSize)/numFitUnmodified
			// (which will be positive or negative accordingly). This number can then be distributed further proportionally among the dash's
			// length and the clear's length.

			// We always want to have at least one dash/clear pair (i.e., "|===|   |===|"); also, we need to avoid division by zero below.
			numFitUnmodified = std::max(1, numFitUnmodified);

			// Can be either positive or negative
			float pairwiseLengthDifference = (remainderDistance - maxDashSize)/numFitUnmodified;
			dashSize += pairDashRatio * pairwiseLengthDifference;
			clearSize += (1 - pairDashRatio) * pairwiseLengthDifference;

			// ------------------------------------------------------------------------------------------------

			SDashedLine dashedLine;
			SimRender::ConstructDashedLine(straightLine, dashedLine, dashSize, clearSize);

			// Build overlay lines for dashes
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

	// See if there have been any changes to the SoD by grabbing the visibility edge points and comparing them to the previous ones
	std::vector<std::vector<SVisibilitySegment> > newVisibilitySegments;
	for (size_t i = 0; i < m_Path.size(); ++i)
	{
		std::vector<SVisibilitySegment> tmp;
		newVisibilitySegments.push_back(tmp);
		GetVisibilitySegments(newVisibilitySegments[i], i);
	}

	// Check if the full path changed, then reconstruct all overlay lines, otherwise check if a segment changed and update that.
	if (m_VisibilitySegments.size() != newVisibilitySegments.size())
	{
		// Save the new visibility segments to compare against next time
		m_VisibilitySegments = newVisibilitySegments;
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
				// Save the new visibility segments to compare against next time
				m_VisibilitySegments[i] = newVisibilitySegments[i];
				ConstructOverlayLines(i);
			}
		}
	}
}

void CCmpRallyPointRenderer::GetClosestsEdgePointFrom(CFixedVector2D& result, CFixedVector2D& start, CmpPtr<ICmpPosition> cmpPosition, CmpPtr<ICmpFootprint> cmpFootprint) const
{
	ENSURE(cmpPosition);
	ENSURE(cmpFootprint);

	// Grab the shape and dimensions of the footprint
	entity_pos_t footprintSize0, footprintSize1, footprintHeight;
	ICmpFootprint::EShape footprintShape;
	cmpFootprint->GetShape(footprintShape, footprintSize0, footprintSize1, footprintHeight);

	// Grab the center of the footprint
	CFixedVector2D center = cmpPosition->GetPosition2D();

	switch (footprintShape)
	{
		case ICmpFootprint::SQUARE:
		{
			// In this case, footprintSize0 and 1 indicate the size along the X and Z axes, respectively.
			// The building's footprint could be rotated any which way, so let's get the rotation around the Y axis
			// and the rotated unit vectors in the X/Z plane of the shape's footprint
			// (the Footprint itself holds only the outline, the Position holds the orientation)

			// Sinus and cosinus of the Y axis rotation angle (aka the yaw)
			fixed s, c;
			fixed a = cmpPosition->GetRotation().Y;
			sincos_approx(a, s, c);
			// Unit vector along the rotated X axis
			CFixedVector2D u(c, -s);
			// Unit vector along the rotated Z axis
			CFixedVector2D v(s, c);
			CFixedVector2D halfSize(footprintSize0 / 2, footprintSize1 / 2);

			CFixedVector2D footprintEdgePoint = Geometry::NearestPointOnSquare(start - center, u, v, halfSize);
			result = center + footprintEdgePoint;
			break;
		}
		case ICmpFootprint::CIRCLE:
		{
			// In this case, both footprintSize0 and 1 indicate the circle's radius
			// Transform target to the point nearest on the edge.
			CFixedVector2D centerVec2D(center.X, center.Y);
			CFixedVector2D centerToLast(start - centerVec2D);
			centerToLast.Normalize();
			result = centerVec2D + (centerToLast.Multiply(footprintSize0));
			break;
		}
	}
}

void CCmpRallyPointRenderer::ReduceSegmentsByVisibility(std::vector<CVector2D>& coords, unsigned maxSegmentLinks, bool floating) const
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
	pass_class_t passabilityClass = cmpPathFinder->GetPassabilityClass(m_LinePassabilityClass);

	// Save the first base node
	newCoords.push_back(coords[0]);

	size_t baseNodeIdx = 0;
	size_t curNodeIdx = 1;

	float baseNodeY;
	entity_pos_t baseNodeX;
	entity_pos_t baseNodeZ;

	// Set initial base node coords
	baseNodeX = fixed::FromFloat(coords[baseNodeIdx].X);
	baseNodeZ = fixed::FromFloat(coords[baseNodeIdx].Y);
	baseNodeY = cmpTerrain->GetExactGroundLevel(coords[baseNodeIdx].X, coords[baseNodeIdx].Y);
	if (floating)
		baseNodeY = std::max(baseNodeY, cmpWaterManager->GetExactWaterLevel(coords[baseNodeIdx].X, coords[baseNodeIdx].Y));

	while (curNodeIdx < coords.size())
	{
		// This needs to be true at all times, otherwise we're checking visibility between a point and itself.
		ENSURE(curNodeIdx > baseNodeIdx);

		entity_pos_t curNodeX = fixed::FromFloat(coords[curNodeIdx].X);
		entity_pos_t curNodeZ = fixed::FromFloat(coords[curNodeIdx].Y);
		float curNodeY = cmpTerrain->GetExactGroundLevel(coords[curNodeIdx].X, coords[curNodeIdx].Y);
		if (floating)
			curNodeY = std::max(curNodeY, cmpWaterManager->GetExactWaterLevel(coords[curNodeIdx].X, coords[curNodeIdx].Y));

		// Find out whether curNode is visible from baseNode (careful; this is in 2D only; terrain height differences are ignored!)
		bool curNodeVisible = cmpPathFinder->CheckMovement(obstructionFilter, baseNodeX, baseNodeZ, curNodeX, curNodeZ, lineRadius, passabilityClass);

		// Since height differences are ignored by CheckMovement, let's call two points visible from one another only if they're at
		// roughly the same terrain elevation
		// TODO: this could probably use some tuning
		curNodeVisible = curNodeVisible && (fabsf(curNodeY - baseNodeY) < 3.f);
		if (maxSegmentLinks > 0)
			// Max. amount of node-to-node links to be eliminated (unsigned subtraction is valid because curNodeIdx is always > baseNodeIdx)
			curNodeVisible = curNodeVisible && ((curNodeIdx - baseNodeIdx) <= maxSegmentLinks);

		if (!curNodeVisible)
		{
			// Current node is not visible from the base node, so the previous one was the last visible point from baseNode and should
			// hence become the new base node for further iterations.

			// If curNodeIdx is adjacent to the current baseNode (which is possible due to steep height differences, e.g. hills), then
			// we should take care not to stay stuck at the current base node
			if (curNodeIdx > baseNodeIdx + 1)
			{
				baseNodeIdx = curNodeIdx - 1;
			}
			else
			{
				// curNodeIdx == baseNodeIdx + 1
				baseNodeIdx = curNodeIdx;
				// Move the next candidate node one forward so that we don't test a point against itself in the next iteration
				++curNodeIdx;
			}

			// Add new base node to output list
			newCoords.push_back(coords[baseNodeIdx]);

			// Update base node coordinates
			baseNodeX = fixed::FromFloat(coords[baseNodeIdx].X);
			baseNodeZ = fixed::FromFloat(coords[baseNodeIdx].Y);
			baseNodeY = cmpTerrain->GetExactGroundLevel(coords[baseNodeIdx].X, coords[baseNodeIdx].Y);
			if (floating)
				baseNodeY = std::max(baseNodeY, cmpWaterManager->GetExactWaterLevel(coords[baseNodeIdx].X, coords[baseNodeIdx].Y));
		}

		++curNodeIdx;
	}

	// We always need to add the last point back to the array; if e.g. all the points up to the last one are all visible from the current
	// base node, then the loop above just ends and no endpoint is ever added to the list.
	ENSURE(curNodeIdx == coords.size());
	newCoords.push_back(coords[coords.size() - 1]);

	coords.swap(newCoords);
}

void CCmpRallyPointRenderer::GetVisibilitySegments(std::vector<SVisibilitySegment>& out, size_t index) const
{
	out.clear();

	if (m_Path[index].size() < 2)
		return;

	CmpPtr<ICmpRangeManager> cmpRangeMgr(GetSystemEntity());

	player_id_t currentPlayer = static_cast<player_id_t>(GetSimContext().GetCurrentDisplayedPlayer());
	ICmpRangeManager::CLosQuerier losQuerier(cmpRangeMgr->GetLosQuerier(currentPlayer));

	// Go through the path node list, comparing each node's visibility with the previous one. If it changes, end the current segment and start
	// a new one at the next point.

	const float terrainSize = static_cast<float>(TERRAIN_TILE_SIZE);
	bool lastVisible = losQuerier.IsExplored(
		(fixed::FromFloat(m_Path[index][0].X / terrainSize)).ToInt_RoundToNearest(),
		(fixed::FromFloat(m_Path[index][0].Y / terrainSize)).ToInt_RoundToNearest()
	);
	// Starting node index of the current segment
	size_t curSegmentStartIndex = 0;

	for (size_t k = 1; k < m_Path[index].size(); ++k)
	{
		// Grab tile indices for this coord
		int i = (fixed::FromFloat(m_Path[index][k].X / terrainSize)).ToInt_RoundToNearest();
		int j = (fixed::FromFloat(m_Path[index][k].Y / terrainSize)).ToInt_RoundToNearest();

		bool nodeVisible = losQuerier.IsExplored(i, j);
		if (nodeVisible != lastVisible)
		{
			// Visibility changed; write out the segment that was just completed and get ready for the new one
			out.push_back(SVisibilitySegment(lastVisible, curSegmentStartIndex, k - 1));

			curSegmentStartIndex = k - 1;
			lastVisible = nodeVisible;
		}

	}

	// Terminate the last segment
	out.push_back(SVisibilitySegment(lastVisible, curSegmentStartIndex, m_Path[index].size() - 1));

	MergeVisibilitySegments(out);
}

void CCmpRallyPointRenderer::MergeVisibilitySegments(std::vector<SVisibilitySegment>& segments)
{
	// Scan for single-point segments; if they are inbetween two other segments, delete them and merge the surrounding segments.
	// If they're at either end of the path, include them in their bordering segment (but only if those bordering segments aren't
	// themselves single-point segments, because then we would want those to get absorbed by its surrounding ones first).

	// First scan for absorptions of single-point surrounded segments (i.e. excluding edge segments)
	size_t numSegments = segments.size();

	// WARNING: FOR LOOP TRICKERY AHEAD!
	for (size_t i = 1; i < numSegments - 1;)
	{
		SVisibilitySegment& segment = segments[i];
		if (segment.IsSinglePoint())
		{
			// Since the segments' visibility alternates, the surrounding ones should have the same visibility
			ENSURE(segments[i-1].m_Visible == segments[i+1].m_Visible);

			// Make previous segment span all the way across to the next
			segments[i-1].m_EndIndex = segments[i+1].m_EndIndex;
			// Erase this segment
			segments.erase(segments.begin() + i);
			// And the next (we removed [i], so [i+1] is now at position [i])
			segments.erase(segments.begin() + i);
			// We removed 2 segments, so update the loop condition
			numSegments -= 2;
			// In the next iteration, i should still point to the segment right after the one that got expanded, which is now
			// at position i; so don't increment i here
		}
		else
		{
			++i;
		}
	}

	ENSURE(numSegments == segments.size());

	// Check to see if the first segment needs to be merged with its neighbour
	if (segments.size() >= 2 && segments[0].IsSinglePoint())
	{
		int firstSegmentStartIndex = segments.front().m_StartIndex;
		ENSURE(firstSegmentStartIndex == 0);
		// At this point, the second segment should never be a single-point segment
		ENSURE(!segments[1].IsSinglePoint());

		segments.erase(segments.begin());
		segments.front().m_StartIndex = firstSegmentStartIndex;
	}

	// check to see if the last segment needs to be merged with its neighbour
	if (segments.size() >= 2 && segments[segments.size()-1].IsSinglePoint())
	{
		int lastSegmentEndIndex = segments.back().m_EndIndex;
		// At this point, the second-to-last segment should never be a single-point segment
		ENSURE(!segments[segments.size()-2].IsSinglePoint());

		segments.pop_back();
		segments.back().m_EndIndex = lastSegmentEndIndex;
	}

	// --------------------------------------------------------------------------------------------------------
	// At this point, every segment should have at least 2 points
	for (size_t i = 0; i < segments.size(); ++i)
	{
		ENSURE(!segments[i].IsSinglePoint());
		ENSURE(segments[i].m_EndIndex > segments[i].m_StartIndex);
	}
}

void CCmpRallyPointRenderer::RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling)
{
	// We only get here if the rally point is set and should be displayed
	for(std::vector<SOverlayTexturedLine>& row : m_TexturedOverlayLines)
		for (SOverlayTexturedLine& col : row) {
			if (col.m_Coords.empty())
				continue;
			if (culling && !col.IsVisibleInFrustum(frustum))
				continue;
			collector.Submit(&col);
		}

	if (m_EnableDebugNodeOverlay && !m_DebugNodeOverlays.empty())
	{
		for (std::vector<SOverlayLine>& row : m_DebugNodeOverlays)
			for (SOverlayLine& col : row)
				if (!col.m_Coords.empty())
					collector.Submit(&col);
	}
}

void CCmpRallyPointRenderer::AddPosition_wrapper(const CFixedVector2D& pos)
{
	AddPosition(pos, false);
}

void CCmpRallyPointRenderer::SetPosition(const CFixedVector2D& pos)
{
	if (!(m_RallyPoints.size() == 1 && m_RallyPoints.front() == pos))
	{
		m_RallyPoints.clear();
		AddPosition(pos, true);
		// Don't need to UpdateMessageSubscriptions here since AddPosition already calls it
	}
}

void CCmpRallyPointRenderer::UpdatePosition(u32 rallyPointId, const CFixedVector2D& pos)
{
	if (rallyPointId >= m_RallyPoints.size())
		return;

	m_RallyPoints[rallyPointId] = pos;

	UpdateMarkers();

	// Compute a new path for the current, and if existing the next rally point
	RecomputeRallyPointPath_wrapper(rallyPointId);
	if (rallyPointId + 1 < m_RallyPoints.size())
		RecomputeRallyPointPath_wrapper(rallyPointId + 1);
}

void CCmpRallyPointRenderer::SetDisplayed(bool displayed)
{
	if (m_Displayed != displayed)
	{
		m_Displayed = displayed;

		// Set color after all dependent components are deserialized
		if (displayed && m_LineColor.r < 0)
		{
			UpdateLineColor();
			ConstructAllOverlayLines();
		}

		// Move the markers out of oblivion and back into the real world, or vice-versa
		UpdateMarkers();

		// Check for changes to the SoD and update the overlay lines accordingly. We need to do this here because this method
		// only takes effect when the display flag is active; we need to pick up changes to the SoD that might have occurred
		// while this rally point was not being displayed.
		UpdateOverlayLines();

		UpdateMessageSubscriptions();
	}
}

void CCmpRallyPointRenderer::Reset()
{
	for (entity_id_t& componentId : m_MarkerEntityIds)
	{
		if (componentId != INVALID_ENTITY)
		{
			GetSimContext().GetComponentManager().DestroyComponentsSoon(componentId);
			componentId = INVALID_ENTITY;
		}
	}
	m_RallyPoints.clear();
	m_MarkerEntityIds.clear();
	m_LastOwner = INVALID_PLAYER;
	m_LastMarkerCount = 0;
	RecomputeAllRallyPointPaths();
	UpdateMessageSubscriptions();
}

void CCmpRallyPointRenderer::UpdateColor()
{
	UpdateLineColor();
	ConstructAllOverlayLines();
}

void CCmpRallyPointRenderer::AddPosition(CFixedVector2D pos, bool recompute)
{
	m_RallyPoints.push_back(pos);
	UpdateMarkers();

	if (recompute)
		RecomputeAllRallyPointPaths();
	else
		RecomputeRallyPointPath_wrapper(m_RallyPoints.size() - 1);

	UpdateMessageSubscriptions();
}

bool CCmpRallyPointRenderer::IsSet() const
{
	return !m_RallyPoints.empty();
}
