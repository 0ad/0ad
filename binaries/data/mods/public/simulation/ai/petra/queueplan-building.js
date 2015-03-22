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

	if (this.template.requiredTech() && !gameState.isResearched(this.template.requiredTech()))
		return false;

	return (gameState.findBuilders(this.type).length != 0);
};

m.ConstructionPlan.prototype.start = function(gameState)
{
	Engine.ProfileStart("Building construction start");

	var builders = gameState.findBuilders(this.type).toEntityArray();

	// We don't care which builder we assign, since they won't actually
	// do the building themselves - all we care about is that there is
	// some unit that can start the foundation

	var pos = this.findGoodPosition(gameState);
	if (!pos)
	{
		gameState.ai.HQ.stopBuild(gameState, this.type);
		Engine.ProfileStop();
		return;
	}
	else if (this.metadata && this.metadata.expectedGain)
	{
		// Check if this market is still worth building (others may have been built making it useless)
		let tradeManager = gameState.ai.HQ.tradeManager;
		tradeManager.checkRoutes(gameState);
		if (!tradeManager.isNewMarketWorth(this.metadata.expectedGain))
		{
			Engine.ProfileStop();
			return;
		}
	}
	gameState.buildingsBuilt++;

	if (this.metadata === undefined)
		this.metadata = { "base": pos.base };
	else if (this.metadata.base === undefined)
		this.metadata.base = pos.base;

	if (pos.access)
		this.metadata.access = pos.access;   // needed for Docks whose position is on water
	else
		this.metadata.access = gameState.ai.accessibility.getAccessValue([pos.x, pos.z]);

	if (this.template.buildCategory() === "Dock")
	{
		// adjust a bit the position if needed
		// TODO we would need groundLevel and waterLevel to do it properly
		let cosa = Math.cos(pos.angle);
		let sina = Math.sin(pos.angle);
		let shiftMax = gameState.ai.HQ.territoryMap.cellSize;
		for (let shift = 0; shift <= shiftMax; shift += 2)
		{
			builders[0].construct(this.type, pos.x-shift*sina, pos.z-shift*cosa, pos.angle, this.metadata);
			if (shift > 0)
				builders[0].construct(this.type, pos.x+shift*sina, pos.z+shift*cosa, pos.angle, this.metadata);
		}
	}
	else if (pos.x == pos.xx && pos.z == pos.zz)
		builders[0].construct(this.type, pos.x, pos.z, pos.angle, this.metadata);
	else // try with the lowest, move towards us unless we're same
	{
		for (let step = 0; step <= 1; step += 0.2)
			builders[0].construct(this.type, (step*pos.x + (1-step)*pos.xx), (step*pos.z + (1-step)*pos.zz),
				pos.angle, this.metadata);
	}
	this.onStart(gameState);
	Engine.ProfileStop();

	// TODO should have a ConstructionStarted even in case the construct order fails
	if (this.metadata && this.metadata.proximity)
		gameState.ai.HQ.navalManager.createTransportIfNeeded(gameState, this.metadata.proximity, [pos.x, pos.z], this.metadata.access);
};

