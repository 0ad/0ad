/**
 * HeightPlacer constants determining whether the extrema should be included by the placer too.
 */
const Elevation_ExcludeMin_ExcludeMax = 0;
const Elevation_IncludeMin_ExcludeMax = 1;
const Elevation_ExcludeMin_IncludeMax = 2;
const Elevation_IncludeMin_IncludeMax = 3;

/**
 * The HeightPlacer provides all points between the minimum and maximum elevation that meet the Constraint,
 * even if they are far from the passable area of the map.
 */
function HeightPlacer(mode, minElevation, maxElevation)
{
	this.withinHeightRange =
		mode == Elevation_ExcludeMin_ExcludeMax ? position => g_Map.getHeight(position) >  minElevation && g_Map.getHeight(position) < maxElevation :
		mode == Elevation_IncludeMin_ExcludeMax ? position => g_Map.getHeight(position) >= minElevation && g_Map.getHeight(position) < maxElevation :
		mode == Elevation_ExcludeMin_IncludeMax ? position => g_Map.getHeight(position) >  minElevation && g_Map.getHeight(position) <= maxElevation :
		mode == Elevation_IncludeMin_IncludeMax ? position => g_Map.getHeight(position) >= minElevation && g_Map.getHeight(position) <= maxElevation :
		undefined;

	if (!this.withinHeightRange)
		throw new Error("Invalid HeightPlacer mode: " + mode);
}

HeightPlacer.prototype.place = function(constraint)
{
	let mapSize = g_Map.getSize();

	return getPointsInBoundingBox(getBoundingBox([new Vector2D(0, 0), new Vector2D(mapSize - 1, mapSize - 1)])).filter(
		point => this.withinHeightRange(point) && constraint.allows(point));
};
