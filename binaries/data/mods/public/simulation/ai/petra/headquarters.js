var PETRA = function(m)
{
/* Headquarters
 * Deal with high level logic for the AI. Most of the interesting stuff gets done here.
 * Some tasks:
	-defining RESS needs
	-BO decisions.
		> training workers
		> building stuff (though we'll send that to bases)
		> researching
	-picking strategy (specific manager?)
	-diplomacy (specific manager?)
	-planning attacks
	-picking new CC locations.
 */

m.HQ = function(Config)
{
	this.Config = Config;
	
	this.targetNumBuilders = this.Config.Economy.targetNumBuilders; // number of workers we want building stuff

	this.econState = "growth";	// existing values: growth, townPhasing.
	this.phaseStarted = undefined;

	// cache the rates.
	this.wantedRates = { "food": 0, "wood": 0, "stone":0, "metal": 0 };
	this.currentRates = { "food": 0, "wood": 0, "stone":0, "metal": 0 };
	this.lastFailedGather = { "wood": undefined, "stone": undefined, "metal": undefined }; 


	// this means we'll have about a big third of women, and thus we can maximize resource gathering rates.
	this.femaleRatio = this.Config.Economy.femaleRatio;

	this.lastTerritoryUpdate = -1;
	this.stopBuilding = []; // list of buildings to stop (temporarily) production because no room

	this.towerStartTime = 0;
	this.towerLapseTime = this.Config.Military.towerLapseTime;
	this.fortressStartTime = 0;
	this.fortressLapseTime = this.Config.Military.fortressLapseTime;

	this.baseManagers = {};
	this.attackManager = new m.AttackManager(this.Config);
	this.defenseManager = new m.DefenseManager(this.Config);
	this.tradeManager = new m.TradeManager(this.Config);
	this.navalManager = new m.NavalManager(this.Config);
	this.researchManager = new m.ResearchManager(this.Config);
	this.garrisonManager = new m.GarrisonManager();
};

// More initialisation for stuff that needs the gameState
m.HQ.prototype.init = function(gameState, queues)
{
	this.territoryMap = m.createTerritoryMap(gameState);
	// initialize base map. Each pixel is a base ID, or 0 if not or not accessible
	this.basesMap = new API3.Map(gameState.sharedScript);
	// area of 10 cells on the border of the map : 0=inside map, 1=border map, 2=outside map
	this.borderMap = m.createBorderMap(gameState);
	// initialize frontier map. Each cell is 2 if on the near frontier, 1 on the frontier and 0 otherwise
	this.frontierMap = m.createFrontierMap(gameState, this.borderMap);
	// list of allowed regions
	this.allowedRegions = {};

	// try to determine if we have a water map
	this.navalMap = false;
	this.navalRegions = [];

	var totalSize = gameState.getMap().width * gameState.getMap().width;
	var minLandSize = Math.floor(0.2*totalSize);
	var minWaterSize = Math.floor(0.3*totalSize);
	for (var i = 0; i < gameState.ai.accessibility.regionSize.length; ++i)
	{
		if (i === gameState.ai.myIndex)
			this.allowedRegions[i] = true;
		else if (gameState.ai.accessibility.regionType[i] === "land" && gameState.ai.accessibility.regionSize[i] > 20)
		{
			var seaIndex = this.getSeaIndex(gameState, gameState.ai.myIndex, i);
			if (!seaIndex)
				continue;
			this.allowedRegions[i] = true;
			if (gameState.ai.accessibility.regionSize[i] > minLandSize)
			{
				this.navalMap = true;
				if (this.navalRegions.indexOf(seaIndex) === -1)
					this.navalRegions.push(seaIndex);
			}
		}
		else if (gameState.ai.accessibility.regionType[i] === "water" && gameState.ai.accessibility.regionSize[i] > minWaterSize)
		{
			this.navalMap = true;
			if (this.navalRegions.indexOf(i) === -1)
				this.navalRegions.push(i);
		}
	}
	if (this.Config.debug > 0)
	{
		for (var region in this.allowedRegions)
			warn(" >>> zone " + region + " taille " + gameState.ai.accessibility.regionSize[region]);
	}

	if (this.Config.difficulty === 0)
		this.targetNumWorkers = Math.max(1, Math.min(40, Math.floor(gameState.getPopulationMax())));
	else if (this.Config.difficulty === 1)
		this.targetNumWorkers = Math.max(1, Math.min(60, Math.floor(gameState.getPopulationMax())));
	else
		this.targetNumWorkers = Math.max(1, Math.min(120,Math.floor(gameState.getPopulationMax()/3.0)));

	// Let's get our initial situation here.
	// TODO: improve on this.
	// TODO: aknowledge bases, assign workers already.
	var ents = gameState.getEntities().filter(API3.Filters.byOwner(PlayerID));
	var ccEnts = ents.filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	
	var workersNB = ents.filter(API3.Filters.byClass("Worker")).length;
	
	for (var i = 0; i < ccEnts.length; ++i)
	{
		this.baseManagers[i+1] = new m.BaseManager(this.Config);
		this.baseManagers[i+1].init(gameState);
		this.baseManagers[i+1].setAnchor(gameState, ccEnts[i]);
	}
	this.updateTerritories(gameState);

	if (this.baseManagers[1])     // Affects entities in the different bases
	{
		var self = this;
		var width = gameState.getMap().width;
		ents.forEach( function (ent) {
			if (ent.hasClass("Trader"))
				this.tradeManager.assignTrader(ent);
			var pos = ent.position();
			if (!pos)
			{
				// TODO temporarily assigned to base 1. Certainly a garrisoned unit,
				// should assign it to the base of the garrison holder
				self.baseManagers[1].assignEntity(ent);
				return;
			}
			ent.setMetadata(PlayerID, "access", gameState.ai.accessibility.getAccessValue(ent.position()));
			var x = Math.round(pos[0] / gameState.cellSize);
			var z = Math.round(pos[1] / gameState.cellSize);
			var id = x + width*z;
			for each (var base in self.baseManagers)
			{
				if (base.territoryIndices.indexOf(id) === -1)
					continue;
				base.assignEntity(ent);
				if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
					base.assignResourceToDropsite(gameState, ent);
				return;
			}
			// entity outside our territory, assign it to base 1
			self.baseManagers[1].assignEntity(ent);
			if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
				self.baseManagers[1].assignResourceToDropsite(gameState, ent);

		});
	}

	// we now have enough data to decide on a few things.
	
	// immediatly build a wood dropsite if possible.
	if (this.baseManagers[1])
	{
		var newDP = this.baseManagers[1].findBestDropsiteLocation(gameState, "wood");
		if (newDP.quality > 40 && this.canBuild(gameState, "structures/{civ}_storehouse"))
			queues.dropsites.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse", { "base": 1 }, newDP.pos));
	}

	// adapt our starting strategy to the available resources
	// - if on a small island, favor fishing and require less fields to save room for buildings
	var startingSize = 0;
	for (var region in this.allowedRegions)
	{
		for each (var base in this.baseManagers)
		{
			if (!base.anchor || base.accessIndex !== +region)
				continue;
			startingSize += gameState.ai.accessibility.regionSize[region];
			break;
		}
	}
	if (this.Config.debug > 0)
		warn("starting size " + startingSize + "(cut at 1500 for fish pushing)");
	if (startingSize < 1500)
	{
		this.Config.Economy.popForDock = Math.min(this.Config.Economy.popForDock, 16);
		this.Config.Economy.initialFields = Math.min(this.Config.Economy.initialFields, 3);
		this.Config.Economy.targetNumFishers = Math.max(this.Config.Economy.targetNumFishers, 2);
	}
	// - count the available wood resource, and allow rushes only if enough (we should otherwise favor expansion)
	var startingWood = gameState.getResources()["wood"];
	var check = {};
	for (var proxim of ["nearby", "medium", "faraway"])
	{
		for each (var base in this.baseManagers)
		{
			for each (var supply in base.dropsiteSupplies["wood"][proxim])
			{
				if (check[supply.id])    // avoid double counting as same resource can appear several time
					continue;
				check[supply.id] = true;
				startingWood += supply.ent.resourceSupplyAmount();
			}
		}
	}
	if (this.Config.debug > 0)
		warn("startingWood: " + startingWood + "(cut at 8500 for no rush and 6000 for saveResources)");
	if (startingWood < 6000)
	{
		this.saveResources = true;
		this.Config.Economy.initialFields = Math.min(this.Config.Economy.initialFields, 2);
	}

	this.attackManager.init(gameState, queues, (startingWood > 8500));  // rush allowed 3rd argument = true
	this.navalManager.init(gameState, queues);
	this.defenseManager.init(gameState);
	this.tradeManager.init(gameState);

	// TODO: change that to something dynamic.
	var civ = gameState.playerData.civ;
	
	// load units and buildings from the config files
	
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

// returns the sea index linking regions 1 and region 2 (supposed to be different land region)
// otherwise return undefined
// for the moment, only the case land-sea-land is supported
m.HQ.prototype.getSeaIndex = function (gameState, index1, index2)
{
	var path = gameState.ai.accessibility.getTrajectToIndex(index1, index2);
	if (path && path.length === 3 && gameState.ai.accessibility.regionType[path[1]] === "water")
		return path[1];
	else
	{
		if (this.Config.debug > 0)
		{
			warn("bad path from " + index1 + " to " + index2 + " ??? " + uneval(path));
			warn(" regionLinks start " + uneval(gameState.ai.accessibility.regionLinks[index1]));
			warn(" regionLinks end   " + uneval(gameState.ai.accessibility.regionLinks[index2]));
		}
		return undefined;
	}
};

m.HQ.prototype.checkEvents = function (gameState, events, queues)
{
	var CreateEvents = events["Create"];
	for (var evt of CreateEvents)
	{
		// Let's check if we have a building set to create a new base.
		var ent = gameState.getEntityById(evt.entity);
		if (!ent || !ent.isOwn(PlayerID))
			continue;
			
		if (ent.getMetadata(PlayerID, "base") === -1)
		{
			// Okay so let's try to create a new base around this.
			var bID = m.playerGlobals[PlayerID].uniqueIDBases;
			this.baseManagers[bID] = new m.BaseManager(this.Config);
			this.baseManagers[bID].init(gameState, true);
			this.baseManagers[bID].setAnchor(gameState, ent);

			// Let's get a few units out there to build this.
			var builders = this.bulkPickWorkers(gameState, bID, 10);
			if (builders !== false)
			{
				builders.forEach(function (worker) {
					worker.setMetadata(PlayerID, "base", bID);
					worker.setMetadata(PlayerID, "subrole", "builder");
					worker.setMetadata(PlayerID, "target-foundation", ent.id());
				});
			}
		}
	}

	var ConstructionEvents = events["ConstructionFinished"];
	for (var evt of ConstructionEvents)
	{
		// Let's check if we have a building set to create a new base.
		// TODO: move to the base manager.
		if (evt.newentity)
		{
			var ent = gameState.getEntityById(evt.newentity);
			if (!ent || !ent.isOwn(PlayerID))
				continue;

			if (ent.getMetadata(PlayerID, "baseAnchor") == true)
			{
				var base = ent.getMetadata(PlayerID, "base");
				if (this.baseManagers[base].constructing)
					this.baseManagers[base].constructing = false;
				this.baseManagers[base].anchor = ent;
				this.baseManagers[base].buildings.updateEnt(ent);
				this.updateTerritories(gameState);
				// let us hope this new base will fix our resource shortage
				// TODO check it really does so
				this.saveResources = undefined;
			}
			else if (ent.hasTerritoryInfluence())
				this.updateTerritories(gameState);
		}
	}

	// deal with the different rally points of training units: the rally point is set when the training starts
	// for the time being, only autogarrison is used

	var TrainingEvents = events["TrainingStarted"];
	for (var evt of TrainingEvents)
	{
		var ent = gameState.getEntityById(evt.entity);
		if (!ent || !ent.isOwn(PlayerID))
			continue;

		if (!ent._entity.trainingQueue || !ent._entity.trainingQueue.length)
			continue;
		var metadata = ent._entity.trainingQueue[0].metadata;
		if (metadata.garrisonType)
			ent.setRallyPoint(ent, "garrison");  // trained units will autogarrison
		else
			ent.unsetRallyPoint();
	}

	var TrainingEvents = events["TrainingFinished"];
	for (var evt of TrainingEvents)
	{
		for (var entId of evt.entities)
		{
			var ent = gameState.getEntityById(entId);
			if (!ent || !ent.isOwn(PlayerID))
				continue;

			if (!ent.position())
			{
				// we are autogarrisoned, check that the holder is registered in the garrisonManager
				var holderId = ent.unitAIOrderData()[0]["target"];
				var holder = gameState.getEntityById(holderId);
				if (holder)
					this.garrisonManager.registerHolder(gameState, holder);
			}
			else if (ent.getMetadata(PlayerID, "garrisonType"))
			{
				// we were supposed to be autogarrisoned, but this has failed (may-be full)
				ent.getMetadata(PlayerID, "garrisonType", undefined);
			}
		}
	}
};

// Called by the "town phase" research plan once it's started
m.HQ.prototype.OnTownPhase = function(gameState)
{
	if (this.Config.difficulty >= 2 && this.femaleRatio > 0.4)
		this.femaleRatio = 0.4;

	this.phaseStarted = 2;
};

// Called by the "city phase" research plan once it's started
m.HQ.prototype.OnCityPhase = function(gameState)
{
	if (this.Config.difficulty >= 2 && this.femaleRatio > 0.3)
		this.femaleRatio = 0.3;

	this.phaseStarted = 3;
};

// This code trains females and citizen workers, trying to keep close to a ratio of females/CS
// TODO: this should choose a base depending on which base need workers
// TODO: also there are several things that could be greatly improved here.
m.HQ.prototype.trainMoreWorkers = function(gameState, queues)
{
	// Count the workers in the world and in progress
	var numFemales = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("units/{civ}_support_female_citizen"), true);

	// counting the workers that aren't part of a plan
	var numWorkers = 0;
	gameState.getOwnUnits().forEach (function (ent) {
		if (ent.getMetadata(PlayerID, "role") == "worker" && ent.getMetadata(PlayerID, "plan") == undefined)
			numWorkers++;
	});
	var numInTraining = 0;
	gameState.getOwnTrainingFacilities().forEach(function(ent) {
		ent.trainingQueue().forEach(function(item) {
			if (item.metadata && item.metadata.role && item.metadata.role == "worker" && item.metadata.plan == undefined)
				numWorkers += item.count;
			numInTraining += item.count;
		});
	});

	// Adapt the batch size of the first queued workers and females to the present population
	// to ease a possible recovery if our population was drastically reduced by an attack
	if (numWorkers < 12)
		var size = 1;
	else
		var size =  Math.min(5, Math.ceil(numWorkers / 10));
	if (queues.villager.queue[0])
		queues.villager.queue[0].number = Math.min(queues.villager.queue[0].number, size);
	if (queues.citizenSoldier.queue[0])
		queues.citizenSoldier.queue[0].number = Math.min(queues.citizenSoldier.queue[0].number, size);

	var numQueuedF = queues.villager.countQueuedUnits();
	var numQueuedS = queues.citizenSoldier.countQueuedUnits();
	var numQueued = numQueuedS + numQueuedF;
	var numTotal = numWorkers + numQueued;

	if (this.saveResources && numTotal > this.Config.Economy.popForTown + 10)
		return;
	if (numTotal > this.targetNumWorkers || (numTotal >= this.Config.Economy.popForTown 
		&& gameState.currentPhase() === 1 && !gameState.isResearching(gameState.townPhase())))
		return;
	if (numQueued > 50 || (numQueuedF > 20 && numQueuedS > 20) || numInTraining > 15)
		return;

	// default template
	var template = gameState.applyCiv("units/{civ}_support_female_citizen");
	// anticipate the optimal batch size when this queue will start
	if (numTotal < 12)
		var size = 1;
	else
		var size = Math.min(5, Math.ceil(numTotal / 10));

	// Choose whether we want soldiers instead.
	if ((numFemales+numQueuedF) > 8 && (numFemales+numQueuedF)/numTotal > this.femaleRatio)
	{
		if (numTotal < 45)
			template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["cost", 1], ["speed", 0.5], ["costsResource", 0.5, "stone"], ["costsResource", 0.5, "metal"]]);
		else
			template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["strength", 1] ]);
		if (!template)
			template = gameState.applyCiv("units/{civ}_support_female_citizen");
	}

	// base "0" means "auto"
	if (template === gameState.applyCiv("units/{civ}_support_female_citizen"))
		queues.villager.addItem(new m.TrainingPlan(gameState, template, { "role": "worker", "base": 0 }, size, size));
	else
		queues.citizenSoldier.addItem(new m.TrainingPlan(gameState, template, { "role": "worker", "base": 0 }, size, size));
};

