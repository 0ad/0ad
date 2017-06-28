var PETRA = function(m)
{

/**
 * Defines a construction plan, ie a building.
 * We'll try to fing a good position if non has been provided
 */

m.ConstructionPlan = function(gameState, type, metadata, position)
{
	if (!m.QueuePlan.call(this, gameState, type, metadata))
		return false;

	this.position = position ? position : 0;

	this.category = "building";

	return true;
};

m.ConstructionPlan.prototype = Object.create(m.QueuePlan.prototype);

m.ConstructionPlan.prototype.canStart = function(gameState)
{
	if (gameState.ai.HQ.turnCache.buildingBuilt)   // do not start another building if already one this turn
		return false;

	if (!this.isGo(gameState))
		return false;

	if (this.template.requiredTech() && !gameState.isResearched(this.template.requiredTech()))
		return false;

	return gameState.findBuilder(this.type) !== undefined;
};

m.ConstructionPlan.prototype.start = function(gameState)
{
	Engine.ProfileStart("Building construction start");

	// We don't care which builder we assign, since they won't actually
	// do the building themselves - all we care about is that there is
	// at least one unit that can start the foundation
	let builder = gameState.findBuilder(this.type);

	let pos = this.findGoodPosition(gameState);
	if (!pos)
	{
		gameState.ai.HQ.stopBuild(gameState, this.type);
		Engine.ProfileStop();
		return;
	}

	if (this.metadata && this.metadata.expectedGain)
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
	gameState.ai.HQ.turnCache.buildingBuilt = true;

	if (this.metadata === undefined)
		this.metadata = { "base": pos.base };
	else if (this.metadata.base === undefined)
		this.metadata.base = pos.base;

	if (pos.access)
		this.metadata.access = pos.access;   // needed for Docks whose position is on water
	else
		this.metadata.access = gameState.ai.accessibility.getAccessValue([pos.x, pos.z]);

	if (this.template.buildPlacementType() === "shore")
	{
		// adjust a bit the position if needed
		let cosa = Math.cos(pos.angle);
		let sina = Math.sin(pos.angle);
		let shiftMax = gameState.ai.HQ.territoryMap.cellSize;
		for (let shift = 0; shift <= shiftMax; shift += 2)
		{
			builder.construct(this.type, pos.x-shift*sina, pos.z-shift*cosa, pos.angle, this.metadata);
			if (shift > 0)
				builder.construct(this.type, pos.x+shift*sina, pos.z+shift*cosa, pos.angle, this.metadata);
		}
	}
	else if (pos.xx === undefined || (pos.x == pos.xx && pos.z == pos.zz))
		builder.construct(this.type, pos.x, pos.z, pos.angle, this.metadata);
	else // try with the lowest, move towards us unless we're same
	{
		for (let step = 0; step <= 1; step += 0.2)
			builder.construct(this.type, step*pos.x + (1-step)*pos.xx, step*pos.z + (1-step)*pos.zz,
				pos.angle, this.metadata);
	}
	this.onStart(gameState);
	Engine.ProfileStop();

	if (this.metadata && this.metadata.proximity)
		gameState.ai.HQ.navalManager.createTransportIfNeeded(gameState, this.metadata.proximity, [pos.x, pos.z], this.metadata.access);
};

m.ConstructionPlan.prototype.findGoodPosition = function(gameState)
{
	let template = this.template;

	if (template.buildPlacementType() === "shore")
		return this.findDockPosition(gameState);

	if (template.hasClass("Storehouse") && this.metadata.base)
	{
		// recompute the best dropsite location in case some conditions have changed
		let base = gameState.ai.HQ.getBaseByID(this.metadata.base);
		let type = this.metadata.type ? this.metadata.type : "wood";
		let newpos = base.findBestDropsiteLocation(gameState, type);
		if (newpos && newpos.quality > 0)
		{
			let pos = newpos.pos;
			return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "base": this.metadata.base };
		}
	}

	if (!this.position)
	{
		if (template.hasClass("CivCentre"))
		{
			let pos;
			if (this.metadata && this.metadata.resource)
			{
				let proximity = this.metadata.proximity ? this.metadata.proximity : undefined;
				pos = gameState.ai.HQ.findEconomicCCLocation(gameState, template, this.metadata.resource, proximity);
			}
			else
				pos = gameState.ai.HQ.findStrategicCCLocation(gameState, template);

			if (pos)
				return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "base": 0 };
			return false;
		}
		else if (template.hasClass("DefenseTower") || template.hasClass("Fortress") || template.hasClass("ArmyCamp"))
		{
			let pos = gameState.ai.HQ.findDefensiveLocation(gameState, template);
			if (pos)
				return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "base": pos[2] };

			if (template.hasClass("DefenseTower") || gameState.getPlayerCiv() === "mace" || gameState.getPlayerCiv() === "maur" ||
				gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_fortress"), true) > 0 ||
				gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_army_camp"), true) > 0)
				return false;
			// if this fortress is our first siege unit builder, just try the standard placement as we want siege units
		}
		else if (template.hasClass("Market"))	// Docks (i.e. NavalMarket) are done before
		{
			let pos = gameState.ai.HQ.findMarketLocation(gameState, template);
			if (pos && pos[2] > 0)
			{
				if (!this.metadata)
					this.metadata = {};
				this.metadata.expectedGain = pos[3];
				return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "base": pos[2] };
			}
			else if (!pos)
				return false;
		}
	}

	// Compute each tile's closeness to friendly structures:

	let placement = new API3.Map(gameState.sharedScript, "territory");
	let cellSize = placement.cellSize; // size of each tile

	let alreadyHasHouses = false;

	if (this.position)	// If a position was specified then place the building as close to it as possible
	{
		let x = Math.floor(this.position[0] / cellSize);
		let z = Math.floor(this.position[1] / cellSize);
		placement.addInfluence(x, z, 255);
	}
	else	// No position was specified so try and find a sensible place to build
	{
		// give a small > 0 level as the result of addInfluence is constrained to be > 0
		// if we really need houses (i.e. townPhasing without enough village building), do not apply these constraints
		if (this.metadata && this.metadata.base !== undefined)
		{
			let base = this.metadata.base;
			for (let j = 0; j < placement.map.length; ++j)
				if (gameState.ai.HQ.basesMap.map[j] == base)
					placement.map[j] = 45;
		}
		else
		{
			for (let j = 0; j < placement.map.length; ++j)
				if (gameState.ai.HQ.basesMap.map[j] !== 0)
					placement.map[j] = 45;
		}

		if (!gameState.ai.HQ.requireHouses || !template.hasClass("House"))
		{
			gameState.getOwnStructures().forEach(function(ent) {
				let pos = ent.position();
				let x = Math.round(pos[0] / cellSize);
				let z = Math.round(pos[1] / cellSize);

				if (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf("food") !== -1)
				{
					if (template.hasClass("Field") || template.hasClass("Corral"))
						placement.addInfluence(x, z, 80/cellSize, 50);
					else // If this is not a field add a negative influence because we want to leave this area for fields
						placement.addInfluence(x, z, 80/cellSize, -20);
				}
				else if (template.hasClass("House"))
				{
					if (ent.hasClass("House"))
					{
						placement.addInfluence(x, z, 60/cellSize, 40);    // houses are close to other houses
						alreadyHasHouses = true;
					}
					else if (!ent.hasClass("StoneWall") || ent.hasClass("Gates"))
						placement.addInfluence(x, z, 60/cellSize, -40);   // and further away from other stuffs
				}
				else if (template.hasClass("Farmstead") && (!ent.hasClass("Field") && !ent.hasClass("Corral") &&
					(!ent.hasClass("StoneWall") || ent.hasClass("Gates"))))
					placement.addInfluence(x, z, 100/cellSize, -25);       // move farmsteads away to make room (StoneWall test needed for iber)
				else if (template.hasClass("GarrisonFortress") && ent.genericName() == "House")
					placement.addInfluence(x, z, 120/cellSize, -50);
				else if (template.hasClass("Military"))
					placement.addInfluence(x, z, 40/cellSize, -40);
				else if (template.genericName() === "Rotary Mill" && ent.hasClass("Field"))
					placement.addInfluence(x, z, 60/cellSize, 40);
			});
		}
		if (template.hasClass("Farmstead"))
		{
			for (let j = 0; j < placement.map.length; ++j)
			{
				let value = placement.map[j] - gameState.sharedScript.resourceMaps.wood.map[j]/3;
				placement.map[j] = value >= 0 ? value : 0;
				if (gameState.ai.HQ.borderMap.map[j] & m.fullBorder_Mask)
					placement.map[j] /= 2;	// we need space around farmstead, so disfavor map border
			}
		}
	}

	// Requires to be inside our territory, and inside our base territory if required
	// and if our first market, put it on border if possible to maximize distance with next market
	let favorBorder = template.hasClass("BarterMarket");
	let disfavorBorder = gameState.currentPhase() > 1 && !template.hasDefensiveFire();
	let preferredBase = this.metadata && this.metadata.preferredBase;
	if (this.metadata && this.metadata.base !== undefined)
	{
		let base = this.metadata.base;
		for (let j = 0; j < placement.map.length; ++j)
		{
			if (gameState.ai.HQ.basesMap.map[j] != base)
				placement.map[j] = 0;
			else if (favorBorder && gameState.ai.HQ.borderMap.map[j] & m.border_Mask)
				placement.map[j] += 50;
			else if (disfavorBorder && !(gameState.ai.HQ.borderMap.map[j] & m.fullBorder_Mask) && placement.map[j] > 0)
				placement.map[j] += 10;

			if (placement.map[j] > 0)
			{
				let x = (j % placement.width + 0.5) * cellSize;
				let z = (Math.floor(j / placement.width) + 0.5) * cellSize;
				if (gameState.ai.HQ.isNearInvadingArmy([x, z]))
					placement.map[j] = 0;
			}
		}
	}
	else
	{
		for (let j = 0; j < placement.map.length; ++j)
		{
			if (gameState.ai.HQ.basesMap.map[j] === 0)
				placement.map[j] = 0;
			else if (favorBorder && gameState.ai.HQ.borderMap.map[j] & m.border_Mask)
				placement.map[j] += 50;
			else if (disfavorBorder && !(gameState.ai.HQ.borderMap.map[j] & m.fullBorder_Mask) && placement.map[j] > 0)
				placement.map[j] += 10;

			if (preferredBase && gameState.ai.HQ.basesMap.map[j] == this.metadata.preferredBase)
				placement.map[j] += 200;

			if (placement.map[j] > 0)
			{
				let x = (j % placement.width + 0.5) * cellSize;
				let z = (Math.floor(j / placement.width) + 0.5) * cellSize;
				if (gameState.ai.HQ.isNearInvadingArmy([x, z]))
					placement.map[j] = 0;
			}
		}
	}

	// Find the best non-obstructed:
	// Find target building's approximate obstruction radius, and expand by a bit to make sure we're not too close,
	// this allows room for units to walk between buildings.
	// note: not for houses and dropsites who ought to be closer to either each other or a resource.
	// also not for fields who can be stacked quite a bit

	let obstructions = m.createObstructionMap(gameState, 0, template);
	//obstructions.dumpIm(template.buildPlacementType() + "_obstructions.png");

	let radius = 0;
	if (template.hasClass("Fortress") || this.type === gameState.applyCiv("structures/{civ}_siege_workshop") ||
		this.type === gameState.applyCiv("structures/{civ}_elephant_stables"))
		radius = Math.floor((template.obstructionRadius() + 12) / obstructions.cellSize);
	else if (template.resourceDropsiteTypes() === undefined && !template.hasClass("House") && !template.hasClass("Field"))
		radius = Math.ceil((template.obstructionRadius() + 4) / obstructions.cellSize);
	else
		radius = Math.ceil((template.obstructionRadius() + 0.5) / obstructions.cellSize);

	let bestTile;
	let bestVal;
	if (template.hasClass("House") && !alreadyHasHouses)
	{
		// try to get some space to place several houses first
		bestTile = placement.findBestTile(3*radius, obstructions);
		bestVal = bestTile[1];
	}

	if (bestVal === undefined || bestVal === -1)
	{
		bestTile = placement.findBestTile(radius, obstructions);
		bestVal = bestTile[1];
	}
	let bestIdx = bestTile[0];

	if (bestVal <= 0)
		return false;

	let x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	let z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;

	if (template.hasClass("House") || template.hasClass("Field") || template.resourceDropsiteTypes() !== undefined)
	{
		let secondBest = obstructions.findNearestObstructed(bestIdx, radius);
		if (secondBest >= 0)
		{
			x = (secondBest % obstructions.width + 0.5) * obstructions.cellSize;
			z = (Math.floor(secondBest / obstructions.width) + 0.5) * obstructions.cellSize;
		}
	}

	let territorypos = placement.gamePosToMapPos([x, z]);
	let territoryIndex = territorypos[0] + territorypos[1]*placement.width;
	// default angle = 3*Math.PI/4;
	return { "x": x, "z": z, "angle": 3*Math.PI/4, "base": gameState.ai.HQ.basesMap.map[territoryIndex] };
};

