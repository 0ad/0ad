/**
 * @param {Array} templates - Each Item is an Object that contains the properties "templateName" and optionally "margin," "constraints" and "painters".
 */
function CityPainter(templates, angle, playerID)
{
	this.angle = angle;
	this.playerID = playerID;
	this.templates = templates.map(template => {

		const obstructionSize = getObstructionSize(template.templateName, template.margin || 0);
		return {
			"templateName": template.templateName,
			"maxCount": template.maxCount !== undefined ? template.maxCount : Infinity,
			"constraint": template.constraints && new AndConstraint(template.constraints),
			"painter": template.painters && new MultiPainter(template.painters),
			"obstructionCorners": [
				new Vector2D(0, 0),
				new Vector2D(obstructionSize.x, 0),
				new Vector2D(0, obstructionSize.y),
				obstructionSize
			]
		};
	});
}

CityPainter.prototype.paint = function(area)
{
	let templates = this.templates;

	const templateCounts = {};
	for (const template of this.templates)
		templateCounts[template.templateName] = 0;

	const mapCenter = g_Map.getCenter();
	const mapSize = g_Map.getSize();

	// TODO: Due to the rounding, this is wasting a lot of space.
	// The city would be much denser if it would test for actual shape intersection or
	// if it would use a custom, more fine-grained obstruction grid
	const tileClass = g_Map.createTileClass();

	const processed = new Array(mapSize).fill(0).map(() => new Uint8Array(mapSize));

	for (let x = 0; x < mapSize; x += 0.5)
		for (let y = 0; y < mapSize; y += 0.5)
		{
			const point = new Vector2D(x, y).rotateAround(this.angle, mapCenter).round();
			if (!area.contains(point) || processed[point.x][point.y] || !g_Map.validTilePassable(point))
				continue;

			processed[point.x][point.y] = 1;

			for (const template of shuffleArray(templates))
			{
				if (template.constraint && !template.constraint.allows(point))
					continue;

				// Randomize building angle while keeping it aligned
				const buildingAngle = this.angle + randIntInclusive(0, 3) * Math.PI / 2;

				// Place the entity if all points are within the boundaries and don't collide with the other entities
				const obstructionCorners = template.obstructionCorners.map(obstructionCorner =>
					Vector2D.add(point, obstructionCorner.clone().rotate(buildingAngle)));

				const obstructionPoints = new ConvexPolygonPlacer(obstructionCorners, 0).place(
					new AndConstraint([new StayAreasConstraint([area]), avoidClasses(tileClass, 0), new PassableMapAreaConstraint()]));

				if (!obstructionPoints)
					continue;

				g_Map.placeEntityPassable(template.templateName, this.playerID, Vector2D.average(obstructionCorners), -buildingAngle);

				if (template.painter)
					template.painter.paint(new Area(obstructionPoints));

				for (const obstructionPoint of obstructionPoints)
					tileClass.add(obstructionPoint);

				++templateCounts[template.templateName];
				templates = templates.filter(template => templateCounts[template.templateName] < template.maxCount);
				break;
			}
		}
};