// picks the best template based on parameters and classes
m.HQ.prototype.findBestTrainableUnit = function(gameState, classes, requirements)
{
	var units = gameState.findTrainableUnits(classes);
	
	if (units.length === 0)
		return undefined;

	var parameters = requirements.slice();
	var remainingResources = this.getTotalResourceLevel(gameState);    // resources (estimation) still gatherable in our territory
	var availableResources = gameState.ai.queueManager.getAvailableResources(gameState); // available (gathered) resources
	for (var type in remainingResources)
	{
		if (type === "food")
			continue;
		if (availableResources[type] > 800)
			continue;
		if (remainingResources[type] > 800)
			continue;
		else if (remainingResources[type] > 400)
			var costsResource = 0.6;
		else
			var costsResource = 0.2;
		var toAdd = true;
		for (var param of parameters)
		{
			if (param[0] !== "costsResource" || param[2] !== type)
				continue; 			
			param[1] = Math.min( param[1], costsResource );
			toAdd = false;
			break;
		}
		if (toAdd)
			parameters.push( [ "costsResource", costsResource, type ] );
	}

	units.sort(function(a, b) {// }) {
		var aDivParam = 0, bDivParam = 0;
		var aTopParam = 0, bTopParam = 0;
		for (var i in parameters) {
			var param = parameters[i];
			
			if (param[0] == "base") {
				aTopParam = param[1];
				bTopParam = param[1];
			}
			if (param[0] == "strength") {
				aTopParam += m.getMaxStrength(a[1]) * param[1];
				bTopParam += m.getMaxStrength(b[1]) * param[1];
			}
			if (param[0] == "siegeStrength") {
				aTopParam += m.getMaxStrength(a[1], "Structure") * param[1];
				bTopParam += m.getMaxStrength(b[1], "Structure") * param[1];
			}
			if (param[0] == "speed") {
				aTopParam += a[1].walkSpeed() * param[1];
				bTopParam += b[1].walkSpeed() * param[1];
			}
			
			if (param[0] == "cost") {
				aDivParam += a[1].costSum() * param[1];
				bDivParam += b[1].costSum() * param[1];
			}
			// requires a third parameter which is the resource
			if (param[0] == "costsResource") {
				if (a[1].cost()[param[2]])
					aTopParam *= param[1];
				if (b[1].cost()[param[2]])
					bTopParam *= param[1];
			}
			if (param[0] == "canGather") {
				// checking against wood, could be anything else really.
				if (a[1].resourceGatherRates() && a[1].resourceGatherRates()["wood.tree"])
					aTopParam *= param[1];
				if (b[1].resourceGatherRates() && b[1].resourceGatherRates()["wood.tree"])
					bTopParam *= param[1];
			}
		}
		return -(aTopParam/(aDivParam+1)) + (bTopParam/(bDivParam+1));
	});
	return units[0][0];
};

