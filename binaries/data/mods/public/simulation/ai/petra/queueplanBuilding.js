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

	return gameState.ai.HQ.buildManager.hasBuilder(this.type);
};

m.ConstructionPlan.prototype.start = function(gameState)
{
	Engine.ProfileStart("Building construction start");

	// We don't care which builder we assign, since they won't actually do
	// the building themselves - all we care about is that there is at least
	// one unit that can start the foundation (should always be the case here).
	let builder = gameState.findBuilder(this.type);
	if (!builder)
	{
		API3.warn("petra error: builder not found when starting construction.");
		Engine.ProfileStop();
		return;
	}

	let pos = this.findGoodPosition(gameState);
	if (!pos)
	{
		gameState.ai.HQ.buildManager.setUnbuildable(gameState, this.type, 90, "room");
		Engine.ProfileStop();
		return;
	}

	if (this.metadata && this.metadata.expectedGain && (!this.template.hasClass("BarterMarket") ||
	    gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities()))
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

	if (this.template.buildPlacementType() == "shore")
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
	else if (pos.xx === undefined || pos.x == pos.xx && pos.z == pos.zz)
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

	if (template.buildPlacementType() == "shore")
		return this.findDockPosition(gameState);

	let HQ = gameState.ai.HQ;
	if (template.hasClass("Storehouse") && this.metadata && this.metadata.base)
	{
		// recompute the best dropsite location in case some conditions have changed
		let base = HQ.getBaseByID(this.metadata.base);
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
				pos = HQ.findEconomicCCLocation(gameState, template, this.metadata.resource, proximity);
			}
			else
				pos = HQ.findStrategicCCLocation(gameState, template);

			if (pos)
				return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "base": 0 };
			// No possible location, try to build instead a dock in a not-enemy island
			let templateName = gameState.applyCiv("structures/{civ}_dock");
			if (gameState.ai.HQ.canBuild(gameState, templateName) && !gameState.isTemplateDisabled(templateName))
			{
				template = gameState.getTemplate(templateName);
				if (template && gameState.getResources().canAfford(new API3.Resources(template.cost())))
					this.buildOverseaDock(gameState, template);
			}
			return false;
		}
		else if (template.hasClass("DefenseTower") || template.hasClass("Fortress") || template.hasClass("ArmyCamp"))
		{
			let pos = HQ.findDefensiveLocation(gameState, template);
			if (pos)
				return { "x": pos[0], "z": pos[1], "angle": 3*Math.PI/4, "base": pos[2] };
			// if this fortress is our first one, just try the standard placement
			if (!template.hasClass("Fortress") || gameState.getOwnEntitiesByClass("Fortress", true).hasEntities())
				return false;
		}
		else if (template.hasClass("Market"))	// Docks (i.e. NavalMarket) are done before
		{
			let pos = HQ.findMarketLocation(gameState, template);
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
		// if we really need houses (i.e. Phasing without enough village building), do not apply these constraints
		if (this.metadata && this.metadata.base !== undefined)
		{
			let base = this.metadata.base;
			for (let j = 0; j < placement.map.length; ++j)
				if (HQ.basesMap.map[j] == base)
					placement.set(j, 45);
		}
		else
		{
			for (let j = 0; j < placement.map.length; ++j)
				if (HQ.basesMap.map[j] != 0)
					placement.set(j, 45);
		}

		if (!HQ.requireHouses || !template.hasClass("House"))
		{
			gameState.getOwnStructures().forEach(function(ent) {
				let pos = ent.position();
				let x = Math.round(pos[0] / cellSize);
				let z = Math.round(pos[1] / cellSize);

				let struct = m.getBuiltEntity(gameState, ent);
				if (struct.resourceDropsiteTypes() && struct.resourceDropsiteTypes().indexOf("food") != -1)
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
				else if (template.hasClass("GarrisonFortress") && ent.hasClass("House"))
					placement.addInfluence(x, z, 120/cellSize, -50);
				else if (template.hasClass("Military"))
					placement.addInfluence(x, z, 40/cellSize, -40);
				else if (template.genericName() == "Rotary Mill" && ent.hasClass("Field"))
					placement.addInfluence(x, z, 60/cellSize, 40);
			});
		}
		if (template.hasClass("Farmstead"))
		{
			for (let j = 0; j < placement.map.length; ++j)
			{
				let value = placement.map[j] - gameState.sharedScript.resourceMaps.wood.map[j]/3;
				if (HQ.borderMap.map[j] & m.fullBorder_Mask)
					value /= 2;	// we need space around farmstead, so disfavor map border
				placement.set(j, value);
			}
		}
	}

	// Requires to be inside our territory, and inside our base territory if required
	// and if our first market, put it on border if possible to maximize distance with next market
	let favorBorder = template.hasClass("BarterMarket");
	let disfavorBorder = gameState.currentPhase() > 1 && !template.hasDefensiveFire();
	let favoredBase = this.metadata && (this.metadata.favoredBase ||
		         (this.metadata.militaryBase ? HQ.findBestBaseForMilitary(gameState) : undefined));
	if (this.metadata && this.metadata.base !== undefined)
	{
		let base = this.metadata.base;
		for (let j = 0; j < placement.map.length; ++j)
		{
			if (HQ.basesMap.map[j] != base)
				placement.map[j] = 0;
			else if (placement.map[j] > 0)
			{
				if (favorBorder && HQ.borderMap.map[j] & m.border_Mask)
					placement.set(j, placement.map[j] + 50);
				else if (disfavorBorder && !(HQ.borderMap.map[j] & m.fullBorder_Mask))
					placement.set(j, placement.map[j] + 10);

				let x = (j % placement.width + 0.5) * cellSize;
				let z = (Math.floor(j / placement.width) + 0.5) * cellSize;
				if (HQ.isNearInvadingArmy([x, z]))
					placement.map[j] = 0;
			}
		}
	}
	else
	{
		for (let j = 0; j < placement.map.length; ++j)
		{
			if (HQ.basesMap.map[j] == 0)
				placement.map[j] = 0;
			else if (placement.map[j] > 0)
			{
				if (favorBorder && HQ.borderMap.map[j] & m.border_Mask)
					placement.set(j, placement.map[j] + 50);
				else if (disfavorBorder && !(HQ.borderMap.map[j] & m.fullBorder_Mask))
					placement.set(j, placement.map[j] + 10);

				let x = (j % placement.width + 0.5) * cellSize;
				let z = (Math.floor(j / placement.width) + 0.5) * cellSize;
				if (HQ.isNearInvadingArmy([x, z]))
					placement.map[j] = 0;
				else if (favoredBase && HQ.basesMap.map[j] == favoredBase)
					placement.set(j, placement.map[j] + 100);
			}
		}
	}

	// Find the best non-obstructed:
	// Find target building's approximate obstruction radius, and expand by a bit to make sure we're not too close,
	// this allows room for units to walk between buildings.
	// note: not for houses and dropsites who ought to be closer to either each other or a resource.
	// also not for fields who can be stacked quite a bit

	let obstructions = m.createObstructionMap(gameState, 0, template);
	// obstructions.dumpIm(template.buildPlacementType() + "_obstructions.png");

	let radius = 0;
	if (template.hasClass("Fortress") || template.hasClass("Workshop") ||
		this.type == gameState.applyCiv("structures/{civ}_elephant_stables"))
		radius = Math.floor((template.obstructionRadius().max + 8) / obstructions.cellSize);
	else if (template.resourceDropsiteTypes() === undefined && !template.hasClass("House") &&
	         !template.hasClass("Field") && !template.hasClass("BarterMarket"))
		radius = Math.ceil((template.obstructionRadius().max + 4) / obstructions.cellSize);
	else
		radius = Math.ceil(template.obstructionRadius().max / obstructions.cellSize);

	let bestTile;
	if (template.hasClass("House") && !alreadyHasHouses)
	{
		// try to get some space to place several houses first
		bestTile = placement.findBestTile(3*radius, obstructions);
		if (!bestTile.val)
			bestTile = undefined;
	}

	if (!bestTile)
		bestTile = placement.findBestTile(radius, obstructions);

	if (!bestTile.val)
		return false;

	let bestIdx = bestTile.idx;

	let x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	let z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;

	let territorypos = placement.gamePosToMapPos([x, z]);
	let territoryIndex = territorypos[0] + territorypos[1]*placement.width;
	// default angle = 3*Math.PI/4;
	return { "x": x, "z": z, "angle": 3*Math.PI/4, "base": HQ.basesMap.map[territoryIndex] };
};

