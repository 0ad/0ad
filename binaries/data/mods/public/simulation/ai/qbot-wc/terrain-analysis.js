/*
 * TerrainAnalysis inherits from Map
 * 
 * This creates a suitable passability map for pathfinding units and provides the findClosestPassablePoint() function.
 * This is intended to be a base object for the terrain analysis modules to inherit from. 
 */ 

function TerrainAnalysis(gameState){
	var passabilityMap = gameState.getMap();

	var obstructionMask = gameState.getPassabilityClassMask("pathfinderObstruction");
	obstructionMask |= gameState.getPassabilityClassMask("default");
	
	var obstructionTiles = new Uint16Array(passabilityMap.data.length); 
	for (var i = 0; i < passabilityMap.data.length; ++i) 
	{ 
		obstructionTiles[i] = (passabilityMap.data[i] & obstructionMask) ? 0 : 65535; 
	}
	
	this.Map(gameState, obstructionTiles);
};

copyPrototype(TerrainAnalysis, Map);

// Returns the (approximately) closest point which is passable by searching in a spiral pattern 
TerrainAnalysis.prototype.findClosestPassablePoint = function(startPoint, quick, limitDistance){
	var w = this.width;
	var p = startPoint;
	var direction = 1;
	
	if (p[0] + w*p[1] > 0 && p[0] + w*p[1] < this.length &&
			this.map[p[0] + w*p[1]] != 0){
		if (this.countConnected(p, 10) >= 10){
			return p;
		}
	}
	
	var count = 0;
	// search in a spiral pattern.
	for (var i = 1; i < w; i++){
		for (var j = 0; j < 2; j++){
			for (var k = 0; k < i; k++){
				p[j] += direction;
				if (p[0] + w*p[1] > 0 && p[0] + w*p[1] < this.length &&
						this.map[p[0] + w*p[1]] != 0){
					if (quick || this.countConnected(p, 10) >= 10){
						return p;
					}
				}
				if (limitDistance && count > 40){
					return undefined;
				}
				count += 1;
			}
		}
		direction *= -1;
	}
	
	return undefined;
};

// Counts how many accessible tiles there are connected to the start Point.  If there are >= maxCount then it stops.
// This is inefficient for large areas so maxCount should be kept small for efficiency.
TerrainAnalysis.prototype.countConnected = function(startPoint, maxCount, curCount, checked){
	curCount = curCount || 0;
	checked = checked || [];
	
	var w = this.width;
	
	var positions = [[0,1], [0,-1], [1,0], [-1,0]];
	
	curCount += 1; // add 1 for the current point
	checked.push(startPoint);
	if (curCount >= maxCount){
		return curCount;
	}
	
	for (var i in positions){
		var p = [startPoint[0] + positions[i][0], startPoint[1] + positions[i][1]];
		if (p[0] + w*p[1] > 0 && p[0] + w*p[1] < this.length &&
				this.map[p[0] + w*p[1]] != 0 && !(p in checked)){
			curCount += this.countConnected(p, maxCount, curCount, checked);
		}
	}
	return curCount;
};

/*
 * PathFinder inherits from TerrainAnalysis
 *  
 * Used to create a list of distinct paths between two points. 
 * 
 * Currently it works with a basic implementation which should be improved.
 * 
 * TODO: Make this use territories.
 */


function PathFinder(gameState){
	this.TerrainAnalysis(gameState);
}

copyPrototype(PathFinder, TerrainAnalysis);

/*
 * Returns a list of distinct paths to the destination.  Currently paths are distinct if they are more than 
 * blockRadius apart at a distance of blockPlacementRadius from the destination.  Where blockRadius and 
 * blockPlacementRadius are defined in walkGradient
 */
