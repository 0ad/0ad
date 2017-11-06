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