// TODO for dock, we should allow building them outside territory, and we should check that we are along the right sea
m.ConstructionPlan.prototype.findGoodPosition = function(gameState)
{

	var template = this.template;

	if (template.buildCategory() === "Dock")
		return this.findDockPosition(gameState);

	if (!this.position)
	{
		if (template.hasClass("CivCentre"))
		{
			if (this.metadata && this.metadata.resource)
			{
				var proximity = this.metadata.proximity ? this.metadata.proximity : undefined;
				var pos = gameState.ai.HQ.findEconomicCCLocation(gameState, template, this.metadata.resource, proximity);
			}
			else
				var pos = gameState.ai.HQ.findStrategicCCLocation(gameState, template);

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
			else if (!template.hasClass("Fortress") || gameState.civ() === "mace" || gameState.civ() === "maur" ||
				gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_fortress"), true)
				+ gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_army_camp"), true) > 0)
				// if this fortress is our first siege unit builder, just try the standard placement as we want siege units
				return false;
		}
		else if (template.hasClass("Market"))	// Docks (i.e. NavalMarket) are done before
		{
			var pos = gameState.ai.HQ.findMarketLocation(gameState, template);
			if (pos && pos[2] > 0)
			{
				if (!this.metadata)
					this.metadata = {};
				this.metadata.expectedGain = pos[3];
				return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "xx": pos[0], "zz": pos[1], "base": pos[2] };
			}
			else if (!pos)
				return false;
		}
	}

	// Compute each tile's closeness to friendly structures:

	var placement = new API3.Map(gameState.sharedScript, "territory");
	var cellSize = placement.cellSize; // size of each tile
	
	var alreadyHasHouses = false;

	if (this.position)	// If a position was specified then place the building as close to it as possible
	{
		var x = Math.floor(this.position[0] / cellSize);
		var z = Math.floor(this.position[1] / cellSize);
		placement.addInfluence(x, z, 255);
	}
	else	// No position was specified so try and find a sensible place to build
	{
		// give a small > 0 level as the result of addInfluence is constrained to be > 0
		// if we really need houses (i.e. townPhasing without enough village building), do not apply these constraints
		if (this.metadata && this.metadata.base !== undefined)
		{
			var base = this.metadata.base;
			for (var j = 0; j < placement.map.length; ++j)
				if (gameState.ai.HQ.basesMap.map[j] == base)
					placement.map[j] = 45;
		}
		else
		{
			for (var j = 0; j < placement.map.length; ++j)
				if (gameState.ai.HQ.basesMap.map[j] != 0)
					placement.map[j] = 45;
		}

		if (!gameState.ai.HQ.requireHouses || !template.hasClass("House"))
		{
			gameState.getOwnStructures().forEach(function(ent) {
				var pos = ent.position();
				var x = Math.round(pos[0] / cellSize);
				var z = Math.round(pos[1] / cellSize);

				if (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf("food") !== -1)
				{
					if (template.hasClass("Field"))
						placement.addInfluence(x, z, 20, 50);
					else // If this is not a field add a negative influence because we want to leave this area for fields
						placement.addInfluence(x, z, 20, -20);
				}
				else if (template.hasClass("House"))
				{
					if (ent.hasClass("House"))
					{
						placement.addInfluence(x, z, 15, 40);    // houses are close to other houses
						alreadyHasHouses = true;
					}
					else
						placement.addInfluence(x, z, 15, -40);   // and further away from other stuffs
				}
				else if (template.hasClass("Farmstead") && (!ent.hasClass("Field")
					&& (!ent.hasClass("StoneWall") || ent.hasClass("Gates"))))
					placement.addInfluence(x, z, 25, -25);       // move farmsteads away to make room (StoneWall test needed for iber)
				else if (template.hasClass("GarrisonFortress") && ent.genericName() == "House")
					placement.addInfluence(x, z, 30, -50);
				else if (template.hasClass("Military"))
					placement.addInfluence(x, z, 10, -40);
			});
		}

		if (template.hasClass("Farmstead"))
		{
			for (var j = 0; j < placement.map.length; ++j)
			{
				var value = placement.map[j] - (gameState.sharedScript.resourceMaps["wood"].map[j])/3;
				placement.map[j] = value >= 0 ? value : 0;
				if (gameState.ai.HQ.borderMap.map[j] > 0)
					placement.map[j] /= 2;	// we need space around farmstead, so disfavor map border
			}
		}
	}

	// requires to be inside our territory, and inside our base territory if required
	// and if our first market, put it on border if possible to maximize distance with next market
	var favorBorder = template.hasClass("BarterMarket");
	var disfavorBorder = (gameState.currentPhase() > 1 && 
		(!template.getDefaultArrow() && !template.getArrowMultiplier()));
	var preferredBase = (this.metadata && this.metadata.preferredBase);
	if (this.metadata && this.metadata.base !== undefined)
	{
		var base = this.metadata.base;
		for (var j = 0; j < placement.map.length; ++j)
		{
			if (gameState.ai.HQ.basesMap.map[j] != base)
				placement.map[j] = 0;
			else if (favorBorder && gameState.ai.HQ.borderMap.map[j] > 0)
				placement.map[j] += 50;
			else if (disfavorBorder && gameState.ai.HQ.borderMap.map[j] == 0 && placement.map[j] > 0)
				placement.map[j] += 10;

			if (placement.map[j] > 0)
			{
				var x = (j % placement.width + 0.5) * cellSize;
				var z = (Math.floor(j / placement.width) + 0.5) * cellSize;
				if (gameState.ai.HQ.isDangerousLocation([x, z]))
					placement.map[j] = 0;
			}
		}
	}
	else
	{
		for (var j = 0; j < placement.map.length; ++j)
		{
			if (gameState.ai.HQ.basesMap.map[j] == 0)
				placement.map[j] = 0;
			else if (favorBorder && gameState.ai.HQ.borderMap.map[j] > 0)
				placement.map[j] += 50;
			else if (disfavorBorder && gameState.ai.HQ.borderMap.map[j] == 0 && placement.map[j] > 0)
				placement.map[j] += 10;

			if (preferredBase && gameState.ai.HQ.basesMap.map[j] == this.metadata.preferredBase)
				placement.map[j] += 200;

			if (placement.map[j] > 0)
			{
				var x = (j % placement.width + 0.5) * cellSize;
				var z = (Math.floor(j / placement.width) + 0.5) * cellSize;
				if (gameState.ai.HQ.isDangerousLocation([x, z]))
					placement.map[j] = 0;
			}
		}
	}

	// Find the best non-obstructed:	
	// Find target building's approximate obstruction radius, and expand by a bit to make sure we're not too close,
	// this allows room for units to walk between buildings.
	// note: not for houses and dropsites who ought to be closer to either each other or a resource.
	// also not for fields who can be stacked quite a bit

	var obstructions = m.createObstructionMap(gameState, 0, template);
	//obstructions.dumpIm(template.buildCategory() + "_obstructions.png");

	var radius = 0;
	if (template.hasClass("Fortress") || this.type === gameState.applyCiv("structures/{civ}_siege_workshop")
		|| this.type === gameState.applyCiv("structures/{civ}_elephant_stables"))
		radius = Math.floor((template.obstructionRadius() + 12) / obstructions.cellSize);
	else if (template.resourceDropsiteTypes() === undefined && !template.hasClass("House") && !template.hasClass("Field"))
		radius = Math.ceil((template.obstructionRadius() + 4) / obstructions.cellSize);
	else
		radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);

	if (template.hasClass("House") && !alreadyHasHouses)
	{
		// try to get some space to place several houses first
		var bestTile = placement.findBestTile(3*radius, obstructions);
		var bestIdx = bestTile[0];
		var bestVal = bestTile[1];
	}
	
	if (bestVal === undefined || bestVal == -1)
	{
		var bestTile = placement.findBestTile(radius, obstructions);
		var bestIdx = bestTile[0];
		var bestVal = bestTile[1];
	}

	if (bestVal <= 0)
		return false;

	var x = ((bestIdx % obstructions.width) + 0.5) * obstructions.cellSize;
	var z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;
	var xx = x;
	var zz = z;

	if (template.hasClass("House") || template.hasClass("Field") || template.resourceDropsiteTypes() !== undefined)
	{
		if (obstructions.cellSize != 4)  // new pathFinder branch
		{
			let secondBest = obstructions.findNearestObstructed(bestIdx, radius);
			if (secondBest >= 0)
			{
				x = ((secondBest % obstructions.width) + 0.5) * obstructions.cellSize;
				z = (Math.floor(secondBest / obstructions.width) + 0.5) * obstructions.cellSize;
				xx = x;
				zz = z;
			}
		}
		else
		{
			obstructions.expandInfluences();
			let secondBest = obstructions.findLowestNeighbor(x,z);
			xx = secondBest[0];
			zz = secondBest[1];
		}
	}

	let territorypos = placement.gamePosToMapPos([x,z]);
	let territoryIndex = territorypos[0] + territorypos[1]*placement.width;
	// default angle = 3*Math.PI/4;
	return { "x": x, "z": z, "angle": 3*Math.PI/4, "xx": xx, "zz": zz, "base": gameState.ai.HQ.basesMap.map[territoryIndex] };
};

