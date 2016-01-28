var PETRA = function(m)
{

// other map functions
m.TERRITORY_PLAYER_MASK = 0x1F;

m.createObstructionMap = function(gameState, accessIndex, template)
{
	var passabilityMap = gameState.getMap();
	var territoryMap = gameState.ai.territoryMap;
	var ratio = territoryMap.cellSize / passabilityMap.cellSize;
	
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
	
	var passMap;
	var obstructionMask;
	if (placementType == "shore")
	{
		passMap = gameState.ai.accessibility.navalPassMap;
		obstructionMask = gameState.getPassabilityClassMask("building-shore");
	}
	else
	{
		passMap = gameState.ai.accessibility.landPassMap;
		obstructionMask = gameState.getPassabilityClassMask("building-land");
	}

	for (let k = 0; k < territoryMap.data.length; ++k)
	{
		let tilePlayer = (territoryMap.data[k] & m.TERRITORY_PLAYER_MASK);
		if ((!buildNeutral && tilePlayer == 0) ||
			(!buildOwn && tilePlayer == PlayerID) ||
			(!buildAlly && tilePlayer != PlayerID && gameState.isPlayerAlly(tilePlayer)) ||
			(!buildEnemy && tilePlayer != 0 && gameState.isPlayerEnemy(tilePlayer)))
			continue;
		let x = ratio * (k % territoryMap.width);
		let y = ratio * (Math.floor(k / territoryMap.width));
		for (let ix = 0; ix < ratio; ++ix)
		{
			for (let iy = 0; iy < ratio; ++iy)
			{
				let i = x + ix + (y + iy)*passabilityMap.width;
				if (placementType != "shore" && accessIndex && accessIndex !== passMap[i])
					continue;
				if (!(passabilityMap.data[i] & obstructionMask))
					obstructionTiles[i] = 255;
			}
		}
	}

	var map = new API3.Map(gameState.sharedScript, "passability", obstructionTiles);
	map.setMaxVal(255);

	if (template && template.buildDistance())
	{
		let minDist = +template.buildDistance().MinDistance;
		let fromClass = template.buildDistance().FromClass;
		if (minDist && fromClass)
		{
			let cellSize = passabilityMap.cellSize;
			let cellDist = 1 + minDist / cellSize;
			let structures = gameState.getOwnStructures().filter(API3.Filters.byClass(fromClass));
			for (let ent of structures.values())
			{
				if (!ent.position())
					continue;
				let pos = ent.position();
				let x = Math.round(pos[0] / cellSize);
				let z = Math.round(pos[1] / cellSize);
				map.addInfluence(x, z, cellDist, -255, "constant");
			}
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
	var border = Math.round(80 / map.cellSize);
	var passabilityMap = gameState.sharedScript.passabilityMap;
	var obstructionMask = gameState.getPassabilityClassMask("unrestricted");
	if (gameState.ai.circularMap)
	{
		let ic = (width - 1) / 2;
		let radcut = (ic - border) * (ic - border);
		for (let j = 0; j < map.length; ++j)
		{
			let dx = j%width - ic;
			let dy = Math.floor(j/width) - ic;
			let radius = dx*dx + dy*dy;
			if (radius < radcut)
				continue;
			map.map[j] = 2;
			let ind = API3.getMapIndices(j, map, passabilityMap);
			for (let k of ind)
			{
				if (passabilityMap.data[k] & obstructionMask)
					continue;
				map.map[j] = 1;
				break;
			}
		}
	}
	else
	{
		let borderCut = width - border;
		for (let j = 0; j < map.length; ++j)
		{
			let ix = j%width;
			let iy = Math.floor(j/width);
			if (ix < border || ix >= borderCut || iy < border || iy >= borderCut)
			{
				map.map[j] = 2;
				let ind = API3.getMapIndices(j, map, passabilityMap);
				for (let k of ind)
				{
					if (passabilityMap.data[k] & obstructionMask)
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
	var insideSmall = Math.round(45 / map.cellSize);
	var insideLarge = Math.round(80 / map.cellSize);	// should be about the range of towers

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