// returns an entity collection of workers through BaseManager.pickBuilders
// TODO: when same accessIndex, sort by distance
m.HQ.prototype.bulkPickWorkers = function(gameState, newBaseID, number)
{
	var accessIndex = this.baseManagers[newBaseID].accessIndex;
	if (!accessIndex)
		return false;
	// sorting bases by whether they are on the same accessindex or not.
	var baseBest = API3.AssocArraytoArray(this.baseManagers).sort(function (a,b) {
		if (a.accessIndex === accessIndex && b.accessIndex !== accessIndex)
			return -1;
		else if (b.accessIndex === accessIndex && a.accessIndex !== accessIndex)
			return 1;
		return 0;
	});

	var needed = number;
	var workers = new API3.EntityCollection(gameState.sharedScript);
	for (var i in baseBest)
	{
		if (baseBest[i].ID === newBaseID)
			continue;
		baseBest[i].pickBuilders(gameState, workers, needed);
		if (workers.length < number)
			needed = number - workers.length;
		else
			break;
	}
	if (workers.length == 0)
		return false;
	return workers;
};

m.HQ.prototype.getTotalResourceLevel = function(gameState)
{
	var total = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	for each (var base in this.baseManagers)
		for (var type in total)
			total[type] += base.getResourceLevel(gameState, type);

	return total;
};

// returns the current gather rate
// This is not per-se exact, it performs a few adjustments ad-hoc to account for travel distance, stuffs like that.
m.HQ.prototype.GetCurrentGatherRates = function(gameState)
{
	for (var type in this.wantedRates)
		this.currentRates[type] = 0;
	
	for each (var base in this.baseManagers)
		base.getGatherRates(gameState, this.currentRates);

	return this.currentRates;
};


/* Pick the resource which most needs another worker
 * How this works:
 * We get the rates we would want to have to be able to deal with our plans
 * We get our current rates
 * We compare; we pick the one where the discrepancy is highest.
 * Need to balance long-term needs and possible short-term needs.
 */
m.HQ.prototype.pickMostNeededResources = function(gameState)
{
	var self = this;
	
	this.wantedRates = gameState.ai.queueManager.wantedGatherRates(gameState);
	var currentRates = this.GetCurrentGatherRates(gameState);

	// let's get our ideal number.
	var types = Object.keys(this.wantedRates);

	types.sort(function(a, b) {
		var va = (Math.max(0,self.wantedRates[a] - currentRates[a]))/ (currentRates[a]+1);
		var vb = (Math.max(0,self.wantedRates[b] - currentRates[b]))/ (currentRates[b]+1);
		
		// If they happen to be equal (generally this means "0" aka no need), make it fair.
		if (va === vb)
			return (self.wantedRates[b]/(currentRates[b]+1)) - (self.wantedRates[a]/(currentRates[a]+1));
		return vb-va;
	});
	return types;
};

