/*
 * TerrainAnalysis, inheriting from the Map Component.
 * 
 * This creates a suitable passability map for pathfinding units and provides the findClosestPassablePoint() function.
 * This is part of the Shared Script, and thus should only be used for things that are non-player specific.
 * This.map is a map of the world, where particular stuffs are pointed with a value
 * For example, impassable land is 0, water is 200, areas near tree (ie forest grounds) are 41…
 * This is intended for use with 8 bit maps for reduced memory usage.
 * Upgraded from QuantumState's original TerrainAnalysis for qBot.
 * You may notice a lot of the A* star codes differ only by a few things.
 * It's wanted: each does a very slightly different things
 * But truly separating optimizes.
 */

function TerrainAnalysis(sharedScript,rawState){
	var self = this;
	this.cellSize = 4;

	var passabilityMap = rawState.passabilityMap;

	this.width = passabilityMap.width;
	this.height = passabilityMap.height;
	
	// the first two won't change, the third is a reference to a value updated by C++
	this.obstructionMaskLand = rawState.passabilityClasses["default"];
	this.obstructionMaskWater = rawState.passabilityClasses["ship"];
	this.obstructionMask = rawState.passabilityClasses["pathfinderObstruction"];

	var obstructionTiles = new Uint8Array(passabilityMap.data.length);
	
	/* Generated map legend:
	 0 is impassable
	 200 is deep water (ie non-passable by land units)
	 201 is shallow water (passable by land units and water units)
	 255 is land (or extremely shallow water where ships can't go).
	 40 is "tree".
	 The following 41-49 range is "near a tree", with the second number showing how many trees this tile neighbors.
	 30 is "geological component", such as a mine
	*/
	
	for (var i = 0; i < passabilityMap.data.length; ++i)
	{
		// If impassable for land units, set to 0, else to 255.
		obstructionTiles[i] = (passabilityMap.data[i] & this.obstructionMaskLand) ? 0 : 255;
		
		if (!(passabilityMap.data[i] & this.obstructionMaskWater) && obstructionTiles[i] === 0)
			obstructionTiles[i] = 200; // if navigable and not walkable (ie basic water), set to 200.
		else if (!(passabilityMap.data[i] & this.obstructionMaskWater) && obstructionTiles[i] === 255)
			obstructionTiles[i] = 201; // navigable and walkable.
	}

	var square = [ [-1,-1], [-1,0], [-1, 1], [0,1], [1,1], [1,0], [1,-1], [0,-1], [0,0] ];
	var xx = 0;
	var yy = 0;
	var value = 0;
	var pos = [];
	var x = 0;
	var y = 0;
	var radius = 0;
	for (var entI in sharedScript._entities)
	{
		var ent = sharedScript._entities[entI];
		if (ent.hasClass("ForestPlant") === true) {
			pos = this.gamePosToMapPos(ent.position());
			x = pos[0];
			y = pos[1];
			// unless it's impassable already, mark it as 40.
			if (obstructionTiles[x + y*this.width] !== 0)
				obstructionTiles[x + y*this.width] = 40;
			for (i in square)
			{
				xx = square[i][0];
				yy = square[i][1];
				if (x+i[0] >= 0 && x+xx < this.width && y+yy >= 0 && y+yy < this.height) {
					value = obstructionTiles[(x+xx) + (y+yy)*this.width];
					if (value === 255)
						obstructionTiles[(x+xx) + (y+yy)*this.width] = 41;
					else if (value < 49 && value > 40)
						obstructionTiles[(x+xx) + (y+yy)*this.width] = value + 1;
				}
			}
		} else if (ent.hasClass("Geology") === true) {
			radius = Math.floor(ent.obstructionRadius() / 4);
			pos = this.gamePosToMapPos(ent.position());
			x = pos[0];
			y = pos[1];
			// Unless it's impassable, mark as 30. This takes precedence over trees.
			obstructionTiles[x + y*this.width] = obstructionTiles[x + y*this.width] === 0 ? 0 : 30;
			for (var xx = -radius; xx <= radius;xx++)
				for (var yy = -radius; yy <= radius;yy++)
					if (x+xx >= 0 && x+xx < this.width && y+yy >= 0 && y+yy < this.height)
						obstructionTiles[(x+xx) + (y+yy)*this.width] = obstructionTiles[(x+xx) + (y+yy)*this.width] === 0 ? 0 : 30;
		}
	}
	// Okay now we have a pretty good knowledge of the map.
	this.Map(rawState, obstructionTiles);

	this.obstructionMaskLand = null;
	this.obstructionMaskWater = null;
	this.obstructionMask = null;
};

