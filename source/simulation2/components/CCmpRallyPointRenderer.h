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

#ifndef INCLUDED_CCMPRALLYPOINTRENDERER
#define INCLUDED_CCMPRALLYPOINTRENDERER

#include "ICmpRallyPointRenderer.h"
#include "graphics/Overlay.h"
#include "graphics/TextureManager.h"
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
#include "ps/Profile.h"
#include "renderer/Renderer.h"

struct SVisibilitySegment
{
	bool m_Visible;
	size_t m_StartIndex;
	size_t m_EndIndex; // Inclusive

	SVisibilitySegment(bool visible, size_t startIndex, size_t endIndex)
		: m_Visible(visible), m_StartIndex(startIndex), m_EndIndex(endIndex)
	{}

	bool operator==(const SVisibilitySegment& other) const
	{
		return m_Visible == other.m_Visible && m_StartIndex == other.m_StartIndex && m_EndIndex == other.m_EndIndex;
	}

	bool operator!=(const SVisibilitySegment& other) const
	{
		return !(*this == other);
	}

	bool IsSinglePoint() const
	{
		return m_StartIndex == m_EndIndex;
	}
};

class CCmpRallyPointRenderer : public ICmpRallyPointRenderer
{
public:
	static std::string GetSchema();
	static void ClassInit(CComponentManager& componentManager);

	virtual void Init(const CParamNode& paramNode);
	virtual void Deinit();

	virtual void Serialize(ISerializer& UNUSED(serialize));
	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize));

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global));

	/*
	 * Must be called whenever m_Displayed or the size of m_RallyPoints change,
	 * to determine whether we need to respond to render messages.
	 */
	virtual void UpdateMessageSubscriptions();

	virtual void AddPosition_wrapper(const CFixedVector2D& pos);

	virtual void SetPosition(const CFixedVector2D& pos);

	virtual void UpdatePosition(u32 rallyPointId, const CFixedVector2D& pos);

	virtual void SetDisplayed(bool displayed);

	virtual void Reset();

	virtual void UpdateColor();

	/**
	 * Returns true if at least one display rally point is set; i.e., if we have a point to render our marker/line at.
	 */
	virtual bool IsSet() const;

	DEFAULT_COMPONENT_ALLOCATOR(RallyPointRenderer)

protected:
	/**
	 * Display position of the rally points. Note that this are merely the display positions; they not necessarily the same as the
	 * actual positions used in the simulation at any given time. In particular, we need this separate copy to support
	 * instantaneously rendering the rally point markers/lines when the user sets one in-game (instead of waiting until the
	 * network-synchronization code sets it on the RallyPoint component, which might take up to half a second).
	 */
	std::vector<CFixedVector2D> m_RallyPoints;
	/**
	 * Full path to the rally points as returned by the pathfinder, with some post-processing applied to reduce zig/zagging.
	 */
	std::vector<std::vector<CVector2D> > m_Path;
	/**
	 * Visibility segments of the rally point paths; splits the path into SoD/non-SoD segments.
	 */
	std::vector<std::vector<SVisibilitySegment> > m_VisibilitySegments;
	/**
	 * Should we render the rally points and the path lines? (set from JS when e.g. the unit is selected/deselected)
	 */
	bool m_Displayed;
	/**
	 * Smooth the path before rendering?
	 */
	bool m_SmoothPath;
	/**
	 * Entity IDs of the rally point markers.
	 */
	std::vector<entity_id_t> m_MarkerEntityIds;

	size_t m_LastMarkerCount;
	/**
	 * Last seen owner of this entity (used to keep track of ownership changes).
	 */
	player_id_t m_LastOwner;
	/**
	 * Template name of the rally point markers.
	 */
	std::wstring m_MarkerTemplate;

	/**
	 * Marker connector line settings (loaded from XML)
	 */
	float m_LineThickness;
	CColor m_LineColor;
	CColor m_LineDashColor;
	SOverlayTexturedLine::LineCapType m_LineStartCapType;
	SOverlayTexturedLine::LineCapType m_LineEndCapType;
	std::wstring m_LineTexturePath;
	std::wstring m_LineTextureMaskPath;
	/**
	 * Pathfinder passability class to use for computing the (long-range) marker line path.
	 */
	std::string m_LinePassabilityClass;

	CTexturePtr m_Texture;
	CTexturePtr m_TextureMask;

	/**
	 * Textured overlay lines to be used for rendering the marker line. There can be multiple because we may need to render
	 * dashes for segments that are inside the SoD.
	 */
	std::vector<std::vector<SOverlayTexturedLine> > m_TexturedOverlayLines;

	/**
	 * Draw little overlay circles to indicate where the exact path points are.
	 */
	bool m_EnableDebugNodeOverlay;
	std::vector<std::vector<SOverlayLine> > m_DebugNodeOverlays;

private:
	/**
	 * Helper function for AddPosition_wrapper and SetPosition.
	 */
	void AddPosition(CFixedVector2D pos, bool recompute);

	/**
	* Helper function to set the line color to its owner's color.
	*/
	void UpdateLineColor();

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
	 * Get the point on the footprint edge that's as close from "start" as possible.
	 */
	void GetClosestsEdgePointFrom(CFixedVector2D& result, CFixedVector2D& start, CmpPtr<ICmpPosition> cmpPosition, CmpPtr<ICmpFootprint> cmpFootprint) const;

	/**
	 * Returns a list of indices of waypoints in the current path (m_Path[index]) where the LOS visibility changes, ordered from
	 * building/previous rally point to rally point. Used to construct the overlay line segments and track changes to the SoD.
	 */
	void GetVisibilitySegments(std::vector<SVisibilitySegment>& out, size_t index) const;

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
	void ReduceSegmentsByVisibility(std::vector<CVector2D>& coords, unsigned maxSegmentLinks = 0, bool floating = true) const;

	/**
	 * Helper function to GetVisibilitySegments, factored out for testing. Merges single-point segments with its neighbouring
	 * segments. You should not have to call this method directly.
	 */
	static void MergeVisibilitySegments(std::vector<SVisibilitySegment>& segments);

	void RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling);
};

REGISTER_COMPONENT_TYPE(RallyPointRenderer)

#endif // INCLUDED_CCMPRALLYPOINTRENDERER