/**
 * Placement of buildings with Dock build category
 * metadata.proximity is defined when first dock without any territory
 */
m.ConstructionPlan.prototype.findDockPosition = function(gameState)
{
	let template = this.template;
	let territoryMap = gameState.ai.HQ.territoryMap;

	let obstructions = m.createObstructionMap(gameState, 0, template);
	//obstructions.dumpIm(template.buildPlacementType() + "_obstructions.png");

	let bestIdx;
	let bestJdx;
	let bestAngle;
	let bestLand;
	let bestWater;
	let bestVal = -1;
	let navalPassMap = gameState.ai.accessibility.navalPassMap;

	let width = gameState.ai.HQ.territoryMap.width;
	let cellSize = gameState.ai.HQ.territoryMap.cellSize;

	let nbShips = gameState.ai.HQ.navalManager.transportShips.length;
	let wantedLand = this.metadata && this.metadata.land ? this.metadata.land : null;
	let wantedSea = this.metadata && this.metadata.sea ? this.metadata.sea : null;
	let proxyAccess = this.metadata && this.metadata.proximity ? gameState.ai.accessibility.getAccessValue(this.metadata.proximity) : null;
	if (nbShips === 0 && proxyAccess && proxyAccess > 1)
	{
		wantedLand = {};
		wantedLand[proxyAccess] = true;
	}
	let dropsiteTypes = template.resourceDropsiteTypes();
	let radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);

	let halfSize = 0;    // used for dock angle
	let halfDepth = 0;   // used by checkPlacement
	let halfWidth = 0;   // used by checkPlacement
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

	// res is a measure of the amount of resources around, and maxRes is the max value taken into account
	// water is a measure of the water space around, and maxWater is the max value that can be returned by checkDockPlacement
	const maxRes = 10;
	const maxWater = 16;
	for (let j = 0; j < territoryMap.length; ++j)
	{
		if (!this.isDockLocation(gameState, j, halfDepth, wantedLand, wantedSea))
			continue;
		let dist;
		if (!proxyAccess)
		{
			// if not in our (or allied) territory, we do not want it too far to be able to defend it
			dist = this.getFrontierProximity(gameState, j);
			if (dist > 4)
				continue;
		}
		let i = territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;
		if (wantedSea && navalPassMap[i] !== wantedSea)
			continue;

		let res = dropsiteTypes ? Math.min(maxRes, this.getResourcesAround(gameState, dropsiteTypes, j, 80)) : maxRes;
		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		if (proxyAccess)
		{
			// if proximity is given, we look for the nearest point
			dist = API3.SquareVectorDistance(this.metadata.proximity, pos);
			dist = Math.sqrt(dist) + 20 * (maxRes - res);
		}
		else
			dist += 0.6 * (maxRes - res);

		// Add a penalty if on the map border as ship movement will be difficult
		if (gameState.ai.HQ.borderMap.map[j] & m.fullBorder_Mask)
			dist += 2;
		// do a pre-selection, supposing we will have the best possible water
		if (bestIdx !== undefined && dist > bestVal + maxWater)
			continue;

		let x = (i % obstructions.width + 0.5) * obstructions.cellSize;
		let z = (Math.floor(i / obstructions.width) + 0.5) * obstructions.cellSize;
		let angle = this.getDockAngle(gameState, x, z, halfSize);
		if (angle === false)
			continue;
		let ret = this.checkDockPlacement(gameState, x, z, halfDepth, halfWidth, angle);
		if (!ret || !gameState.ai.HQ.landRegions[ret.land])
			continue;
		// final selection now that the checkDockPlacement water is known
		if (bestIdx !== undefined && dist + maxWater - ret.water > bestVal)
			continue;
		if (this.metadata.proximity && gameState.ai.accessibility.regionSize[ret.land] < 4000)
			continue;
		if (gameState.ai.HQ.isDangerousLocation(gameState, pos, halfSize))
			continue;

		bestVal = dist + maxWater - ret.water;
		bestIdx = i;
		bestJdx = j;
		bestAngle = angle;
		bestLand = ret.land;
		bestWater = ret.water;
	}

	if (bestVal < 0)
		return false;

	// if no good place with enough water around and still in first phase, wait for expansion at the next phase
	if (!this.metadata.proximity && bestWater < 10 && gameState.currentPhase() == 1)
		return false;

	let x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	let z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;

	// Assign this dock to a base
	let baseIndex = gameState.ai.HQ.basesMap.map[bestJdx];
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

	return { "x": x, "z": z, "angle": bestAngle, "base": baseIndex, "access": bestLand };
};