copyPrototype(TerrainAnalysis, Map);

// Returns the (approximately) closest point which is passable by searching in a spiral pattern 
TerrainAnalysis.prototype.findClosestPassablePoint = function(startPoint, onLand, limitDistance, quickscope){
	var w = this.width;
	var p = startPoint;
	var direction = 1;
	
	if (p[0] + w*p[1] < 0 || p[0] + w*p[1] >= this.length) {
		return undefined;
	}
	// quickscope
	if (this.map[p[0] + w*p[1]] === 255) {
		if (this.countConnected(p[0] + w*p[1], onLand) >= 2) {
			return p;
		}
	}
	
	var count = 0;
	// search in a spiral pattern. We require a value that is actually accessible in this case, ie 255, 201 or 41 if land, 200/201 if water.
	for (var i = 1; i < w; i++){
		for (var j = 0; j < 2; j++){
			for (var k = 0; k < i; k++){
				p[j] += direction;
				// if the value is not markedly inaccessible
				var index = p[0] + w*p[1];
				if (this.map[index] !== 0 && this.map[index] !== 90 && this.map[index] !== 120 && this.map[index] !== 30 && this.map[index] !== 40){
					if (quickscope || this.countConnected(index, onLand) >= 2){
						return p;
					}
				}
				if (limitDistance !== undefined && count > limitDistance){
					return undefined;
				}
				count++;
			}
		}
		direction *= -1;
	}
	
	return undefined;
};

// Returns an estimate of a tile accessibility. It checks neighboring cells over two levels.
// returns a count. It's not integer. About 2 should be fairly accessible already.
TerrainAnalysis.prototype.countConnected = function(startIndex, byLand){
	var count = 0.0;
	
	var w = this.width;
	var positions = [[0,1], [0,-1], [1,0], [-1,0], [1,1], [-1,-1], [1,-1], [-1,1],
					 [0,2], [0,-2], [2,0], [-2,0], [2,2], [-2,-2], [2,-2], [-2,2]/*,
					 [1,2], [1,-2], [2,1], [-2,1], [-1,2], [-1,-2], [2,-1], [-2,-1]*/];
	
	for (i in positions) {
		var index = startIndex + positions[i][0] + positions[i][1]*w;
		if (this.map[index] !== 0) {
			if (byLand) {
				if (this.map[index] === 201) count++;
				else if (this.map[index] === 255) count++;
				else if (this.map[index] === 41) count++;
				else if (this.map[index] === 42) count += 0.5;
				else if (this.map[index] === 43) count += 0.3;
				else if (this.map[index] === 44) count += 0.13;
				else if (this.map[index] === 45) count += 0.08;
				else if (this.map[index] === 46) count += 0.05;
				else if (this.map[index] === 47) count += 0.03;
			} else {
				if (this.map[index] === 201) count++;
				if (this.map[index] === 200) count++;
			}
		}
	}
	return count;
};

// TODO: for now this resets to 255.
TerrainAnalysis.prototype.updateMapWithEvents = function(sharedAI) {
	var self = this;
	
	var events = sharedAI.events;
	var passabilityMap = sharedAI.passabilityMap;
	
	// looking for creation or destruction of entities, and updates the map accordingly.
	for (var i in events) {
		var e = events[i];
		if (e.type === "Destroy") {
			if (e.msg.entityObj){
				var ent = e.msg.entityObj;
				if (ent.hasClass("Geology")) {
					var x = self.gamePosToMapPos(ent.position())[0];
					var y = self.gamePosToMapPos(ent.position())[1];
					// remove it. Don't really care about surrounding and possible overlappings.
					var radius = Math.floor(ent.obstructionRadius() / self.cellSize);
					for (var xx = -radius; xx <= radius;xx++)
						for (var yy = -radius; yy <= radius;yy++)
						{
							if (x+xx >= 0 && x+xx < self.width && y+yy >= 0 && y+yy < self.height && this.map[(x+xx) + (y+yy)*self.width] === 30)
							{
								this.map[(x+xx) + (y+yy)*self.width] = 255;
							}
						}
				} else if (ent.hasClass("ForestPlant")){
					var x = self.gamePosToMapPos(ent.position())[0];
					var y = self.gamePosToMapPos(ent.position())[1];
					var nbOfNeigh = 0;
					for (var xx = -1; xx <= 1;xx++)
						for (var yy = -1; yy <= 1;yy++)
						{
							if (xx == 0 && yy == 0)
								continue;
							if (this.map[(x+xx) + (y+yy)*self.width] === 40)
								nbOfNeigh++;
							else if (this.map[(x+xx) + (y+yy)*self.width] === 41)
							{
								this.map[(x+xx) + (y+yy)*self.width] = 255;
							}
							else if (this.map[(x+xx) + (y+yy)*self.width] > 41 && this.map[(x+xx) + (y+yy)*self.width] < 50)
								this.map[(x+xx) + (y+yy)*self.width] = this.map[(x+xx) + (y+yy)*self.width] - 1;
						}
					if (nbOfNeigh > 0)
						this.map[x + y*self.width] = this.map[x + y*self.width] = 40 + nbOfNeigh;
					else
						this.map[x + y*self.width] = this.map[x + y*self.width] = 255;
				}
			}
		}
	}
}

