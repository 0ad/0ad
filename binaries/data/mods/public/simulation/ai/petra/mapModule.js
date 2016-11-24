var PETRA = function(m)
{

/** map functions */

m.TERRITORY_PLAYER_MASK = 0x1F;
m.TERRITORY_BLINKING_MASK = 0x40;

m.createObstructionMap = function(gameState, accessIndex, template)
{
	let passabilityMap = gameState.getMap();
	let territoryMap = gameState.ai.territoryMap;
	let ratio = territoryMap.cellSize / passabilityMap.cellSize;

	// default values
	let placementType = "land";
	let buildOwn = true;
	let buildAlly = true;
	let buildNeutral = true;
	let buildEnemy = false;
	// If there is a template then replace the defaults
	if (template)
	{
		placementType = template.buildPlacementType();
		buildOwn = template.hasBuildTerritory("own");
		buildAlly = template.hasBuildTerritory("ally");
		buildNeutral = template.hasBuildTerritory("neutral");
		buildEnemy = template.hasBuildTerritory("enemy");
	}
	let obstructionTiles = new Uint8Array(passabilityMap.data.length);

	let passMap;
	let obstructionMask;
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
		let isConnected = (territoryMap.data[k] & m.TERRITORY_BLINKING_MASK) == 0;
		if (tilePlayer === PlayerID)
		{
			if (!buildOwn || !buildNeutral && !isConnected)
				continue;
		}
		else if (gameState.isPlayerMutualAlly(tilePlayer))
		{
			if (!buildAlly || !buildNeutral && !isConnected)
				continue;
		}
		else if (tilePlayer === 0)
		{
			if (!buildNeutral)
				continue;
		}
		else
		{
			if (!buildEnemy)
				continue;
		}

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

	let map = new API3.Map(gameState.sharedScript, "passability", obstructionTiles);
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
	let map = gameState.ai.territoryMap;

	let ret = new API3.Map(gameState.sharedScript, "territory", map.data);
	ret.getOwner = function(p) { return this.point(p) & m.TERRITORY_PLAYER_MASK; };
	ret.getOwnerIndex = function(p) { return this.map[p] & m.TERRITORY_PLAYER_MASK; };
	return ret;
};

/** flag cells around the border of the map (2 if all points into that cell are inaccessible, 1 otherwise) */
m.createBorderMap = function(gameState)
{
	let map = new API3.Map(gameState.sharedScript, "territory");
	let width = map.width;
	let border = Math.round(80 / map.cellSize);
	let passabilityMap = gameState.sharedScript.passabilityMap;
	let obstructionMask = gameState.getPassabilityClassMask("unrestricted");
	if (gameState.circularMap)
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

/** map of our frontier : 2 means narrow border, 1 means large border */
m.createFrontierMap = function(gameState)
{
	let alliedVictory = gameState.getAlliedVictory();
	let territoryMap = gameState.ai.HQ.territoryMap;
	let borderMap = gameState.ai.HQ.borderMap;
	const around = [ [-0.7,0.7], [0,1], [0.7,0.7], [1,0], [0.7,-0.7], [0,-1], [-0.7,-0.7], [-1,0] ];

	let map = new API3.Map(gameState.sharedScript, "territory");
	let width = map.width;
	let insideSmall = Math.round(45 / map.cellSize);
	let insideLarge = Math.round(80 / map.cellSize);	// should be about the range of towers

	for (let j = 0; j < territoryMap.length; ++j)
	{
		if (territoryMap.getOwnerIndex(j) !== PlayerID || borderMap.map[j] > 1)
			continue;
		let ix = j%width;
		let iz = Math.floor(j/width);
		for (let a of around)
		{
			let jx = ix + Math.round(insideSmall*a[0]);
			if (jx < 0 || jx >= width)
				continue;
			let jz = iz + Math.round(insideSmall*a[1]);
			if (jz < 0 || jz >= width)
				continue;
			if (borderMap.map[jx+width*jz] > 1)
				continue;
			let territoryOwner = territoryMap.getOwnerIndex(jx+width*jz);
			let safe = (alliedVictory && gameState.isPlayerAlly(territoryOwner)) || territoryOwner === PlayerID;
			if (!safe)
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
			territoryOwner = territoryMap.getOwnerIndex(jx+width*jz);
			safe = (alliedVictory && gameState.isPlayerAlly(territoryOwner)) || territoryOwner === PlayerID;
			if (!safe)
				map.map[j] = 1;
		}
	}

//    m.debugMap(gameState, map);
	return map;
};

m.debugMap = function(gameState, map)
{
	let width = map.width;
	let cell = map.cellSize;
	gameState.getEntities().forEach( function (ent) {
		let pos = ent.position();
		if (!pos)
			return;
		let x = Math.round(pos[0] / cell);
		let z = Math.round(pos[1] / cell);
		let id = x + width*z;
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
