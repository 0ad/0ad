var API3 = function(m)
{

/*
 * TerrainAnalysis, inheriting from the Map Component.
 * 
 * This creates a suitable passability map.
 * This is part of the Shared Script, and thus should only be used for things that are non-player specific.
 * This.map is a map of the world, where particular stuffs are pointed with a value
 * For example, impassable land is 0, water is 200, areas near tree (ie forest grounds) are 41…
 * This is intended for use with 8 bit maps for reduced memory usage.
 * Upgraded from QuantumState's original TerrainAnalysis for qBot.
 */

m.TerrainAnalysis = function()
{
};

m.copyPrototype(m.TerrainAnalysis, m.Map);

m.TerrainAnalysis.prototype.init = function(sharedScript, rawState)
{
	var passabilityMap = rawState.passabilityMap;
	this.width = passabilityMap.width;
	this.height = passabilityMap.height;
	this.cellSize = passabilityMap.cellSize;

	var obstructionMaskLand = rawState.passabilityClasses["default-terrain-only"];
	var obstructionMaskWater = rawState.passabilityClasses["ship-terrain-only"];

	var obstructionTiles = new Uint8Array(passabilityMap.data.length);

	/* Generated map legend:
	 0 is impassable
	 200 is deep water (ie non-passable by land units)
	 201 is shallow water (passable by land units and water units)
	 255 is land (or extremely shallow water where ships can't go).
	*/
	
	for (let i = 0; i < passabilityMap.data.length; ++i)
	{
		// If impassable for land units, set to 0, else to 255.
		obstructionTiles[i] = (passabilityMap.data[i] & obstructionMaskLand) ? 0 : 255;
		
		if (!(passabilityMap.data[i] & obstructionMaskWater) && obstructionTiles[i] === 0)
			obstructionTiles[i] = 200; // if navigable and not walkable (ie basic water), set to 200.
		else if (!(passabilityMap.data[i] & obstructionMaskWater) && obstructionTiles[i] === 255)
			obstructionTiles[i] = 201; // navigable and walkable.
	}

	this.Map(rawState, "passability", obstructionTiles);
};

/*
 * Accessibility inherits from TerrainAnalysis
 *  
 * This can easily and efficiently determine if any two points are connected.
 * it can also determine if any point is "probably" reachable, assuming the unit can get close enough
 * for optimizations it's called after the TerrainAnalyser has finished initializing his map
 * so this can use the land regions already.
 */
m.Accessibility = function()
{	
};

m.copyPrototype(m.Accessibility, m.TerrainAnalysis);

m.Accessibility.prototype.init = function(rawState, terrainAnalyser)
{
	this.Map(rawState, "passability", terrainAnalyser.map);
	this.landPassMap = new Uint16Array(terrainAnalyser.length);
	this.navalPassMap = new Uint16Array(terrainAnalyser.length);

	this.maxRegions = 65535;
	this.regionSize = [];
	this.regionType = []; // "inaccessible", "land" or "water";
	// ID of the region associated with an array of region IDs.
	this.regionLinks = [];
	
	// initialized to 0, it's more optimized to start at 1 (I'm checking that if it's not 0, then it's already aprt of a region, don't touch);
	// However I actually store all unpassable as region 1 (because if I don't, on some maps the toal nb of region is over 256, and it crashes as the mapis 8bit.)
	// So start at 2.
	this.regionID = 2;
	
	for (var i = 0; i < this.landPassMap.length; ++i)
	{
		if (this.map[i] !== 0)
		{	// any non-painted, non-inacessible area.
			if (this.landPassMap[i] === 0 && this.floodFill(i,this.regionID,false))
				this.regionType[this.regionID++] = "land";
			if (this.navalPassMap[i] === 0 && this.floodFill(i,this.regionID,true))
				this.regionType[this.regionID++] = "water";
		}
		else if (this.landPassMap[i] === 0)
		{	// any non-painted, inacessible area.
			this.floodFill(i,1,false);
			this.floodFill(i,1,true);
		}
	}
	
	// calculating region links. Regions only touching diagonaly are not linked.
	// since we're checking all of them, we'll check from the top left to the bottom right
	var w = this.width;
	for (var x = 0; x < this.width-1; ++x)
	{
		for (var y = 0; y < this.height-1; ++y)
		{
			// checking right.
			var thisLID = this.landPassMap[x+y*w];
			var thisNID = this.navalPassMap[x+y*w];
			var rightLID = this.landPassMap[x+1+y*w];
			var rightNID = this.navalPassMap[x+1+y*w];
			var bottomLID = this.landPassMap[x+y*w+w];
			var bottomNID = this.navalPassMap[x+y*w+w];
			if (thisLID > 1)
			{
				if (rightNID > 1)
					if (this.regionLinks[thisLID].indexOf(rightNID) === -1)
						this.regionLinks[thisLID].push(rightNID);
				if (bottomNID > 1)
					if (this.regionLinks[thisLID].indexOf(bottomNID) === -1)
						this.regionLinks[thisLID].push(bottomNID);
			}
			if (thisNID > 1)
			{
				if (rightLID > 1)
					if (this.regionLinks[thisNID].indexOf(rightLID) === -1)
						this.regionLinks[thisNID].push(rightLID);
				if (bottomLID > 1)
					if (this.regionLinks[thisNID].indexOf(bottomLID) === -1)
						this.regionLinks[thisNID].push(bottomLID);
				if (thisLID > 1)
					if (this.regionLinks[thisNID].indexOf(thisLID) === -1)
						this.regionLinks[thisNID].push(thisLID);
			}
		}
	}
	
	//warn(uneval(this.regionLinks));

	//Engine.DumpImage("LandPassMap.png", this.landPassMap, this.width, this.height, 255);
	//Engine.DumpImage("NavalPassMap.png", this.navalPassMap, this.width, this.height, 255);
};

m.Accessibility.prototype.getAccessValue = function(position, onWater)
{
	var gamePos = this.gamePosToMapPos(position);
	if (onWater === true)
		return this.navalPassMap[gamePos[0] + this.width*gamePos[1]];
	var ret = this.landPassMap[gamePos[0] + this.width*gamePos[1]];
	if (ret === 1)
	{
		// quick spiral search.
		var indx = [ [-1,-1],[-1,0],[-1,1],[0,1],[1,1],[1,0],[1,-1],[0,-1]];
		for (let i of indx)
		{
			let id0 = gamePos[0] + i[0];
			let id1 = gamePos[1] + i[1];
			if (id0 < 0 || id0 >= this.width || id1 < 0 || id1 >= this.width)
				continue;
			ret = this.landPassMap[id0 + this.width*id1];
			if (ret !== 1)
				return ret;
		}
	}
	return ret;
};

m.Accessibility.prototype.getTrajectTo = function(start, end)
{
	var pstart = this.gamePosToMapPos(start);
	var istart = pstart[0] + pstart[1]*this.width;
	var pend = this.gamePosToMapPos(end);
	var iend = pend[0] + pend[1]*this.width;
	
	var onLand = true;
	if (this.landPassMap[istart] <= 1 && this.navalPassMap[istart] > 1)
		onLand = false;
	if (this.landPassMap[istart] <= 1 && this.navalPassMap[istart] <= 1)
		return false;
	
	var endRegion = this.landPassMap[iend];
	if (endRegion <= 1 && this.navalPassMap[iend] > 1)
		endRegion = this.navalPassMap[iend];
	else if (endRegion <= 1)
		return false;
	
	if (onLand)
		var startRegion = this.landPassMap[istart];
	else
		var startRegion = this.navalPassMap[istart];

	return this.getTrajectToIndex(startRegion, endRegion);
};

// Return a "path" of accessibility indexes from one point to another, including the start and the end indexes
// this can tell you what sea zone you need to have a dock on, for example.
// assumes a land unit unless start point is over deep water.
m.Accessibility.prototype.getTrajectToIndex = function(istart, iend)
{
	if (istart === iend)
		return [istart];

	let trajects = new Set();
	let explored = new Set();
	trajects.add([istart]);
	explored.add(istart);
	while (trajects.size)
	{
		for (let traj of trajects)
		{
			let ilast = traj[traj.length-1];
			for (let inew of this.regionLinks[ilast])
			{
				if (inew === iend)
					return traj.concat(iend);
				if (explored.has(inew))
					continue;
				trajects.add(traj.concat(inew));
				explored.add(inew);
			}
			trajects.delete(traj);
		}
	}
	return undefined;
};

m.Accessibility.prototype.getRegionSize = function(position, onWater){
	var pos = this.gamePosToMapPos(position);
	var index = pos[0] + pos[1]*this.width;
	var ID = (onWater === true) ? this.navalPassMap[index] : this.landPassMap[index];
	if (this.regionSize[ID] === undefined)
		return 0;
	return this.regionSize[ID];
};

m.Accessibility.prototype.getRegionSizei = function(index, onWater) {
	if (this.regionSize[this.landPassMap[index]] === undefined && (!onWater || this.regionSize[this.navalPassMap[index]] === undefined))
		return 0;
	if (onWater && this.regionSize[this.navalPassMap[index]] > this.regionSize[this.landPassMap[index]])
		return this.regionSize[this.navalPassMap[index]];
	return this.regionSize[this.landPassMap[index]];
};

// Implementation of a fast flood fill. Reasonably good performances for JS.
m.Accessibility.prototype.floodFill = function(startIndex, value, onWater)
{
	if (value > this.maxRegions)
	{
		error("AI accessibility map: too many regions.");
		this.landPassMap[startIndex] = 1;
		this.navalPassMap[startIndex] = 1;
		return false;
	}

	if ((!onWater && this.landPassMap[startIndex] !== 0) || (onWater && this.navalPassMap[startIndex] !== 0) )
		return false;	// already painted.

	var floodFor = "land";
	if (this.map[startIndex] === 0)
	{
		this.landPassMap[startIndex] = 1;
		this.navalPassMap[startIndex] = 1;
		return false;
	}

	if (onWater === true)
	{
		if (this.map[startIndex] !== 200 && this.map[startIndex] !== 201)
		{
			this.navalPassMap[startIndex] = 1;	// impassable for naval
			return false;	// do nothing
		}
		floodFor = "water";
	}
	else if (this.map[startIndex] === 200)
	{
		this.landPassMap[startIndex] = 1;	// impassable for land
		return false;
	}
	
	// here we'll be able to start.
	for (var i = this.regionSize.length; i <= value; ++i)
	{
		this.regionLinks.push([]);
		this.regionSize.push(0);
		this.regionType.push("inaccessible");
	}
	var w = this.width;
	var h = this.height;

	var y = 0;
	// Get x and y from index
	var IndexArray = [startIndex];
	var newIndex = 0;
	while(IndexArray.length)
	{		
		newIndex = IndexArray.pop();

		y = 0;
		var loop = false;
		// vertical iteration
		do {
			--y;
			loop = false;
			var index = newIndex + w*y;
			if (index < 0)
				break;
			if (floodFor === "land" && this.landPassMap[index] === 0 && this.map[index] !== 0 && this.map[index] !== 200)
				loop = true;
			else if (floodFor === "water" && this.navalPassMap[index] === 0 && (this.map[index] === 200 || this.map[index] === 201))
				loop = true;
			else
				break;
		} while (loop === true);	// should actually break
		++y;
		var reachLeft = false;
		var reachRight = false;
		do {
			var index = newIndex + w*y;
			
			if (floodFor === "land" && this.landPassMap[index] === 0 && this.map[index] !== 0 && this.map[index] !== 200)
			{
				this.landPassMap[index] = value;
				this.regionSize[value]++;
			}
			else if (floodFor === "water" && this.navalPassMap[index] === 0 && (this.map[index] === 200 || this.map[index] === 201))
			{
				this.navalPassMap[index] = value;
				this.regionSize[value]++;
			}
			else
				break;
			
			if (index%w > 0)
			{
				if (floodFor === "land" && this.landPassMap[index -1] === 0 && this.map[index -1] !== 0 && this.map[index -1] !== 200)
				{
					if (!reachLeft)
					{
						IndexArray.push(index -1);
						reachLeft = true;
					}
				}
				else if (floodFor === "water" && this.navalPassMap[index -1] === 0 && (this.map[index -1] === 200 || this.map[index -1] === 201))
				{
					if (!reachLeft)
					{
						IndexArray.push(index -1);
						reachLeft = true;
					}
				}
				else if (reachLeft)
					reachLeft = false;
			}

			if (index%w < w - 1)
			{
				if (floodFor === "land" && this.landPassMap[index +1] === 0 && this.map[index +1] !== 0 && this.map[index +1] !== 200)
				{
					if (!reachRight)
					{
						IndexArray.push(index +1);
						reachRight = true;
					}
				}
				else if (floodFor === "water" && this.navalPassMap[index +1] === 0 && (this.map[index +1] === 200 || this.map[index +1] === 201))
				{
					if (!reachRight)
					{
						IndexArray.push(index +1);
						reachRight = true;
					}
				}
				else if (reachRight)
					reachRight = false;
			}
			++y;
		} while (index/w < h-1);	// should actually break
	}
	return true;
};

// creates a map of resource density
m.SharedScript.prototype.createResourceMaps = function(sharedScript)
{	
	for (let resource in this.decreaseFactor)
	{
		// if there is no resourceMap create one with an influence for everything with that resource
		if (!this.resourceMaps[resource])
		{
			// We're creting them 8-bit. Things could go above 255 if there are really tons of resources
			// But at that point the precision is not really important anyway. And it saves memory.
			this.resourceMaps[resource] = new m.Map(sharedScript, "resource");
			this.CCResourceMaps[resource] = new m.Map(sharedScript, "resource");
		}
	}
	var cellSize = this.resourceMaps["wood"].cellSize;
	for (let ent of sharedScript._entities.values())
	{
		if (ent && ent.position() && ent.resourceSupplyType() && ent.resourceSupplyType().generic !== "treasure") {
			var resource = ent.resourceSupplyType().generic;
			if (!this.resourceMaps[resource])
				continue;
			var x = Math.floor(ent.position()[0] / cellSize);
			var z = Math.floor(ent.position()[1] / cellSize);
			var strength = Math.floor(ent.resourceSupplyMax()/this.decreaseFactor[resource]);
			if (resource === "wood")
			{
				this.CCResourceMaps[resource].addInfluence(x, z, 60/cellSize, strength, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 36/cellSize, strength/2, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 36/cellSize, strength/2);
			}
			else if (resource === "stone" || resource === "metal")
			{
				this.CCResourceMaps[resource].addInfluence(x, z, 120/cellSize, strength, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 48/cellSize, strength/2, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 48/cellSize, strength/2);
			}
		}
	}
};


// TODO: make it regularly update stone+metal mines and their resource levels.
// creates and maintains a map of unused resource density
// this also takes dropsites into account.
// resources that are "part" of a dropsite are not counted.
m.SharedScript.prototype.updateResourceMaps = function(sharedScript, events)
{	
	for (let resource in this.decreaseFactor)
	{
		// if there is no resourceMap create one with an influence for everything with that resource
		if (!this.resourceMaps[resource])
		{
			// We're creting them 8-bit. Things could go above 255 if there are really tons of resources
			// But at that point the precision is not really important anyway. And it saves memory.
			this.resourceMaps[resource] = new m.Map(sharedScript, "resource");
			this.CCResourceMaps[resource] = new m.Map(sharedScript, "resource");
		}
	}
	var cellSize = this.resourceMaps["wood"].cellSize;
	// Look for destroy events and subtract the entities original influence from the resourceMap
	// TODO: perhaps do something when dropsites appear/disappear.
	let destEvents = events["Destroy"];
	let createEvents = events["Create"];
	
	for (let e of destEvents)
	{
		if (!e.entityObj)
			continue;
		let ent = e.entityObj;
		if (ent && ent.position() && ent.resourceSupplyType() && ent.resourceSupplyType().generic !== "treasure")
		{
			let resource = ent.resourceSupplyType().generic;
			if (!this.resourceMaps[resource])
				continue;
			let x = Math.floor(ent.position()[0] / cellSize);
			let z = Math.floor(ent.position()[1] / cellSize);
			let strength = Math.floor(ent.resourceSupplyMax()/this.decreaseFactor[resource]);
			if (resource === "wood")
			{
				this.CCResourceMaps[resource].addInfluence(x, z, 60/cellSize, -strength, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 36/cellSize, -strength/2, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 36/cellSize, -strength/2);
			}
			else if (resource === "stone" || resource === "metal")
			{
				this.CCResourceMaps[resource].addInfluence(x, z, 120/cellSize, -strength, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 48/cellSize, -strength/2, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 48/cellSize, -strength/2);
			}
		}
	}
	for (let e of createEvents)
	{
		if (!e.entity || !sharedScript._entities.has(e.entity))
			continue;
		let ent = sharedScript._entities.get(e.entity);
		if (ent && ent.position() && ent.resourceSupplyType() && ent.resourceSupplyType().generic !== "treasure")
		{
			let resource = ent.resourceSupplyType().generic;
			if (!this.resourceMaps[resource])
				continue;
			let x = Math.floor(ent.position()[0] / cellSize);
			let z = Math.floor(ent.position()[1] / cellSize);
			let strength = Math.floor(ent.resourceSupplyMax()/this.decreaseFactor[resource]);
			if (resource === "wood")
			{
				this.CCResourceMaps[resource].addInfluence(x, z, 60/cellSize, strength, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 36/cellSize, strength/2, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 36/cellSize, strength/2);
			}
			else if (resource === "stone" || resource === "metal")
			{
				this.CCResourceMaps[resource].addInfluence(x, z, 120/cellSize, strength, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 48/cellSize, strength/2, "constant");
				this.resourceMaps[resource].addInfluence(x, z, 48/cellSize, strength/2);
			}
		}
	}
};

return m;

}(API3);