/*
 * Accessibility inherits from TerrainAnalysis
 *  
 * This can easily and efficiently determine if any two points are connected.
 * it can also determine if any point is "probably" reachable, assuming the unit can get close enough
 * for optimizations it's called after the TerrainAnalyser has finished initializing his map
 * so this can use the land regions already.
 */
function Accessibility(rawState, terrainAnalyser){
	var self = this;
	
	this.Map(rawState, terrainAnalyser.map);
	this.passMap = new Uint8Array(terrainAnalyser.length);

	this.regionSize = [];
	this.regionSize.push(0);
	// initialized to 0, so start to 1 for optimization
	this.regionID = 1;
	for (var i = 0; i < this.passMap.length; ++i) {
		if (this.passMap[i] === 0 && this.map[i] !== 0) {	// any non-painted, non-inacessible area.
			this.regionSize.push(0);	// updated
			this.floodFill(i,this.regionID,false);
			this.regionID++;
		} else if (this.passMap[i] === 0) {	// any non-painted, inacessible area.
			this.floodFill(i,1,false);
		}
	}
}
copyPrototype(Accessibility, TerrainAnalysis);

Accessibility.prototype.getAccessValue = function(position){
	var gamePos = this.gamePosToMapPos(position);
	return this.passMap[gamePos[0] + this.width*gamePos[1]];
};

// Returns true if a point is deemed currently accessible (is not blocked by surrounding trees...)
// NB: accessible means that you can reach it from one side, not necessariliy that you can go ON it.
Accessibility.prototype.isAccessible = function(gameState, position, onLand){
	var gamePos = this.gamePosToMapPos(position);
	
	// quick check
	if (this.countConnected(gamePos[0] + this.width*gamePos[1], onLand) >= 2) {
		return true;
	}
	return false;
};

// Return true if you can go from a point to a point without switching means of transport
// Hardcore means is also checks for isAccessible at the end (it checks for either water or land though, beware).
// This is a blind check and not a pathfinder: for all it knows there is a huge block of trees in the middle.
Accessibility.prototype.pathAvailable = function(gameState, start,end, hardcore){
	var pstart = this.gamePosToMapPos(start);
	var istart = pstart[0] + pstart[1]*this.width;
	var pend = this.gamePosToMapPos(end);
	var iend = pend[0] + pend[1]*this.width;

	if (this.passMap[istart] === this.passMap[iend]) {
		if (hardcore && (this.isAccessible(gameState, end,true) || this.isAccessible(gameState, end,false)))
			return true;
		else if (hardcore)
			return false;
		return true;
	}
	return false;
};
Accessibility.prototype.getRegionSize = function(position){
	var pos = this.gamePosToMapPos(position);
	var index = pos[0] + pos[1]*this.width;
	if (this.regionSize[this.passMap[index]] === undefined)
		return 0;
	return this.regionSize[this.passMap[index]];
};
Accessibility.prototype.getRegionSizei = function(index) {
	if (this.regionSize[this.passMap[index]] === undefined)
		return 0;
	return this.regionSize[this.passMap[index]];
};

