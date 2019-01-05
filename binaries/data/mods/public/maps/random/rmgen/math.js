const g_TileVertices = deepfreeze([
	new Vector2D(0, 0),
	new Vector2D(0, 1),
	new Vector2D(1, 0),
	new Vector2D(1, 1)
]);

const g_AdjacentCoordinates = deepfreeze([
	new Vector2D(1, 0),
	new Vector2D(1, 1),
	new Vector2D(0, 1),
	new Vector2D(-1, 1),
	new Vector2D(-1, 0),
	new Vector2D(-1, -1),
	new Vector2D(0, -1),
	new Vector2D(1, -1)
]);

function diskArea(radius)
{
	return Math.PI * Math.square(radius);
}

/**
 * Returns the angle of the vector between point 1 and point 2.
 * The angle is counterclockwise from the positive x axis.
 */
function getAngle(x1, z1, x2, z2)
{
	return Math.atan2(z2 - z1, x2 - x1);
}

/**
 * Get pointCount points equidistantly located on a circle.
 * @param {Vector2D} center
 */
function distributePointsOnCircle(pointCount, startAngle, radius, center)
{
	return distributePointsOnCircularSegment(pointCount, 2 * Math.PI * (pointCount - 1) / pointCount, startAngle, radius, center);
}

/**
 * Get pointCount points equidistantly located on a circular segment, including both endpoints.
 */
function distributePointsOnCircularSegment(pointCount, maxAngle, startAngle, radius, center)
{
	let points = [];
	let angle = [];
	pointCount = Math.round(pointCount);

	for (let i = 0; i < pointCount; ++i)
	{
		angle[i] = startAngle + maxAngle * i / Math.max(1, pointCount - 1);
		points[i] = Vector2D.add(center, new Vector2D(radius, 0).rotate(-angle[i]));
	}

	return [points, angle];
}

/**
 * Returns the shortest distance from a point to a line.
 * The sign of the return value determines the direction!
 *
 * @param {Vector2D} - lineStart, lineEnd, point
 */
function distanceOfPointFromLine(lineStart, lineEnd, point)
{
	// Since the cross product is the area of the parallelogram with the vectors for sides and
	// one of the two vectors having length one, that area equals the distance between the points.
	return Vector2D.sub(lineStart, lineEnd).normalize().cross(Vector2D.sub(point, lineEnd));
}

/**
 * Returns whether the two lines of the given width going through the given Vector2D intersect.
 */
function testLineIntersection(start1, end1, start2, end2, width)
{
	let start1end1 = Vector2D.sub(start1, end1);
	let start2end2 = Vector2D.sub(start2, end2);
	let start1start2 = Vector2D.sub(start1, start2);

	return (
		Math.abs(distanceOfPointFromLine(start1, end1, start2)) < width ||
		Math.abs(distanceOfPointFromLine(start1, end1, end2)) < width ||
		Math.abs(distanceOfPointFromLine(start2, end2, start1)) < width ||
		Math.abs(distanceOfPointFromLine(start2, end2, end1)) < width ||
		start1end1.cross(start1start2) * start1end1.cross(Vector2D.sub(start1, end2)) <= 0 &&
		start2end2.cross(start1start2) * start2end2.cross(Vector2D.sub(start2, end1)) >= 0);
}

/**
 * Returns the topleft and bottomright coordinate of the given two points.
 */
function getBoundingBox(points)
{
	let min = points[0].clone();
	let max = points[0].clone();

	for (let point of points)
	{
		min.set(Math.min(min.x, point.x), Math.min(min.y, point.y));
		max.set(Math.max(max.x, point.x), Math.max(max.y, point.y));
	}

	return {
		"min": min,
		"max": max
	};
}

function getPointsInBoundingBox(boundingBox)
{
	let points = [];
	for (let x = boundingBox.min.x; x <= boundingBox.max.x; ++x)
		for (let y = boundingBox.min.y; y <= boundingBox.max.y; ++y)
			points.push(new Vector2D(x, y));
	return points;
}

/**
 * Get the order of the given points to get the shortest closed path (similar to the traveling salesman problem).
 * @param {Vectro2D[]} points - Points the path should go through
 * @returns {number[]} Ordered indices, same length as points
 */
function sortPointsShortestCycle(points)
{
	let order = [];
	let distances = [];
	if (points.length <= 3)
	{
		for (let i = 0; i < points.length; ++i)
			order.push(i);

		return order;
	}

	// Just add the first 3 points
	let pointsToAdd = points.map(p => p.clone());
	for (let i = 0; i < 3; ++i)
	{
		order.push(i);
		pointsToAdd.shift();
		if (i)
			distances.push(points[order[i]].distanceTo(points[order[i - 1]]));
	}

	distances.push(points[order[0]].distanceTo(points[order[order.length - 1]]));

	// Add remaining points so the path lengthens the least
	let numPointsToAdd = pointsToAdd.length;
	for (let i = 0; i < numPointsToAdd; ++i)
	{
		let indexToAddTo;
		let minEnlengthen = Infinity;
		let minDist1 = 0;
		let minDist2 = 0;
		for (let k = 0; k < order.length; ++k)
		{
			let dist1 = pointsToAdd[0].distanceTo(points[order[k]]);
			let dist2 = pointsToAdd[0].distanceTo(points[order[(k + 1) % order.length]]);

			let enlengthen = dist1 + dist2 - distances[k];
			if (enlengthen < minEnlengthen)
			{
				indexToAddTo = k;
				minEnlengthen = enlengthen;
				minDist1 = dist1;
				minDist2 = dist2;
			}
		}
		order.splice(indexToAddTo + 1, 0, i + 3);
		distances.splice(indexToAddTo, 1, minDist1, minDist2);
		pointsToAdd.shift();
	}

	return order;
}
