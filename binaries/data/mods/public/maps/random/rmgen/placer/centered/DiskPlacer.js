/**
 * Returns all points on a disk at the given location that meet the constraint.
 */
function DiskPlacer(radius, centerPosition = undefined)
{
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

	for (let x = 0; x < g_Map.getSize(); ++x)
		for (let y = 0; y < g_Map.getSize(); ++y)
		{
			let point = new Vector2D(x, y);
			if (this.centerPosition.distanceTo(point) <= this.radius && constraint.allows(point))
				points.push(point);
		}

	return points;
};