// Implementation of a fast flood fill. Reasonably good performances. Runs once at startup.
// TODO: take big zones of impassable trees into account?
Accessibility.prototype.floodFill = function(startIndex, value, onWater)
{
	this.s = startIndex;
	if (this.passMap[this.s] !== 0) {
		return false;	// already painted.
	}
	
	this.floodFor = "land";
	if (this.map[this.s] === 200 || (this.map[this.s] === 201 && onWater === true))
		this.floodFor = "water";
	else if (this.map[this.s] === 0)
		this.floodFor = "impassable";

	var w = this.width;
	var h = this.height;
		
	var x = 0;
	var y = 0;
	// Get x and y from index
	var IndexArray = [this.s];
	var newIndex = 0;
	while(IndexArray.length){
		
		newIndex = IndexArray.pop();

		y = 0;
		var loop = false;
		// vertical iteration
		do {
			--y;
			loop = false;
			var index = +newIndex + w*y;
			if (index < 0)
				break;
			if (this.floodFor === "impassable" && this.map[index] === 0 && this.passMap[index] === 0) {
				loop = true;
			} else if (this.floodFor === "land" && this.passMap[index] === 0 && this.map[index] !== 0 && this.map[index] !== 200) {
				loop = true;
			} else if (this.floodFor === "water" && this.passMap[index] === 0 && (this.map[index] === 200 || (this.map[index] === 201 && this.onWater)) ) {
				loop = true;
			} else {
				break;
			}
		} while (loop === true)	// should actually break
		++y;
		var reachLeft = false;
		var reachRight = false;
		loop = true;
		do {
			var index = +newIndex + w*y;
			if (this.floodFor === "impassable" && this.map[index] === 0 && this.passMap[index] === 0) {
				this.passMap[index] = value;
				this.regionSize[value]++;
			} else if (this.floodFor === "land" && this.passMap[index] === 0 && this.map[index] !== 0 && this.map[index] !== 200) {
				this.passMap[index] = value;
				this.regionSize[value]++;
			} else if (this.floodFor === "water" && this.passMap[index] === 0 && (this.map[index] === 200 || (this.map[index] === 201 && this.onWater)) ) {
				this.passMap[index] = value;
				this.regionSize[value]++;
			} else {
				break;
			}
			
			if (index%w > 0)
			{
				if (this.floodFor === "impassable" && this.map[index -1] === 0 && this.passMap[index -1] === 0) {
					if(!reachLeft) {
						IndexArray.push(index -1);
						reachLeft = true;
					}
				} else if (this.floodFor === "land" && this.passMap[index -1] === 0 && this.map[index -1] !== 0 && this.map[index -1] !== 200) {
					if(!reachLeft) {
						IndexArray.push(index -1);
						reachLeft = true;
					}
				} else if (this.floodFor === "water" && this.passMap[index -1] === 0 && (this.map[index -1] === 200 || (this.map[index -1] === 201 && this.onWater)) ) {
					if(!reachLeft) {
						IndexArray.push(index -1);
						reachLeft = true;
					}
				} else if(reachLeft) {
					reachLeft = false;
				}
			}
			if (index%w < w - 1)
			{
				if (this.floodFor === "impassable" && this.map[index +1] === 0 && this.passMap[index +1] === 0) {
					if(!reachRight) {
						IndexArray.push(index +1);
						reachRight = true;
					}
				} else if (this.floodFor === "land" && this.passMap[index +1] === 0 && this.map[index +1] !== 0 && this.map[index +1] !== 200) {
					if(!reachRight) {
						IndexArray.push(index +1);
						reachRight = true;
					}
				} else if (this.floodFor === "water" && this.passMap[index +1] === 0 && (this.map[index +1] === 200 || (this.map[index +1] === 201 && this.onWater)) ) {
					if(!reachRight) {
						IndexArray.push(index +1);
						reachRight = true;
					}
				} else if(reachRight) {
					reachRight = false;
				}
			}
			++y;
		} while (index/w < w)	// should actually break
	}
	return true;
}

function landSizeCounter(rawState,terrainAnalyzer) {
	var self = this;
	
	this.passMap = terrainAnalyzer.map;

	var map = new Uint8Array(this.passMap.length);
	this.Map(rawState,map);

	
	for (var i = 0; i < this.passMap.length; ++i) {
		if (this.passMap[i] !== 0)
			this.map[i] = 255;
		else
			this.map[i] = 0;
	}
	
	this.expandInfluences();
}
copyPrototype(landSizeCounter, TerrainAnalysis);

