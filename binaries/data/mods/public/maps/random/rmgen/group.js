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
 * @param x, z - The location the group is placed around. Can be omitted if the x and z properties are set externally.
 */
function SimpleGroup(objects, avoidSelf = false, tileClass = undefined, x = -1, z = -1)
{
	this.objects = objects;
	this.tileClass = tileClass;
	this.avoidSelf = avoidSelf;
	this.x = x;
	this.z = z;
}

SimpleGroup.prototype.place = function(player, constraint)
{
	let resultObjs = [];

	// Test if the Objects can be placed at the given location
	// Place none of them if one can't be placed.
	for (let object of this.objects)
	{
		let objs = object.place(this.x, this.z, player, this.avoidSelf, constraint);

		if (objs === undefined)
			return undefined;

		resultObjs = resultObjs.concat(objs);
	}

	// Add all objects to the map
	for (let obj of resultObjs)
	{
		let position = new Vector2D(obj.position.x, obj.position.z);

		if (g_Map.validTile(position))
			g_Map.addObject(obj);

		if (this.tileClass)
			this.tileClass.add(position.clone().floor());
	}

	return resultObjs;
};

/**
 * Randomly choses one of the given Objects and places it just like the SimpleGroup.
 */
function RandomGroup(objects, avoidSelf = false, tileClass = undefined, x = -1, z = -1)
{
	this.simpleGroup = new SimpleGroup([pickRandom(objects)], avoidSelf, tileClass, x, z);
}

RandomGroup.prototype.place = function(player, constraint)
{
	return this.simpleGroup.place(player, constraint);
};
