var PETRA = function(m)
{

// Defines a construction plan, ie a building.
// We'll try to fing a good position if non has been provided

m.ConstructionPlan = function(gameState, type, metadata, position)
{
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
m.ConstructionPlan.prototype.canStart = function(gameState)
{
	if (gameState.buildingsBuilt > 0)   // do not start another building if already one this turn
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

m.ConstructionPlan.prototype.start = function(gameState)
{

	var builders = gameState.findBuilders(this.type).toEntityArray();

	// We don't care which builder we assign, since they won't actually
	// do the building themselves - all we care about is that there is
	// some unit that can start the foundation

	var pos = this.findGoodPosition(gameState);
	if (!pos){
		if (this.template.hasClass("Naval"))
			gameState.ai.HQ.dockFailed = true;
		gameState.ai.HQ.stopBuilding.push(this.type);
		return;
	}
	gameState.buildingsBuilt++;

	if (this.metadata === undefined)
		this.metadata = { "base": pos.base };
	else if (this.metadata.base === undefined)
		this.metadata.base = pos.base;

	if (gameState.getTemplate(this.type).buildCategory() === "Dock")
	{
		for (var angle = 0; angle < Math.PI * 2; angle += Math.PI/4)
			builders[0].construct(this.type, pos.x, pos.z, angle, this.metadata);
	}
	else
	{
		// try with the lowest, move towards us unless we're same
		if (pos.x == pos.xx && pos.z == pos.zz)
			builders[0].construct(this.type, pos.x, pos.z, pos.angle, this.metadata);
		else
		{
			for (var step = 0; step <= 1; step += 0.2)
				builders[0].construct(this.type, (step*pos.x + (1-step)*pos.xx), (step*pos.z + (1-step)*pos.zz), pos.angle, this.metadata);
		}
	}
	this.onStart(gameState);
};

m.ConstructionPlan.prototype.findGoodPosition = function(gameState)
{
	var template = gameState.getTemplate(this.type);

	if (!this.position)
	{
		if (template.hasClass("CivCentre"))
		{
			if (this.metadata.type)
				var pos = gameState.ai.HQ.findEconomicCCLocation(gameState, this.metadata.type);
			else
				var pos = gameState.ai.HQ.findStrategicCCLocation(gameState);

			if (pos)
			    return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "xx": pos[0], "zz": pos[1], "base": 0 };
			else
				return false;
		}
		else if (template.hasClass("Tower") || template.hasClass("Fortress"))
		{
			var pos = gameState.ai.HQ.findDefensiveLocation(gameState, template);

			if (pos)
				return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "xx": pos[0], "zz": pos[1], "base": pos[2] };
			else
			{
				gameState.ai.HQ.stopBuilding.push(this.type);
				return false;
			}
		}
		else if (template.hasClass("Market"))
		{
			var pos = gameState.ai.HQ.findMarketLocation(gameState, template);
			if (pos && pos[2] > 0)
				return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "xx": pos[0], "zz": pos[1], "base": pos[2] };
			else if (!pos)
			{
				gameState.ai.HQ.stopBuilding.push(this.type);
				return false;
			}
		}
	}

	var cellSize = gameState.cellSize; // size of each tile

	// First, find all tiles that are far enough away from obstructions:

	var obstructionMap = m.createObstructionMap(gameState, 0, template);
	if (template.buildCategory() !== "Dock")
		obstructionMap.expandInfluences();

	//obstructionMap.dumpIm(template.buildCategory() + "_obstructions.png");

	// Compute each tile's closeness to friendly structures:

	var friendlyTiles = new API3.Map(gameState.sharedScript);
	
	var alreadyHasHouses = false;

	if (this.position)	// If a position was specified then place the building as close to it as possible
	{
		var x = Math.floor(this.position[0] / cellSize);
		var z = Math.floor(this.position[1] / cellSize);
		friendlyTiles.addInfluence(x, z, 255);
	}
	else	// No position was specified so try and find a sensible place to build
	{
		// give a small > 0 level as the result of addInfluence is constrained to be > 0 
		if (this.metadata && this.metadata.base !== undefined)
		{
			var base = this.metadata.base;
			for (var j = 0; j < friendlyTiles.map.length; ++j)
				if (gameState.ai.HQ.basesMap.map[j] === base)
					friendlyTiles.map[j] = 30;
		}
		else
		{
			for (var j = 0; j < friendlyTiles.map.length; ++j)
				if (gameState.ai.HQ.basesMap.map[j] !== 0)
					friendlyTiles.map[j] = 30;
		}

		gameState.getOwnStructures().forEach(function(ent) {
			var pos = ent.position();
			var x = Math.round(pos[0] / cellSize);
			var z = Math.round(pos[1] / cellSize);

			if (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf("food") !== -1)
			{
				if (template.hasClass("Field"))
					friendlyTiles.addInfluence(x, z, 20, 50);
				else // If this is not a field add a negative influence because we want to leave this area for fields
					friendlyTiles.addInfluence(x, z, 20, -20);
			}
			else if (template.hasClass("House"))
			{
				if (ent.hasClass("House"))
				{
					friendlyTiles.addInfluence(x, z, 15, 40);    // houses are close to other houses
					alreadyHasHouses = true;
				}
				else
					friendlyTiles.addInfluence(x, z, 15, -40);   // and further away from other stuffs
			}
			else if (template.hasClass("Farmstead") && !ent.hasClass("Field"))
				friendlyTiles.addInfluence(x, z, 25, -25);           // move farmsteads away to make room.
			else if (template.hasClass("GarrisonFortress") && ent.genericName() == "House")
				friendlyTiles.addInfluence(x, z, 30, -50);
			else if (template.hasClass("Military"))
				friendlyTiles.addInfluence(x, z, 10, -40);
		});

		if (template.hasClass("Farmstead"))
		{
			for (var j = 0; j < friendlyTiles.map.length; ++j)
			{
				var value = friendlyTiles.map[j] - (gameState.sharedScript.resourceMaps["wood"].map[j])/3;
				friendlyTiles.map[j] = value >= 0 ? value : 0;
				if (gameState.ai.HQ.borderMap.map[j] > 0)
					friendlyTiles.map[j] /= 2;	// we need space around farmstead, so disfavor map border
			}
		}
	}

	// requires to be inside our territory, and inside our base territory if required
	if (this.metadata && this.metadata.base !== undefined)
	{
		var base = this.metadata.base;
		for (var j = 0; j < friendlyTiles.map.length; ++j)
			if (gameState.ai.HQ.basesMap.map[j] !== base)
				friendlyTiles.map[j] = 0;
	}
	else
	{
		for (var j = 0; j < friendlyTiles.map.length; ++j)
			if (gameState.ai.HQ.basesMap.map[j] === 0)
				friendlyTiles.map[j] = 0;
	}
	
	// Find target building's approximate obstruction radius, and expand by a bit to make sure we're not too close, this
	// allows room for units to walk between buildings.
	// note: not for houses and dropsites who ought to be closer to either each other or a resource.
	// also not for fields who can be stacked quite a bit

	var radius = 0;
	if (template.hasClass("Fortress") || this.type == "structures/{civ}_siege_workshop")
		radius = Math.floor(template.obstructionRadius() / cellSize) + 3;
	else if (template.buildCategory() === "Dock")
		radius = 1;
	else if (template.resourceDropsiteTypes() === undefined && !template.hasClass("House") && !template.hasClass("Field"))
		radius = Math.ceil(template.obstructionRadius() / cellSize) + 1;
	else
		radius = Math.ceil(template.obstructionRadius() / cellSize);

	// Find the best non-obstructed
	if (template.hasClass("House") && !alreadyHasHouses)
	{
		// try to get some space first
		var bestTile = friendlyTiles.findBestTile(10, obstructionMap);
		var bestIdx = bestTile[0];
		var bestVal = bestTile[1];
	}
	
	if (bestVal === undefined || bestVal === -1)
	{
		var bestTile = friendlyTiles.findBestTile(radius, obstructionMap);
		var bestIdx = bestTile[0];
		var bestVal = bestTile[1];
	}

	if (bestVal <= 0)
	{
		gameState.ai.HQ.stopBuilding.push(this.type);
		return false;
	}

	var x = ((bestIdx % friendlyTiles.width) + 0.5) * cellSize;
	var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * cellSize;

	if (template.hasClass("House") || template.hasClass("Field") || template.resourceDropsiteTypes() !== undefined)
		var secondBest = obstructionMap.findLowestNeighbor(x,z);
	else
		var secondBest = [x,z];

	// default angle = 3*Math.PI/4;	
	return { "x": x, "z": z, "angle": 3*Math.PI/4, "xx": secondBest[0], "zz": secondBest[1], "base": gameState.ai.HQ.basesMap.map[bestIdx] };
};

return m;
}(PETRA);
