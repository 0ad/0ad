const TERRITORY_PLAYER_MASK = 0x3F;

//TODO: Make this cope with negative cell values
function Map(gameState, originalMap, actualCopy){
	// get the map to find out the correct dimensions
	var gameMap = gameState.getMap();
	this.width = gameMap.width;
	this.height = gameMap.height;
	this.length = gameMap.data.length;

	if (originalMap && actualCopy){
		this.map = new Uint16Array(this.length);
		for (var i = 0; i < originalMap.length; ++i)
			this.map[i] = originalMap[i];
	} else if (originalMap) {
		this.map = originalMap;
	} else {
		this.map = new Uint16Array(this.length);
	}
	this.cellSize = gameState.cellSize;
}

Map.prototype.gamePosToMapPos = function(p){
	return [Math.floor(p[0]/this.cellSize), Math.floor(p[1]/this.cellSize)];
};

Map.prototype.point = function(p){
	var q = this.gamePosToMapPos(p);
	return this.map[q[0] + this.width * q[1]];
};

Map.createObstructionMap = function(gameState, template){
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
		// assume Dock, TODO.
		var obstructionTiles = new Uint16Array(passabilityMap.data.length);
		var okay = false;
		for (var x = 0; x < passabilityMap.width; ++x)
		{
			for (var y = 0; y < passabilityMap.height; ++y)
			{
				okay = false;
				var i = x + y*passabilityMap.width;
				var tilePlayer = (territoryMap.data[i] & TERRITORY_PLAYER_MASK);

				var positions = [[0,1], [1,1], [1,0], [1,-1], [0,-1], [-1,-1], [-1,0], [-1,1]];
				var available = 0;
				for each (stuff in positions)
				{
					var index = x + stuff[0] + (y+stuff[1])*passabilityMap.width;
					var index2 = x + stuff[0]*2 + (y+stuff[1]*2)*passabilityMap.width;
					var index3 = x + stuff[0]*3 + (y+stuff[1]*3)*passabilityMap.width;
					var index4 = x + stuff[0]*4 + (y+stuff[1]*4)*passabilityMap.width;
					if ((passabilityMap.data[index] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index) > 500)
						if ((passabilityMap.data[index2] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index2) > 500)
							if ((passabilityMap.data[index3] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index3) > 500)
								if ((passabilityMap.data[index4] & gameState.getPassabilityClassMask("default")) && gameState.ai.accessibility.getRegionSizei(index4) > 500) {
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
				if (gameState.ai.myIndex !== gameState.ai.accessibility.passMap[i])
					okay = false;
				if (gameState.isPlayerEnemy(tilePlayer) && tilePlayer !== 0)
					okay = false;
				if ((passabilityMap.data[i] & (gameState.getPassabilityClassMask("building-shore") | gameState.getPassabilityClassMask("default"))))
					okay = false;
				obstructionTiles[i] = okay ? 65535 : 0;
			}
		}
	} else {
		var playerID = PlayerID;
		
		var obstructionTiles = new Uint16Array(passabilityMap.data.length);
		for (var i = 0; i < passabilityMap.data.length; ++i)
		{
			var tilePlayer = (territoryMap.data[i] & TERRITORY_PLAYER_MASK);
			var invalidTerritory = (
									(!buildOwn && tilePlayer == playerID) ||
									(!buildAlly && gameState.isPlayerAlly(tilePlayer) && tilePlayer != playerID) ||
									(!buildNeutral && tilePlayer == 0) ||
									(!buildEnemy && gameState.isPlayerEnemy(tilePlayer) && tilePlayer != 0)
									);
			var tileAccessible = (gameState.ai.myIndex === gameState.ai.accessibility.passMap[i]);
			if (placementType === "shore")
				tileAccessible = true;
			obstructionTiles[i] = (!tileAccessible || invalidTerritory || (passabilityMap.data[i] & obstructionMask)) ? 0 : 65535;
		}
	}
		
	var map = new Map(gameState, obstructionTiles);
	
	if (template && template.buildDistance()){
		var minDist = template.buildDistance().MinDistance;
		var category = template.buildDistance().FromCategory;
		if (minDist !== undefined && category !== undefined){
			gameState.getOwnEntities().forEach(function(ent) {
				if (ent.buildCategory() === category && ent.position()){
					var pos = ent.position();
					var x = Math.round(pos[0] / gameState.cellSize);
					var z = Math.round(pos[1] / gameState.cellSize);
					map.addInfluence(x, z, minDist/gameState.cellSize, -65535, 'constant');
				}
			});
		}
	}
	
	return map;
};

Map.createTerritoryMap = function(gameState) {
	var map = gameState.ai.territoryMap;
	
	var ret = new Map(gameState, map.data);
	
	ret.getOwner = function(p) {
		return this.point(p) & TERRITORY_PLAYER_MASK;
	}
	ret.getOwnerIndex = function(p) {
		return this.map[p] & TERRITORY_PLAYER_MASK;
	}
	return ret;
};

Map.prototype.addInfluence = function(cx, cy, maxDist, strength, type) {
	strength = strength ? +strength : +maxDist;
	type = type ? type : 'linear';
	
	var x0 = Math.max(0, cx - maxDist);
	var y0 = Math.max(0, cy - maxDist);
	var x1 = Math.min(this.width, cx + maxDist);
	var y1 = Math.min(this.height, cy + maxDist);
	var maxDist2 = maxDist * maxDist;
	
	var str = 0.0;
	switch (type){
	case 'linear':
		str = +strength / +maxDist;
		break;
	case 'quadratic':
		str = +strength / +maxDist2;
		break;
	case 'constant':
		str = +strength;
		break;
	}
	
	for ( var y = y0; y < y1; ++y) {
		for ( var x = x0; x < x1; ++x) {
			var dx = x - cx;
			var dy = y - cy;
			var r2 = dx*dx + dy*dy;
			if (r2 < maxDist2){
				var quant = 0;
				switch (type){
				case 'linear':
					var r = Math.sqrt(r2);
					quant = str * (maxDist - r);
					break;
				case 'quadratic':
					quant = str * (maxDist2 - r2);
					break;
				case 'constant':
					quant = str;
					break;
				}
				if (-1 * quant > this.map[x + y * this.width]){
					this.map[x + y * this.width] = 0; //set anything which would have gone negative to 0
				}else{
					this.map[x + y * this.width] += quant;
				}
			}
		}
	}
};

Map.prototype.multiplyInfluence = function(cx, cy, maxDist, strength, type) {
	strength = strength ? +strength : +maxDist;
	type = type ? type : 'constant';
	
	var x0 = Math.max(0, cx - maxDist);
	var y0 = Math.max(0, cy - maxDist);
	var x1 = Math.min(this.width, cx + maxDist);
	var y1 = Math.min(this.height, cy + maxDist);
	var maxDist2 = maxDist * maxDist;
	
	var str = 0.0;
	switch (type){
		case 'linear':
			str = strength / maxDist;
			break;
		case 'quadratic':
			str = strength / maxDist2;
			break;
		case 'constant':
			str = strength;
			break;
	}
	
	for ( var y = y0; y < y1; ++y) {
		for ( var x = x0; x < x1; ++x) {
			var dx = x - cx;
			var dy = y - cy;
			var r2 = dx*dx + dy*dy;
			if (r2 < maxDist2){
				var quant = 0;
				switch (type){
					case 'linear':
						var r = Math.sqrt(r2);
						quant = str * (maxDist - r);
						break;
					case 'quadratic':
						quant = str * (maxDist2 - r2);
						break;
					case 'constant':
						quant = str;
						break;
				}
				var machin = this.map[x + y * this.width] * quant;
				if (machin <= 0){
					this.map[x + y * this.width] = 0; //set anything which would have gone negative to 0
				}else{
					this.map[x + y * this.width] = machin;
				}
			}
		}
	}
};
Map.prototype.setInfluence = function(cx, cy, maxDist, value) {
	value = value ? value : 0;
	
	var x0 = Math.max(0, cx - maxDist);
	var y0 = Math.max(0, cy - maxDist);
	var x1 = Math.min(this.width, cx + maxDist);
	var y1 = Math.min(this.height, cy + maxDist);
	var maxDist2 = maxDist * maxDist;
	
	for ( var y = y0; y < y1; ++y) {
		for ( var x = x0; x < x1; ++x) {
			var dx = x - cx;
			var dy = y - cy;
			var r2 = dx*dx + dy*dy;
			if (r2 < maxDist2){
				this.map[x + y * this.width] = value;
			}
		}
	}
};

Map.prototype.sumInfluence = function(cx, cy, radius){
	var x0 = Math.max(0, cx - radius);
	var y0 = Math.max(0, cy - radius);
	var x1 = Math.min(this.width, cx + radius);
	var y1 = Math.min(this.height, cy + radius);
	var radius2 = radius * radius;
	
	var sum = 0;
	
	for ( var y = y0; y < y1; ++y) {
		for ( var x = x0; x < x1; ++x) {
			var dx = x - cx;
			var dy = y - cy;
			var r2 = dx*dx + dy*dy;
			if (r2 < radius2){
				sum += this.map[x + y * this.width];
			}
		}
	}
	return sum;
};

/**
 * Make each cell's 16-bit value at least one greater than each of its
 * neighbours' values. (If the grid is initialised with 0s and 65535s, the
 * result of each cell is its Manhattan distance to the nearest 0.)
 * 
 * TODO: maybe this should be 8-bit (and clamp at 255)?
 */
Map.prototype.expandInfluences = function() {
	var w = this.width;
	var h = this.height;
	var grid = this.map;
	for ( var y = 0; y < h; ++y) {
		var min = 65535;
		for ( var x = 0; x < w; ++x) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
		}

		for ( var x = w - 2; x >= 0; --x) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
		}
	}

	for ( var x = 0; x < w; ++x) {
		var min = 65535;
		for ( var y = 0; y < h; ++y) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
		}

		for ( var y = h - 2; y >= 0; --y) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
		}
	}
};

