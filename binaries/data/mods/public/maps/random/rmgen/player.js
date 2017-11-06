/**
 * Places the given entities at the given location (typically a civic center and starting units).
 * @param civEntities - An array of objects with the Template property and optionally a Count property.
 * The first entity is placed in the center, the other ones surround it.
 */
function placeStartingEntities(fx, fz, playerID, civEntities, dist = 6, orientation = BUILDING_ORIENTATION)
{
	// Place the central structure
	let i = 0;
	let firstTemplate = civEntities[i].Template;
	if (firstTemplate.startsWith("structures/"))
	{
		placeObject(fx, fz, firstTemplate, playerID, orientation);
		++i;
	}

	// Place entities surrounding it
	let space = 2;
	for (let j = i; j < civEntities.length; ++j)
	{
		let angle = orientation - Math.PI * (1 - j / 2);
		let count = civEntities[j].Count || 1;

		for (let num = 0; num < count; ++num)
			placeObject(
				fx + dist * Math.cos(angle) + space * (-num + 0.75 * Math.floor(count / 2)) * Math.sin(angle),
				fz + dist * Math.sin(angle) + space * (num - 0.75 * Math.floor(count / 2)) * Math.cos(angle),
				civEntities[j].Template,
				playerID,
				angle);
	}
}

/**
 * Places the default starting entities as defined by the civilization definition and walls for Iberians.
 */
function placeCivDefaultEntities(fx, fz, playerID, kwargs, dist = 6, orientation = BUILDING_ORIENTATION)
{
	placeStartingEntities(fx, fz, playerID, getStartingEntities(playerID - 1), dist, orientation);

	let civ = getCivCode(playerID - 1);
	if (civ == 'iber' && getMapSize() > 128)
	{
		if (kwargs && kwargs.iberWall == 'towers')
			placePolygonalWall(fx, fz, 15, ['entry'], 'tower', civ, playerID, orientation, 7);
		else if (!kwargs || kwargs.iberWall)
			placeGenericFortress(fx, fz, 20, playerID);
	}
}
