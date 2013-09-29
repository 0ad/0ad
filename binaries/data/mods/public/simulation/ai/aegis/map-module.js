// other map functions

Map.createObstructionMap = function(gameState, accessIndex, template){
	var passabilityMap = gameState.getMap();
	var territoryMap = gameState.ai.territoryMap;
	
	// default values
	var placementType = "land";
	var buildOwn = true;
	var buildAlly = true;
	var buildNeutral = true;
	var buildEnemy = false;
	// If there is a template then replace the defaults
	if (template){
		placementType = template.buildPlacementType();
		buildOwn = template.hasBuildTerritory("own");
		buildAlly = template.hasBuildTerritory("ally");
		buildNeutral = template.hasBuildTerritory("neutral");
		buildEnemy = template.hasBuildTerritory("enemy");
	}
	
	var obstructionMask = gameState.getPassabilityClassMask("foundationObstruction") | gameState.getPassabilityClassMask("building-land");
	
	if (placementType == "shore")
	{
		// TODO: this won't change much, should be cached, it's slow.
		var obstructionTiles = new Uint8Array(passabilityMap.data.length);
		var okay = false;
		for (var x = 0; x < passabilityMap.width; ++x)
		{
			for (var y = 0; y < passabilityMap.height; ++y)
			{
				var i = x + y*passabilityMap.width;
				var tilePlayer = (territoryMap.data[i] & TERRITORY_PLAYER_MASK);
				
				if (gameState.ai.myIndex !== gameState.ai.accessibility.landPassMap[i])
				{
					obstructionTiles[i] = 0;
					continue;
				}
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
				for each (var stuff in positions)
				{
					var index = x + stuff[0] + (y+stuff[1])*passabilityMap.width;
					var index2 = x + stuff[0]*2 + (y+stuff[1]*2)*passabilityMap.width;
					var index3 = x + stuff[0]*3 + (y+stuff[1]*3)*passabilityMap.width;
					var index4 = x + stuff[0]*4 + (y+stuff[1]*4)*passabilityMap.width;
					
					if ((passabilityMap.data[index] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index,true) > 500)
						if ((passabilityMap.data[index2] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index2,true) > 500)
							if ((passabilityMap.data[index3] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index3,true) > 500)
								if ((passabilityMap.data[index4] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index4,true) > 500) {
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
	} else {
		var playerID = PlayerID;
		
		var obstructionTiles = new Uint8Array(passabilityMap.data.length);
		for (var i = 0; i < passabilityMap.data.length; ++i)
		{
			var tilePlayer = (territoryMap.data[i] & TERRITORY_PLAYER_MASK);
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
			if (placementType === "shore")
				tileAccessible = true;
			obstructionTiles[i] = (!tileAccessible || invalidTerritory || (passabilityMap.data[i] & obstructionMask)) ? 0 : 255;
		}
	}
	
	var map = new Map(gameState.sharedScript, obstructionTiles);
	map.setMaxVal(255);
	
	if (template && template.buildDistance()){
		var minDist = template.buildDistance().MinDistance;
		var category = template.buildDistance().FromCategory;
		if (minDist !== undefined && category !== undefined){
			gameState.getOwnEntities().forEach(function(ent) {
											   if (ent.buildCategory() === category && ent.position()){
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



Map.createTerritoryMap = function(gameState) {
	var map = gameState.ai.territoryMap;
	
	var ret = new Map(gameState.sharedScript, map.data);
	
	ret.getOwner = function(p) {
		return this.point(p) & TERRITORY_PLAYER_MASK;
	}
	ret.getOwnerIndex = function(p) {
		return this.map[p] & TERRITORY_PLAYER_MASK;
	}
	return ret;
};
