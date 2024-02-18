/**
 * The RectPlacer returns all tiles between the two given points that meet the Constraint.
 */
function RectPlacer(start, end, failFraction = Infinity)
{
	this.bounds = getBoundingBox([start, end]);
	this.bounds.min.floor();
	this.bounds.max.floor();
	this.failFraction = failFraction;
}

RectPlacer.prototype.place = function(constraint)
{
	const bboxPoints = getPointsInBoundingBox(this.bounds);
	const points = bboxPoints.filter(point => g_Map.inMapBounds(point) && constraint.allows(point));
	return (bboxPoints.length - points.length) / bboxPoints.length <= this.failFraction ? points : undefined;
};
