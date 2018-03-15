/**
 * The EntityObstructionPlacer returns all points on the obstruction of the given template at the given position and angle that meet the constraint.
 * It can be used for more concise collision avoidance.
 */
function EntitiesObstructionPlacer(entities, margin = 0, failFraction = Infinity)
{
	this.entities = entities;
	this.margin = margin;
	this.failFraction = failFraction;
}

EntitiesObstructionPlacer.prototype.place = function(constraint)
{
	let points = [];

	for (let entity of this.entities)
	{
		let halfObstructionSize = getObstructionSize(entity.templateName, this.margin).div(2);

		// Place the entity if all points are within the boundaries and don't collide with the other entities
		let obstructionCorners = [
			new Vector2D(-halfObstructionSize.x, -halfObstructionSize.y),
			new Vector2D(-halfObstructionSize.x, +halfObstructionSize.y),
			new Vector2D(+halfObstructionSize.x, -halfObstructionSize.y),
			new Vector2D(+halfObstructionSize.x, +halfObstructionSize.y)
		].map(corner => Vector2D.add(entity.GetPosition2D(), corner.rotate(-entity.rotation.y)));

		points = points.concat(new ConvexPolygonPlacer(obstructionCorners, this.failFraction).place(constraint))
	}

	return points;
};