PathFinder.prototype.getPaths = function(start, end, mode){
	var s = this.findClosestPassablePoint(this.gamePosToMapPos(start));
	var e = this.findClosestPassablePoint(this.gamePosToMapPos(end));
	
	if (!s || !e){
		return undefined;
	}
	
	var paths = [];
	var i = 0;
	while (true){
		i++;
		//this.dumpIm("terrainanalysis_"+i+".png", 511);
		this.makeGradient(s,e);
		var curPath = this.walkGradient(e, mode);
		
		if (curPath !== undefined){
			paths.push(curPath);
		}else{
			break;
		}
		
		this.wipeGradient();
	}
	
	//this.dumpIm("terrainanalysis.png", 511);
	
	if (paths.length > 0){
		return paths;
	}else{
		return [];
	}
};

// Creates a potential gradient with the start point having the lowest potential
PathFinder.prototype.makeGradient = function(start, end){
	var w = this.width;
	var map = this.map;
	
	// Holds the list of current points to work outwards from
	var stack = [];
	// We store the next level in its own stack
	var newStack = [];
	// Relative positions or new cells from the current one.  We alternate between the adjacent 4 and 8 cells
	// so that there is an average 1.5 distance for diagonals which is close to the actual sqrt(2) ~ 1.41
	var positions = [[[0,1], [0,-1], [1,0], [-1,0]], 
	                 [[0,1], [0,-1], [1,0], [-1,0], [1,1], [-1,-1], [1,-1], [-1,1]]];
	
	//Set the distance of the start point to be 1 to distinguish it from the impassable areas
	map[start[0] + w*(start[1])] = 1;
	stack.push(start);
	
	// while there are new points being added to the stack
	while (stack.length > 0){
		//run through the current stack
		while (stack.length > 0){
			var cur = stack.pop();
			// stop when we reach the end point
			if (cur[0] == end[0] && cur[1] == end[1]){
				return;
			}
			
			var dist = map[cur[0] + w*(cur[1])] + 1;
			// Check the positions adjacent to the current cell
			for (var i = 0; i < positions[dist % 2].length; i++){
				var pos = positions[dist % 2][i];
				var cell = cur[0]+pos[0] + w*(cur[1]+pos[1]);
				if (cell >= 0 && cell < this.length && map[cell] > dist){
					map[cell] = dist;
					newStack.push([cur[0]+pos[0], cur[1]+pos[1]]);
				}
			}
		}
		// Replace the old empty stack with the newly filled one.
		stack = newStack;
		newStack = [];
	}
	
};

// Clears the map to just have the obstructions marked on it.
PathFinder.prototype.wipeGradient = function(){
	for (var i = 0; i < this.length; i++){
		if (this.map[i] > 0){
			this.map[i] = 65535;
		}
	}
};

// Returns the path down a gradient from the start to the bottom of the gradient, returns a point for every 20 cells in normal mode
// in entryPoints mode this returns the point where the path enters the region near the destination, currently defined 
// by blockPlacementRadius.  Note doesn't return a path when the destination is within the blockpoint radius.
PathFinder.prototype.walkGradient = function(start, mode){
	var positions = [[0,1], [0,-1], [1,0], [-1,0], [1,1], [-1,-1], [1,-1], [-1,1]];
	
	var path = [[start[0]*this.cellSize, start[1]*this.cellSize]];
	
	var blockPoint = undefined;
	var blockPlacementRadius = 45;
	var blockRadius = 23;
	var count = 0;
	
	var cur = start;
	var w = this.width;
	var dist = this.map[cur[0] + w*cur[1]];
	var moved = false;
	while (this.map[cur[0] + w*cur[1]] !== 0){
		for (var i = 0; i < positions.length; i++){
			var pos = positions[i];
			var cell = cur[0]+pos[0] + w*(cur[1]+pos[1]);
			if (cell >= 0 && cell < this.length && this.map[cell] > 0 &&  this.map[cell] < dist){
				dist = this.map[cell];
				cur = [cur[0]+pos[0], cur[1]+pos[1]];
				moved = true;
				count++;
				// Mark the point to put an obstruction at before calculating the next path
				if (count === blockPlacementRadius){
					blockPoint = cur;
				}
				// Add waypoints to the path, fairly well spaced apart.
				if (count % 40 === 0){
					path.unshift([cur[0]*this.cellSize, cur[1]*this.cellSize]);
				}
				break;
			}
		}
		if (!moved){
			break;
		}
		moved = false;
	}
	if (blockPoint === undefined){
		return undefined;
	}
	
	// Add an obstruction to the map at the blockpoint so the next path will take a different route.
	this.addInfluence(blockPoint[0], blockPoint[1], blockRadius, -1000000, 'constant');
	if (mode === 'entryPoints'){
		// returns the point where the path enters the blockPlacementRadius
		return [blockPoint[0] * this.cellSize, blockPoint[1] * this.cellSize];
	}else{
		// return a path of points 20 squares apart on the route
		return path;
	}
};