/**
 * Placement of buildings with Dock build category
 * metadata.proximity is defined when first dock without any territory
 */
m.ConstructionPlan.prototype.findDockPosition = function(gameState)
{
	var template = this.template;

	var territoryMap = gameState.ai.HQ.territoryMap;

	var obstructions = m.createObstructionMap(gameState, 0, template);
	//obstructions.dumpIm(template.buildCategory() + "_obstructions.png");

	var bestIdx = undefined;
	var bestJdx = undefined;
	var bestAngle = undefined;
	var bestLand = undefined;
	var bestVal = -1;
	var landPassMap = gameState.ai.accessibility.landPassMap;
	var navalPassMap = gameState.ai.accessibility.navalPassMap;

	var width = gameState.ai.HQ.territoryMap.width;
	var cellSize = gameState.ai.HQ.territoryMap.cellSize;
	var nbShips = gameState.ai.HQ.navalManager.transportShips.length;
	var proxyAccess = undefined;
	if (this.metadata.proximity)
		proxyAccess = gameState.ai.accessibility.getAccessValue(this.metadata.proximity);

	var radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);

	var halfSize = 0;    // used for dock angle
	var halfDepth = 0;   // used by checkPlacement
	var halfWidth = 0;   // used by checkPlacement
	if (template.get("Footprint/Square"))
	{
		halfSize = Math.max(+template.get("Footprint/Square/@depth"), +template.get("Footprint/Square/@width")) / 2;
		halfDepth = +template.get("Footprint/Square/@depth") / 2;
		halfWidth = +template.get("Footprint/Square/@width") / 2;
	}
	else if (template.get("Footprint/Circle"))
	{
		halfSize = +template.get("Footprint/Circle/@radius");
		halfDepth = halfSize;
		halfWidth = halfSize;
	}

	var maxres = 10;
	for (let j = 0; j < territoryMap.length; ++j)
	{
		var i = territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;

		var landAccess = this.getLandAccess(gameState, i, radius+1, obstructions.width);
		if (landAccess.size == 0)
			continue;
		if (this.metadata)
		{
			if (this.metadata.land && !landAccess.has(+this.metadata.land))
				continue;
			if (this.metadata.sea && navalPassMap[i] != +this.metadata.sea)
				continue;
			if (nbShips === 0 && proxyAccess && proxyAccess > 1 && !landAccess.has(proxyAccess))
				continue;
		}

		var res = Math.min(maxres, this.getResourcesAround(gameState, j, 80));

		var dist;
		if (this.metadata.proximity)
		{
			// if proximity is given, we look for the nearest point
			var pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
			dist = API3.SquareVectorDistance(this.metadata.proximity, pos);
			dist = Math.sqrt(dist) + 15 * (maxres - res);
		}
		else
		{
			// if not in our (or allied) territory, we do not want it too far to be able to defend it
			dist = m.getFrontierProximity(gameState, j);
			if (dist > 4)
				continue;
			dist = dist + 0.4 * (maxres - res)
		}
		if (bestIdx !== undefined && dist > bestVal)
			continue;

		var x = ((i % obstructions.width) + 0.5) * obstructions.cellSize;
		var z = (Math.floor(i / obstructions.width) + 0.5) * obstructions.cellSize;
		var angle = this.getDockAngle(gameState, x, z, halfSize);
		if (angle === false)
			continue;
		var land = this.checkDockPlacement(gameState, x, z, halfDepth, halfWidth, angle);
		if (land < 2 || !gameState.ai.HQ.landRegions[land])
			continue;
		if (this.metadata.proximity && gameState.ai.accessibility.regionSize[land] < 4000)
			continue;

		bestVal = dist;
		bestIdx = i;
		bestJdx = j;
		bestAngle = angle;
		bestLand = land;
	}

	if (bestVal < 0)
		return false;

	var x = ((bestIdx % obstructions.width) + 0.5) * obstructions.cellSize;
	var z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;

	// Assign this dock to a base
	var baseIndex = gameState.ai.HQ.basesMap.map[bestJdx];
	if (!baseIndex)
	{
		for (let base of gameState.ai.HQ.baseManagers)
		{
			if (!base.anchor || !base.anchor.position())
				continue;
			if (base.accessIndex !== bestLand)
				continue;
			baseIndex = base.ID;
			break;
		}
		if (!baseIndex)
		{
			if (gameState.ai.HQ.numActiveBase() > 0)
				API3.warn("Petra: dock constructed without base index " + baseIndex);
			else
				baseIndex = gameState.ai.HQ.baseManagers[0].ID;
		}
	}

	return { "x": x, "z": z, "angle": bestAngle, "xx": x, "zz": z, "base": baseIndex, "access": bestLand };
};

