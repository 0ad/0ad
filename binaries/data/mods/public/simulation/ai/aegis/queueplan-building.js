var AEGIS = function(m)
{

// Defines a construction plan, ie a building.
// We'll try to fing a good position if non has been provided

m.ConstructionPlan = function(gameState, type, metadata, position) {
	if (!m.QueuePlan.call(this, gameState, type, metadata))
		return false;

	this.position = position ? position : 0;

	this.category = "building";

	return true;
};

m.ConstructionPlan.prototype = Object.create(m.QueuePlan.prototype);

// checks other than resource ones.
// TODO: change this.
// TODO: if there are specific requirements here, maybe try to do them?
m.ConstructionPlan.prototype.canStart = function(gameState) {
	if (gameState.buildingsBuilt > 0)
		return false;
	
	if (!this.isGo(gameState))
		return false;
	
	// TODO: verify numeric limits etc
	if (this.template.requiredTech() && !gameState.isResearched(this.template.requiredTech()))
	{
		return false;
	}
	var builders = gameState.findBuilders(this.type);

	return (builders.length != 0);
};

m.ConstructionPlan.prototype.start = function(gameState) {
	
	var builders = gameState.findBuilders(this.type).toEntityArray();

	// We don't care which builder we assign, since they won't actually
	// do the building themselves - all we care about is that there is
	// some unit that can start the foundation

	var pos = this.findGoodPosition(gameState);
	if (!pos){
		if (this.template.hasClass("Naval"))
			gameState.ai.HQ.dockFailed = true;
		m.debug("No room to place " + this.type);
		return;
	}
	if (this.template.hasClass("Naval"))
		m.debug (pos);
	gameState.buildingsBuilt++;

	if (gameState.getTemplate(this.type).buildCategory() === "Dock")
	{
		for (var angle = 0; angle < Math.PI * 2; angle += Math.PI/4)
		{
			builders[0].construct(this.type, pos.x, pos.z, angle, this.metadata);
		}
	} else {
		// try with the lowest, move towards us unless we're same
		if (pos.x == pos.xx && pos.z == pos.zz)
			builders[0].construct(this.type, pos.x, pos.z, pos.angle, this.metadata);
		else
		{
			for (var step = 0; step <= 1; step += 0.2)
			{
				builders[0].construct(this.type, (step*pos.x + (1-step)*pos.xx), (step*pos.z + (1-step)*pos.zz), pos.angle, this.metadata);
			}
		}
	}
	this.onStart(gameState);
};

m.ConstructionPlan.prototype.findGoodPosition = function(gameState) {
	var template = gameState.getTemplate(this.type);

	var cellSize = gameState.cellSize; // size of each tile

	// First, find all tiles that are far enough away from obstructions:

	var obstructionMap = m.createObstructionMap(gameState,0, template);
	
	//obstructionMap.dumpIm(template.buildCategory() + "_obstructions_pre.png");

	if (template.buildCategory() !== "Dock")
		obstructionMap.expandInfluences();

	//obstructionMap.dumpIm(template.buildCategory() + "_obstructions.png");

	// Compute each tile's closeness to friendly structures:

	var friendlyTiles = new API3.Map(gameState.sharedScript);
	
	var alreadyHasHouses = false;

	// If a position was specified then place the building as close to it as possible
	if (this.position) {
		var x = Math.floor(this.position[0] / cellSize);
		var z = Math.floor(this.position[1] / cellSize);
		friendlyTiles.addInfluence(x, z, 255);
	} else {
		// No position was specified so try and find a sensible place to build
		if (this.metadata && this.metadata.base !== undefined)
			for each (var px in gameState.ai.HQ.baseManagers[this.metadata.base].territoryIndices)
				friendlyTiles.map[px] = 20;
		gameState.getOwnStructures().forEach(function(ent) {
			var pos = ent.position();
			var x = Math.round(pos[0] / cellSize);
			var z = Math.round(pos[1] / cellSize);

			if (template.hasClass("Field")) {
				if (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf("food") !== -1)
					friendlyTiles.addInfluence(x, z, 20, 50);
			} else if (template.hasClass("House")) {
				if (ent.hasClass("House"))
				{
					friendlyTiles.addInfluence(x, z, 15,40);	// houses are close to other houses
					alreadyHasHouses = true;
				} else {
					friendlyTiles.addInfluence(x, z, 15, -40); // and further away from other stuffs
				}
			} else if (template.hasClass("Farmstead")) {
				// move farmsteads away to make room.
				friendlyTiles.addInfluence(x, z, 25, -25);
			} else {
				if (template.hasClass("GarrisonFortress") && ent.genericName() == "House")
					friendlyTiles.addInfluence(x, z, 30, -50);
				else if (template.hasClass("Military"))
					friendlyTiles.addInfluence(x, z, 10, -40);

				// If this is not a field add a negative influence near the CivCentre because we want to leave this
				// area for fields.
				if (ent.hasClass("CivCentre"))
					friendlyTiles.addInfluence(x, z, 20, -20);
			}
		});

		if (template.hasClass("Farmstead"))
		{
			for (var j = 0; j < gameState.sharedScript.resourceMaps["wood"].map.length; ++j)
			{
				var value = friendlyTiles.map[j] -  (gameState.sharedScript.resourceMaps["wood"].map[j])/3;
				friendlyTiles.map[j] = value >= 0 ? value : 0;
			}
		}

		if (this.metadata && this.metadata.base !== undefined)
			for (var base in gameState.ai.HQ.baseManagers)
				if (base != this.metadata.base)
					for (var j in gameState.ai.HQ.baseManagers[base].territoryIndices)
						friendlyTiles.map[gameState.ai.HQ.baseManagers[base].territoryIndices[j]] = 0;
	}
	
	//friendlyTiles.dumpIm(template.buildCategory() + "_" +gameState.getTimeElapsed() + ".png",	200);
	
	// Find target building's approximate obstruction radius, and expand by a bit to make sure we're not too close, this
	// allows room for units to walk between buildings.
	// note: not for houses and dropsites who ought to be closer to either each other or a resource.
	// also not for fields who can be stacked quite a bit
	var radius = 0;

	if (template.hasClass("GarrisonFortress"))
		radius = Math.floor(template.obstructionRadius() / cellSize) + 2;
	else if (template.buildCategory() === "Dock")
		radius = 1;
	else if (template.resourceDropsiteTypes() === undefined)
		radius = Math.ceil(template.obstructionRadius() / cellSize) + 1;
	else
		radius = Math.ceil(template.obstructionRadius() / cellSize);
	
	// further contract cause walls
	// Note: I'm currently destroying them so that doesn't matter.
	//if (gameState.playerData.civ == "iber")
	//	radius *= 0.95;

	// Find the best non-obstructed
	if (template.hasClass("House") && !alreadyHasHouses) {
		// try to get some space first
		var bestTile = friendlyTiles.findBestTile(10, obstructionMap);
		var bestIdx = bestTile[0];
		var bestVal = bestTile[1];
	}
	
	if (bestVal === undefined || bestVal === -1) {
		var bestTile = friendlyTiles.findBestTile(radius, obstructionMap);
		var bestIdx = bestTile[0];
		var bestVal = bestTile[1];
	}
	if (bestVal === -1) {
		return false;
	}
	
	//friendlyTiles.setInfluence((bestIdx % friendlyTiles.width), Math.floor(bestIdx / friendlyTiles.width), 1, 200);
	//friendlyTiles.dumpIm(template.buildCategory() + "_" +gameState.getTimeElapsed() + ".png",	200);

	var x = ((bestIdx % friendlyTiles.width) + 0.5) * cellSize;
	var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * cellSize;

	if (template.hasClass("House") || template.hasClass("Field") || template.resourceDropsiteTypes() !== undefined)
		var secondBest = obstructionMap.findLowestNeighbor(x,z);
	else
		var secondBest = [x,z];

	// default angle
	var angle = 3*Math.PI/4;
	
	return {
		"x" : x,
		"z" : z,
		"angle" : angle,
		"xx" : secondBest[0],
		"zz" : secondBest[1]
	};
};

return m;
}(AEGIS);