// Would be used to calculate the width of a chokepoint
// NOTE: Doesn't currently work.
PathFinder.prototype.countAttached = function(pos){
	var positions = [[0,1], [0,-1], [1,0], [-1,0]];
	var w = this.width;
	var val = this.map[pos[0] + w*pos[1]];
	
	var stack = [pos];
	var used = {};
	
	while (stack.length > 0){
		var cur = stack.pop();
		used[cur[0] + " " + cur[1]] = true;
		for (var i = 0; i < positions.length; i++){
			var p = positions[i];
			var cell = cur[0]+p[0] + w*(cur[1]+p[1]);
			
		}
	}
};

/*
 * Accessibility inherits from TerrainAnalysis
 *  
 * Determines whether there is a path from one point to another.  It is initialised with a single point (p1) and then 
 * can efficiently determine if another point is reachable from p1.  Initialising the object is costly so it should be 
 * cached.
 */

function Accessibility(gameState, location){
	this.TerrainAnalysis(gameState);
	
	var start = this.findClosestPassablePoint(this.gamePosToMapPos(location));
	
	// Check that the accessible region is a decent size, otherwise obstacles close to the start point can create
	// tiny accessible areas which makes the rest of the map inaceesible.
	var iterations = 0;
	while (this.floodFill(start) < 20 && iterations < 30){
		this.map[start[0] + this.width*(start[1])] = 0;
		start = this.findClosestPassablePoint(this.gamePosToMapPos(location));
		iterations += 1;
	}
	//this.dumpIm("accessibility.png");
}

copyPrototype(Accessibility, TerrainAnalysis);

// Return true if the given point is accessible from the point given when initialising the Accessibility object. #
// If the given point is impassable the closest passable point is used.
Accessibility.prototype.isAccessible = function(position){
	var s = this.findClosestPassablePoint(this.gamePosToMapPos(position), true, true);
	if (!s)
		return false;
	
	return this.map[s[0] + this.width * s[1]] === 1;
};

// fill all of the accessible areas with value 1
Accessibility.prototype.floodFill = function(start){
	var w = this.width;
	var map = this.map;
	
	// Holds the list of current points to work outwards from
	var stack = [];
	// We store new points to be added to the stack temporarily in here while we run through the current stack
	var newStack = [];
	// Relative positions or new cells from the current one.
	var positions = [[0,1], [0,-1], [1,0], [-1,0]];
	
	// Set the start point to be accessible
	map[start[0] + w*(start[1])] = 1;
	stack.push(start);
	
	var count = 0;
	
	// while there are new points being added to the stack
	while (stack.length > 0){
		//run through the current stack
		while (stack.length > 0){
			var cur = stack.pop();
			
			// Check the positions adjacent to the current cell
			for (var i = 0; i < positions.length; i++){
				var pos = positions[i];
				var cell = cur[0]+pos[0] + w*(cur[1]+pos[1]);
				if (cell >= 0 && cell < this.length && map[cell] > 1){
					map[cell] = 1;
					newStack.push([cur[0]+pos[0], cur[1]+pos[1]]);
					count += 1;
				}
			}
		}
		// Replace the old empty stack with the newly filled one.
		stack = newStack;
		newStack = [];
	}
	return count;
};