// Algorithm taken from the function GetDockAngle in simulation/helpers/Commands.js
m.ConstructionPlan.prototype.getDockAngle = function(gameState, x, z, size)
{
	var pos = gameState.ai.accessibility.gamePosToMapPos([x, z]);
	var j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	var seaRef = gameState.ai.accessibility.navalPassMap[j];
	if (seaRef < 2)
		return false;
	const numPoints = 16;
	for (var dist = 0; dist < 4; ++dist)
	{
		var waterPoints = [];
		for (var i = 0; i < numPoints; ++i)
		{
			let angle = (i/numPoints)*2*Math.PI;
			pos = [x - (1+dist)*size*Math.sin(angle), z + (1+dist)*size*Math.cos(angle)];
			pos = gameState.ai.accessibility.gamePosToMapPos(pos);
			let j = pos[0] + pos[1]*gameState.ai.accessibility.width;
			if (gameState.ai.accessibility.navalPassMap[j] == seaRef)
				waterPoints.push(i);
		}
		var length = waterPoints.length;
		if (!length)
			continue;
		var consec = [];
		for (var i = 0; i < length; ++i)
		{
			var count = 0;
			for (var j = 0; j < (length-1); ++j)
			{
				if (((waterPoints[(i + j) % length]+1) % numPoints) == waterPoints[(i + j + 1) % length])
					++count;
				else
					break;
			}
			consec[i] = count;
		}
		var start = 0;
		var count = 0;
		for (var c in consec)
		{
			if (consec[c] > count)
			{
				start = c;
				count = consec[c];
			}
		}
		
		// If we've found a shoreline, stop searching
		if (count != numPoints-1)
			return -((waterPoints[start] + consec[start]/2) % numPoints)/numPoints*2*Math.PI;
	}
	return false;
};

