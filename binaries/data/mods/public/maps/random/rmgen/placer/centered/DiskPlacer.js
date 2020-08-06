/**
 * Returns all points on a disk at the given location that meet the constraint.
 */
function DiskPlacer(radius, centerPosition = undefined)
{
	this.radiusSquared = Math.square(radius);
	this.radius = radius;
	this.centerPosition = undefined;

	if (centerPosition)
		this.setCenterPosition(centerPosition);
}

DiskPlacer.prototype.setCenterPosition = function(position)
{
	this.centerPosition = deepfreeze(position.clone().round());
};

DiskPlacer.prototype.place = function(constraint)
{
	let points = [];

	const xMin = Math.floor(Math.max(0, this.centerPosition.x - this.radius));
	const yMin = Math.floor(Math.max(0, this.centerPosition.y - this.radius));
	const xMax = Math.ceil(Math.min(g_Map.getSize() - 1, this.centerPosition.x + this.radius));
	const yMax = Math.ceil(Math.min(g_Map.getSize() - 1, this.centerPosition.y + this.radius));

	let it = new Vector2D();
	for (it.x = xMin; it.x <= xMax; ++it.x)
		for (it.y = yMin; it.y <= yMax; ++it.y)
		{
			if (this.centerPosition.distanceToSquared(it) <= this.radiusSquared && constraint.allows(it))
				points.push(it.clone());
		}

	return points;
};
