/**
 * @file An Area is a set of points identified by an ID that was registered with the RandomMap.
 */

function Area(points, id)
{
	this.points = points;
	this.id = id;
}

Area.prototype.getID = function()
{
	return this.id;
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
