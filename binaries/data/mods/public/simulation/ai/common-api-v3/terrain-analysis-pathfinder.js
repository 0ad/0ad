// An implementation of A* as a pathfinder.
// It's oversamplable, and has a specific "distance from block"
// variable to avoid narrow passages if wanted.
// It can carry a calculation over multiple turns.
// It can work over water, or land.
// Note: while theoretically possible, when this goes from land to water
// It will return the path to the "boarding" point and
// a new path will need to be created.

// this should only be called by an AI player after setting gamestate.ai

// The initializer creates an expanded influence map for checking.
// It's not extraordinarily slow, but it might be.
function aStarPath(gameState, onWater, disregardEntities, targetTerritory) {
	var self = this;
	
	// get the terrain analyzer map as a reference.
	this.Map(gameState.ai, gameState.ai.terrainAnalyzer.map);
	// get the accessibility as a reference
	this.accessibility = gameState.ai.accessibility;
	this.terrainAnalyzer = gameState.ai.terrainAnalyzer;
	
	if (onWater) {
		this.waterPathfinder = true;
	} else
		this.waterPathfinder = false;
	
	var pathObstruction = gameState.sharedScript.passabilityClasses["pathfinderObstruction"];
	var passMap = gameState.sharedScript.passabilityMap;
	var terrMap = gameState.sharedScript.territoryMap;


	this.widthMap = new Uint8Array(this.map.length);
	for (var i = 0; i < this.map.length; ++i) {
		if (this.map[i] === 0)
			this.widthMap[i] = 0;
		else if (!disregardEntities &&  (((terrMap.data[i] & 0x3F) !== gameState.ai.player && (terrMap.data[i] & 0x3F) !== 0 && (terrMap.data[i] & 0x3F) !== targetTerritory)
										 || (passMap.data[i] & pathObstruction)))
			this.widthMap[i] = 1;	// we try to avoid enemy territory and pathfinder obstructions.
		else if (!disregardEntities && this.map[i] === 30)
			this.widthMap[i] = 0;
		else if (!disregardEntities && this.map[i] === 40)
			this.widthMap[i] = 0;
		else if (!disregardEntities && this.map[i] === 41)
			this.widthMap[i] = 2;
		else if (!disregardEntities && this.map[i] === 42)
			this.widthMap[i] = 1;
		else if (!onWater && this.map[i] === 201)
			this.widthMap[i] = 1;
		else
			this.widthMap[i] = 255;
	}
	this.expandInfluences(255,this.widthMap);
}
copyPrototype(aStarPath, Map);

// marks some points of the map as impassable. This can be used to create different paths, or to avoid going through some areas.
aStarPath.prototype.markImpassableArea = function(cx, cy, Distance) {
	[cx,cy] = this.gamePosToMapPos([cx,cy]);
	var x0 = Math.max(0, cx - Distance);
	var y0 = Math.max(0, cy - Distance);
	var x1 = Math.min(this.width, cx + Distance);
	var y1 = Math.min(this.height, cy + Distance);
	var maxDist2 = Distance * Distance;
	
	for ( var y = y0; y < y1; ++y) {
		for ( var x = x0; x < x1; ++x) {
			var dx = x - cx;
			var dy = y - cy;
			var r2 = dx*dx + dy*dy;
			if (r2 < maxDist2){
				this.widthMap[x + y * this.width] = 0;
			}
		}
	}
};


