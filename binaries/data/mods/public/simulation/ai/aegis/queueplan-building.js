var ConstructionPlan = function(gameState, type, metadata, startTime, expectedTime, position) {
	this.type = gameState.applyCiv(type);
	this.position = position;

	this.metadata = metadata;

	this.ID = uniqueIDBOPlans++;

	this.template = gameState.getTemplate(this.type);
	if (!this.template) {
		return false;
	}
	
	this.category = "building";
	this.cost = new Resources(this.template.cost());
	this.number = 1; // The number of buildings to build
	
	if (!startTime)
		this.startTime = 0;
	else
		this.startTime = startTime;

	if (!expectedTime)
		this.expectedTime = -1;
	else
		this.expectedTime = expectedTime;
	return true;
};

// return true if we willstart amassing resource for this plan
ConstructionPlan.prototype.isGo = function(gameState) {
	return (gameState.getTimeElapsed() > this.startTime);
};

// checks other than resource ones.
// TODO: change this.
ConstructionPlan.prototype.canStart = function(gameState) {
	if (gameState.buildingsBuilt > 0)
		return false;
	
	// TODO: verify numeric limits etc
	if (this.template.requiredTech() && !gameState.isResearched(this.template.requiredTech()))
	{
		return false;
	}
	var builders = gameState.findBuilders(this.type);

	return (builders.length != 0);
};

ConstructionPlan.prototype.start = function(gameState) {
	
	var builders = gameState.findBuilders(this.type).toEntityArray();

	// We don't care which builder we assign, since they won't actually
	// do the building themselves - all we care about is that there is
	// some unit that can start the foundation

	var pos = this.findGoodPosition(gameState);
	if (!pos){
		if (this.template.hasClass("Naval"))
			gameState.ai.HQ.dockFailed = true;
		debug("No room to place " + this.type);
		return;
	}
	if (this.template.hasClass("Naval"))
		debug (pos);
	gameState.buildingsBuilt++;

	if (gameState.getTemplate(this.type).buildCategory() === "Dock")
	{
		for (var angle = 0; angle < Math.PI * 2; angle += Math.PI/4)
		{
			builders[0].construct(this.type, pos.x, pos.z, angle, this.metadata);
		}
	} else
		builders[0].construct(this.type, pos.x, pos.z, pos.angle, this.metadata);
};

ConstructionPlan.prototype.getCost = function() {
	var costs = new Resources();
	costs.add(this.cost);
	return costs;
};

ConstructionPlan.prototype.findGoodPosition = function(gameState) {
	var template = gameState.getTemplate(this.type);

	var cellSize = gameState.cellSize; // size of each tile

	// First, find all tiles that are far enough away from obstructions:

	var obstructionMap = Map.createObstructionMap(gameState,0, template);
	
	//obstructionMap.dumpIm(template.buildCategory() + "_obstructions_pre.png");

	if (template.buildCategory() !== "Dock")
		obstructionMap.expandInfluences();

	//obstructionMap.dumpIm(template.buildCategory() + "_obstructions.png");

	// Compute each tile's closeness to friendly structures:

	var friendlyTiles = new Map(gameState.sharedScript);
	
	var alreadyHasHouses = false;

	// If a position was specified then place the building as close to it as possible
	if (this.position){
		var x = Math.round(this.position[0] / cellSize);
		var z = Math.round(this.position[1] / cellSize);
		friendlyTiles.addInfluence(x, z, 200);
	} else {
		// No position was specified so try and find a sensible place to build
		gameState.getOwnEntities().forEach(function(ent) {
			if (ent.hasClass("Structure")) {
				var infl = 32;
				if (ent.hasClass("CivCentre"))
					infl *= 4;
	
				var pos = ent.position();
				var x = Math.round(pos[0] / cellSize);
				var z = Math.round(pos[1] / cellSize);
										   
				if (ent.buildCategory() == "Wall") {	// no real blockers, but can't build where they are
					friendlyTiles.addInfluence(x, z, 2,-1000);
					return;
				}

				if (template._template.BuildRestrictions.Category === "Field"){
					if (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf("food") !== -1){
						if (ent.hasClass("CivCentre"))
							friendlyTiles.addInfluence(x, z, infl/4, infl);
						else
							 friendlyTiles.addInfluence(x, z, infl, infl);
										   
					}
				}else{
					if (template.genericName() == "House" && ent.genericName() == "House") {
						friendlyTiles.addInfluence(x, z, 15.0,20,'linear');	// houses are close to other houses
						alreadyHasHouses = true;
					} else if (template.hasClass("GarrisonFortress") && ent.genericName() == "House")
					{
						friendlyTiles.addInfluence(x, z, 30, -50);
					} else if (template.genericName() == "House") {
						friendlyTiles.addInfluence(x, z, Math.ceil(infl/4.0),-infl/2.0);	// houses are farther away from other buildings but houses
					} else if (template.hasClass("GarrisonFortress"))
					{
						friendlyTiles.addInfluence(x, z, 20, 10);
						friendlyTiles.addInfluence(x, z, 10, -40, 'linear');
					} else if (ent.genericName() != "House") // houses have no influence on other buildings
					{
						friendlyTiles.addInfluence(x, z, infl);
						//avoid building too close to each other if possible.
						friendlyTiles.addInfluence(x, z, 5, -5, 'linear');
					}
						// If this is not a field add a negative influence near the CivCentre because we want to leave this
						// area for fields.
					if (ent.hasClass("CivCentre") && template.genericName() != "House"){
						friendlyTiles.addInfluence(x, z, Math.floor(infl/8), Math.floor(-infl/2));
					} else if (ent.hasClass("CivCentre")) {
						friendlyTiles.addInfluence(x, z, infl/3.0, infl + 1);
						friendlyTiles.addInfluence(x, z, Math.ceil(infl/5.0), -(infl/2.0), 'linear');
					}
				}
			}
		});
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
	if (template.genericName() == "Field")
		radius = Math.ceil(template.obstructionRadius() / cellSize);
	else if (template.hasClass("GarrisonFortress"))
		radius = Math.ceil(template.obstructionRadius() / cellSize) + 2;
	else if (template.buildCategory() === "Dock")
		radius = 1;//Math.floor(template.obstructionRadius() / cellSize);
	else if (!template.hasClass("DropsiteWood") && !template.hasClass("DropsiteStone") && !template.hasClass("DropsiteMetal"))
		radius = Math.ceil(template.obstructionRadius() / cellSize + 1);
	else
		radius = Math.ceil(template.obstructionRadius() / cellSize);
	
	// further contract cause walls
	// Note: I'm currently destroying them so that doesn't matter.
	//if (gameState.playerData.civ == "iber")
	//	radius *= 0.95;

	// Find the best non-obstructed
	if (template.genericName() == "House" && !alreadyHasHouses) {
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

	// default angle
	var angle = 3*Math.PI/4;
	
	return {
		"x" : x+2,
		"z" : z+2,
		"angle" : angle
	};
};