/**
 * Placement of buildings with Dock build category
 * metadata.proximity is defined when first dock without any territory
 * => we try to minimize distance from our current point
 * metadata.oversea is defined for dock in oversea islands
 * => we try to maximize distance to our current docks (for trade)
 * otherwise standard dock on an island where we already have a cc
 * => we try not to be too far from our territory
 * In all cases, we add a bonus for nearby resources, and when a large extend of water in front ot it.
 */
m.ConstructionPlan.prototype.findDockPosition = function(gameState)
{
	let template = this.template;
	let territoryMap = gameState.ai.HQ.territoryMap;

	let obstructions = m.createObstructionMap(gameState, 0, template);
	// obstructions.dumpIm(template.buildPlacementType() + "_obstructions.png");

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
	let oversea = this.metadata && this.metadata.oversea ? this.metadata.oversea : null;
	if (nbShips == 0 && proxyAccess && proxyAccess > 1)
	{
		wantedLand = {};
		wantedLand[proxyAccess] = true;
	}
	let dropsiteTypes = template.resourceDropsiteTypes();
	let radius = Math.ceil(template.obstructionRadius().max / obstructions.cellSize);

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
	let ccEnts = oversea ? gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre")) : null;
	let docks = oversea ? gameState.getOwnStructures().filter(API3.Filters.byClass("Dock")) : null;
	// Normalisation factors (only guessed, no attempt to optimize them)
	let factor = proxyAccess ? 1 : oversea ? 0.2 : 40;
	for (let j = 0; j < territoryMap.length; ++j)
	{
		if (!this.isDockLocation(gameState, j, halfDepth, wantedLand, wantedSea))
			continue;
		let score = 0;
		if (!proxyAccess && !oversea)
		{
			// if not in our (or allied) territory, we do not want it too far to be able to defend it
			score = this.getFrontierProximity(gameState, j);
			if (score > 4)
				continue;
			score *= factor;
		}
		let i = territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;
		if (wantedSea && navalPassMap[i] != wantedSea)
			continue;

		let res = dropsiteTypes ? Math.min(maxRes, this.getResourcesAround(gameState, dropsiteTypes, j, 80)) : maxRes;
		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];

		// If proximity is given, we look for the nearest point
		if (proxyAccess)
			score = API3.VectorDistance(this.metadata.proximity, pos);

		// Bonus for resources
		score += 20 * (maxRes - res);

		if (oversea)
		{
			// Not much farther to one of our cc than to enemy ones
			let enemyDist;
			let ownDist;
			for (let cc of ccEnts.values())
			{
				let owner = cc.owner();
				if (owner != PlayerID && !gameState.isPlayerEnemy(owner))
					continue;
				let dist = API3.SquareVectorDistance(pos, cc.position());
				if (owner == PlayerID && (!ownDist || dist < ownDist))
					ownDist = dist;
				if (gameState.isPlayerEnemy(owner) && (!enemyDist || dist < enemyDist))
					enemyDist = dist;
			}
			if (ownDist && enemyDist && enemyDist < 0.5 * ownDist)
				continue;

			// And maximize distance for trade.
			let dockDist = 0;
			for (let dock of docks.values())
			{
				if (m.getSeaAccess(gameState, dock) != navalPassMap[i])
					continue;
				let dist = API3.SquareVectorDistance(pos, dock.position());
				if (dist > dockDist)
					dockDist = dist;
			}
			if (dockDist > 0)
			{
				dockDist = Math.sqrt(dockDist);
				if (dockDist > width * cellSize) // Could happen only on square maps, but anyway we don't want to be too far away
					continue;
				score += factor * (width * cellSize - dockDist);
			}
		}

		// Add a penalty if on the map border as ship movement will be difficult
		if (gameState.ai.HQ.borderMap.map[j] & m.fullBorder_Mask)
			score += 20;

		// Do a pre-selection, supposing we will have the best possible water
		if (bestIdx !== undefined && score > bestVal + 5 * maxWater)
			continue;

		let x = (i % obstructions.width + 0.5) * obstructions.cellSize;
		let z = (Math.floor(i / obstructions.width) + 0.5) * obstructions.cellSize;
		let angle = this.getDockAngle(gameState, x, z, halfSize);
		if (angle == false)
			continue;
		let ret = this.checkDockPlacement(gameState, x, z, halfDepth, halfWidth, angle);
		if (!ret || !gameState.ai.HQ.landRegions[ret.land])
			continue;
		// Final selection now that the checkDockPlacement water is known
		if (bestIdx !== undefined && score + 5 * (maxWater - ret.water) > bestVal)
			continue;
		if (this.metadata.proximity && gameState.ai.accessibility.regionSize[ret.land] < 4000)
			continue;
		if (gameState.ai.HQ.isDangerousLocation(gameState, pos, halfSize))
			continue;

		bestVal = score + maxWater - ret.water;
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
		baseIndex = -2;	// We'll do an anchorless base around it

	return { "x": x, "z": z, "angle": bestAngle, "base": baseIndex, "access": bestLand };
};

