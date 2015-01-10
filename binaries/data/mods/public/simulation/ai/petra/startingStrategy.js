var PETRA = function(m)
{
/**
 * determines the strategy to adopt when starting a new game, depending on the initial conditions
 */

m.HQ.prototype.gameAnalysis = function(gameState)
{
	// Analysis of the terrain and the different access regions
	this.regionAnalysis(gameState);

	// Make a list of buildable structures from the config file
	this.structureAnalysis(gameState);

	// Let's get our initial situation here.
	let nobase = new m.BaseManager(gameState, this.Config);
	nobase.init(gameState);
	nobase.accessIndex = 0;
	this.baseManagers.push(nobase);   // baseManagers[0] will deal with unit/structure without base
	var ccEnts = gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre"));
	for (let cc of ccEnts.values())
	{
		let newbase = new m.BaseManager(gameState, this.Config);
		newbase.init(gameState);
		newbase.setAnchor(gameState, cc);
		this.baseManagers.push(newbase);
	}
	this.updateTerritories(gameState);

	// Assign entities and resources in the different bases
	this.assignStartingEntities(gameState);

	// Check if we will ever be able to produce units
	this.canBuildUnits = true;
	if (!gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre")).length)
	{
		var template = gameState.applyCiv("structures/{civ}_civil_centre");
		if (gameState.isDisabledTemplates(template) || !gameState.getTemplate(template).available(gameState))
		{
			if (this.Config.debug > 1)
				API3.warn(" this AI is unable to produce any units");
			this.canBuildUnits = false;
			this.dispatchUnits(gameState);
		}
	}

	this.attackManager.init(gameState);
	this.navalManager.init(gameState);
	this.tradeManager.init(gameState);

	// configure our first base strategy
	if (this.baseManagers.length > 1)
		this.configFirstBase(gameState);
};

/**
 * Assign the starting entities to the different bases
 */
m.HQ.prototype.assignStartingEntities = function(gameState)
{
	var defaultbase = (this.numActiveBase() > 0) ? 1 : 0;
	var width = gameState.getMap().width;
	for (var ent of gameState.getOwnEntities().values())
	{
		// do not affect merchant ship immediately to trade as they may-be useful for transport
		if (ent.hasClass("Trader") && !ent.hasClass("Ship"))
			this.tradeManager.assignTrader(ent);

		var pos = ent.position();
		if (!pos)
		{
			// TODO should support recursive garrisoning. Make a warning for now
			if (ent.isGarrisonHolder() && ent.garrisoned().length)
				API3.warn("Petra warning: support for garrisoned units inside garrisoned holders not yet implemented");
			continue;
		}

		// make sure we have not rejected small regions with units (TODO should probably also check with other non-gaia units)
		let gamepos = gameState.ai.accessibility.gamePosToMapPos(pos);
		let index = gamepos[0] + gamepos[1]*gameState.ai.accessibility.width;
		let land = gameState.ai.accessibility.landPassMap[index];
		if (land > 1 && !this.landRegions[land])
			this.landRegions[land] = true;
		let sea = gameState.ai.accessibility.navalPassMap[index];
		if (sea > 1 && !this.navalRegions[sea])
			this.navalRegions[sea] = true;

		// if garrisoned units inside, ungarrison them except if a ship in which case we will make a transport 
		if (ent.isGarrisonHolder() && ent.garrisoned().length && !ent.hasClass("Ship"))
			for (let id of ent.garrisoned())
				ent.unload(id);

		ent.setMetadata(PlayerID, "access", gameState.ai.accessibility.getAccessValue(pos));
		var bestbase = undefined;
		for (var i = 1; i < this.baseManagers.length; ++i)
		{
			var base = this.baseManagers[i];
			if (base.territoryIndices.indexOf(index) === -1)
				continue;
			base.assignEntity(ent);
			if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
				base.assignResourceToDropsite(gameState, ent);
			bestbase = base;
			break;
		}
		if (!bestbase)
		{
			// entity outside our territory
			var bestbase = m.getBestBase(ent, gameState);
			bestbase.assignEntity(ent);
			if (bestbase.ID !== this.baseManagers[0].ID && ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
				bestbase.assignResourceToDropsite(gameState, ent);
		}
		// now assign entities garrisoned inside this entity
		if (ent.isGarrisonHolder() && ent.garrisoned().length)
			for (let id of ent.garrisoned())
				bestBase.assignEntity(gameState.getEntityByID(id));
	}
};

/**
 * determine the main land Index (or water index if none)
 * as well as the list of allowed (land andf water) regions
 */
m.HQ.prototype.regionAnalysis = function(gameState)
{
	let landIndex = undefined;
	let seaIndex = undefined;
	var ccEnts = gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre"));
	for (let cc of ccEnts.values())
	{
		let land = gameState.ai.accessibility.getAccessValue(cc.position());
		if (land > 1)
		{
			landIndex = land;
			break;
		}
	}
	if (!landIndex)
	{
		for (let ent of gameState.getOwnEntities().values())
		{
			if (!ent.position() || (!ent.hasClass("Unit") && !ent.trainableEntities()))
				continue;
			let land = gameState.ai.accessibility.getAccessValue(ent.position());
			if (land > 1)
			{
				landIndex = land;
				break;
			}
			let sea = gameState.ai.accessibility.getAccessValue(ent.position(), true);
			if (!seaIndex && sea > 1)
				seaIndex = sea;
		}
	}
	if (!landIndex && !seaIndex)
		API3.warn("Petra error: it does not know how to interpret this map");

	var passabilityMap = gameState.getMap();
	var totalSize = passabilityMap.width * passabilityMap.width;
	var minLandSize = Math.floor(0.1*totalSize);
	var minWaterSize = Math.floor(0.3*totalSize);
	var cellArea = passabilityMap.cellSize * passabilityMap.cellSize;  
	for (var i = 0; i < gameState.ai.accessibility.regionSize.length; ++i)
	{
		if (landIndex && i == landIndex)
			this.landRegions[i] = true;
		else if (gameState.ai.accessibility.regionType[i] === "land" && cellArea*gameState.ai.accessibility.regionSize[i] > 320)
		{
			if (landIndex)
			{
				var sea = this.getSeaIndex(gameState, landIndex, i);
				if (sea && (gameState.ai.accessibility.regionSize[i] > minLandSize || gameState.ai.accessibility.regionSize[sea] > minWaterSize))
				{
					this.navalMap = true;
					this.landRegions[i] = true;
					this.navalRegions[sea] = true;
				}
			}
			else
			{
				var traject = gameState.ai.accessibility.getTrajectToIndex(seaIndex, i);
				if (traject && traject.length === 2)
				{
					this.navalMap = true;
					this.landRegions[i] = true;
					this.navalRegions[seaIndex] = true;
				}
			}
		}
		else if (gameState.ai.accessibility.regionType[i] === "water" && gameState.ai.accessibility.regionSize[i] > minWaterSize)
		{
			this.navalMap = true;
			this.navalRegions[i] = true;
		}
	}

	if (this.Config.debug < 3)
		return;
	for (var region in this.landRegions)
		API3.warn(" >>> zone " + region + " taille " + cellArea*gameState.ai.accessibility.regionSize[region]);
	API3.warn(" navalMap " + this.navalMap);
	API3.warn(" landRegions " + uneval(this.landRegions));
	API3.warn(" navalRegions " + uneval(this.navalRegions));
};

/**
 * load units and buildings from the config files
 * TODO: change that to something dynamic
 */
m.HQ.prototype.structureAnalysis = function(gameState)
{
	var civ = gameState.playerData.civ;
	if (civ in this.Config.buildings.base)
		this.bBase = this.Config.buildings.base[civ];
	else
		this.bBase = this.Config.buildings.base['default'];

	if (civ in this.Config.buildings.advanced)
		this.bAdvanced = this.Config.buildings.advanced[civ];
	else
		this.bAdvanced = this.Config.buildings.advanced['default'];	
	for (var i in this.bBase)
		this.bBase[i] = gameState.applyCiv(this.bBase[i]);
	for (var i in this.bAdvanced)
		this.bAdvanced[i] = gameState.applyCiv(this.bAdvanced[i]);
};

/**
 * set strategy if game without construction:
 *   - if one of our allies has a cc, affect a small fraction of our army for his defense, the rest will attack
 *   - otherwise all units will attack
 */
m.HQ.prototype.dispatchUnits = function(gameState)
{
	var allycc = gameState.getExclusiveAllyEntities().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	if (allycc.length)
	{
		if (this.Config.debug > 1)
			API3.warn(" We have allied cc " + allycc.length + " and " + gameState.getOwnUnits().length + " units ");
		var units = gameState.getOwnUnits();
		var num = Math.max(Math.min(Math.round(0.08*(1+this.Config.personality.cooperative)*units.length), 20), 5);
		var num1 = Math.floor(num / 2);
		var num2 = num1;
		// first pass to affect ranged infantry
		units.filter(API3.Filters.byClassesAnd(["Infantry", "Ranged"])).forEach(function (ent) {
			if (!num || !num1)
				return;
			if (ent.getMetadata(PlayerID, "allied"))
				return;
			var access = gameState.ai.accessibility.getAccessValue(ent.position());
			for (var cc of allycc)
			{
				if (!cc.position())
					continue;
				if (gameState.ai.accessibility.getAccessValue(cc.position()) != access)
					continue;
				--num;
				--num1;
				ent.setMetadata(PlayerID, "allied", true);
				var range = 1.5 * cc.footprintRadius();
				ent.moveToRange(cc.position()[0], cc.position()[1], range, range);
				break;
			}
		});
		// second pass to affect melee infantry
		units.filter(API3.Filters.byClassesAnd(["Infantry", "Melee"])).forEach(function (ent) {
			if (!num || !num2)
				return;
			if (ent.getMetadata(PlayerID, "allied"))
				return;
			var access = gameState.ai.accessibility.getAccessValue(ent.position());
			for (var cc of allycc)
			{
				if (!cc.position())
					continue;
				if (gameState.ai.accessibility.getAccessValue(cc.position()) != access)
					continue;
				--num;
				--num2;
				ent.setMetadata(PlayerID, "allied", true);
				var range = 1.5 * cc.footprintRadius();
				ent.moveToRange(cc.position()[0], cc.position()[1], range, range);
				break;
			}
		});
		// and now complete the affectation, including all support units
		units.forEach(function (ent) {
			if (!num && !ent.hasClass("Support"))
				return;
			if (ent.getMetadata(PlayerID, "allied"))
				return;
			var access = gameState.ai.accessibility.getAccessValue(ent.position());
			for (var cc of allycc)
			{
				if (!cc.position())
					continue;
				if (gameState.ai.accessibility.getAccessValue(cc.position()) != access)
					continue;
				if (!ent.hasClass("Support"))
					--num;
				ent.setMetadata(PlayerID, "allied", true);
				var range = 1.5 * cc.footprintRadius();
				ent.moveToRange(cc.position()[0], cc.position()[1], range, range);
				break;
			}
		});
	}
};

/**
 * configure our first base expansion
 *   - if on a small island, favor fishing
 *   - count the available wood resource, and allow rushes only if enough (we should otherwise favor expansion)
 */
m.HQ.prototype.configFirstBase = function(gameState)
{
	if (this.baseManagers.length < 2)
		return;

	var startingSize = 0;
	for (let region in this.landRegions)
	{
		for (let base of this.baseManagers)
		{
			if (!base.anchor || base.accessIndex != +region)
				continue;
			startingSize += gameState.ai.accessibility.regionSize[region];
			break;
		}
	}
	var cell = gameState.getMap().cellSize;
	startingSize = startingSize * cell * cell;
	if (this.Config.debug > 1)
		API3.warn("starting size " + startingSize + "(cut at 24000 for fish pushing)");
	if (startingSize < 24000)
	{
		this.saveSpace = true;
		this.Config.Economy.popForDock = Math.min(this.Config.Economy.popForDock, 16);
		this.Config.Economy.targetNumFishers = Math.max(this.Config.Economy.targetNumFishers, 2);
	}

	// - count the available wood resource, and allow rushes only if enough (we should otherwise favor expansion)
	var startingWood = gameState.getResources()["wood"];
	var check = {};
	for (var proxim of ["nearby", "medium", "faraway"])
	{
		for (let base of this.baseManagers)
		{
			for (var supply of base.dropsiteSupplies["wood"][proxim])
			{
				if (check[supply.id])    // avoid double counting as same resource can appear several time
					continue;
				check[supply.id] = true;
				startingWood += supply.ent.resourceSupplyAmount();
			}
		}
	}
	if (this.Config.debug > 1)
		API3.warn("startingWood: " + startingWood + "(cut at 8500 for no rush and 6000 for saveResources)");
	if (startingWood < 6000)
		this.saveResources = true;
	if (startingWood > 8500 && this.canBuildUnits)
		this.attackManager.setRushes();

	// immediatly build a wood dropsite if possible.
	var template = "structures/{civ}_storehouse";
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(template), true) === 0 && this.canBuild(gameState, template))
	{
		var newDP = this.baseManagers[1].findBestDropsiteLocation(gameState, "wood");
		if (newDP.quality > 40)
			gameState.ai.queues.dropsites.addItem(new m.ConstructionPlan(gameState, template, { "base": this.baseManagers[1].ID }, newDP.pos));
	}
};

return m;

}(PETRA);
