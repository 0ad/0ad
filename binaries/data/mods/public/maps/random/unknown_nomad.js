RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmbiome");
RMS.LoadLibrary("the_unknown");

g_PlayerBases = false;
g_AllowNaval = true;

createUnknownMap();

var distmin = Math.square(scaleByMapSize(60, 240));
playerIDs = sortAllPlayers();

for (var i = 0; i < numPlayers; i++)
{
	var placableArea = [];
	for (var mx = 0; mx < mapSize; mx++)
	{
		for (var mz = 0; mz < mapSize; mz++)
		{
			if (!g_Map.validT(mx, mz, 3))
				continue;

			var placable = true;
			for (var c = 0; c < i; c++)
				if ((playerX[c] - mx)*(playerX[c] - mx) + (playerZ[c] - mz)*(playerZ[c] - mz) < distmin)
					placable = false;
			if (!placable)
				continue;

			if (g_Map.getHeight(mx, mz) >= 3 && g_Map.getHeight(mx, mz) <= 3.12)
				placableArea.push([mx, mz]);
		}
	}

	if (!placableArea.length)
	{
		for (var mx = 0; mx < mapSize; ++mx)
		{
			for (var mz = 0; mz < mapSize; mz++)
			{
				if (!g_Map.validT(mx, mz, 3))
					continue;

				var placable = true;
				for (var c = 0; c < i; c++)
					if ((playerX[c] - mx)*(playerX[c] - mx) + (playerZ[c] - mz)*(playerZ[c] - mz) < distmin/4)
						placable = false;
				if (!placable)
					continue;

				if (g_Map.getHeight(mx, mz) >= 3 && g_Map.getHeight(mx, mz) <= 3.12)
					placableArea.push([mx, mz]);
			}
		}
	}

	if (!placableArea.length)
		for (var mx = 0; mx < mapSize; ++mx)
			for (var mz = 0; mz < mapSize; ++mz)
				if (g_Map.getHeight(mx, mz) >= 3 && g_Map.getHeight(mx, mz) <= 3.12)
					placableArea.push([mx, mz]);

	[playerX[i], playerZ[i]] = pickRandom(placableArea);
}

for (var i = 0; i < numPlayers; ++i)
{
	var id = playerIDs[i];
	log("Creating units for player " + id + "...");

	// get the x and z in tiles
	var ix = playerX[i];
	var iz = playerZ[i];

	playerX[i] = tilesToFraction(playerX[i]);
	playerZ[i] = tilesToFraction(playerZ[i]);

	var civEntities = getStartingEntities(playerIDs[i]);
	var angle = randFloat(0, TWO_PI);
	for (var j = 0; j < civEntities.length; ++j)
	{
		// TODO: Make an rmlib function to get only non-structure starting entities and loop over those
		if (!civEntities[j].Template.startsWith("units/"))
			continue;

		var count = civEntities[j].Count || 1;
		var jx = ix + 2 * cos(angle);
		var jz = iz + 2 * sin(angle);
		var kAngle = randFloat(0, TWO_PI);
		for (var k = 0; k < count; ++k)
			placeObject(jx + cos(kAngle + k*TWO_PI/count), jz + sin(kAngle + k*TWO_PI/count), civEntities[j].Template, id, randFloat(0, TWO_PI));
		angle += TWO_PI / 3;
	}

	{
		if (g_MapSettings.StartingResources < 500)
		{
			var loop = (g_MapSettings.StartingResources < 200) ? 2 : 1;
			for (let l = 0; l < loop; ++l)
			{
				var angle = randFloat(0, TWO_PI);
				var rad = randFloat(3, 5);
				var jx = ix + rad * cos(angle);
				var jz = iz + rad * sin(angle);
				placeObject(jx, jz, "gaia/special_treasure_wood", 0, randFloat(0, TWO_PI));
				var angle = randFloat(0, TWO_PI);
				var rad = randFloat(3, 5);
				var jx = ix + rad * cos(angle);
				var jz = iz + rad * sin(angle);
				placeObject(jx, jz, "gaia/special_treasure_stone", 0, randFloat(0, TWO_PI));
				var angle = randFloat(0, TWO_PI);
				var rad = randFloat(3, 5);
				var jx = ix + rad * cos(angle);
				var jz = iz + rad * sin(angle);
				placeObject(jx, jz, "gaia/special_treasure_metal", 0, randFloat(0, TWO_PI));
			}
		}
	}
}

// Don't place resources into the starting units
markPlayerArea("small");

createUnknownObjects();

ExportMap();