/** Algorithm taken from the function GetDockAngle in simulation/helpers/Commands.js */
m.ConstructionPlan.prototype.getDockAngle = function(gameState, x, z, size)
{
	let pos = gameState.ai.accessibility.gamePosToMapPos([x, z]);
	let k = pos[0] + pos[1]*gameState.ai.accessibility.width;
	let seaRef = gameState.ai.accessibility.navalPassMap[k];
	if (seaRef < 2)
		return false;
	const numPoints = 16;
	for (let dist = 0; dist < 4; ++dist)
	{
		let waterPoints = [];
		for (let i = 0; i < numPoints; ++i)
		{
			let angle = 2 * Math.PI * i / numPoints;
			pos = [x - (1+dist)*size*Math.sin(angle), z + (1+dist)*size*Math.cos(angle)];
			pos = gameState.ai.accessibility.gamePosToMapPos(pos);
			if (pos[0] < 0 || pos[0] >= gameState.ai.accessibility.width ||
			    pos[1] < 0 || pos[1] >= gameState.ai.accessibility.height)
				continue;
			let j = pos[0] + pos[1]*gameState.ai.accessibility.width;
			if (gameState.ai.accessibility.navalPassMap[j] === seaRef)
				waterPoints.push(i);
		}
		let length = waterPoints.length;
		if (!length)
			continue;
		let consec = [];
		for (let i = 0; i < length; ++i)
		{
			let count = 0;
			for (let j = 0; j < length-1; ++j)
			{
				if ((waterPoints[(i + j) % length]+1) % numPoints == waterPoints[(i + j + 1) % length])
					++count;
				else
					break;
			}
			consec[i] = count;
		}
		let start = 0;
		let count = 0;
		for (let c in consec)
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

/**
 * Algorithm taken from checkPlacement in simulation/components/BuildRestriction.js
 * to determine the special dock requirements
 * returns {"land": land index for this dock, "water": amount of water around this spot}
 */
m.ConstructionPlan.prototype.checkDockPlacement = function(gameState, x, z, halfDepth, halfWidth, angle)
{
	let sz = halfDepth * Math.sin(angle);
	let cz = halfDepth * Math.cos(angle);
	// center back position
	let pos = gameState.ai.accessibility.gamePosToMapPos([x - sz, z - cz]);
	let j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	let land = gameState.ai.accessibility.landPassMap[j];
	if (land < 2)
		return null;
	// center front position
	pos = gameState.ai.accessibility.gamePosToMapPos([x + sz, z + cz]);
	j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	if (gameState.ai.accessibility.landPassMap[j] > 1 || gameState.ai.accessibility.navalPassMap[j] < 2)
		return null;
	// additional constraints compared to BuildRestriction.js to assure we have enough place to build
	let sw = halfWidth * Math.cos(angle) * 3 / 4;
	let cw = halfWidth * Math.sin(angle) * 3 / 4;
	pos = gameState.ai.accessibility.gamePosToMapPos([x - sz + sw, z - cz - cw]);
	j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	if (gameState.ai.accessibility.landPassMap[j] != land)
		return null;
	pos = gameState.ai.accessibility.gamePosToMapPos([x - sz - sw, z - cz + cw]);
	j = pos[0] + pos[1]*gameState.ai.accessibility.width;
	if (gameState.ai.accessibility.landPassMap[j] != land)
		return null;
	let water = 0;
 	let sp = 15 * Math.sin(angle);
	let cp = 15 * Math.cos(angle);
	for (let i = 1; i < 5; ++i)
	{
		pos = gameState.ai.accessibility.gamePosToMapPos([x + sz + i*(sp+sw), z + cz + i*(cp-cw)]);
		if (pos[0] < 0 || pos[0] >= gameState.ai.accessibility.width ||
		    pos[1] < 0 || pos[1] >= gameState.ai.accessibility.height)
			break;
		j = pos[0] + pos[1]*gameState.ai.accessibility.width;
		if (gameState.ai.accessibility.landPassMap[j] > 1 || gameState.ai.accessibility.navalPassMap[j] < 2)
			break;
		pos = gameState.ai.accessibility.gamePosToMapPos([x + sz + i*sp, z + cz + i*cp]);
		if (pos[0] < 0 || pos[0] >= gameState.ai.accessibility.width ||
		    pos[1] < 0 || pos[1] >= gameState.ai.accessibility.height)
			break;
		j = pos[0] + pos[1]*gameState.ai.accessibility.width;
		if (gameState.ai.accessibility.landPassMap[j] > 1 || gameState.ai.accessibility.navalPassMap[j] < 2)
			break;
		pos = gameState.ai.accessibility.gamePosToMapPos([x + sz + i*(sp-sw), z + cz + i*(cp+cw)]);
		if (pos[0] < 0 || pos[0] >= gameState.ai.accessibility.width ||
		    pos[1] < 0 || pos[1] >= gameState.ai.accessibility.height)
			break;
		j = pos[0] + pos[1]*gameState.ai.accessibility.width;
		if (gameState.ai.accessibility.landPassMap[j] > 1 || gameState.ai.accessibility.navalPassMap[j] < 2)
			break;
		water += 4;
	}
	return {"land": land, "water": water};
};

/**
 * fast check if we can build a dock: returns false if nearest land is farther than the dock dimension
 * if the (object) wantedLand is given, this nearest land should have one of these accessibility
 * if wantedSea is given, this tile should be inside this sea
 */
const around = [ [ 1.0, 0.0], [ 0.87, 0.50], [ 0.50, 0.87], [ 0.0, 1.0], [-0.50, 0.87], [-0.87, 0.50],
                 [-1.0, 0.0], [-0.87,-0.50], [-0.50,-0.87], [ 0.0,-1.0], [ 0.50,-0.87], [ 0.87,-0.50] ];

m.ConstructionPlan.prototype.isDockLocation = function(gameState, j, dimension, wantedLand, wantedSea)
{
	let width = gameState.ai.HQ.territoryMap.width;
	let cellSize = gameState.ai.HQ.territoryMap.cellSize;
	let dist = dimension + 2*cellSize;

	let x = (j%width + 0.5) * cellSize;
	let z = (Math.floor(j/width) + 0.5) * cellSize;
	for (let a of around)
	{
		let pos = gameState.ai.accessibility.gamePosToMapPos([x + dist*a[0], z + dist*a[1]]);
		if (pos[0] < 0 || pos[0] >= gameState.ai.accessibility.width)
			continue;
		if (pos[1] < 0 || pos[1] >= gameState.ai.accessibility.height)
			continue;
		let k = pos[0] + pos[1]*gameState.ai.accessibility.width;
		let landPass = gameState.ai.accessibility.landPassMap[k];
		if (landPass < 2 || (wantedLand && !wantedLand[landPass]))
			continue;
		pos = gameState.ai.accessibility.gamePosToMapPos([x - dist*a[0], z - dist*a[1]]);
		if (pos[0] < 0 || pos[0] >= gameState.ai.accessibility.width)
			continue;
		if (pos[1] < 0 || pos[1] >= gameState.ai.accessibility.height)
			continue;
		k = pos[0] + pos[1]*gameState.ai.accessibility.width;
		if (wantedSea && gameState.ai.accessibility.navalPassMap[k] !== wantedSea)
			continue;
		else if (!wantedSea && gameState.ai.accessibility.navalPassMap[k] < 2)
			continue;
		return true;
	}

	return false;
};

/**
 * return a measure of the proximity to our frontier (including our allies)
 * 0=inside, 1=less than 24m, 2= less than 48m, 3= less than 72m, 4=less than 96m, 5=above 96m
 */
m.ConstructionPlan.prototype.getFrontierProximity = function(gameState, j)
{
	let alliedVictory = gameState.getAlliedVictory();
	let territoryMap = gameState.ai.HQ.territoryMap;
	let territoryOwner = territoryMap.getOwnerIndex(j);
	if (territoryOwner === PlayerID || alliedVictory && gameState.isPlayerAlly(territoryOwner))
		return 0;

	let borderMap = gameState.ai.HQ.borderMap;
	let width = territoryMap.width;
	let step = Math.round(24 / territoryMap.cellSize);
	let ix = j % width;
	let iz = Math.floor(j / width);
	let best = 5;
	for (let a of around)
	{
		for (let i = 1; i < 5; ++i)
		{
			let jx = ix + Math.round(i*step*a[0]);
			if (jx < 0 || jx >= width)
				continue;
			let jz = iz + Math.round(i*step*a[1]);
			if (jz < 0 || jz >= width)
				continue;
			if (borderMap.map[jx+width*jz] & m.outside_Mask)
				continue;
			territoryOwner = territoryMap.getOwnerIndex(jx+width*jz);
			if (alliedVictory && gameState.isPlayerAlly(territoryOwner) || territoryOwner === PlayerID)
			{
				best = Math.min(best, i);
				break;
			}
		}
		if (best === 1)
			break;
	}

	return best;
};

/**
 * get the sum of the resources (except food) around, inside a given radius
 * resources have a weight (1 if dist=0 and 0 if dist=size) doubled for wood
 */
m.ConstructionPlan.prototype.getResourcesAround = function(gameState, types, i, radius)
{
	let resourceMaps = gameState.sharedScript.resourceMaps;
	let w = resourceMaps.wood.width;
	let cellSize = resourceMaps.wood.cellSize;
	let size = Math.floor(radius / cellSize);
	let ix = i % w;
	let iy = Math.floor(i / w);
	let total = 0;
	let nbcell = 0;
	for (let k of types)
	{
		if (k === "food" || !resourceMaps[k])
			continue;
		let weigh0 = k === "wood" ? 2 : 1;
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
					let ddx = dx > 0 ? dx : -dx;
					let weight = weigh0 * (dxmax - ddx) / size;
					total += weight * resourceMaps[k].map[kx + w * ky];
					nbcell += weight;
				}
			}
			if (dy === 0)
				continue;
			ky = iy - dy;
			if (ky >= 0 && ky < w)
			{
				for (let dx = -dxmax; dx <= dxmax; ++dx)
				{
					let kx = ix + dx;
					if (kx < 0 || kx >= w)
						continue;
					let ddx = dx > 0 ? dx : -dx;
					let weight = weigh0 * (dxmax - ddx) / size;
					total += weight * resourceMaps[k].map[kx + w * ky];
					nbcell += weight;
				}
			}
		}
	}
	return nbcell ? total / nbcell : 0;
};