// Returns the best position to build a new Civil Centre
// Whose primary function would be to reach new resources of type "resource".
m.HQ.prototype.findEconomicCCLocation = function(gameState, template, resource, fromStrategic)
{	
	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.

	Engine.ProfileStart("findEconomicCCLocation");

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0, template);
	obstructions.expandInfluences();

	var ccEnts = gameState.getEntities().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	var dpEnts = gameState.getOwnDropsites().filter(API3.Filters.not(API3.Filters.byClassesOr(["CivCentre", "Elephant"]))).toEntityArray();
	var ccList = [];
	for (var cc of ccEnts)
		ccList.push({"pos": cc.position(), "ally": gameState.isPlayerAlly(cc.owner())});
	var dpList = [];
	for (var dp of dpEnts)
		dpList.push({"pos": dp.position()});

	var width = this.territoryMap.width;
	var radius = Math.ceil(template.obstructionRadius() / gameState.cellSize);
	var bestIdx = undefined;
	var bestVal = undefined;

	for (var j = 0; j < this.territoryMap.length; ++j)
	{	
		if (this.territoryMap.getOwnerIndex(j) !== 0 || this.borderMap.map[j] === 2)
			continue;
		// We require that it is accessible from our starting position
		var index = gameState.ai.accessibility.landPassMap[j];
		if (!this.allowedRegions[index])
			continue;
		// and with enough room around to build the cc
		if (obstructions.map[j] <= radius)
			continue;

		var norm = 0.5;   // TODO adjust it, knowing that we will sum 5 maps
		// checking distance to other cc
		var pos = [j%width+0.5, Math.floor(j/width)+0.5];
		pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
		var minDist = Math.min();

		for (var cc of ccList)
		{
			var dist = API3.SquareVectorDistance(cc.pos, pos);
			if (dist < 14000)    // Reject if too near from any cc
			{
				norm = 0
				break;
			}
			if (!cc.ally)
				continue;
			if (dist < 20000)    // Reject if too near from an allied cc
			{
				norm = 0
				break;
			}
			if (dist < 40000)   // Disfavor if quite near an allied cc
				norm *= 0.5;
			if (dist < minDist)
				minDist = dist;
		}
		if (norm == 0)
			continue;
		if (minDist > 170000 && !this.navalMap)   // Reject if too far from any allied cc (-> not connected)
		{
			norm = 0;
			continue;
		}
		else if (minDist > 130000)     // Disfavor if quite far from any allied cc
		{
			if (this.navalMap)
			{
				if (minDist > 250000)
					norm *= 0.5;
				else
					norm *= 0.8;
			}
			else
				norm *= 0.5;
		}

		for (var dp of dpList)
		{
			var dist = API3.SquareVectorDistance(dp.pos, pos);
			if (dist < 3600)
			{
				norm = 0;
				break;
			}
			else if (dist < 6400)
				norm *= 0.5;
		}
		if (norm == 0)
			continue;

		if (this.borderMap.map[j] === 1)	// disfavor the borders of the map
			norm *= 0.5;
		
		var val = 2*gameState.sharedScript.CCResourceMaps[resource].map[j]
			+ gameState.sharedScript.CCResourceMaps["wood"].map[j]
			+ gameState.sharedScript.CCResourceMaps["stone"].map[j]
			+ gameState.sharedScript.CCResourceMaps["metal"].map[j];
		val *= norm;

		if (bestVal !== undefined && val < bestVal)
			continue;
		bestVal = val;
		bestIdx = j;
	}

	Engine.ProfileStop();

/*	if (m.DebugEnabled())
	{
		gameState.sharedScript.CCResourceMaps["wood"].dumpIm("woodMap.png", 300);
		gameState.sharedScript.CCResourceMaps["stone"].dumpIm("stoneMap.png", 300);
		gameState.sharedScript.CCResourceMaps["metal"].dumpIm("metalMap.png", 300);
		locateMap.dumpIm("cc_placement_base_" + best[1] + ".png",300);
		obstructions.dumpIm("cc_placement_base_" + best[1] + "_obs.png", 20);
	} */

	var cut = 60;
	if (fromStrategic)  // be less restrictive
		cut = 30;
	if (this.Config.debug)
		warn("on a trouve une base avec best (cut=" + cut + ") = " + bestVal);
	// not good enough.
	if (bestVal < cut)
		return false;
	
	var x = (bestIdx%width + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx/width) + 0.5) * gameState.cellSize;

	// Define a minimal number of wanted ships in the seas reaching this new base
	var index = gameState.ai.accessibility.landPassMap[bestIdx];
	for each (var base in this.baseManagers)
	{
		if (base.anchor && base.accessIndex !== index)
		{
			var sea = this.getSeaIndex(gameState, base.accessIndex, index);
			if (sea !== undefined)
				this.navalManager.setMinimalTransportShips(gameState, sea, 1);
		}
	}

	return [x,z];
};

// Returns the best position to build a new Civil Centre
// Whose primary function would be to assure territorial continuity with our allies
m.HQ.prototype.findStrategicCCLocation = function(gameState, template)
{	
	// This builds a map. The procedure is fairly simple.
	// We minimize the Sum((dist-300)**2) where the sum is on the three nearest allied CC
	// with the constraints that all CC have dist > 200 and at least one have dist < 400
	// This needs at least 2 CC. Otherwise, go back to economic CC.

	// TODO add CC foundations (needed for allied)
	var ccEnts = gameState.getEntities().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	var ccList = [];
	var numAllyCC = 0;
	for (var cc of ccEnts)
	{
		var ally = gameState.isPlayerAlly(cc.owner());
		ccList.push({"pos": cc.position(), "ally": ally});
		if (ally)
			++numAllyCC;
	}
	if (numAllyCC < 2)
		return this.findEconomicCCLocation(gameState, template, "wood", true);

	Engine.ProfileStart("findStrategicCCLocation");

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0, template);
	obstructions.expandInfluences();

	var width = this.territoryMap.width;
	var radius =  Math.ceil(template.obstructionRadius() / gameState.cellSize);
	var bestIdx = undefined;
	var bestVal = undefined;
	var currentVal, delta;
	var distcc0, distcc1, distcc2;

	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.territoryMap.getOwnerIndex(j) !== 0 || this.borderMap.map[j] === 2)
			continue;
		// We require that it is accessible from our starting position
		var index = gameState.ai.accessibility.landPassMap[j];
		if (!this.allowedRegions[index])
			continue;
		// and with enough room around to build the cc
		if (obstructions.map[j] <= radius)
			continue;

		// checking distances to other cc
		var pos = [j%width+0.5, Math.floor(j/width)+0.5];
		pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
		var minDist = Math.min();
		distcc0 = undefined;

		for (var cc of ccList)
		{
			var dist = API3.SquareVectorDistance(cc.pos, pos);
			if (dist < 14000)    // Reject if too near from any cc
			{
				minDist = 0;
				break;
			}
			if (!cc.ally)
				continue;
			if (dist < 40000)    // Reject if quite near from ally cc
			{
				minDist = 0;
				break;
			}
			if (dist < minDist)
				minDist = dist;

			if (!distcc0 || dist < distcc0)
			{
				distcc2 = distcc1;
				distcc1 = distcc0;
				distcc0 = dist;
			}
			else if (!distcc1 || dist < distcc1)
			{
				distcc2 = distcc1;
				distcc1 = dist;
			}
			else if (!distcc2 || dist < distcc2)
				distcc2 = dist;
		}
		if (minDist < 1 || (minDist > 170000 && !this.navalMap))
			continue;

		delta = Math.sqrt(distcc0) - 300;  // favor a distance of 300
		currentVal = delta*delta;
		delta = Math.sqrt(distcc1) - 300;
		currentVal += delta*delta;
		if (distcc2)
		{
			delta = Math.sqrt(distcc2) - 300;
			currentVal += delta*delta;
		}
		// disfavor border of the map
		if (this.borderMap.map[j] === 1)
			currentVal += 10000;		

		if (bestVal !== undefined && currentVal > bestVal)
			continue;
		bestVal = currentVal;
		bestIdx = j;
	}

	if (this.Config.debug > 0)
		warn("We've found a strategic base with bestVal = " + bestVal);	

	Engine.ProfileStop();

	if (bestVal === undefined)
		return undefined;

	var x = (bestIdx%width + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx/width) + 0.5) * gameState.cellSize;

	// Define a minimal number of wanted ships in the seas reaching this new base
	var index = gameState.ai.accessibility.landPassMap[bestIdx];
	for each (var base in this.baseManagers)
	{
		if (base.anchor && base.accessIndex !== index)
		{
			var sea = this.getSeaIndex(gameState, base.accessIndex, index);
			if (sea !== undefined)
				this.navalManager.setMinimalTransportShips(gameState, sea, 1);
		}
	}

	return [x,z];
};

