/**
 * @file An Area is a set of Vector2D points and a cache to lookup membership quickly.
 */

function Area(points)
{
	this.points = deepfreeze(points);

	let mapSize = g_Map.getSize();

	this.cache = new Array(mapSize).fill(0).map(() => new Uint8Array(mapSize));
	for (let point of points)
		this.cache[point.x][point.y] = 1;
}

Area.prototype.getPoints = function()
{
	return this.points;
};

Area.prototype.contains = function(point)
{
	return g_Map.inMapBounds(point) && this.cache[point.x][point.y] == 1;
};

Area.prototype.getClosestPointTo = function(position)
{
	if (!this.points.length)
		return undefined;

	let closestPoint = this.points[0];
	let shortestDistance = Infinity;

	for (let point of this.points)
	{
		let currentDistance = point.distanceToSquared(position);
		if (currentDistance < shortestDistance)
		{
			shortestDistance = currentDistance;
			closestPoint = point;
		}
	}

	return closestPoint;
};