m.ConstructionPlan.prototype.isGo = function(gameState)
{
	if (this.goRequirement && this.goRequirement === "houseNeeded")
	{
		if (!gameState.ai.HQ.canBuild(gameState, "structures/{civ}_house"))
			return false;
		if (gameState.getPopulationMax() <= gameState.getPopulationLimit())
			return false;
		let freeSlots = gameState.getPopulationLimit() - gameState.getPopulation();
		for (let ent of gameState.getOwnFoundations().values())
			freeSlots += ent.getPopulationBonus();

		if (gameState.ai.HQ.saveResources)
			return freeSlots <= 10;
		if (gameState.getPopulation() > 55)
			return freeSlots <= 21;
		if (gameState.getPopulation() > 30)
			return freeSlots <= 15;
		return freeSlots <= 10;
	}
	return true;
};

m.ConstructionPlan.prototype.onStart = function(gameState)
{
	if (this.queueToReset)
		gameState.ai.queueManager.changePriority(this.queueToReset, gameState.ai.Config.priorities[this.queueToReset]);
};

m.ConstructionPlan.prototype.Serialize = function()
{
	return {
		"category": this.category,
		"type": this.type,
		"ID": this.ID,
		"metadata": this.metadata,
		"cost": this.cost.Serialize(),
		"number": this.number,
		"position": this.position,
		"goRequirement": this.goRequirement || undefined,
		"queueToReset": this.queueToReset || undefined
	};
};

m.ConstructionPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data)
		this[key] = data[key];

	let cost = new API3.Resources();
	cost.Deserialize(data.cost);
	this.cost = cost;
};

return m;
}(PETRA);
