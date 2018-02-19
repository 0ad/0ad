/**
 * @file A Group tests if a set of entities specified in the constructor can be placed and
 * potentially places some of them (typically all or none).
 *
 * The location is defined by the x and z property of the Group instance and can be modified externally.
 * The Group is free to determine whether, where exactly and how many entities to place.
 *
 * The Constraint to test against and the future owner of the entities are passed by the caller.
 * Typically Groups are called from createObjectGroup with the location set in the constructor or
 * from createObjectGroups that randomizes the x and z property of the Group before calling place.
 */

/**
 * Places all of the given Objects.
 *
 * @param objects - An array of Objects, for instance SimpleObjects.
 * @param avoidSelf - Objects will not overlap.
 * @param tileClass - Optional TileClass that tiles with placed entities are marked with.
 * @param centerPosition - The location the group is placed around. Can be omitted if the property is set externally.
 */
function SimpleGroup(objects, avoidSelf = false, tileClass = undefined, centerPosition = undefined)
{
	this.objects = objects;
	this.tileClass = tileClass;
	this.avoidSelf = avoidSelf;
	this.centerPosition = undefined;

	if (centerPosition)
		this.setCenterPosition(centerPosition);
}

SimpleGroup.prototype.setCenterPosition = function(position)
{
	this.centerPosition = deepfreeze(position.clone().round());
};

SimpleGroup.prototype.place = function(player, constraint)
{
	let entitySpecsResult = [];

	// Test if the Objects can be placed at the given location
	// Place none of them if one can't be placed.
	for (let object of this.objects)
	{
		let entitySpecs = object.place(this.centerPosition, player, this.avoidSelf, constraint);

		if (!entitySpecs)
			return undefined;

		entitySpecsResult = entitySpecsResult.concat(entitySpecs);
	}


	// Create and place entities as specified
	let entities = [];
	for (let entitySpecs of entitySpecsResult)
	{
		// The Object must ensure that non-actor entities are not placed at the impassable map-border
		entities.push(
			g_Map.placeEntityAnywhere(entitySpecs.templateName, entitySpecs.playerID, entitySpecs.position, entitySpecs.angle));

		if (this.tileClass)
			this.tileClass.add(entitySpecs.position.clone().floor());
	}

	return entities;
};

/**
 * Randomly choses one of the given Objects and places it just like the SimpleGroup.
 */
function RandomGroup(objects, avoidSelf = false, tileClass = undefined, centerPosition = undefined)
{
	this.simpleGroup = new SimpleGroup([pickRandom(objects)], avoidSelf, tileClass, centerPosition);
}

RandomGroup.prototype.setCenterPosition = function(position)
{
	this.simpleGroup.setCenterPosition(position);
};

RandomGroup.prototype.place = function(player, constraint)
{
	return this.simpleGroup.place(player, constraint);
};