Map.prototype.findBestTile = function(radius, obstructionTiles){
	// Find the best non-obstructed tile
	var bestIdx = 0;
	var bestVal = -1;
	for ( var i = 0; i < this.length; ++i) {
		if (obstructionTiles.map[i] > radius) {
			var v = this.map[i];
			if (v > bestVal) {
				bestVal = v;
				bestIdx = i;
			}
		}
	}
	
	return [bestIdx, bestVal];
};

// Multiplies current map by 3 if in my territory
Map.prototype.multiplyTerritory = function(gameState,map,evenNeutral){
	for (var i = 0; i < this.length; ++i){
		if (map.getOwnerIndex(i) === PlayerID)
			this.map[i] *= 2.5;
		else if (map.getOwnerIndex(i) !== PlayerID && (map.getOwnerIndex(i) !== 0 || evenNeutral))
			this.map[i] = 0;
	}
};

// sets to 0 any enemy territory
Map.prototype.annulateTerritory = function(gameState,map,evenNeutral){
	for (var i = 0; i < this.length; ++i){
		if (map.getOwnerIndex(i) !== PlayerID && (map.getOwnerIndex(i) !== 0 || evenNeutral))
			this.map[i] = 0;
	}
};

// Multiplies current map by the parameter map pixelwise
Map.prototype.multiply = function(map, onlyBetter,divider,maxMultiplier){
	for (var i = 0; i < this.length; ++i){
		if (map.map[i]/divider > 1)
			this.map[i] = Math.min(maxMultiplier*this.map[i], this.map[i] * (map.map[i]/divider));
	}
};
// add to current map by the parameter map pixelwise
Map.prototype.add = function(map){
	for (var i = 0; i < this.length; ++i){
		this.map[i] += +map.map[i];
	}
};
// add to current map by the parameter map pixelwise with a multiplier
Map.prototype.madd = function(map, multiplier){
	for (var i = 0; i < this.length; ++i){
		this.map[i] += +map.map[i]*multiplier;
	}
};
// add to current map if the map is not null in that point
Map.prototype.addIfNotNull = function(map){
	for (var i = 0; i < this.length; ++i){
		if (this.map[i] !== 0)
			this.map[i] += +map.map[i];
	}
};
// add to current map by the parameter map pixelwise
Map.prototype.subtract = function(map){
	for (var i = 0; i < this.length; ++i){
		this.map[i] = this.map[i] - map.map[i] < 0 ? 0 : this.map[i] - map.map[i];
	}
};
// add to current map by the parameter map pixelwise with a multiplier
Map.prototype.subtractMultiplied = function(map,multiple){
	for (var i = 0; i < this.length; ++i){
		this.map[i] = this.map[i] - map.map[i]*multiple < 0 ? 0 : this.map[i] - map.map[i]*multiple;
	}
};

Map.prototype.dumpIm = function(name, threshold){
	name = name ? name : "default.png";
	threshold = threshold ? threshold : 65500;
	Engine.DumpImage(name, this.map, this.width, this.height, threshold);
};