// sending gamestate creates a map
// (you run the risk of "jumping" over obstacles or weird behavior.
aStarPath.prototype.getPath = function(start, end, Sampling, preferredWidth, iterationLimit, gamestate)
{
	this.Sampling = Sampling >= 1 ? Sampling : 1;
	this.minWidth = 1;
	this.preferredWidth = (preferredWidth !== undefined && preferredWidth >= this.Sampling) ? preferredWidth : this.Sampling;
	
	if (start[0] < 0 || this.gamePosToMapPos(start)[0] >= this.width || start[1] < 0 || this.gamePosToMapPos(start)[1] >= this.height)
		return undefined;
	
	var s = this.terrainAnalyzer.findClosestPassablePoint(this.gamePosToMapPos(start), !this.waterPathfinder,500,false);
	var e = this.terrainAnalyzer.findClosestPassablePoint(this.gamePosToMapPos(end), !this.waterPathfinder,500,true);
	
	var w = this.width;
	
	if (!s || !e) {
		return undefined;
	}
	if (gamestate !== undefined)
	{
		this.TotorMap = new Map(gamestate);
		this.TotorMap.addInfluence(s[0],s[1],1,200,'constant');
		this.TotorMap.addInfluence(e[0],e[1],1,200,'constant');
	}
	this.iterationLimit = 9000000000;
	if (iterationLimit !== undefined)
		this.iterationLimit = iterationLimit;
	
	this.s = s[0] + w*s[1];
	this.e = e[0] + w*e[1];
	
	if (this.waterPathfinder && this.map[this.s] !== 200 && this.map[this.s] !== 201)
	{
		debug ("Trying a path over water, but we are on land, aborting");
		return undefined;
	} else if (!this.waterPathfinder && this.map[this.s] === 200)
	{
		debug ("Trying a path over land, but we are over water, aborting");
		return undefined;
	}
	
	this.onWater = this.waterPathfinder;
	this.pathChangesTransport = false;
	
	// We are going to create a map, it's going to take memory. To avoid OOM errors, GC before we do so.
	//Engine.ForceGC();

	this.openList = [];
	this.parentSquare = new Uint32Array(this.map.length);
	this.isOpened = new Boolean(this.map.length);
	this.fCostArray = new Uint32Array(this.map.length);
	this.gCostArray = new Uint32Array(this.map.length);
	this.currentSquare = this.s;
	
	this.totalIteration = 0;
	
	this.isOpened[this.s] = true;
	this.openList.push(this.s);
	this.fCostArray[this.s] = SquareVectorDistance([this.s%w, Math.floor(this.s/w)], [this.e%w, Math.floor(this.e/w)]);
	this.gCostArray[this.s] = 0;
	this.parentSquare[this.s] = this.s;
	
	return this.continuePath(gamestate);
	
}
// in case it's not over yet, this can carry on the calculation of a path over multiple turn until it's over
aStarPath.prototype.continuePath = function(gamestate)
{
	var w = this.width;
	var h = this.height;
	var positions = [[0,1], [0,-1], [1,0], [-1,0], [1,1], [-1,-1], [1,-1], [-1,1]];
	var cost = [100,100,100,100,150,150,150,150];
	
	//creation of variables used in the loop
	var found = false;
	var shortcut = false;
	
	var infinity = Math.min();
	var currentDist = infinity;
	
	var e = this.e;
	var s = this.s;
	
	var iteration = 0;
	
	var target = [this.e%w, Math.floor(this.e/w)];
	
	var changes = {};
	
	var tIndex = 0;
	// on to A*
	while (found === false && this.openList.length !== 0 && iteration < this.iterationLimit) {
		currentDist = infinity;
		
		if (shortcut === true) {
			this.currentSquare = this.openList.shift();
		} else {
			for (var i in this.openList)
			{
				var sum = this.fCostArray[this.openList[i]] + this.gCostArray[this.openList[i]];
				if (sum < currentDist)
				{
					this.currentSquare = this.openList[i];
					tIndex = i;
					currentDist = sum;
				}
			}
			this.openList.splice(tIndex,1);
		}
		if (!this.onWater && this.map[this.currentSquare] === 200) {
			this.onWater = true;
		} else if (this.onWater && (this.map[this.currentSquare] !== 200 && this.map[this.currentSquare] !== 201)) {
			this.onWater = false;
		}
		
		shortcut = false;
		this.isOpened[this.currentSquare] = false;
		
		if (gamestate !== undefined)
			this.TotorMap.addInfluence(this.currentSquare % w, Math.floor(this.currentSquare / w),1,40,'constant');
		
		for (var i in positions)
		{
			var index = 0 + this.currentSquare +positions[i][0]*this.Sampling +w*this.Sampling*positions[i][1];
			if (this.widthMap[index] >= this.minWidth || (this.onWater && this.map[index] > 0 && this.map[index] !== 200 && this.map[index] !== 201)
				|| (!this.onWater && this.map[this.index] === 200))
			{
				if(this.isOpened[index] === undefined)
				{
					this.parentSquare[index] = this.currentSquare;
					
					this.fCostArray[index] = SquareVectorDistance([index%w, Math.floor(index/w)], target);// * cost[i];
					this.gCostArray[index] = this.gCostArray[this.currentSquare] + cost[i] * this.Sampling;// - this.map[index];
					
					if (!this.onWater && this.map[index] === 200) {
						this.gCostArray[index] += 10000;
					} else if (this.onWater && this.map[index] !== 200) {
						this.gCostArray[index] += 10000;
					}
					
					if (this.widthMap[index] < this.preferredWidth)
						this.gCostArray[index] += 200 * (this.preferredWidth-this.widthMap[index]);
					
					if (this.map[index] === 200 || (this.map[index] === 201 && this.onWater))
						this.gCostArray[index] += 1000;
					
					if (this.openList[0] !== undefined && this.fCostArray[this.openList[0]] + this.gCostArray[this.openList[0]] > this.fCostArray[index] + this.gCostArray[index])
					{
						this.openList.unshift(index);
						shortcut = true;
					} else {
						this.openList.push(index);
					}
					this.isOpened[index] = true;
					if (SquareVectorDistance( [index%w, Math.floor(index/w)] , target) <= this.Sampling*this.Sampling-1) {
						if (this.e != index)
							this.parentSquare[this.e] = index;
						found = true;
						break;
					}
				} else {
					var addCost = 0;
					if (!this.onWater && this.map[index] === 200) {
						addCost += 10000;
					} else if (this.onWater && this.map[index] !== 200) {
						addCost += 10000;
					}
					if (this.widthMap[index] < this.preferredWidth)
						addCost += 200 * (this.preferredWidth-this.widthMap[index]);
					
					if (this.map[index] === 200 || (this.map[index] === 201 && this.onWater))
						addCost += 1000;

					// already on the Open or closed list
					if (this.gCostArray[index] > cost[i] * this.Sampling + addCost + this.gCostArray[this.currentSquare])
					{
						this.parentSquare[index] = this.currentSquare;
						this.gCostArray[index] = cost[i] * this.Sampling + addCost  + this.gCostArray[this.currentSquare];
					}
				}
			}
		}
		iteration++;
	}
	this.totalIteration += iteration;
	if (iteration === this.iterationLimit && found === false && this.openList.length !== 0)
	{
		// we've got to assume that we stopped because we reached the upper limit of iterations
		return "toBeContinued";
	}
	//debug (this.totalIteration);
	var paths = [];
	if (found) {
		this.currentSquare = e;
		var lastPosx = 0;
		var lastPosy = 0;
		while (this.parentSquare[this.currentSquare] !== s)
		{
			this.currentSquare = this.parentSquare[this.currentSquare];
			
			if (!this.onWater && this.map[this.currentSquare] === 200) {
				//debug ("We must cross water, going " +this.currentSquare + " from parent " + this.parentSquare[this.currentSquare]);
				this.pathChangesTransport = true;
				changes[this.currentSquare] = true;
				this.onWater = true;
			} else if (this.onWater && (this.map[this.currentSquare] !== 200 && this.map[this.currentSquare] !== 201)) {
				//debug ("We must cross to the ground, going " +this.currentSquare + " from parent " + this.parentSquare[this.currentSquare]);
				this.pathChangesTransport = true;
				changes[this.currentSquare] = true;
				this.onWater = false;
			}

			if (gamestate !== undefined && changes[this.currentSquare])
				this.TotorMap.addInfluence(this.currentSquare % w, Math.floor(this.currentSquare / w),2,200,'constant');
			if (gamestate !== undefined)
				this.TotorMap.addInfluence(this.currentSquare % w, Math.floor(this.currentSquare / w),1,50,'constant');
			
			if (SquareVectorDistance([lastPosx,lastPosy],[this.currentSquare % w, Math.floor(this.currentSquare / w)]) > 300 || changes[this.currentSquare])
			{
				lastPosx = (this.currentSquare % w);
				lastPosy = Math.floor(this.currentSquare / w);
				paths.push([ [lastPosx*this.cellSize,lastPosy*this.cellSize], changes[this.currentSquare] ]);
				if (gamestate !== undefined)
					this.TotorMap.addInfluence(this.currentSquare % w, Math.floor(this.currentSquare / w),1,50 + paths.length,'constant');
			}
		}
	} else {
		// we have not found a path.
		// what do we do then?
	}

	if (gamestate !== undefined)
		this.TotorMap.dumpIm("Path From " +s +" to " +e +".png",255);
	
	delete this.parentSquare;
	delete this.isOpened;
	delete this.fCostArray;
	delete this.gCostArray;
	
	
	// the return, if defined is [ [path, each waypoint being [position, mustchangeTransport] ], is there any transport change, ]
	if (paths.length > 0) {
		return [paths, this.pathChangesTransport];
	} else {
		return undefined;
	}
	
}