// Some different take on the idea of Quantumstate... What I'll do is make a list of any terrain obstruction...

function aStarPath(gameState, onWater){
	var self = this;
	
	this.passabilityMap = gameState.getMap();
	
	var obstructionMaskLand = gameState.getPassabilityClassMask("default");
	var obstructionMaskWater = gameState.getPassabilityClassMask("ship");
	
	var obstructionTiles = new Uint16Array(this.passabilityMap.data.length);
	for (var i = 0; i < this.passabilityMap.data.length; ++i)
	{
		if (onWater) {
			obstructionTiles[i] = (this.passabilityMap.data[i] & obstructionMaskWater) ? 0 : 255;
		} else {
			obstructionTiles[i] = (this.passabilityMap.data[i] & obstructionMaskLand) ? 0 : 255;
			// We allow water, but we set it at a different index.
			if (!(this.passabilityMap.data[i] & obstructionMaskWater) && obstructionTiles[i] === 0)
				obstructionTiles[i] = 200;
		}
	}
	if (onWater)
		this.onWater = true;
	else
		this.onWater = false;
	this.pathRequiresWater = this.onWater;
	
	this.cellSize = gameState.cellSize;
	
	this.Map(gameState, obstructionTiles);
	this.passabilityMap = new Map(gameState, obstructionTiles, true);
	
	var type = ["wood","stone", "metal"];
	if (onWater)	// trees can perhaps be put into water, I'd doubt so about the rest.
		type = ["wood"];
	for (o in type) {
		var entities = gameState.getResourceSupplies(type[o]);
		entities.forEach(function (supply) { //}){
			var radius = Math.floor(supply.obstructionRadius() / self.cellSize);
			if (type[o] === "wood") {
				for (var xx = -1; xx <= 1;xx++)
					for (var yy = -1; yy <= 1;yy++)
					{
						var x = self.gamePosToMapPos(supply.position())[0];
						var y = self.gamePosToMapPos(supply.position())[1];
						if (x+xx >= 0 && x+xx < self.width && y+yy >= 0 && y+yy < self.height)
						{
							self.map[x+xx + (y+yy)*self.width] = 0;
							self.passabilityMap.map[x+xx + (y+yy)*self.width] = 100; // tree
						}
					}
				self.map[x + y*self.width] = 0;
				self.passabilityMap.map[x + y*self.width] = 0;
			} else {
				for (var xx = -radius; xx <= radius;xx++)
					for (var yy = -radius; yy <= radius;yy++)
					{
						var x = self.gamePosToMapPos(supply.position())[0];
						var y = self.gamePosToMapPos(supply.position())[1];
						if (x+xx >= 0 && x+xx < self.width && y+yy >= 0 && y+yy < self.height)
						{
							self.map[x+xx + (y+yy)*self.width] = 0;
							self.passabilityMap.map[x+xx + (y+yy)*self.width] = 0;
						}
					}
			}
		});
	}
	//this.dumpIm("Non-Expanded Obstructions.png",255);
	this.expandInfluences();
	//this.dumpIm("Expanded Obstructions.png",10);
	//this.BluringRadius = 10;
	//this.Blur(this.BluringRadius); // first steop of bluring
}
copyPrototype(aStarPath, TerrainAnalysis);

