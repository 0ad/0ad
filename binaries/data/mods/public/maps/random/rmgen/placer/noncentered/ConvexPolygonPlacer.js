/**
 * Returns all points on the tilegrid within the convex hull of the given positions.
 */
function ConvexPolygonPlacer(points, failFraction = 0)
{
	this.polygonVertices = this.getConvexHull(points.map(point => point.clone().round()));
	this.failFraction = failFraction;
};

ConvexPolygonPlacer.prototype.place = function(constraint)
{
	let points = [];
	let count = 0;
	let failed = 0;

	for (let point of getPointsInBoundingBox(getBoundingBox(this.polygonVertices)))
	{
		if (this.polygonVertices.some((vertex, i) =>
		      distanceOfPointFromLine(this.polygonVertices[i], this.polygonVertices[(i + 1) % this.polygonVertices.length], point) > 0))
			continue;

		++count;

		if (g_Map.inMapBounds(point) && constraint.allows(point))
			points.push(point);
		else
			++failed;
	}

	return failed <= this.failFraction * count ? points : undefined;
};

/**
 * Applies the gift-wrapping algorithm.
 * Returns a sorted subset of the given points that are the vertices of the convex polygon containing all given points.
 */
ConvexPolygonPlacer.prototype.getConvexHull = function(points)
{
	let uniquePoints = [];
	for (let point of points)
		if (uniquePoints.every(p => p.x != point.x || p.y != point.y))
			uniquePoints.push(point);

	// Start with the leftmost point
	let result = [uniquePoints.reduce((leftMost, point) => point.x < leftMost.x ? point : leftMost, uniquePoints[0])];

	// Add the vector most left of the most recently added point until a cycle is reached
	while (result.length < uniquePoints.length)
	{
		let nextLeftmostPoint;

		// Of all points, find the one that is leftmost
		for (let point of uniquePoints)
		{
			if (point == result[result.length - 1])
				continue;

			if (!nextLeftmostPoint || distanceOfPointFromLine(nextLeftmostPoint, result[result.length - 1], point) <= 0)
				nextLeftmostPoint = point;
		}

		// If it was a known one, then the remaining points are inside this hull
		if (result.indexOf(nextLeftmostPoint) != -1)
			break;

		result.push(nextLeftmostPoint);
	}

	return result;
};