// Algorithm taken from checkPlacement in simulation/components/BuildRestriction.js
// to determine the special dock requirements
m.ConstructionPlan.prototype.checkDockPlacement = function(gameState, x, z, halfDepth, halfWidth, angle)
{
	let sz = halfDepth * Math.sin(angle);
	let cz = halfDepth * Math.cos(angle);
	// center back position
	let pos = gameState.ai.accessibility.gamePosToMapPos([x - sz, z - cz]);
	let j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	let ret = gameState.ai.accessibility.landPassMap[j];
	if (ret < 2)
		return 0;
	// center front position
	pos = gameState.ai.accessibility.gamePosToMapPos([x + sz, z + cz]);
	j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	if (gameState.ai.accessibility.landPassMap[j] > 1 || gameState.ai.accessibility.navalPassMap[j] < 2)
		return 0;
	// additional constraints compared to BuildRestriction.js to assure we have enough place to build
	let sw = halfWidth * Math.cos(angle) * 3 / 4;
	let cw = halfWidth * Math.sin(angle) * 3 / 4;
	pos = gameState.ai.accessibility.gamePosToMapPos([x - sz + sw, z - cz - cw]);
	j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	if (gameState.ai.accessibility.landPassMap[j] != ret)
		return 0;
	pos = gameState.ai.accessibility.gamePosToMapPos([x - sz - sw, z - cz + cw]);
	j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	if (gameState.ai.accessibility.landPassMap[j] != ret)
		return 0;
	return ret;
};

