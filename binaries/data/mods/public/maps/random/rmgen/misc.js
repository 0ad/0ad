/////////////////////////////////////////////////////////////////////////////////////////
// createStartingPlayerEntities
//
//	Creates the starting player entities
//	fx&fz: position of player base
//	playerid: id of player
//	civEntities: use getStartingEntities(id-1) fo this one
//	orientation: orientation of the main base building, default is BUILDING_ORIENTATION
//
///////////////////////////////////////////////////////////////////////////////////////////
function createStartingPlayerEntities(fx, fz, playerid, civEntities, orientation = BUILDING_ORIENTATION)
{
	var uDist = 6;
	var uSpace = 2;
	placeObject(fx, fz, civEntities[0].Template, playerid, orientation);
	for (var j = 1; j < civEntities.length; ++j)
	{
		var uAngle = orientation - PI * (2-j) / 2;
		var count = (civEntities[j].Count !== undefined ? civEntities[j].Count : 1);
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = fx + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = fz + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			placeObject(ux, uz, civEntities[j].Template, playerid, uAngle);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// placeCivDefaultEntities
//
//	Creates the default starting player entities depending on the players civ
//	fx&fy: position of player base
//	playerid: id of player
//	kwargs: Takes some optional keyword arguments to tweek things
//		'iberWall': may be false, 'walls' (default) or 'towers'. Determines the defensive structures Iberians get as civ bonus
//		'orientation': angle of the main base building, default is BUILDING_ORIENTATION
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeCivDefaultEntities(fx, fz, playerid, kwargs = {})
{
	// Unpack kwargs
	var iberWall = 'walls';
	if (getMapSize() <= 128)
		iberWall = false;
	if ('iberWall' in kwargs)
		iberWall = kwargs['iberWall'];
	var orientation = BUILDING_ORIENTATION;
	if ('orientation' in kwargs)
		orientation = kwargs['orientation'];
	// Place default civ starting entities
	var civ = getCivCode(playerid-1);
	var civEntities = getStartingEntities(playerid-1);
	var uDist = 6;
	var uSpace = 2;
	placeObject(fx, fz, civEntities[0].Template, playerid, orientation);
	for (var j = 1; j < civEntities.length; ++j)
	{
		var uAngle = orientation - PI * (2-j) / 2;
		var count = (civEntities[j].Count !== undefined ? civEntities[j].Count : 1);
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = fx + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = fz + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			placeObject(ux, uz, civEntities[j].Template, playerid, uAngle);
		}
	}
	// Add defensive structiures for Iberians as their civ bonus
	if (civ == 'iber' && iberWall != false)
	{
		if (iberWall == 'towers')
			placePolygonalWall(fx, fz, 15, ['entry'], 'tower', civ, playerid, orientation, 7);
		else
			placeGenericFortress(fx, fz, 20/*radius*/, playerid);
	}
}

function placeDefaultChicken(playerX, playerZ, tileClass, constraint = undefined, template = "gaia/fauna_chicken")
{
	for (let j = 0; j < 2; ++j)
		for (var tries = 0; tries < 10; ++tries)
		{
			let aAngle = randFloat(0, TWO_PI);

			// Roman and ptolemian civic centers have a big footprint!
			let aDist = 9;

			let aX = round(playerX + aDist * cos(aAngle));
			let aZ = round(playerZ + aDist * sin(aAngle));

			let group = new SimpleGroup(
				[new SimpleObject(template, 5,5, 0,2)],
				true, tileClass, aX, aZ
			);

			if (createObjectGroup(group, 0, constraint))
				break;
		}
}

/**
 * Typically used for placing grass tufts around the civic centers.
 */
function placeDefaultDecoratives(playerX, playerZ, template, tileclass, radius, constraint = undefined)
{
	for (let i = 0; i < PI * radius * radius / 250; ++i)
	{
		let angle = randFloat(0, 2 * PI);
		let dist = radius - randIntInclusive(5, 11);

		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(template, 2, 5, 0, 1, -PI/8, PI/8)],
				false,
				tileclass,
				Math.round(playerX + dist * Math.cos(angle)),
				Math.round(playerZ + dist * Math.sin(angle))
			), 0, constraint);
	}
}

function modifyTilesBasedOnHeight(minHeight, maxHeight, mode, func)
{
	for (let qx = 0; qx < g_Map.size; ++qx)
		for (let qz = 0; qz < g_Map.size; ++qz)
		{
			let height = g_Map.getHeight(qx, qz);
			if (mode == 0 && height >  minHeight && height < maxHeight ||
			    mode == 1 && height >= minHeight && height < maxHeight ||
			    mode == 2 && height >  minHeight && height <= maxHeight ||
			    mode == 3 && height >= minHeight && height <= maxHeight)
			func(qx, qz);
		}
}

function paintTerrainBasedOnHeight(minHeight, maxHeight, mode, terrain)
{
	modifyTilesBasedOnHeight(minHeight, maxHeight, mode, (qx, qz) => {
		placeTerrain(qx, qz, terrain);
	});
}

function paintTileClassBasedOnHeight(minHeight, maxHeight, mode, tileclass)
{
	modifyTilesBasedOnHeight(minHeight, maxHeight, mode, (qx, qz) => {
		addToClass(qx, qz, tileclass);
	});
}

function unPaintTileClassBasedOnHeight(minHeight, maxHeight, mode, tileclass)
{
	modifyTilesBasedOnHeight(minHeight, maxHeight, mode, (qx, qz) => {
		removeFromClass(qx, qz, tileclass);
	});
}