aStarPath.prototype.getPath = function(start,end,optimized, minSampling, iterationLimit , gamestate)
{
	if (minSampling === undefined)
		this.minSampling = 2;
	else this.minSampling = minSampling;
	
	if (start[0] < 0 || this.gamePosToMapPos(start)[0] >= this.width || start[1] < 0 || this.gamePosToMapPos(start)[1] >= this.height)
		return undefined;
	
	var s = this.findClosestPassablePoint(this.gamePosToMapPos(start));
	var e = this.findClosestPassablePoint(this.gamePosToMapPos(end));
	
	if (!s || !e){
		return undefined;
	}
	
	var w = this.width;
	var h = this.height;
	
	this.optimized = optimized;
	if (this.minSampling < 1)
		this.minSampling = 1;
	
	if (gamestate !== undefined)
	{
		this.TotorMap = new Map(gamestate);
		this.TotorMap.addInfluence(s[0],s[1],1,200,'constant');
		this.TotorMap.addInfluence(e[0],e[1],1,200,'constant');
	}
	this.iterationLimit = 65500;
	if (iterationLimit !== undefined)
		this.iterationLimit = iterationLimit;
	
	this.s = s[0] + w*s[1];
	this.e = e[0] + w*e[1];
	
	// I was using incredibly slow associative arrays before…
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
	//debug ("Initialized okay");
	return this.continuePath(gamestate);
	
}
// in case it's not over yet, this can carry on the calculation of a path over multiple turn until it's over
aStarPath.prototype.continuePath = function(gamestate)
{
	var w = this.width;
	var h = this.height;
	var positions = [[0,1], [0,-1], [1,0], [-1,0], [1,1], [-1,-1], [1,-1], [-1,1]];
	var cost = [100,100,100,100,150,150,150,150];
	var invCost = [1,1,1,1,0.8,0.8,0.8,0.8];
	//creation of variables used in the loop
	var found = false;
	var nouveau = false;
	var shortcut = false;
	var Sampling = this.minSampling;
	var closeToEnd = false;
	var infinity = Math.min();
	var currentDist = infinity;
	var e = this.e;
	var s = this.s;

	var iteration = 0;
	// on to A*
	while (found === false && this.openList.length !== 0 && iteration < this.iterationLimit){
		currentDist = infinity;
		
		if (shortcut === true) {
			this.currentSquare = this.openList.shift();
		} else {
			for (i in this.openList)
			{
				var sum = this.fCostArray[this.openList[i]] + this.gCostArray[this.openList[i]];
				if (sum < currentDist)
				{
					this.currentSquare = this.openList[i];
					currentDist = sum;
				}
			}
			this.openList.splice(this.openList.indexOf(this.currentSquare),1);
		}
		if (!this.onWater && this.passabilityMap.map[this.currentSquare] === 200) {
			this.onWater = true;
			this.pathRequiresWater = true;
		} else if (this.onWater && this.passabilityMap.map[this.currentSquare] !== 200)
			this.onWater = false;
		
		shortcut = false;
		this.isOpened[this.currentSquare] = false;
		
		// optimizaiton: can make huge jumps if I know there's nothing in the way
		Sampling = this.minSampling;
		if (this.optimized === true) {
			Sampling = Math.floor( (+this.map[this.currentSquare]-this.minSampling)/Sampling )*Sampling;
			if (Sampling < this.minSampling)
				Sampling = this.minSampling;
		}
		/*
		var diagSampling = Math.floor(Sampling / 1.5);
		if (diagSampling < this.minSampling)
			diagSampling = this.minSampling;
		*/
		var target = [this.e%w, Math.floor(this.e/w)];
		closeToEnd = false;
		if (SquareVectorDistance([this.currentSquare%w, Math.floor(this.currentSquare/w)], target) <= Sampling*Sampling)
		{
			closeToEnd = true;
			Sampling = 1;
		}
		if (gamestate !== undefined)
			this.TotorMap.addInfluence(this.currentSquare % w, Math.floor(this.currentSquare / w),1,40,'constant');
		
		for (i in positions)
		{
			//var hereSampling = cost[i] == 1 ? Sampling : diagSampling;
			var index = 0 + this.currentSquare +positions[i][0]*Sampling +w*Sampling*positions[i][1];
			if (this.map[index] >= Sampling)
			{
				if(this.isOpened[index] === undefined)
				{
					this.parentSquare[index] = this.currentSquare;
					
					this.fCostArray[index] = SquareVectorDistance([index%w, Math.floor(index/w)], target);// * cost[i];
					this.gCostArray[index] = this.gCostArray[this.currentSquare] + cost[i] * Sampling;// - this.map[index];

					if (!this.onWater && this.passabilityMap.map[index] === 200) {
						this.gCostArray[index] += this.width*this.width*2;
					} else if (this.onWater && this.passabilityMap.map[index] !== 200) {
						this.gCostArray[index] += this.fCostArray[index];
					} else if (!this.onWater && this.passabilityMap.map[index] === 100) {
						this.gCostArray[index] += 100;
					}
					
					if (this.openList[0] !== undefined && this.fCostArray[this.openList[0]] + this.gCostArray[this.openList[0]] > this.fCostArray[index] + this.gCostArray[index])
					{
						this.openList.unshift(index);
						shortcut = true;
					} else {
						this.openList.push(index);
					}
					this.isOpened[index] = true;
					if (closeToEnd === true && (index === e || index - 1 === e || index + 1 === e || index - w === e || index + w === e
												|| index + 1 + w === e || index + 1 - w === e || index - 1 + w === e|| index - 1 - w === e)) {
						this.parentSquare[this.e] = this.currentSquare;
						found = true;
						break;
					}
				} else {
					var addCost = 0;
					if (!this.onWater && this.passabilityMap.map[index] === 200) {
						addCost = this.width*this.width*2;
					} else if (this.onWater && this.passabilityMap.map[index] !== 200) {
						addCost = this.fCostArray[index];
					} else if (!this.onWater && this.passabilityMap.map[index] === 100) {
						addCost += 100;
					}
					//addCost -=  this.map[index];
					// already on the Open or closed list
					if (this.gCostArray[index] > cost[i] * Sampling + addCost + this.gCostArray[this.currentSquare])
					{
						this.parentSquare[index] = this.currentSquare;
						this.gCostArray[index] = cost[i] * Sampling + addCost  + this.gCostArray[this.currentSquare];
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
		var lastPos = [0,0];
		while (this.parentSquare[this.currentSquare] !== s)
		{
			this.currentSquare = this.parentSquare[this.currentSquare];
			if (gamestate !== undefined)
				this.TotorMap.addInfluence(this.currentSquare % w, Math.floor(this.currentSquare / w),1,50,'constant');
			if (SquareVectorDistance(lastPos,[this.currentSquare % w, Math.floor(this.currentSquare / w)]) > 300)
			{
				lastPos = [ (this.currentSquare % w) * this.cellSize, Math.floor(this.currentSquare / w) * this.cellSize];
				paths.push(lastPos);
				if (gamestate !== undefined)
					this.TotorMap.addInfluence(this.currentSquare % w, Math.floor(this.currentSquare / w),1,100,'constant');
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
	
	if (paths.length > 0) {
		return [paths, this.pathRequiresWater];
	} else {
		return undefined;
	}
	
}

/**
 * Make each cell's 8-bit value at least one greater than each of its
 * neighbours' values. (If the grid is initialised with 0s and things high enough (> 100 on most maps), the
 * result of each cell is its Manhattan distance to the nearest 0.)
 */
aStarPath.prototype.expandInfluences = function() {
	var w = this.width;
	var h = this.height;
	var grid = this.map;
	for ( var y = 0; y < h; ++y) {
		var min = 8;
		for ( var x = 0; x < w; ++x) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > 8)
				min = 8;
		}
		
		for ( var x = w - 2; x >= 0; --x) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > 8)
				min = 8;
		}
	}
	
	for ( var x = 0; x < w; ++x) {
		var min = 8;
		for ( var y = 0; y < h; ++y) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > 8)
				min = 8;
		}
		
		for ( var y = h - 2; y >= 0; --y) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > 8)
				min = 8;
		}
	}
};