// get the list of all the land access from this position
m.ConstructionPlan.prototype.getLandAccess = function(gameState, i, radius, w)
{
	var access = new Set();
	var landPassMap = gameState.ai.accessibility.landPassMap;
	var kx = i % w;
	var ky = Math.floor(i / w);
	var land;
	for (let dy = 0; dy <= radius; ++dy)
	{
		let dxmax = radius - dy;
		let xp = kx + (ky + dy)*w;
		let xm = kx + (ky - dy)*w;
		for (let dx = -dxmax; dx <= dxmax; ++dx)
		{
			if (kx + dx < 0 || kx + dx >= w)
				continue;
			if (ky + dy >= 0 && ky + dy < w)
			{
				land = landPassMap[xp + dx];
				if (land > 1 && !access.has(land))
					access.add(land);
			}
			if (ky - dy >= 0 && ky - dy < w)
			{
				land = landPassMap[xm + dx];
				if (land > 1 && !access.has(land))
					access.add(land);
			}
		}
	}
	return access;
};

// get the sum of the resources (except food) around, inside a given radius
// resources have a weight (1 if dist=0 and 0 if dist=size) doubled for wood
m.ConstructionPlan.prototype.getResourcesAround = function(gameState, i, radius)
{
	let resourceMaps = gameState.sharedScript.resourceMaps;
	let w = resourceMaps["wood"].width;
	let cellSize = resourceMaps["wood"].cellSize;
	let size = Math.floor(radius / cellSize);
	let ix = i % w;
	let iy = Math.floor(i / w);
	let total = 0;
	let nbcell = 0;
	for (let k in resourceMaps)
	{
		if (k === "food")
			continue;
		let weigh0 = (k === "wood") ? 2 : 1;
		for (let dy = 0; dy <= size; ++dy)
		{
			let dxmax = size - dy;
			let ky = iy + dy;
			if (ky >= 0 && ky < w)
			{
				for (let dx = -dxmax; dx <= dxmax; ++dx)
				{
					let kx = ix + dx;
					if (kx < 0 || kx >= w)
						continue;
					let ddx = (dx > 0) ? dx : -dx;
					let weight = weigh0 * (dxmax - ddx) / size;
					total += weight * resourceMaps[k].map[kx + w * ky];
					nbcell += weight;
				}
			}
			if (dy == 0)
				continue;
			ky = iy - dy;
			if (ky  >= 0 && ky < w)
			{
				for (let dx = -dxmax; dx <= dxmax; ++dx)
				{
					let kx = ix + dx;
					if (kx < 0 || kx >= w)
						continue;
					let ddx = (dx > 0) ? dx : -dx;
					let weight = weigh0 * (dxmax - ddx) / size;
					total += weight * resourceMaps[k].map[kx + w * ky];
					nbcell += weight;
				}
			}
		}
	}
	return (nbcell ? (total / nbcell) : 0);
};

m.ConstructionPlan.prototype.Serialize = function()
{
	let prop = {
		"category": this.category,
		"type": this.type,
		"ID": this.ID,
		"metadata": this.metadata,
		"cost": this.cost.Serialize(),
		"number": this.number,
		"position": this.position,
		"lastIsGo": this.lastIsGo,
	};

	let func = {
		"isGo": uneval(this.isGo),
		"onStart": uneval(this.onStart)
	};

	return { "prop": prop, "func": func };
};

m.ConstructionPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.prop)
		this[key] = data.prop[key];

	let cost = new API3.Resources();
	cost.Deserialize(data.prop.cost);
	this.cost = cost;

	for (let fun in data.func)
		this[fun] = eval(data.func[fun]);
};

return m;
}(PETRA);