// Returns the best position to build a new market: if the allies already have a market, build it as far as possible
// from it, although not in our border to be able to defend it easily. If no allied market, our second market will
// follow the same logic
// TODO check that it is on same accessIndex
m.HQ.prototype.findMarketLocation = function(gameState, template)
{
	var markets = gameState.getAllyEntities().filter(API3.Filters.byClass("Market")).toEntityArray();
	if (!markets.length)
		markets = gameState.getOwnStructures().filter(API3.Filters.byClass("Market")).toEntityArray();

	if (!markets.length)	// this is the first market. For the time being, place it arbitrarily by the ConstructionPlan
		return [-1, -1, -1];

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0, template);
	obstructions.expandInfluences();

	var width = this.territoryMap.width;
	var bestIdx = undefined;
	var bestVal = undefined;
	var radius = Math.ceil(template.obstructionRadius() / gameState.cellSize);

	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		// do not try on the border of our territory
		if (this.frontierMap.map[j] === 2)
			continue;
		if (this.basesMap.map[j] === 0)   // inaccessible cell
			continue;
		if (obstructions.map[j] <= radius)  // check room around
			continue;
		var index = gameState.ai.accessibility.landPassMap[j];
		if (!this.allowedRegions[index])
			continue;

		var pos = [j%width+0.5, Math.floor(j/width)+0.5];
		pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
		// checking distances to other markets
		var maxDist = 0;
		for (var market of markets)
		{
			if (template.hasClass("Dock") && market.hasClass("Dock"))
			{
				// TODO check that there are on the same sea. For the time being, we suppose it is true
			}
			else if (gameState.ai.accessibility.getAccessValue(market.position()) !== index)
				continue;
			var dist = API3.SquareVectorDistance(market.position(), pos);
			if (dist > maxDist)
				maxDist = dist;
		}
		if (maxDist == 0)
			continue;
		if (bestVal !== undefined && maxDist < bestVal)
			continue;
		bestVal = maxDist;
		bestIdx = j;
	}

	if (this.Config.debug > 0)
		warn("We found a market position with bestVal = " + bestVal);	

	if (bestVal === undefined)  // no constraints. For the time being, place it arbitrarily by the ConstructionPlan
		return [-1, -1, -1];

	var x = (bestIdx%width + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx/width) + 0.5) * gameState.cellSize;
	return [x, z, this.basesMap.map[bestIdx]];
};

// Returns the best position to build defensive buildings (fortress and towers)
// Whose primary function is to defend our borders
m.HQ.prototype.findDefensiveLocation = function(gameState, template)
{	
	// We take the point in our territory which is the nearest to any enemy cc
	// but requiring a minimal distance with our other defensive structures
	// and not in range of any enemy defensive structure to avoid building under fire.

	var ownStructures = gameState.getOwnStructures().filter(API3.Filters.byClassesOr(["Fortress", "Tower"])).toEntityArray();
	var enemyStructures = gameState.getEnemyStructures().filter(API3.Filters.byClassesOr(["CivCentre", "Fortress", "Tower"])).toEntityArray();

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0, template);
	obstructions.expandInfluences();

	var width = this.territoryMap.width;
	var bestIdx = undefined;
	var bestVal = undefined;
	if (template.hasClass("Fortress"))
		var radius = Math.floor(template.obstructionRadius() / gameState.cellSize) + 2;
	else
		var radius = Math.ceil(template.obstructionRadius() / gameState.cellSize);

	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		// do not try if well inside or outside territory
		if (this.frontierMap.map[j] === 0)
			continue
		if (this.frontierMap.map[j] === 1 && template.hasClass("Tower"))
			continue;
		if (this.basesMap.map[j] === 0)   // inaccessible cell
			continue;
		if (obstructions.map[j] <= radius)  // check room around
			continue;

		var pos = [j%width+0.5, Math.floor(j/width)+0.5];
		pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
		// checking distances to other structures
		var minDist = Math.min();
		for (var str of enemyStructures)
		{
			if (str.foundationProgress() !== undefined)
				continue;
			var strPos = str.position();
			if (!strPos)
				continue;
			var dist = API3.SquareVectorDistance(strPos, pos);
			if (dist < 6400) //  TODO check on true attack range instead of this 80*80 
			{
				minDist = -1;
				break;
			}
			if (str.hasClass("CivCentre") && dist < minDist)
				minDist = dist;
		}
		if (minDist < 0)
			continue;

		for (var str of ownStructures)
		{
			var strPos = str.position();
			if (!strPos)
				continue;
			var dist = API3.SquareVectorDistance(strPos, pos);
			if ((template.hasClass("Tower") && str.hasClass("Tower")) || (template.hasClass("Fortress") && str.hasClass("Fortress")))
				var cutDist = 4225; //  TODO check on true buildrestrictions instead of this 65*65
			else
				var cutDist = 900;  //  30*30   TODO maybe increase it
			if (dist < cutDist)
			{
				minDist = -1;
				break;
			}
		}
		if (minDist < 0)
			continue;
		if (bestVal !== undefined && minDist > bestVal)
			continue;
		bestVal = minDist;
		bestIdx = j;
	}

	if (bestVal === undefined)
		return undefined;

	var x = (bestIdx%width + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx/width) + 0.5) * gameState.cellSize;
	return [x, z, this.basesMap.map[bestIdx]];
};

m.HQ.prototype.buildTemple = function(gameState, queues)
{
	if (gameState.currentPhase() < 3 || queues.economicBuilding.countQueuedUnits() !== 0 ||
		gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_temple"), true) !== 0)
		return;
	if (!this.canBuild(gameState, "structures/{civ}_temple"))
		return;
	queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_temple"));
};

m.HQ.prototype.buildMarket = function(gameState, queues)
{
	if (gameState.getPopulation() < this.Config.Economy.popForMarket ||
		queues.economicBuilding.countQueuedUnitsWithClass("BarterMarket") !== 0 ||
		gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) !== 0)
		return;
	if (!this.canBuild(gameState, "structures/{civ}_market"))
		return;
	queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_market"));
};

// Build a farmstead to go to town phase faster and prepare for research. Only really active on higher diff mode.
m.HQ.prototype.buildFarmstead = function(gameState, queues)
{
	// Only build one farmstead for the time being ("DropsiteFood" does not refer to CCs)
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_farmstead"), true) > 0)
		return;
	// Wait to have at least one dropsite and house before the farmstead
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_storehouse"), true) == 0)
		return;
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_house"), true) == 0)
		return;
	if (queues.economicBuilding.countQueuedUnitsWithClass("DropsiteFood") > 0)
		return;
	if (!this.canBuild(gameState, "structures/{civ}_farmstead"))
		return;

	queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_farmstead"));
};