// Implementation of A* as a flood fill. Possibility of (clever) oversampling
// for efficiency or for disregarding too small passages.
// can operate over several turns, though default is only one turn.
landSizeCounter.prototype.getAccessibleLandSize = function(position, sampling, mode, OnlyBuildable, sizeLimit, iterationLimit)
{
	if (sampling === undefined)
		this.Sampling = 1;
	else
		this.Sampling = sampling < 1 ? 1 : sampling;
	
	// this checks from the actual starting point. If that is inaccessible (0), it returns undefined;
	if (position.length !== undefined) {
		// this is an array
		if (position[0] < 0 || this.gamePosToMapPos(position)[0] >= this.width || position[1] < 0 || this.gamePosToMapPos(position)[1] >= this.height)
			return undefined;
		
		var s = this.gamePosToMapPos(position);
		this.s = s[0] + w*s[1];
		if (this.map[this.s] === 0 || this.map[this.s] === 200 || (OnlyBuildable === true && this.map[this.s] === 201) ) {
			return undefined;
		}
	} else {
		this.s = position;
		if (this.map[this.s] === 0 || this.map[this.s] === 200 || (OnlyBuildable === true && this.map[this.s] === 201) ) {
			return undefined;
		}
	}
		
	if (mode === undefined)
		this.mode = "default";
	else
		this.mode = mode;

	if (sizeLimit === undefined)
		this.sizeLimit = 300000;
	else
		this.sizeLimit = sizeLimit;

	var w = this.width;
	var h = this.height;
	
	// max map size is 512*512, this is higher.
	this.iterationLimit = 300000;
	if (iterationLimit !== undefined)
		this.iterationLimit = iterationLimit;
	
	this.openList = [];
	this.isOpened = new Boolean(this.map.length);
	this.gCostArray = new Uint16Array(this.map.length);

	this.currentSquare = this.s;
	this.isOpened[this.s] = true;
	this.openList.push(this.s);
	this.gCostArray[this.s] = 0;
	
	this.countedValue = 1;
	this.countedArray = [this.s];
	
	if (OnlyBuildable !== undefined)
		this.onlyBuildable = OnlyBuildable;
	else
		this.onlyBuildable = true;
	
	return this.continueLandSizeCalculation();
}
landSizeCounter.prototype.continueLandSizeCalculation = function()
{
	var w = this.width;
	var h = this.height;
	var positions = [[0,1], [0,-1], [1,0], [-1,0], [1,1], [-1,-1], [1,-1], [-1,1]];
	var cost = [10,10,10,10,15,15,15,15];
	
	//creation of variables used in the loop
	var nouveau = false;
	var shortcut = false;
	var Sampling = this.Sampling;
	var infinity = Math.min();
	var currentDist = infinity;
	
	var iteration = 0;
	while (this.openList.length !== 0 && iteration < this.iterationLimit && this.countedValue < this.sizeLimit && this.countedArray.length < this.sizeLimit){
		currentDist = infinity;
		for (i in this.openList)
		{
			var sum = this.gCostArray[this.openList[i]];
			if (sum < currentDist)
			{
				this.currentSquare = this.openList[i];
				currentDist = sum;
			}
		}
		this.openList.splice(this.openList.indexOf(this.currentSquare),1);

		shortcut = false;
		this.isOpened[this.currentSquare] = false;
		for (i in positions) {
			var index = 0 + this.currentSquare + positions[i][0]*Sampling + w*Sampling*positions[i][1];
			if (this.passMap[index] !== 0 && this.passMap[index] !== 200 && this.map[index] >= Sampling && (!this.onlyBuildable || this.passMap[index] !== 201)) {
				if(this.isOpened[index] === undefined) {
					if (this.mode === "default")
						this.countedValue++;
					else if (this.mode === "array")
						this.countedArray.push(index);
					this.gCostArray[index] = this.gCostArray[this.currentSquare] + cost[i] * Sampling;
					this.openList.push(index);
					this.isOpened[index] = true;
				}
			}
		}
		iteration++;
	}
	
	if (iteration === this.iterationLimit && this.openList.length !== 0 && this.countedValue !== this.sizeLimit && this.countedArray.length !== this.sizeLimit)
	{
		// we've got to assume that we stopped because we reached the upper limit of iterations
		return "toBeContinued";
	}
		
	delete this.parentSquare;
	delete this.isOpened;
	delete this.fCostArray;
	delete this.gCostArray;
	
	if (this.mode === "default")
		return this.countedValue;
	else if (this.mode === "array")
		return this.countedArray;
	return undefined;
}
