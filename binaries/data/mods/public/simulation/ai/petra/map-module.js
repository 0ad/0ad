var PETRA = function(m)
{

// other map functions
m.TERRITORY_PLAYER_MASK = 0x3F;

m.createObstructionMap = function(gameState, accessIndex, template)
{
	var passabilityMap = gameState.getMap();
	var territoryMap = gameState.ai.territoryMap;
	
	// default values
	var placementType = "land";
	var buildOwn = true;
	var buildAlly = true;
	var buildNeutral = true;
	var buildEnemy = false;
	// If there is a template then replace the defaults
	if (template)
	{
		placementType = template.buildPlacementType();
		buildOwn = template.hasBuildTerritory("own");
		buildAlly = template.hasBuildTerritory("ally");
		buildNeutral = template.hasBuildTerritory("neutral");
		buildEnemy = template.hasBuildTerritory("enemy");
	}
	var obstructionTiles = new Uint8Array(passabilityMap.data.length);
	
	if (placementType == "shore")
	{
		var obstructionMask = gameState.getPassabilityClassMask("foundationObstruction")
			| gameState.getPassabilityClassMask("building-shore")
			| gameState.getPassabilityClassMask("default");

		var okay = false;
		for (var x = 0; x < passabilityMap.width; ++x)
		{
			for (var y = 0; y < passabilityMap.height; ++y)
			{
				var i = x + y*passabilityMap.width;
				var xter = Math.floor((x+0.5)*passabilityMap.cellSize / territoryMap.cellSize);
				var yter = Math.floor((y+0.5)*passabilityMap.cellSize / territoryMap.cellSize);
				var iter = xter + yter*territoryMap.width;
				var tilePlayer = (territoryMap.data[iter] & m.TERRITORY_PLAYER_MASK);
				
				if (gameState.isPlayerEnemy(tilePlayer) && tilePlayer !== 0)
				{
					obstructionTiles[i] = 0;
					continue;
				}
				if ((passabilityMap.data[i] & (gameState.getPassabilityClassMask("building-shore") | gameState.getPassabilityClassMask("default"))))
				{
					obstructionTiles[i] = 0;
					continue;
				}

				okay = false;
				var positions = [[0,1], [1,1], [1,0], [1,-1], [0,-1], [-1,-1], [-1,0], [-1,1]];
				var available = 0;
				for (var stuff of positions)
				{
					var index = x + stuff[0] + (y+stuff[1])*passabilityMap.width;
					var index2 = x + stuff[0]*2 + (y+stuff[1]*2)*passabilityMap.width;
					var index3 = x + stuff[0]*3 + (y+stuff[1]*3)*passabilityMap.width;
					var index4 = x + stuff[0]*4 + (y+stuff[1]*4)*passabilityMap.width;
					
					if ((passabilityMap.data[index] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index,true) > 500)
						if ((passabilityMap.data[index2] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index2,true) > 500)
							if ((passabilityMap.data[index3] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index3,true) > 500)
								if ((passabilityMap.data[index4] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index4,true) > 500)
								{
									if (available < 2)
										available++;
									else
										okay = true;
								}
				}
				// checking for accessibility: if a neighbor is inaccessible, this is too. If it's not on the same "accessible map" as us, we crash-i~u.
				var radius = 3;
				for (var xx = -radius;xx <= radius; xx++)
					for (var yy = -radius;yy <= radius; yy++)
					{
						var id = x + xx + (y+yy)*passabilityMap.width;
						if (id > 0 && id < passabilityMap.data.length)
							if (gameState.ai.terrainAnalyzer.map[id] === 0 || gameState.ai.terrainAnalyzer.map[id] == 30 || gameState.ai.terrainAnalyzer.map[id] == 40)
								okay = false;
					}
				obstructionTiles[i] = okay ? 255 : 0;
			}

		}
	}
	else
	{
		var playerID = PlayerID;
		var obstructionMask = gameState.getPassabilityClassMask("foundationObstruction")
			| gameState.getPassabilityClassMask("building-land");
		
		for (var i = 0; i < passabilityMap.data.length; ++i)
		{
			var x = i % passabilityMap.width;
			var y = Math.floor(i / passabilityMap.width);
			var xter = Math.floor((x+0.5)*passabilityMap.cellSize / territoryMap.cellSize);
			var yter = Math.floor((y+0.5)*passabilityMap.cellSize / territoryMap.cellSize);
			var iter = xter + yter*territoryMap.width;
			var tilePlayer = (territoryMap.data[iter] & m.TERRITORY_PLAYER_MASK);
			var invalidTerritory = (
				(!buildOwn && tilePlayer == playerID) ||
				(!buildAlly && gameState.isPlayerAlly(tilePlayer) && tilePlayer != playerID) ||
				(!buildNeutral && tilePlayer == 0) ||
				(!buildEnemy && gameState.isPlayerEnemy(tilePlayer) && tilePlayer != 0)
			);
			if (accessIndex)
				var tileAccessible = (accessIndex === gameState.ai.accessibility.landPassMap[i]);
			else
				var tileAccessible = true;
			obstructionTiles[i] = (!tileAccessible || invalidTerritory || (passabilityMap.data[i] & obstructionMask)) ? 0 : 255;
		}
	}
	
	var map = new API3.Map(gameState.sharedScript, "passability", obstructionTiles);
	map.setMaxVal(255);
	
	if (template && template.buildDistance())
	{
		var minDist = template.buildDistance().MinDistance;
		var category = template.buildDistance().FromCategory;
		if (minDist !== undefined && category !== undefined)
		{
			gameState.getOwnStructures().forEach(function(ent) {
				if (ent.buildCategory() === category && ent.position())
				{
					var pos = ent.position();
					var x = Math.round(pos[0] / gameState.cellSize);
					var z = Math.round(pos[1] / gameState.cellSize);
					map.addInfluence(x, z, minDist/gameState.cellSize, -255, 'constant');
				}
			});
		}
	}

	return map;
};


m.createTerritoryMap = function(gameState)
{
	var map = gameState.ai.territoryMap;

	var ret = new API3.Map(gameState.sharedScript, "territory", map.data);	
	ret.getOwner = function(p) { return this.point(p) & m.TERRITORY_PLAYER_MASK; };
	ret.getOwnerIndex = function(p) { return this.map[p] & m.TERRITORY_PLAYER_MASK; };
	return ret;
};

// flag cells around the border of the map (2 if all points into that cell are inaccessible, 1 otherwise) 
m.createBorderMap = function(gameState)
{
	var map = new API3.Map(gameState.sharedScript, "territory");
	var width = map.width;
	var border = Math.round(60 / map.cellSize);
	var passabilityMap = gameState.sharedScript.passabilityMap;
	var obstructionLandMask = gameState.getPassabilityClassMask("default");
	var obstructionWaterMask = gameState.getPassabilityClassMask("ship");
	if (gameState.ai.circularMap)
	{
		var ic = (width - 1) / 2;
		var radcut = (ic - border) * (ic - border);
		for (var j = 0; j < map.length; ++j)
		{
			var dx = j%width - ic;
			var dy = Math.floor(j/width) - ic;
			var radius = dx*dx + dy*dy;
			if (radius < radcut)
				continue;
			map.map[j] = 2;
			var ind = API3.getMapIndices(j, map, passabilityMap);
			for (var k of ind)
			{
				if ((passabilityMap.data[j] & obstructionLandMask) && (passabilityMap.data[j] & obstructionWaterMask))
					continue;
				map.map[j] = 1;
				break;
			}
		}
	}
	else
	{
		var borderCut = width - border;
		for (var j = 0; j < map.length; ++j)
		{
			var ix = j%width;
			var iy = Math.floor(j/width);
			if (ix < border || ix >= borderCut || iy < border || iy >= borderCut)
			{
				map.map[j] = 2;
				var ind = API3.getMapIndices(j, map, passabilityMap);
				for (var k of ind)
				{
					if ((passabilityMap.data[j] & obstructionLandMask) && (passabilityMap.data[j] & obstructionWaterMask))
						continue;
					map.map[j] = 1;
					break;
				}
			}
		}
	}

//	map.dumpIm("border.png", 5);
	return map;
};

// map of our frontier : 2 means narrow border, 1 means large border
m.createFrontierMap = function(gameState)
{
	var territoryMap = gameState.ai.HQ.territoryMap;
	var borderMap = gameState.ai.HQ.borderMap;
	const around = [ [-0.7,0.7], [0,1], [0.7,0.7], [1,0], [0.7,-0.7], [0,-1], [-0.7,-0.7], [-1,0] ];

	var map = new API3.Map(gameState.sharedScript, "territory");
	var width = map.width;
	var insideSmall = Math.round(40 / map.cellSize);
	var insideLarge = Math.round(60 / map.cellSize);

	for (var j = 0; j < territoryMap.length; ++j)
	{
		if (territoryMap.getOwnerIndex(j) !== PlayerID || borderMap.map[j] > 1)
			continue;
		var ix = j%width;
		var iz = Math.floor(j/width);
		for (var a of around)
		{
			var jx = ix + Math.round(insideSmall*a[0]);
			if (jx < 0 || jx >= width)
				continue;
			var jz = iz + Math.round(insideSmall*a[1]);
			if (jz < 0 || jz >= width)
				continue;
			if (borderMap.map[jx+width*jz] > 1)
				continue;
			if (!gameState.isPlayerAlly(territoryMap.getOwnerIndex(jx+width*jz)))
			{
				map.map[j] = 2;
				break;
			}
			jx = ix + Math.round(insideLarge*a[0]);
			if (jx < 0 || jx >= width)
				continue;
			jz = iz + Math.round(insideLarge*a[1]);
			if (jz < 0 || jz >= width)
				continue;
			if (borderMap.map[jx+width*jz] > 1)
				continue;
			if (!gameState.isPlayerAlly(territoryMap.getOwnerIndex(jx+width*jz)))
				map.map[j] = 1;
		}
	}

//    m.debugMap(gameState, map);
	return map;
};

// return a measure of the proximity to our frontier (including our allies)
// 0=inside, 1=less than 16m, 2= less than 32m, 3= less than 48m, 4=less than 64m, 5=above 64m
m.getFrontierProximity = function(gameState, j)
{
	var territoryMap = gameState.ai.HQ.territoryMap;
	var borderMap = gameState.ai.HQ.borderMap;
	const around = [ [-0.7,0.7], [0,1], [0.7,0.7], [1,0], [0.7,-0.7], [0,-1], [-0.7,-0.7], [-1,0] ];

	var width = territoryMap.width;
	var step = Math.round(16 / territoryMap.cellSize);

	if (gameState.isPlayerAlly(territoryMap.getOwnerIndex(j)))
		return 0;

	var ix = j%width;
	var iz = Math.floor(j/width);
	var best = 5;
	for (let a of around)
	{
		for (let i = 1; i < 5; ++i)
		{
			let jx = ix + Math.round(i*step*a[0]);
			if (jx < 0 || jx >= width)
				continue;
			var jz = iz + Math.round(i*step*a[1]);
			if (jz < 0 || jz >= width)
				continue;
			if (borderMap.map[jx+width*jz] > 1)
				continue;
			if (gameState.isPlayerAlly(territoryMap.getOwnerIndex(jx+width*jz)))
			{
				best = Math.min(best, i);
				break;
			}
		}
		if (best === 1)
			break;
	}

	return best;
};

m.debugMap = function(gameState, map)
{
	var width = map.width;
	var cell = map.cellSize;
	gameState.getEntities().forEach( function (ent) {
		var pos = ent.position();
		if (!pos)
			return;
		var x = Math.round(pos[0] / cell);
		var z = Math.round(pos[1] / cell);
		var id = x + width*z;
		if (map.map[id] == 1)
			Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [2,0,0]});
		else if (map.map[id] == 2)
			Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,2,0]});
		else if (map.map[id] == 3)
			Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0,2]});
	});
};

return m;
}(PETRA);