// build more houses if needed.
// kinda ugly, lots of special cases to both build enough houses but not tooo many…
m.HQ.prototype.buildMoreHouses = function(gameState,queues)
{
	if (gameState.getPopulationMax() <= gameState.getPopulationLimit())
		return;

	var numPlanned = queues.house.length();
	if (numPlanned < 3 || (numPlanned < 5 && gameState.getPopulation() > 80))
	{
		var plan = new m.ConstructionPlan(gameState, "structures/{civ}_house");
		// make the difficulty available to the isGo function without having to pass it as argument
		var difficulty = this.Config.difficulty;
		var self = this;
		// change the starting condition according to the situation.
		plan.isGo = function (gameState) {
			if (!self.canBuild(gameState, "structures/{civ}_house"))
				return false;
			if (gameState.getPopulationMax() <= gameState.getPopulationLimit())
				return false;
			var HouseNb = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_house"), true);

			var freeSlots = 0;
			// TODO how to modify with tech
			var popBonus = gameState.getTemplate(gameState.applyCiv("structures/{civ}_house")).getPopulationBonus();
			freeSlots = gameState.getPopulationLimit() + HouseNb*popBonus - gameState.getPopulation();
			if (self.saveResources)
				return (freeSlots <= 10);
			else if (gameState.getPopulation() > 55 && difficulty > 1)
				return (freeSlots <= 21);
			else if (gameState.getPopulation() >= 30 && difficulty > 0)
				return (freeSlots <= 15);
			else
				return (freeSlots <= 10);
		};
		queues.house.addItem(plan);
	}

	if (numPlanned > 0 && this.econState == "townPhasing")
	{
		var count = gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length;
		var index = this.stopBuilding.indexOf(gameState.applyCiv("structures/{civ}_house"));
		if (count < 5 && index !== -1)
		{
			if (this.Config.debug > 0)
				warn("no room to place a house ... try to be less restrictive");
			this.stopBuilding.splice(index, 1);
			this.requireHouses = true;
		}
		var houseQueue = queues.house.queue;
		for (var i = 0; i < numPlanned; ++i)
		{
			if (houseQueue[i].isGo(gameState))
				++count;
			else if (count < 5)
			{
				houseQueue[i].isGo = function () { return true; };
				++count;
			}
		}
	}

	if (this.requireHouses && gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length >= 5)
		this.requireHouses = undefined;

	// When population limit too tight
	//    - if no room to build, try to improve with technology
	//    - otherwise increase temporarily the priority of houses
	var HouseNb = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_house"), true);
	var freeSlots = 0;
	var popBonus = gameState.getTemplate(gameState.applyCiv("structures/{civ}_house")).getPopulationBonus();
	freeSlots = gameState.getPopulationLimit() + HouseNb*popBonus - gameState.getPopulation();
	if (freeSlots < 5)
	{
		var index = this.stopBuilding.indexOf(gameState.applyCiv("structures/{civ}_house"));
		if (index !== -1)
		{
			if (this.Config.debug > 0)
				warn("no room to place a house ... try to improve with technology");
			this.researchManager.researchPopulationBonus(gameState, queues);
		}
		else if (index === -1)
			var priority = 2*this.Config.priorities.house;
	}
	else
		var priority = this.Config.priorities.house;
	if (priority && priority !== gameState.ai.queueManager.getPriority("house"))
		gameState.ai.queueManager.changePriority("house", priority);
};

// checks the status of the territory expansion. If no new economic bases created, build some strategic ones.
m.HQ.prototype.checkBaseExpansion = function(gameState,queues)
{
	if (queues.civilCentre.length() > 0)
		return;
	// first expand if we have not enough room available for buildings
	if (this.stopBuilding.length > 1)
	{
		if (this.Config.debug > 1)
			warn("try to build a new base because not enough room to build " + uneval(this.stopBuilding));
		this.buildNewBase(gameState, queues);
		return;
	}
	// then expand if we have lots of units
	var numUnits = 	gameState.getOwnUnits().length;
	var numCCs = gameState.countEntitiesByType(this.bBase[0], true);
	var popForBase = this.Config.Economy.popForTown + 20;
	if (this.saveResources)
		popForBase = this.Config.Economy.popForTown + 5;
	if (Math.floor(numUnits/popForBase) >= numCCs)
	{
		if (this.Config.debug > 1)
			warn("try to build a new base because of population " + numUnits + " for " + numCCs + " CCs");
		this.buildNewBase(gameState, queues);
	}
};

m.HQ.prototype.buildNewBase = function(gameState, queues, type)
{
	if (gameState.currentPhase() === 1 && !gameState.isResearching(gameState.townPhase()))
		return false;
	if (gameState.countFoundationsByType(this.bBase[0], true) > 0 || queues.civilCentre.length() > 0)
		return false;
	if (!this.canBuild(gameState, this.bBase[0]))
		return false;

	// base "-1" means new base.
	if (this.Config.debug > 0)
		warn("new base planned with type " + type);
	queues.civilCentre.addItem(new m.ConstructionPlan(gameState, this.bBase[0], { "base": -1, "type": type }));
	return true;
};

// Deals with building fortresses and towers along our border with enemies.
m.HQ.prototype.buildDefenses = function(gameState, queues)
{
	if (this.saveResources)
		return;

	if (gameState.currentPhase() > 2 || gameState.isResearching(gameState.cityPhase()))
	{
		// try to build fortresses
		var fortressType = "structures/{civ}_fortress";
		if (gameState.civ() === "celt")
		{
			if (Math.random() > 0.5)
				fortressType = "structures/{civ}_fortress_b";
			else
				fortressType = "structures/{civ}_fortress_g";
		}
		if (queues.defenseBuilding.length() === 0 && this.canBuild(gameState, fortressType))
		{
			if (gameState.civ() !== "celt")
				var numFortresses = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_fortress"), true);
			else
				var numFortresses = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_fortress_b"), true)
					+ gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_fortress_g"), true);
			if (gameState.ai.elapsedTime > (1 + 0.10*numFortresses)*this.fortressLapseTime + this.fortressStartTime)
			{
				this.fortressStartTime = gameState.ai.elapsedTime;
				queues.defenseBuilding.addItem(new m.ConstructionPlan(gameState, fortressType));
			}
		}

		// let's add a siege building plan to the current attack plan if there is none currently.
		var numSiegeBuilder = 0;
		if (gameState.civ() !== "celt" && gameState.civ() !== "mace" && gameState.civ() !== "maur")
			numSiegeBuilder += gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_fortress"), true);
		if (gameState.civ() === "celt")
			numSiegeBuilder += (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_fortress_b"), true)
				+ gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_fortress_g"), true));
		if (gameState.civ() === "mace" || gameState.civ() === "maur" || gameState.civ() === "rome")
			numSiegeBuilder += gameState.countEntitiesByType(this.bAdvanced[0], true);
		if (numSiegeBuilder > 0)
		{
			var attack = undefined;
			// There can only be one upcoming attack
			if (this.attackManager.upcomingAttacks["Attack"].length !== 0)
				var attack = this.attackManager.upcomingAttacks["Attack"][0];
			else if (this.attackManager.upcomingAttacks["HugeAttack"].length !== 0)
				var attack = this.attackManager.upcomingAttacks["HugeAttack"][0];

			if (attack && !attack.unitStat["Siege"])
				attack.addSiegeUnits(gameState);
		}
	}

	if (gameState.currentPhase() < 2 
		|| queues.defenseBuilding.length() !== 0 
		|| !this.canBuild(gameState, "structures/{civ}_defense_tower"))
		return;	

	var numTowers = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_defense_tower"), true);
	if (gameState.ai.elapsedTime > (1 + 0.10*numTowers)*this.towerLapseTime + this.towerStartTime)
	{
		this.towerStartTime = gameState.ai.elapsedTime;
		queues.defenseBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_defense_tower"));
	}
	// TODO  otherwise protect markets and civilcentres
};

m.HQ.prototype.buildBlacksmith = function(gameState, queues)
{
	if (gameState.getPopulation() < this.Config.Military.popForBlacksmith 
		|| queues.militaryBuilding.length() !== 0
		|| gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_blacksmith"), true) > 0)
		return;
	// build a market before the blacksmith
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) == 0)
		return;

	if (this.canBuild(gameState, "structures/{civ}_blacksmith"))
		queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_blacksmith"));
};

