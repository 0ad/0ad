/**
 * @file An Object tries to find locations around a location and returns an array of Entity items holding the template names, owners and locations on success.
 */

/**
 * The SimpleObject attempts to find locations for a random amount of entities with a random distance to the given center.
 */
function SimpleObject(templateName, minCount, maxCount, minDistance, maxDistance, minAngle = 0, maxAngle = 2 * Math.PI)
{
	this.templateName = templateName;
	this.minCount = minCount;
	this.maxCount = maxCount;
	this.minDistance = minDistance;
	this.maxDistance = maxDistance;
	this.minAngle = minAngle;
	this.maxAngle = maxAngle;

	if (minCount > maxCount)
		throw new Error("SimpleObject: minCount should be less than or equal to maxCount");

	if (minDistance > maxDistance)
		throw new Error("SimpleObject: minDistance should be less than or equal to maxDistance");

	if (minAngle > maxAngle)
		throw new Error("SimpleObject: minAngle should be less than or equal to maxAngle");
}

SimpleObject.prototype.place = function(centerX, centerZ, player, avoidSelf, constraint, maxFailCount = 20)
{
	let entities = [];
	let failCount = 0;

	for (let i = 0; i < randIntInclusive(this.minCount, this.maxCount); ++i)
		while (true)
		{
			let distance = randFloat(this.minDistance, this.maxDistance);
			let angle = randomAngle();

			let x = centerX + 0.5 + distance * Math.cos(angle);
			let z = centerZ + 0.5 + distance * Math.sin(angle);

			if (g_Map.validT(x, z) &&
			    (!avoidSelf || entities.every(ent => Math.euclidDistance2DSquared(x, z, ent.position.x, ent.position.z) >= 1)) &&
			    constraint.allows(Math.floor(x), Math.floor(z)))
			{
				entities.push(new Entity(this.templateName, player, x, z, randFloat(this.minAngle, this.maxAngle)));
				break;
			}
			else if (failCount++ > maxFailCount)
				return undefined;
		}

	return entities;
};

/**
 * Same as SimpleObject, but choses one of the given templates at random.
 */
function RandomObject(templateNames, minCount, maxCount, minDistance, maxDistance, minAngle, maxAngle)
{
	this.simpleObject = new SimpleObject(pickRandom(templateNames), minCount, maxCount, minDistance, maxDistance, minAngle, maxAngle);
}

RandomObject.prototype.place = function(centerX, centerZ, player, avoidSelf, constraint, maxFailCount = 20)
{
	return this.simpleObject.place(centerX, centerZ, player, avoidSelf, constraint, maxFailCount);
};
