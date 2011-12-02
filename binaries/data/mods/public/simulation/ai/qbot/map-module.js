// TODO: Make this cope with negative cell values 

function Map(gameState, originalMap){
	// get the map to find out the correct dimensions
	var gameMap = gameState.getMap();
	this.width = gameMap.width;
	this.height = gameMap.height;
	this.length = gameMap.data.length;
	if (originalMap){
		this.map = originalMap;
	}else{
		this.map = new Uint16Array(this.length);
	}
	this.cellSize = gameState.cellSize;
}

Map.prototype.gamePosToMapPos = function(p){
	return [Math.round(p[0]/this.cellSize), Math.round(p[1]/this.cellSize)];
};

Map.createObstructionMap = function(gameState, template){
	var passabilityMap = gameState.getMap();
	var territoryMap = gameState.getTerritoryMap(); 
	
	const TERRITORY_PLAYER_MASK = 0x7F;
	
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

	var obstructionMask = gameState.getPassabilityClassMask("foundationObstruction");
	// Only accept valid land tiles (we don't handle docks yet)
	switch(placementType){
		case "shore":
			obstructionMask |= gameState.getPassabilityClassMask("building-shore");
			break;
		case "land":
		default:
			obstructionMask |= gameState.getPassabilityClassMask("building-land");
			break;
	}
	
	var playerID = gameState.getPlayerID();  
	
	var obstructionTiles = new Uint16Array(passabilityMap.data.length); 
	for (var i = 0; i < passabilityMap.data.length; ++i) 
	{
		var tilePlayer = (territoryMap.data[i] & TERRITORY_PLAYER_MASK); 
		var invalidTerritory = ( 
			(!buildOwn && tilePlayer == playerID) || 
			(!buildAlly && gameState.isPlayerAlly(tilePlayer) && tilePlayer != playerID) || 
			(!buildNeutral && tilePlayer == 0) || 
			(!buildEnemy && gameState.isPlayerEnemy(tilePlayer) && tilePlayer !=0) 
		);
		var tileAccessible = (gameState.ai.accessibility.map[i] == 1);
		obstructionTiles[i] = (!tileAccessible || invalidTerritory || (passabilityMap.data[i] & obstructionMask)) ? 0 : 65535; 
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

Map.createTerritoryMap = function(gameState){
	var map = gameState.ai.territoryMap;

	var obstructionTiles = new Uint16Array(map.data.length);
	for ( var i = 0; i < map.data.length; ++i){
		obstructionTiles[i] = map.data[i] & 0x7F;
	}
	
	return new Map(gameState, obstructionTiles);
};

Map.prototype.addInfluence = function(cx, cy, maxDist, strength, type) {
	strength = strength ? strength : maxDist;
	type = type ? type : 'linear';
	
	var x0 = Math.max(0, cx - maxDist);
	var y0 = Math.max(0, cy - maxDist);
	var x1 = Math.min(this.width, cx + maxDist);
	var y1 = Math.min(this.height, cy + maxDist);
	var maxDist2 = maxDist * maxDist;
	
	var str = 0;
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
				
				if (-1 * quant > this.map[x + y * this.width]){
					this.map[x + y * this.width] = 0; //set anything which would have gone negative to 0
				}else{
					this.map[x + y * this.width] += quant;
				}
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

// Multiplies current map by the parameter map pixelwise 
Map.prototype.multiply = function(map){
	for (var i = 0; i < this.length; i++){
		this.map[i] *= map.map[i];
	}
};

Map.prototype.dumpIm = function(name, threshold){
	name = name ? name : "default.png";
	threshold = threshold ? threshold : 256;
	Engine.DumpImage(name, this.map, this.width, this.height, threshold);
};