/**
 * Find a good island to build a dock.
 */
m.ConstructionPlan.prototype.buildOverseaDock = function(gameState, template)
{
	let docks = gameState.getOwnStructures().filter(API3.Filters.byClass("Dock"));
	if (!docks.hasEntities())
		return;

	let passabilityMap = gameState.getPassabilityMap();
	let cellArea = passabilityMap.cellSize * passabilityMap.cellSize;
	let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));

	let land = {};
	let found;
	for (let i = 0; i < gameState.ai.accessibility.regionSize.length; ++i)
	{
		if (gameState.ai.accessibility.regionType[i] != "land" ||
		    cellArea * gameState.ai.accessibility.regionSize[i] < 3600)
			continue;
		let keep = true;
		for (let dock of docks.values())
		{
			if (m.getLandAccess(gameState, dock) != i)
				continue;
			keep = false;
			break;
		}
		if (!keep)
			continue;
		let sea;
		for (let cc of ccEnts.values())
		{
			let ccAccess = m.getLandAccess(gameState, cc);
			if (ccAccess != i)
			{
				if (cc.owner() == PlayerID && !sea)
					sea = gameState.ai.HQ.getSeaBetweenIndices(gameState, ccAccess, i);
				continue;
			}
			// Docks on island where we have a cc are already done elsewhere
			if (cc.owner() == PlayerID || gameState.isPlayerEnemy(cc.owner()))
			{
				keep = false;
				break;
			}
		}
		if (!keep || !sea)
			continue;
		land[i] = true;
		found = true;
	}
	if (!found)
		return;
	if (!gameState.ai.HQ.navalMap)
		API3.warn("petra.findOverseaLand on a non-naval map??? we should never go there ");

	let oldTemplate = this.template;
	let oldMetadata = this.metadata;
	this.template = template;
	let pos;
	this.metadata = { "land": land, "oversea": true };
	pos = this.findDockPosition(gameState);
	if (pos)
	{
		let type = template.templateName();
		let builder = gameState.findBuilder(type);
		this.metadata.base = pos.base;
		// Adjust a bit the position if needed
		let cosa = Math.cos(pos.angle);
		let sina = Math.sin(pos.angle);
		let shiftMax = gameState.ai.HQ.territoryMap.cellSize;
		for (let shift = 0; shift <= shiftMax; shift += 2)
		{
			builder.construct(type, pos.x-shift*sina, pos.z-shift*cosa, pos.angle, this.metadata);
			if (shift > 0)
				builder.construct(type, pos.x+shift*sina, pos.z+shift*cosa, pos.angle, this.metadata);
		}
	}
	this.template = oldTemplate;
	this.metadata = oldMetadata;
	return;
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
			if (gameState.ai.accessibility.navalPassMap[j] == seaRef)
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
	return { "land": land, "water": water };
};

