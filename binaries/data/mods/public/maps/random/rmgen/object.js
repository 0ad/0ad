/**
 * @file An Object tries to find locations around a location and returns an array of Entity items holding the template names, owners and locations on success.
 */

/**
 * The SimpleObject attempts to find locations for a random amount of entities with a random distance to the given center.
 */
function SimpleObject(templateName, minCount, maxCount, minDistance, maxDistance, minAngle = 0, maxAngle = 2 * Math.PI, avoidDistance = 1)
{
	this.templateName = templateName;
	this.minCount = minCount;
	this.maxCount = maxCount;
	this.minDistance = minDistance;
	this.maxDistance = maxDistance;
	this.minAngle = minAngle;
	this.maxAngle = maxAngle;
	this.avoidDistance = avoidDistance;

	if (minCount > maxCount)
		throw new Error("SimpleObject: minCount should be less than or equal to maxCount");

	if (minDistance > maxDistance)
		throw new Error("SimpleObject: minDistance should be less than or equal to maxDistance");

	if (minAngle > maxAngle)
		throw new Error("SimpleObject: minAngle should be less than or equal to maxAngle");
}

SimpleObject.prototype.place = function(centerPosition, playerID, avoidPositions, constraint, maxRetries)
{
	let entitySpecs = [];
	let numRetries = 0;
	let validTile = pos => this.templateName.startsWith(g_ActorPrefix) ? g_Map.validTile(pos) : g_Map.validTilePassable(pos);

	for (let i = 0; i < randIntInclusive(this.minCount, this.maxCount); ++i)
		while (true)
		{
			let distance = randFloat(this.minDistance, this.maxDistance);
			let angle = randomAngle();

			let position = Vector2D.sum([centerPosition, new Vector2D(0.5, 0.5), new Vector2D(distance, 0).rotate(-angle)]);

			if (validTile(position) &&
			    (!avoidPositions ||
			        entitySpecs.every(entSpec => entSpec.position.distanceTo(position) >= this.avoidDistance) &&
			        avoidPositions.every(avoid => avoid.position.distanceTo(position) >= Math.max(this.avoidDistance, avoid.distance))) &&
			    constraint.allows(position.clone().floor()))
			{
				entitySpecs.push({
					"templateName": this.templateName,
					"playerID": playerID,
					"position": position,
					"angle": randFloat(this.minAngle, this.maxAngle)
				});
				break;
			}
			else if (numRetries++ > maxRetries)
				return undefined;
		}

	return entitySpecs;
};

/**
 * Same as SimpleObject, but choses one of the given templates at random.
 */
function RandomObject(templateNames, minCount, maxCount, minDistance, maxDistance, minAngle, maxAngle)
{
	this.simpleObject = new SimpleObject(pickRandom(templateNames), minCount, maxCount, minDistance, maxDistance, minAngle, maxAngle);
}

RandomObject.prototype.place = function(centerPosition, player, avoidPositions, constraint, maxRetries)
{
	return this.simpleObject.place(centerPosition, player, avoidPositions, constraint, maxRetries);
};