// Deals with constructing military buildings (barracks, stables…)
// They are mostly defined by Config.js. This is unreliable since changes could be done easily.
// TODO: We need to determine these dynamically. Also doesn't build fortresses since the above function does that.
// TODO: building placement is bad. Choice of buildings is also fairly dumb.
m.HQ.prototype.constructTrainingBuildings = function(gameState, queues)
{
	var barrackNb = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_barracks"), true);
	var preferredBase = this.findBestBaseForMilitary(gameState);

	if (this.canBuild(gameState, "structures/{civ}_barracks") && queues.militaryBuilding.length() === 0)
	{
		// first barracks.
		if (barrackNb === 0 && (gameState.getPopulation() > this.Config.Military.popForBarracks1 ||
			(this.econState == "townPhasing" && gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length < 5)))
		{
			var priority = this.Config.priorities.militaryBuilding;
			gameState.ai.queueManager.changePriority("militaryBuilding", 2*priority);
			var plan = new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase });
			plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("militaryBuilding", priority); };
			queues.militaryBuilding.addItem(plan);
		}
		// second barracks, then 3rd barrack, and optional 4th for some civs as they rely on barracks more.
		else if (barrackNb === 1 && gameState.getPopulation() > this.Config.Military.popForBarracks2)
			queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase }));
		else if (barrackNb === 2 && gameState.getPopulation() > this.Config.Military.popForBarracks2 + 20)
			queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase" : preferredBase }));
		else if (barrackNb === 3 && gameState.getPopulation() > this.Config.Military.popForBarracks2 + 50
			&& (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber"))
			queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase" : preferredBase }));
	}

	//build advanced military buildings
	if (gameState.currentPhase() > 2 && gameState.getPopulation() > 80 && queues.militaryBuilding.length() === 0 && this.bAdvanced.length !== 0)
	{
		var nAdvanced = 0;
		for (var advanced of this.bAdvanced)
			nAdvanced += gameState.countEntitiesAndQueuedByType(advanced, true);

		if (nAdvanced === 0 || (nAdvanced < this.bAdvanced.length && gameState.getPopulation() > 120))
		{
			for (var advanced of this.bAdvanced)
			{
				if (gameState.countEntitiesAndQueuedByType(advanced, true) < 1 && this.canBuild(gameState, advanced))
				{
					queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, advanced, { "preferredBase" : preferredBase }));
					break;
				}
			}
		}
	}
};

/**
 *  Construct military building in bases nearest to the ennemies  TODO revisit as the nearest one may not be accessible
 */
m.HQ.prototype.findBestBaseForMilitary = function(gameState)
{
	var ccEnts = gameState.getEntities().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	var bestBase = 1;
	var distMin = Math.min();
	for (var cc of ccEnts)
	{
		if (cc.owner() != PlayerID)
			continue;
		for (var cce of ccEnts)
		{
			if (gameState.isPlayerAlly(cce.owner()))
				continue;
			var dist = API3.SquareVectorDistance(cc.position(), cce.position());
			if (dist > distMin)
				continue;
			bestBase = cc.getMetadata(PlayerID, "base");
			distMin = dist;
		}
	}
	return bestBase;
};

/**
 * train with highest priority ranged infantry in the nearest civil centre from a given set of positions
 * and garrison them there for defense
 */  
m.HQ.prototype.trainEmergencyUnits = function(gameState, positions)
{
	var available = gameState.ai.queueManager.getAvailableResources(gameState);
	var total = gameState.getResources();
	var distcut = 20000;

	var nearestAnchor = undefined;
	var templateAnchor = undefined;
	var distmin = undefined;
	var rangedUnit = false;
	for (var pos of positions)
	{
		var access = gameState.ai.accessibility.getAccessValue(pos);
		// check nearest base anchor
		for each (var base in this.baseManagers)
		{
			if (!base.anchor || !base.anchor.position())
				continue;
			if (base.anchor.getMetadata(PlayerID, "access") !== access)
				continue;
			var trainables = base.anchor.trainableEntities();
			if (!trainables)     // base still in construction
				continue;
			var queue = base.anchor._entity.trainingQueue
			if (queue)
			{
				var time = 0;
				for (var item of queue)
					if (item.progress > 0 || (item.metadata && item.metadata.trainer))
						time += item.timeRemaining;
				if (time/1000 > 5)
					continue;
			}
			var templateFound = undefined;
			for (var trainable of trainables)
			{
				var template = gameState.getTemplate(trainable);
				if (!template.hasClass("Infantry") || !template.hasClass("Ranged")
					|| !template.hasClass("CitizenSoldier"))
					continue;
				if (!total.canAfford(new API3.Resources(template.cost())))
				{
					if (rangedUnit)
						continue;
					// if we still have no ranged units, but can afford a melee unit, let's try it
					var template = gameState.getTemplate(trainable);
					if (!template.hasClass("Infantry") || !template.hasClass("Melee")
						|| !template.hasClass("CitizenSoldier"))
						continue;
					if (!total.canAfford(new API3.Resources(template.cost())))
						continue;
				}
				else
					rangedUnit = true;
				templateFound = [trainable, template];
				if (rangedUnit)
					break;
			}
			if (!templateFound)
				continue;
			var dist = API3.SquareVectorDistance(base.anchor.position(), pos);
			if (nearestAnchor && dist > distmin)
				continue;
			distmin = dist;
			nearestAnchor = base.anchor;
			templateAnchor = templateFound;
		}
	}
	if (!nearestAnchor || distmin > distcut)
		return;

	var autogarrison = rangedUnit;
	var numGarrisoned = this.garrisonManager.numberOfGarrisonedUnits(nearestAnchor);
	if (nearestAnchor._entity.trainingQueue)
	{
		for (var item of nearestAnchor._entity.trainingQueue)
		{
			if (item.metadata && item.metadata["garrisonType"])
				numGarrisoned += item.count;
			else if (!item.progress || !item.metadata || !item.metadata.trainer)
				nearestAnchor.stopProduction(item.id);
		}
	}
	if (rangedUnit && numGarrisoned >= nearestAnchor.garrisonMax())
	{
		// No more room to garrison ... favor a melee unit
		autogarrison = false;
		var trainables = nearestAnchor.trainableEntities();
		for (var trainable of trainables)
		{
			var template = gameState.getTemplate(trainable);
			if (!template.hasClass("Infantry") || !template.hasClass("Melee")
				|| !template.hasClass("CitizenSoldier"))
				continue;
			if (!total.canAfford(new API3.Resources(template.cost())))
				continue;
			templateAnchor = [trainable, template];
			break;
		}
	}
	// Check first if we can afford it without touching other the accounts
	// and if not, take some of ther accounted resources
	// TODO substract only what is needed instead of reset and sort the queues
	var cost = new API3.Resources(templateAnchor[1].cost());
	if (!available.canAfford(cost))
	{
		for (var p in gameState.ai.queueManager.queues)
		{
			// TODO substract only what is needed instead of reseting
			// and do a better sorting of queues
			available.add(gameState.ai.queueManager.accounts[p]);
			gameState.ai.queueManager.accounts[p].reset();
			if (available.canAfford(cost))
				break;
		}
	}
	gameState.ai.queueManager.accounts["emergency"].add(cost);
	var metadata = { "role": "worker", "base": nearestAnchor.getMetadata(PlayerID, "base"), "trainer": nearestAnchor.id() };
	if (autogarrison)
		metadata.garrisonType = "protection";
	gameState.ai.queues.emergency.addItem(new m.TrainingPlan(gameState, templateAnchor[0], metadata, 1, 1));
};

m.HQ.prototype.canBuild = function(gameState, structure)
{
	var type = gameState.applyCiv(structure); 
	// available room to build it
	if (this.stopBuilding.indexOf(type) !== -1)
		return false;

	// build limits
	var template = gameState.getTemplate(type);
	if (!template.available(gameState))
		return false;
	var limits = gameState.getEntityLimits();
	for (var limitClass in limits)
		if (template.hasClass(limitClass) && gameState.getOwnStructures().filter(API3.Filters.byClass(limitClass)).length >= limits[limitClass])
			return false;

	return true;
};