/**
 * fast check if we can build a dock: returns false if nearest land is farther than the dock dimension
 * if the (object) wantedLand is given, this nearest land should have one of these accessibility
 * if wantedSea is given, this tile should be inside this sea
 */
const around = [[ 1.0, 0.0], [ 0.87, 0.50], [ 0.50, 0.87], [ 0.0, 1.0], [-0.50, 0.87], [-0.87, 0.50],
	        [-1.0, 0.0], [-0.87,-0.50], [-0.50,-0.87], [ 0.0,-1.0], [ 0.50,-0.87], [ 0.87,-0.50]];

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
		if (landPass < 2 || wantedLand && !wantedLand[landPass])
			continue;
		pos = gameState.ai.accessibility.gamePosToMapPos([x - dist*a[0], z - dist*a[1]]);
		if (pos[0] < 0 || pos[0] >= gameState.ai.accessibility.width)
			continue;
		if (pos[1] < 0 || pos[1] >= gameState.ai.accessibility.height)
			continue;
		k = pos[0] + pos[1]*gameState.ai.accessibility.width;
		if (wantedSea && gameState.ai.accessibility.navalPassMap[k] != wantedSea)
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
	if (territoryOwner == PlayerID || alliedVictory && gameState.isPlayerAlly(territoryOwner))
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
			if (alliedVictory && gameState.isPlayerAlly(territoryOwner) || territoryOwner == PlayerID)
			{
				best = Math.min(best, i);
				break;
			}
		}
		if (best == 1)
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
		if (k == "food" || !resourceMaps[k])
			continue;
		let weigh0 = k == "wood" ? 2 : 1;
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
			if (dy == 0)
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
	if (this.goRequirement && this.goRequirement == "houseNeeded")
	{
		if (!gameState.ai.HQ.canBuild(gameState, "structures/{civ}_house"))
			return false;
		if (gameState.getPopulationMax() <= gameState.getPopulationLimit())
			return false;
		let freeSlots = gameState.getPopulationLimit() - gameState.getPopulation();
		for (let ent of gameState.getOwnFoundations().values())
		{
			let template = gameState.getBuiltTemplate(ent.templateName());
			if (template)
				freeSlots += template.getPopulationBonus();
		}

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

	this.cost = new API3.Resources();
	this.cost.Deserialize(data.cost);
};

return m;
}(PETRA);