m.HQ.prototype.updateTerritories = function(gameState)
{
	// TODO may-be update also when territory decreases. For the moment, only increases are taking into account
	if (this.lastTerritoryUpdate == gameState.ai.playedTurn)
		return;
	this.lastTerritoryUpdate = gameState.ai.playedTurn;

	var width = this.territoryMap.width;
	var expansion = false;
	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.borderMap.map[j] === 2)
			continue;
		if (this.territoryMap.getOwnerIndex(j) !== PlayerID)
		{
			if (this.basesMap.map[j] === 0)
				continue;
			var baseID = this.basesMap.map[j];
			var index = this.baseManagers[baseID].territoryIndices.indexOf(j);
			if (index === -1)
			{
				warn(" problem in headquarters::updateTerritories for base " + baseID);
				continue;
			}
			this.baseManagers[baseID].territoryIndices.splice(index, 1);
			this.basesMap.map[j] = 0;
		}
		else if (this.basesMap.map[j] === 0)
		{
			var index = gameState.ai.accessibility.landPassMap[j];
			if (!this.allowedRegions[index])
				continue;
			var distmin = Math.min();
			var baseID = undefined;
			var pos = [j%width+0.5, Math.floor(j/width)+0.5];
			pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
			for each (var base in this.baseManagers)
			{
				if (!base.anchor || !base.anchor.position())
					continue;
				if (base.accessIndex !== index)
					continue;
				var dist = API3.SquareVectorDistance(base.anchor.position(), pos);
				if (dist >= distmin)
					continue;
				distmin = dist;
				baseID = base.ID;
			}
			if (!baseID)
				continue;
			this.baseManagers[baseID].territoryIndices.push(j);
			this.basesMap.map[j] = baseID;
			expansion = true;
		}
	}

	this.frontierMap =  m.createFrontierMap(gameState, this.borderMap);

	if (!expansion)
		return;
	// We've increased our territory, so we may have some new room to build
	this.stopBuilding = [];
};

// TODO: use pop(). Currently unused as this is too gameable.
m.HQ.prototype.garrisonAllFemales = function(gameState)
{
	var buildings = gameState.getOwnStructures().filter(API3.Filters.byCanGarrison()).toEntityArray();
	var females = gameState.getOwnUnits().filter(API3.Filters.byClass("Support"));
	
	var cache = {};
	
	females.forEach( function (ent) {
		if (!ent.position())
			return;
		for (var i in buildings)
		{
			var struct = buildings[i];
			if (!cache[struct.id()])
				cache[struct.id()] = 0;
			if (struct.garrisoned() && struct.garrisonMax() - struct.garrisoned().length - cache[struct.id()] > 0)
			{
				ent.garrison(struct);
				cache[struct.id()]++;
				break;
			}
		}
	});
	this.hasGarrisonedFemales = true;
};

m.HQ.prototype.ungarrisonAll = function(gameState) {
	this.hasGarrisonedFemales = false;
	var buildings = gameState.getOwnStructures().filter(API3.Filters.and(API3.Filters.byClass("Structure"),API3.Filters.byCanGarrison())).toEntityArray();
	buildings.forEach( function (struct) {
		if (struct.garrisoned() && struct.garrisoned().length)
			struct.unloadAll();
	});
};

// Count gatherers returning resources in the number of gatherers of resourceSupplies
// to prevent the AI always reaffecting idle workers to these resourceSupplies (specially in naval maps).
m.HQ.prototype.assignGatherers = function(gameState)
{
	for each (var base in this.baseManagers)
	{
		base.workers.forEach( function (worker) {
			if (worker.unitAIState().split(".")[1] !== "RETURNRESOURCE")
				return;
			var orders = worker.unitAIOrderData();
			if (orders.length < 2 || !orders[1].target || orders[1].target !== worker.getMetadata(PlayerID, "supply"))
				return;
			m.AddTCGatherer(gameState, orders[1].target);
		});
	}
};

// Some functions are run every turn
// Others once in a while
m.HQ.prototype.update = function(gameState, queues, events)
{
	Engine.ProfileStart("Headquarters update");
	
	this.territoryMap = m.createTerritoryMap(gameState);

	if (this.Config.debug > 0)
	{
		gameState.getOwnUnits().forEach (function (ent) {
			if (!ent.hasClass("CitizenSoldier") || ent.hasClass("Cavalry"))
				return;
			if (!ent.position())
				return;
			var idlePos = ent.setMetadata(PlayerID, "idlePos");
			if (idlePos === undefined || idlePos[0] !== ent.position()[0] || idlePos[1] !== ent.position()[1])
			{
				ent.setMetadata(PlayerID, "idlePos", ent.position());
				ent.setMetadata(PlayerID, "idleTim", gameState.ai.playedTurn);
				return;
			}
			if (gameState.ai.playedTurn - ent.getMetadata(PlayerID, "idleTim") < 50)
				return;
			warn(" unit idle since " + (gameState.ai.playedTurn-ent.getMetadata(PlayerID, "idleTim")) + " turns");
			warn(" unitai state " + ent.unitAIState());
			warn(" >>> base " + ent.getMetadata(PlayerID, "base"));
			warn(" >>> role " + ent.getMetadata(PlayerID, "role"));
			warn(" >>> subrole " + ent.getMetadata(PlayerID, "subrole"));
			warn(" >>> gather-type " + ent.getMetadata(PlayerID, "gather-type"));
			warn(" >>> target-foundation " + ent.getMetadata(PlayerID, "target-foundation"));
			warn(" >>> PartOfArmy " + ent.getMetadata(PlayerID, "PartOfArmy"));
			warn(" >>> plan " + ent.getMetadata(PlayerID, "plan"));
			warn(" >>> transport " + ent.getMetadata(PlayerID, "transport"));
			ent.setMetadata(PlayerID, "idleTim", gameState.ai.playedTurn);
		});
	}

	this.checkEvents(gameState, events, queues);

	this.researchManager.checkPhase(gameState, queues);

	// TODO find a better way to update
	if (this.phaseStarted && gameState.currentPhase() === this.phaseStarted)
	{
		this.phaseStarted = undefined;
		this.updateTerritories(gameState);
	}
	else if (gameState.ai.playedTurn - this.lastTerritoryUpdate > 100)
		this.updateTerritories(gameState);

	if (this.baseManagers[1])
	{
		this.trainMoreWorkers(gameState, queues);

		if (gameState.ai.playedTurn % 2 === 1)
			this.buildMoreHouses(gameState,queues);

		if (gameState.ai.playedTurn % 4 === 2 && !this.saveResources)
			this.buildFarmstead(gameState, queues);

		if (queues.minorTech.length() === 0 && gameState.ai.playedTurn % 5 === 1)
			this.researchManager.update(gameState, queues);
	}

	if (gameState.currentPhase() > 1)
	{
		// sandbox doesn't expand
		if (this.Config.difficulty > 0 && gameState.ai.playedTurn % 10 === 7)
			this.checkBaseExpansion(gameState, queues);

		if (!this.saveResources)
		{
			this.buildMarket(gameState, queues);
			this.buildBlacksmith(gameState, queues);
			this.buildTemple(gameState, queues);
		}

		if (this.Config.difficulty > 1)
			this.tradeManager.update(gameState, queues);
	}

	this.garrisonManager.update(gameState, events);
	this.defenseManager.update(gameState, events);

	if (!this.saveResources)
		this.constructTrainingBuildings(gameState, queues);

	if (this.Config.difficulty > 0)
		this.buildDefenses(gameState, queues);

	this.assignGatherers(gameState);
	for (var i in this.baseManagers)
	{
		this.baseManagers[i].checkEvents(gameState, events, queues);
		if (((+i + gameState.ai.playedTurn)%(m.playerGlobals[PlayerID].uniqueIDBases - 1)) === 0)
			this.baseManagers[i].update(gameState, queues, events);
	}

	this.navalManager.update(gameState, queues, events);
	
	if (this.Config.difficulty > 0)
		this.attackManager.update(gameState, queues, events);

	Engine.ProfileStop();	// Heaquarters update
};

return m;

}(PETRA);
