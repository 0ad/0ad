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
	
	this.econState = "growth";	// existing values: growth, townPhasing.
	this.phaseStarted = undefined;

	// cache the rates.
	this.wantedRates = { "food": 0, "wood": 0, "stone":0, "metal": 0 };
	this.currentRates = { "food": 0, "wood": 0, "stone":0, "metal": 0 };
	this.lastFailedGather = { "wood": undefined, "stone": undefined, "metal": undefined }; 

	// this means we'll have about a big third of women, and thus we can maximize resource gathering rates.
	this.femaleRatio = this.Config.Economy.femaleRatio;

	this.lastTerritoryUpdate = -1;
	this.stopBuilding = new Map(); // list of buildings to stop (temporarily) production because no room

	this.towerStartTime = 0;
	this.towerLapseTime = this.Config.Military.towerLapseTime;
	this.fortressStartTime = 0;
	this.fortressLapseTime = this.Config.Military.fortressLapseTime;

	this.baseManagers = [];
	this.attackManager = new m.AttackManager(this.Config);
	this.defenseManager = new m.DefenseManager(this.Config);
	this.tradeManager = new m.TradeManager(this.Config);
	this.navalManager = new m.NavalManager(this.Config);
	this.researchManager = new m.ResearchManager(this.Config);
	this.diplomacyManager = new m.DiplomacyManager(this.Config);
	this.garrisonManager = new m.GarrisonManager();
};

// More initialisation for stuff that needs the gameState
m.HQ.prototype.init = function(gameState, queues)
{
	this.territoryMap = m.createTerritoryMap(gameState);
	// initialize base map. Each pixel is a base ID, or 0 if not or not accessible
	this.basesMap = new API3.Map(gameState.sharedScript, "territory");
	// area of n cells on the border of the map : 0=inside map, 1=border map, 2=border+inaccessible
	this.borderMap = m.createBorderMap(gameState);
	// initialize frontier map. Each cell is 2 if on the near frontier, 1 on the frontier and 0 otherwise
	this.frontierMap = m.createFrontierMap(gameState);
	// list of allowed regions
	this.landRegions = {};
	// try to determine if we have a water map
	this.navalMap = false;
	this.navalRegions = {};

	if (this.Config.difficulty < 2)
		this.targetNumWorkers = Math.max(1, Math.min(40, Math.floor(gameState.getPopulationMax())));
	else if (this.Config.difficulty < 3)
		this.targetNumWorkers = Math.max(1, Math.min(60, Math.floor(gameState.getPopulationMax()/2)));
	else
		this.targetNumWorkers = Math.max(1, Math.min(120, Math.floor(gameState.getPopulationMax()/3)));

	this.treasures = gameState.getEntities().filter(function (ent) {
		var type = ent.resourceSupplyType();
		if (type && type.generic === "treasure")
			return true;
		return false;
	});
	this.treasures.registerUpdates();
};

/**
 * initialization needed after deserialization (only called when deserialization)
 */
m.HQ.prototype.postinit = function(gameState)
{
	// Rebuild the base maps from the territory indices of each base
	this.basesMap = new API3.Map(gameState.sharedScript, "territory");
	for (let base of this.baseManagers)
		for (let j of base.territoryIndices)
			this.basesMap.map[j] = base.ID;

	for (let ent of gameState.getOwnEntities().values())
	{
		if (!ent.resourceDropsiteTypes() || ent.hasClass("Elephant"))
			continue;
		let base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
		base.assignResourceToDropsite(gameState, ent);
	}
};

// returns the sea index linking regions 1 and region 2 (supposed to be different land region)
// otherwise return undefined
// for the moment, only the case land-sea-land is supported
m.HQ.prototype.getSeaIndex = function (gameState, index1, index2)
{
	var path = gameState.ai.accessibility.getTrajectToIndex(index1, index2);
	if (path && path.length == 3 && gameState.ai.accessibility.regionType[path[1]] === "water")
		return path[1];
	else
	{
		if (this.Config.debug > 1)
		{
			API3.warn("bad path from " + index1 + " to " + index2 + " ??? " + uneval(path));
			API3.warn(" regionLinks start " + uneval(gameState.ai.accessibility.regionLinks[index1]));
			API3.warn(" regionLinks end   " + uneval(gameState.ai.accessibility.regionLinks[index2]));
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
			
		if (ent.getMetadata(PlayerID, "base") == -1)
		{
			// Okay so let's try to create a new base around this.
			let newbase = new m.BaseManager(gameState, this.Config);
			newbase.init(gameState, true);
			newbase.setAnchor(gameState, ent);
			this.baseManagers.push(newbase);
			// Let's get a few units from other bases there to build this.
			var builders = this.bulkPickWorkers(gameState, newbase, 10);
			if (builders !== false)
			{
				builders.forEach(function (worker) {
					worker.setMetadata(PlayerID, "base", newbase.ID);
					worker.setMetadata(PlayerID, "subrole", "builder");
					worker.setMetadata(PlayerID, "target-foundation", ent.id());
				});
			}
		}
		else if (ent.hasClass("Wonder") && gameState.getGameType() === "wonder")
		{
			// Let's get a few units from other bases there to build this.
			var base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
			var builders = this.bulkPickWorkers(gameState, base, 10);
			if (builders !== false)
			{
				builders.forEach(function (worker) {
					worker.setMetadata(PlayerID, "base", baseID);
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
			if (evt.newentity === evt.entity)  // repaired building
				continue;
			var ent = gameState.getEntityById(evt.newentity);
			if (!ent || !ent.isOwn(PlayerID))
				continue;

			if (ent.getMetadata(PlayerID, "baseAnchor") == true)
			{
				let base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
				if (base.constructing)
					base.constructing = false;
				base.anchor = ent;
				base.anchorId = evt.newentity;
				base.buildings.updateEnt(ent);
				this.updateTerritories(gameState);
				if (base.ID === this.baseManagers[1].ID)
				{
					// this is our first base, let us configure our starting resources
					this.configFirstBase(gameState);
				}
				else
				{
					// let us hope this new base will fix our possible resource shortage
					this.saveResources = undefined;
					this.saveSpace = undefined;
				}
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
		if (metadata && metadata.garrisonType)
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
				ent.setMetadata(PlayerID, "garrisonType", undefined);
			}
		}
	}
};

// Called by the "town phase" research plan once it's started
m.HQ.prototype.OnTownPhase = function(gameState)
{
	if (this.Config.difficulty > 2 && this.femaleRatio > 0.4)
		this.femaleRatio = 0.4;

	this.phaseStarted = 2;
};

// Called by the "city phase" research plan once it's started
m.HQ.prototype.OnCityPhase = function(gameState)
{
	if (this.Config.difficulty > 2 && this.femaleRatio > 0.3)
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

	// Anticipate the optimal batch size when this queue will start
	// and adapt the batch size of the first and second queued workers and females to the present population
	// to ease a possible recovery if our population was drastically reduced by an attack
	// (need to go up to second queued as it is accounted in queueManager)
	if (numWorkers < 12)
		var size = 1;
	else
		var size =  Math.min(5, Math.ceil(numWorkers / 10));
	if (queues.villager.queue[0])
	{
		queues.villager.queue[0].number = Math.min(queues.villager.queue[0].number, size);
		if (queues.villager.queue[1])
			queues.villager.queue[1].number = Math.min(queues.villager.queue[1].number, size);
	}
	if (queues.citizenSoldier.queue[0])
	{
		queues.citizenSoldier.queue[0].number = Math.min(queues.citizenSoldier.queue[0].number, size);
		if (queues.citizenSoldier.queue[1])
			queues.citizenSoldier.queue[1].number = Math.min(queues.citizenSoldier.queue[1].number, size);
	}

	var numQueuedF = queues.villager.countQueuedUnits();
	var numQueuedS = queues.citizenSoldier.countQueuedUnits();
	var numQueued = numQueuedS + numQueuedF;
	var numTotal = numWorkers + numQueued;

	if (this.saveResources && numTotal > this.Config.Economy.popForTown + 10)
		return;
	if (numTotal > this.targetNumWorkers || (numTotal >= this.Config.Economy.popForTown 
		&& gameState.currentPhase() == 1 && !gameState.isResearching(gameState.townPhase())))
		return;
	if (numQueued > 50 || (numQueuedF > 20 && numQueuedS > 20) || numInTraining > 15)
		return;

	// default template
	var template = gameState.applyCiv("units/{civ}_support_female_citizen");

	// Choose whether we want soldiers instead.
	if ((numFemales+numQueuedF) > 8 && (numFemales+numQueuedF)/numTotal > this.femaleRatio)
	{
		if (numTotal < 45)
			var requirements = [ ["cost", 1], ["speed", 0.5], ["costsResource", 0.5, "stone"], ["costsResource", 0.5, "metal"]];
		else
			var requirements = [ ["strength", 1] ];

		template = undefined;
		var proba = Math.random();
		if (proba < 0.6)
		{	// we require at least 30% ranged and 30% melee
			if (proba < 0.3)
				var classes = ["CitizenSoldier", "Infantry", "Ranged"];
			else
				var classes = ["CitizenSoldier", "Infantry", "Melee"];
			template = this.findBestTrainableUnit(gameState, classes, requirements);
		}
		if (!template)
			template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], requirements);
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
	if (classes.indexOf("Hero") != -1)
		var units = gameState.findTrainableUnits(classes, []);
	else if (classes.indexOf("Siege") != -1)	// We do not want siege tower as AI does not know how to use it
		var units = gameState.findTrainableUnits(classes, ["SiegeTower"]);
	else						// We do not want hero when not explicitely specified
		var units = gameState.findTrainableUnits(classes, ["Hero"]);
	
	if (units.length == 0)
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
		for (let param of parameters)
		{
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
m.HQ.prototype.bulkPickWorkers = function(gameState, baseRef, number)
{
	var accessIndex = baseRef.accessIndex;
	if (!accessIndex)
		return false;
	// sorting bases by whether they are on the same accessindex or not.
	var baseBest = this.baseManagers.slice().sort(function (a,b) {
		if (a.accessIndex == accessIndex && b.accessIndex != accessIndex)
			return -1;
		else if (b.accessIndex == accessIndex && a.accessIndex != accessIndex)
			return 1;
		return 0;
	});

	var needed = number;
	var workers = new API3.EntityCollection(gameState.sharedScript);
	for (let base of baseBest)
	{
		if (base.ID === baseRef.ID)
			continue;
		base.pickBuilders(gameState, workers, needed);
		if (workers.length < number)
			needed = number - workers.length;
		else
			break;
	}
	if (workers.length === 0)
		return false;
	return workers;
};

m.HQ.prototype.getTotalResourceLevel = function(gameState)
{
	var total = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	for (var base of this.baseManagers)
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
	
	for (var base of this.baseManagers)
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
	this.wantedRates = gameState.ai.queueManager.wantedGatherRates(gameState);
	var currentRates = this.GetCurrentGatherRates(gameState);

	var needed = [];
	for (var res in this.wantedRates)
		needed.push({ "type": res, "wanted": this.wantedRates[res], "current": currentRates[res] });

	needed.sort(function(a, b) {
		var va = (Math.max(0, a.wanted - a.current))/ (a.current+1);
		var vb = (Math.max(0, b.wanted - b.current))/ (b.current+1);
		
		// If they happen to be equal (generally this means "0" aka no need), make it fair.
		if (va === vb)
			return (b.wanted/(b.current+1)) - (a.wanted/(a.current+1));
		return vb-va;
	});
	return needed;
};

// Returns the best position to build a new Civil Centre
// Whose primary function would be to reach new resources of type "resource".
m.HQ.prototype.findEconomicCCLocation = function(gameState, template, resource, proximity, fromStrategic)
{	
	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.

	Engine.ProfileStart("findEconomicCCLocation");

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0, template);
	obstructions.expandInfluences();

	var ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
	var dpEnts = gameState.getOwnDropsites().filter(API3.Filters.not(API3.Filters.byClassesOr(["CivCentre", "Elephant"])));
	var ccList = [];
	for (var cc of ccEnts.values())
		ccList.push({"pos": cc.position(), "ally": gameState.isPlayerAlly(cc.owner())});
	var dpList = [];
	for (var dp of dpEnts.values())
		dpList.push({"pos": dp.position()});

	var bestIdx = undefined;
	var bestVal = undefined;
	var radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);
	var scale = 250 * 250;
	var proxyAccess = undefined;
	var nbShips = this.navalManager.transportShips.length;
	if (proximity)	// this is our first base
	{
		// if our first base, ensure room around
		radius = Math.ceil((template.obstructionRadius() + 8) / obstructions.cellSize);
		// scale is the typical scale at which we want to find a location for our first base
		// look for bigger scale if we start from a ship (access < 2) or from a small island
		var cellArea = gameState.getMap().cellSize * gameState.getMap().cellSize;
		proxyAccess = gameState.ai.accessibility.getAccessValue(proximity);
		if (proxyAccess < 2 || cellArea*gameState.ai.accessibility.regionSize[proxyAccess] < 24000)
			scale = 400 * 400;
	}

	var width = this.territoryMap.width;
	var cellSize = this.territoryMap.cellSize;

	for (var j = 0; j < this.territoryMap.length; ++j)
	{	
		if (this.territoryMap.getOwnerIndex(j) != 0)
			continue;
		// We require that it is accessible
		var index = gameState.ai.accessibility.landPassMap[j];
		if (!this.landRegions[index])
			continue;
		if (proxyAccess && nbShips === 0 && proxyAccess !== index)
			continue;
		// and with enough room around to build the cc
		var i = API3.getMaxMapIndex(j, this.territoryMap, obstructions);
		if (obstructions.map[i] <= radius)
			continue;

		var norm = 0.5;   // TODO adjust it, knowing that we will sum 5 maps
		// checking distance to other cc
		var pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];

		if (proximity)	// this is our first cc, let's do it near our units 
		{
			var dist = API3.SquareVectorDistance(proximity, pos);
			norm /= (1 + dist/scale);
		}
		else
		{
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
				if (dist < 30000)    // Reject if too near from an allied cc
				{
					norm = 0
					break;
				}
				if (dist < 50000)   // Disfavor if quite near an allied cc
					norm *= 0.5;
				if (dist < minDist)
					minDist = dist;
			}
			if (norm == 0)
				continue;

			if (minDist > 170000 && !this.navalMap)	// Reject if too far from any allied cc (not connected)
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
		}

		if (this.borderMap.map[j] > 0)	// disfavor the borders of the map
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

	var cut = 60;
	if (fromStrategic || proximity)  // be less restrictive
		cut = 30;
	if (this.Config.debug > 1)
		API3.warn("we have found a base for " + resource + " with best (cut=" + cut + ") = " + bestVal);
	// not good enough.
	if (bestVal < cut)
		return false;
	
	var i = API3.getMaxMapIndex(bestIdx, this.territoryMap, obstructions);
	var x = (i % obstructions.width + 0.5) * obstructions.cellSize;
	var z = (Math.floor(i / obstructions.width) + 0.5) * obstructions.cellSize;

	// Define a minimal number of wanted ships in the seas reaching this new base
	var index = gameState.ai.accessibility.landPassMap[i];
	for (var base of this.baseManagers)
	{
		if (!base.anchor || base.accessIndex === index)
			continue;
		var sea = this.getSeaIndex(gameState, base.accessIndex, index);
		if (sea !== undefined)
			this.navalManager.setMinimalTransportShips(gameState, sea, 1);
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

	var ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
	var ccList = [];
	var numAllyCC = 0;
	for (var cc of ccEnts.values())
	{
		var ally = gameState.isPlayerAlly(cc.owner());
		ccList.push({"pos": cc.position(), "ally": ally});
		if (ally)
			++numAllyCC;
	}
	if (numAllyCC < 2)
		return this.findEconomicCCLocation(gameState, template, "wood", undefined, true);

	Engine.ProfileStart("findStrategicCCLocation");

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0, template);
	obstructions.expandInfluences();

	var bestIdx = undefined;
	var bestVal = undefined;
	var radius =  Math.ceil(template.obstructionRadius() / obstructions.cellSize);

	var width = this.territoryMap.width;
	var cellSize = this.territoryMap.cellSize;
	var currentVal, delta;
	var distcc0, distcc1, distcc2;

	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.territoryMap.getOwnerIndex(j) != 0)
			continue;
		// We require that it is accessible
		var index = gameState.ai.accessibility.landPassMap[j];
		if (!this.landRegions[index])
			continue;
		// and with enough room around to build the cc
		var i = API3.getMaxMapIndex(j, this.territoryMap, obstructions);
		if (obstructions.map[i] <= radius)
			continue;

		// checking distances to other cc
		var pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
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
		if (this.borderMap.map[j] > 0)
			currentVal += 10000;		

		if (bestVal !== undefined && currentVal > bestVal)
			continue;
		bestVal = currentVal;
		bestIdx = j;
	}

	if (this.Config.debug > 1)
		API3.warn("We've found a strategic base with bestVal = " + bestVal);	

	Engine.ProfileStop();

	if (bestVal === undefined)
		return undefined;

	var i = API3.getMaxMapIndex(bestIdx, this.territoryMap, obstructions);
	var x = (i % obstructions.width + 0.5) * obstructions.cellSize;
	var z = (Math.floor(i / obstructions.width) + 0.5) * obstructions.cellSize;

	// Define a minimal number of wanted ships in the seas reaching this new base
	var index = gameState.ai.accessibility.landPassMap[i];
	for (var base of this.baseManagers)
	{
		if (!base.anchor || base.accessIndex === index)
			continue;
		var sea = this.getSeaIndex(gameState, base.accessIndex, index);
		if (sea !== undefined)
			this.navalManager.setMinimalTransportShips(gameState, sea, 1);
	}

	return [x,z];
};

// Returns the best position to build a new market: if the allies already have a market, build it as far as possible
// from it, although not in our border to be able to defend it easily. If no allied market, our second market will
// follow the same logic
// TODO check that it is on same accessIndex
m.HQ.prototype.findMarketLocation = function(gameState, template)
{
	var markets = gameState.updatingCollection("ExclusiveAllyMarkets", API3.Filters.byClass("Market"), gameState.getExclusiveAllyEntities()).toEntityArray();
	if (!markets.length)
		markets = gameState.updatingCollection("OwnMarkets", API3.Filters.byClass("Market"), gameState.getOwnStructures()).toEntityArray();

	if (!markets.length)	// this is the first market. For the time being, place it arbitrarily by the ConstructionPlan
		return [-1, -1, -1, 0];

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0, template);
	obstructions.expandInfluences();

	var bestIdx = undefined;
	var bestVal = undefined;
	var radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);
	var isNavalMarket = template.hasClass("NavalMarket");

	var width = this.territoryMap.width;
	var cellSize = this.territoryMap.cellSize;

	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		// do not try on the border of our territory
		if (this.frontierMap.map[j] == 2)
			continue;
		if (this.basesMap.map[j] == 0)   // only in our territory
			continue;
		// with enough room around to build the cc
		var i = API3.getMaxMapIndex(j, this.territoryMap, obstructions);
		if (obstructions.map[i] <= radius)
			continue;
		var index = gameState.ai.accessibility.landPassMap[i];
		if (!this.landRegions[index])
			continue;

		var pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		// checking distances to other markets
		var maxDist = 0;
		for (var market of markets)
		{
			if (isNavalMarket && market.hasClass("NavalMarket"))
			{
				// TODO check that there are on the same sea. For the time being, we suppose it is true
			}
			else if (gameState.ai.accessibility.getAccessValue(market.position()) != index)
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

	if (this.Config.debug > 1)
		API3.warn("We found a market position with bestVal = " + bestVal);	

	if (bestVal === undefined)  // no constraints. For the time being, place it arbitrarily by the ConstructionPlan
		return [-1, -1, -1, 0];

	var expectedGain = Math.round(bestVal / this.Config.distUnitGain);
	if (this.Config.debug > 1)
		API3.warn("this would give a trading gain of " + expectedGain);
	// do not keep it if gain is too small, except if this is our first BarterMarket 
	if (expectedGain < 3 ||
		(expectedGain < 8 && (!template.hasClass("BarterMarket") || gameState.getOwnStructures().filter(API3.Filters.byClass("BarterMarket")).length > 0)))
		return false;

	var i = API3.getMaxMapIndex(bestIdx, this.territoryMap, obstructions);
	var x = (i % obstructions.width + 0.5) * obstructions.cellSize;
	var z = (Math.floor(i / obstructions.width) + 0.5) * obstructions.cellSize;
	return [x, z, this.basesMap.map[bestIdx], expectedGain];
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

	var wonderMode = (gameState.getGameType() === "wonder");
	var wonderDistmin = undefined;
	if (wonderMode)
	{
		var wonders = gameState.getOwnStructures().filter(API3.Filters.byClass("Wonder")).toEntityArray();
		wonderMode = (wonders.length != 0);
		if (wonderMode)
			wonderDistmin = (50 + wonders[0].footprintRadius()) * (50 + wonders[0].footprintRadius());
	}

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0, template);
	obstructions.expandInfluences();

	var bestIdx = undefined;
	var bestVal = undefined;
	var width = this.territoryMap.width;
	var cellSize = this.territoryMap.cellSize;

	var isTower = template.hasClass("Tower");
	var isFortress = template.hasClass("Fortress");
	if (isFortress)
		var radius = Math.floor((template.obstructionRadius() + 8) / obstructions.cellSize);
	else
		var radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);

	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		if (!wonderMode)
		{
			// do not try if well inside or outside territory
			if (this.frontierMap.map[j] == 0)
				continue
			if (this.frontierMap.map[j] == 1 && isTower)
				continue;
		}
		if (this.basesMap.map[j] == 0)   // inaccessible cell
			continue;
		// and with enough room around to build the cc
		var i = API3.getMaxMapIndex(j, this.territoryMap, obstructions);
		if (obstructions.map[i] <= radius)
			continue;

		var pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		// checking distances to other structures
		var minDist = Math.min();

		var dista = 0;
		if (wonderMode)
		{
			dista = API3.SquareVectorDistance(wonders[0].position(), pos);
			if (dista < wonderDistmin)
				continue;
			dista *= 200;   // empirical factor (TODO should depend on map size) to stay near the wonder
		}

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
			if (str.hasClass("CivCentre") && dist + dista < minDist)
				minDist = dist + dista;
		}
		if (minDist < 0)
			continue;

		for (var str of ownStructures)
		{
			var strPos = str.position();
			if (!strPos)
				continue;
			var dist = API3.SquareVectorDistance(strPos, pos);
			if ((isTower && str.hasClass("Tower")) || (isFortress && str.hasClass("Fortress")))
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

	var i = API3.getMaxMapIndex(bestIdx, this.territoryMap, obstructions);
	var x = (i % obstructions.width + 0.5) * obstructions.cellSize;
	var z = (Math.floor(i / obstructions.width) + 0.5) * obstructions.cellSize;
	return [x, z, this.basesMap.map[bestIdx]];
};

m.HQ.prototype.buildTemple = function(gameState, queues)
{
	// at least one market (which have the same queue) should be build before any temple
	if (gameState.currentPhase() < 3 || queues.economicBuilding.countQueuedUnits() != 0
		|| gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_temple"), true) != 0
		|| gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) == 0)
		return;
	if (!this.canBuild(gameState, "structures/{civ}_temple"))
		return;
	queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_temple"));
};

m.HQ.prototype.buildMarket = function(gameState, queues)
{
	if (gameState.getPopulation() < this.Config.Economy.popForMarket ||
		queues.economicBuilding.countQueuedUnitsWithClass("BarterMarket") != 0 ||
		gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) != 0)
		return;
	if (!this.canBuild(gameState, "structures/{civ}_market"))
		return;
	gameState.ai.queueManager.changePriority("economicBuilding", 3*this.Config.priorities.economicBuilding);
	var plan = new m.ConstructionPlan(gameState, "structures/{civ}_market");
	plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("economicBuilding", gameState.ai.Config.priorities.economicBuilding); };
	queues.economicBuilding.addItem(plan);
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
		// change the starting condition according to the situation.
		plan.isGo = function (gameState) {
			if (!gameState.ai.HQ.canBuild(gameState, "structures/{civ}_house"))
				return false;
			if (gameState.getPopulationMax() <= gameState.getPopulationLimit())
				return false;
			var HouseNb = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_house"), true);

			var freeSlots = 0;
			// TODO how to modify with tech
			var popBonus = gameState.getTemplate(gameState.applyCiv("structures/{civ}_house")).getPopulationBonus();
			freeSlots = gameState.getPopulationLimit() + HouseNb*popBonus - gameState.getPopulation();
			if (gameState.ai.HQ.saveResources)
				return (freeSlots <= 10);
			else if (gameState.getPopulation() > 55)
				return (freeSlots <= 21);
			else if (gameState.getPopulation() > 30)
				return (freeSlots <= 15);
			else
				return (freeSlots <= 10);
		};
		queues.house.addItem(plan);
	}

	if (numPlanned > 0 && this.econState == "townPhasing")
	{
		var count = gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length;
		if (count < 5 && this.stopBuilding.has(gameState.applyCiv("structures/{civ}_house")))
		{
			if (this.Config.debug > 1)
				API3.warn("no room to place a house ... try to be less restrictive");
			this.stopBuilding.delete(gameState.applyCiv("structures/{civ}_house"));
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
	var house = gameState.applyCiv("structures/{civ}_house");
	var HouseNb = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_house"), true);
	var freeSlots = 0;
	var popBonus = gameState.getTemplate(house).getPopulationBonus();
	freeSlots = gameState.getPopulationLimit() + HouseNb*popBonus - gameState.getPopulation();
	if (freeSlots < 5)
	{
		if (this.stopBuilding.has(house))
		{
			if (this.stopBuilding.get(house) > gameState.ai.elapsedTime)
			{
				if (this.Config.debug > 1)
					API3.warn("no room to place a house ... try to improve with technology");
				this.researchManager.researchPopulationBonus(gameState, queues);
			}
			else
			{
				this.stopBuilding.delete(house);
				var priority = 2*this.Config.priorities.house;
			}
		}
		else
			var priority = 2*this.Config.priorities.house;
	}
	else
		var priority = this.Config.priorities.house;
	if (priority && priority != gameState.ai.queueManager.getPriority("house"))
		gameState.ai.queueManager.changePriority("house", priority);
};

// checks the status of the territory expansion. If no new economic bases created, build some strategic ones.
m.HQ.prototype.checkBaseExpansion = function(gameState, queues)
{
	if (queues.civilCentre.length() > 0)
		return;
	// first build one cc if all have been destroyed
	if (this.numActiveBase() < 1)
	{
		this.buildFirstBase(gameState);
		return;
	}
	// then expand if we have not enough room available for buildings
	if (this.stopBuilding.size > 1)
	{
		if (this.Config.debug > 2)
			API3.warn("try to build a new base because not enough room to build " + uneval(this.stopBuilding));
		this.buildNewBase(gameState, queues);
		return;
	}
	// then expand if we have lots of units
	var numUnits = 	gameState.getOwnUnits().length;
	var popForBase = this.Config.Economy.popForTown + 20;
	if (this.saveResources)
		popForBase = this.Config.Economy.popForTown + 5;
	if (Math.floor(numUnits/popForBase) >= gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre")).length)
	{
		if (this.Config.debug > 2)
			API3.warn("try to build a new base because of population " + numUnits + " for " + this.numActiveBase() + " CCs");
		this.buildNewBase(gameState, queues);
	}
};

m.HQ.prototype.buildNewBase = function(gameState, queues, resource)
{
	if (this.numActiveBase() > 0 && gameState.currentPhase() == 1 && !gameState.isResearching(gameState.townPhase()))
		return false;
	var template = (this.numActiveBase() > 0) ? this.bBase[0] : gameState.applyCiv("structures/{civ}_civil_centre");
	if (gameState.countFoundationsByType(template, true) > 0 || queues.civilCentre.length() > 0)
		return false;
	if (!this.canBuild(gameState, template))
		return false;

	// base "-1" means new base.
	if (this.Config.debug > 1)
		API3.warn("new base planned with resource " + resource);
	queues.civilCentre.addItem(new m.ConstructionPlan(gameState, template, { "base": -1, "resource": resource }));
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
		if (queues.defenseBuilding.length() == 0 && this.canBuild(gameState, fortressType))
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
			if (this.attackManager.upcomingAttacks["Attack"].length != 0)
				attack = this.attackManager.upcomingAttacks["Attack"][0];
			else if (this.attackManager.upcomingAttacks["HugeAttack"].length != 0)
				attack = this.attackManager.upcomingAttacks["HugeAttack"][0];

			if (attack && !attack.unitStat["Siege"])
				attack.addSiegeUnits(gameState);
		}
	}

	if (gameState.currentPhase() < 2 
		|| queues.defenseBuilding.length() != 0
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
		|| queues.militaryBuilding.length() != 0
		|| gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_blacksmith"), true) > 0)
		return;
	// build a market before the blacksmith
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) == 0)
		return;

	if (this.canBuild(gameState, "structures/{civ}_blacksmith"))
		queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_blacksmith"));
};

m.HQ.prototype.buildWonder = function(gameState, queues)
{
	if (!this.canBuild(gameState, "structures/{civ}_wonder"))
		return;
	if (gameState.ai.queues["wonder"] && gameState.ai.queues["wonder"].length() > 0)
		return;
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_wonder"), true) > 0)
		return;

	if (!gameState.ai.queues["wonder"])
		gameState.ai.queueManager.addQueue("wonder", 1000);
	gameState.ai.queues["wonder"].addItem(new m.ConstructionPlan(gameState, "structures/{civ}_wonder"));
};

// Deals with constructing military buildings (barracks, stables…)
// They are mostly defined by Config.js. This is unreliable since changes could be done easily.
// TODO: We need to determine these dynamically. Also doesn't build fortresses since the above function does that.
// TODO: building placement is bad. Choice of buildings is also fairly dumb.
m.HQ.prototype.constructTrainingBuildings = function(gameState, queues)
{
	if (this.canBuild(gameState, "structures/{civ}_barracks") && queues.militaryBuilding.length() == 0)
	{
		var barrackNb = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_barracks"), true);
		// first barracks.
		if (barrackNb == 0 && (gameState.getPopulation() > this.Config.Military.popForBarracks1 ||
			(this.econState == "townPhasing" && gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length < 5)))
		{
			gameState.ai.queueManager.changePriority("militaryBuilding", 2*this.Config.priorities.militaryBuilding);
			var preferredBase = this.findBestBaseForMilitary(gameState);
			var plan = new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase });
			plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("militaryBuilding", gameState.ai.Config.priorities.militaryBuilding); };
			queues.militaryBuilding.addItem(plan);
		}
		// second barracks, then 3rd barrack, and optional 4th for some civs as they rely on barracks more.
		else if (barrackNb == 1 && gameState.getPopulation() > this.Config.Military.popForBarracks2)
		{
			var preferredBase = this.findBestBaseForMilitary(gameState);
			queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase }));
		}
		else if (barrackNb == 2 && gameState.getPopulation() > this.Config.Military.popForBarracks2 + 20)
		{
			var preferredBase = this.findBestBaseForMilitary(gameState);
			queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase }));
		}
		else if (barrackNb == 3 && gameState.getPopulation() > this.Config.Military.popForBarracks2 + 50
			&& (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber"))
		{
			var preferredBase = this.findBestBaseForMilitary(gameState);
			queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase }));
		}
	}

	//build advanced military buildings
	if (gameState.currentPhase() > 2 && gameState.getPopulation() > 80 && queues.militaryBuilding.length() == 0 && this.bAdvanced.length != 0)
	{
		var nAdvanced = 0;
		for (var advanced of this.bAdvanced)
			nAdvanced += gameState.countEntitiesAndQueuedByType(advanced, true);

		if (nAdvanced == 0 || (nAdvanced < this.bAdvanced.length && gameState.getPopulation() > 120))
		{
			for (var advanced of this.bAdvanced)
			{
				if (gameState.countEntitiesAndQueuedByType(advanced, true) < 1 && this.canBuild(gameState, advanced))
				{
					var preferredBase = this.findBestBaseForMilitary(gameState);
					queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, advanced, { "preferredBase": preferredBase }));
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
	var ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre")).toEntityArray();
	var bestBase = 1;
	var distMin = Math.min();
	for (var cce of ccEnts)
	{
		if (gameState.isPlayerAlly(cce.owner()))
			continue;
		for (var cc of ccEnts)
		{
			if (cc.owner() != PlayerID)
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
	if (gameState.ai.queues.emergency.countQueuedUnits() !== 0)
		return false;

	// find nearest base anchor
	var distcut = 20000;
	var nearestAnchor = undefined;
	var distmin = undefined;
	for (let pos of positions)
	{
		let access = gameState.ai.accessibility.getAccessValue(pos);
		// check nearest base anchor
		for (let base of this.baseManagers)
		{
			if (!base.anchor || !base.anchor.position())
				continue;
			if (base.anchor.getMetadata(PlayerID, "access") !== access)
				continue;
			if (!base.anchor.trainableEntities())	// base still in construction
				continue;
			let queue = base.anchor._entity.trainingQueue
			if (queue)
			{
				let time = 0;
				for (let item of queue)
					if (item.progress > 0 || (item.metadata && item.metadata.garrisonType))
						time += item.timeRemaining;
				if (time/1000 > 5)
					continue;
			}
			let dist = API3.SquareVectorDistance(base.anchor.position(), pos);
			if (nearestAnchor && dist > distmin)
				continue;
			distmin = dist;
			nearestAnchor = base.anchor;
		}
	}
	if (!nearestAnchor || distmin > distcut)
		return false;

	// We will choose randomly ranged and melee units, except when garrisonHolder is full
	// in which case we prefer melee units
	var numGarrisoned = this.garrisonManager.numberOfGarrisonedUnits(nearestAnchor);
	if (nearestAnchor._entity.trainingQueue)
	{
		for (var item of nearestAnchor._entity.trainingQueue)
		{
			if (item.metadata && item.metadata["garrisonType"])
				numGarrisoned += item.count;
			else if (!item.progress && (!item.metadata || !item.metadata.trainer))
				nearestAnchor.stopProduction(item.id);
		}
	}
	var autogarrison = (numGarrisoned < nearestAnchor.garrisonMax());
	var rangedWanted = (Math.random() > 0.5 && autogarrison);

	var total = gameState.getResources();
	var templateFound = undefined;
	var trainables = nearestAnchor.trainableEntities();
	var garrisonArrowClasses = nearestAnchor.getGarrisonArrowClasses();
	for (let trainable of trainables)
	{
		if (gameState.isDisabledTemplates(trainable))
			continue;
		let template = gameState.getTemplate(trainable);
		if (!template.hasClass("Infantry") || !template.hasClass("CitizenSoldier"))
			continue;
		if (autogarrison && !MatchesClassList(garrisonArrowClasses, template.classes()))
			continue;
		if (!total.canAfford(new API3.Resources(template.cost())))
			continue;
		templateFound = [trainable, template];
		if (template.hasClass("Ranged") === rangedWanted)
			break;
	}
	if (!templateFound)
		return false;

	// Check first if we can afford it without touching the other accounts
	// and if not, take some of other accounted resources
	// TODO sort the queues to be substracted
	let queueManager = gameState.ai.queueManager;
	let cost = new API3.Resources(templateFound[1].cost());
	queueManager.setAccounts(gameState, cost, "emergency");
	if (!queueManager.accounts["emergency"].canAfford(cost))
	{
		for (let p in queueManager.queues)
		{
			if (p === "emergency")
				continue;
			queueManager.transferAccounts(cost, p, "emergency");
			if (queueManager.accounts["emergency"].canAfford(cost))
				break;
		}
	}
	var metadata = { "role": "worker", "base": nearestAnchor.getMetadata(PlayerID, "base"), "trainer": nearestAnchor.id() };
	if (autogarrison)
		metadata.garrisonType = "protection";
	gameState.ai.queues.emergency.addItem(new m.TrainingPlan(gameState, templateFound[0], metadata, 1, 1));
	return true;
};

m.HQ.prototype.canBuild = function(gameState, structure)
{
	var type = gameState.applyCiv(structure); 
	// available room to build it
	if (this.stopBuilding.has(type))
	{
		if (this.stopBuilding.get(type) > gameState.ai.elapsedTime)
			return false;
		else
			this.stopBuilding.delete(type);
	}

	if (gameState.isDisabledTemplates(type))
	{
		this.stopBuilding.set(type, Infinity);
		return false;
	}

	var template = gameState.getTemplate(type);
	if (!template)
	{
		this.stopBuilding.set(type, Infinity);
		if (this.Config.debug > 0)
			API3.warn("Petra error: trying to build " + structure + " for civ " + gameState.civ() + " but no template found.");
	}
	if (!template || !template.available(gameState))
		return false;

	if (this.numActiveBase() < 1)
	{
		// if no base, check that we can build outside our territory
		var buildTerritories = template.buildTerritories();
		if (buildTerritories && (!buildTerritories.length || (buildTerritories.length === 1 && buildTerritories[0] === "own")))
		{
			this.stopBuilding.set(type, gameState.ai.elapsedTime + 180);
			return false;
		}
	}

	// build limits
	var limits = gameState.getEntityLimits();
	var category = template.buildCategory();
	if (category && limits[category] && gameState.getEntityCounts()[category] >= limits[category])
		return false;

	return true;
};

m.HQ.prototype.stopBuild = function(gameState, structure)
{
	let type = gameState.applyCiv(structure);
	if (this.stopBuilding.has(type))
		this.stopBuilding.set(type, Math.max(this.stopBuilding.get(type), gameState.ai.elapsedTime + 180));
	else
		this.stopBuilding.set(type, gameState.ai.elapsedTime + 180);
};

m.HQ.prototype.restartBuild = function(gameState, structure)
{
	let type = gameState.applyCiv(structure);
	if (this.stopBuilding.has(type))
		this.stopBuilding.delete(type);
};

m.HQ.prototype.updateTerritories = function(gameState)
{
	// TODO may-be update also when territory decreases. For the moment, only increases are taking into account
	if (this.lastTerritoryUpdate == gameState.ai.playedTurn)
		return;
	this.lastTerritoryUpdate = gameState.ai.playedTurn;

	var passabilityMap = gameState.getMap();
	var width = this.territoryMap.width;
	var cellSize = this.territoryMap.cellSize;
	var expansion = 0;
	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.borderMap.map[j] > 1)
			continue;
		if (this.territoryMap.getOwnerIndex(j) != PlayerID)
		{
			if (this.basesMap.map[j] == 0)
				continue;
			var base = this.getBaseByID(this.basesMap.map[j]);
			var index = base.territoryIndices.indexOf(j);
			if (index == -1)
			{
				API3.warn(" problem in headquarters::updateTerritories for base " + this.basesMap.map[j]);
				continue;
			}
			base.territoryIndices.splice(index, 1);
			this.basesMap.map[j] = 0;
		}
		else if (this.basesMap.map[j] == 0)
		{
			var landPassable = false;
			var ind = API3.getMapIndices(j, this.territoryMap, passabilityMap);
			var access;
			for (var k of ind)
			{
				if (!this.landRegions[gameState.ai.accessibility.landPassMap[k]])
					continue;
				landPassable = true;
				access = gameState.ai.accessibility.landPassMap[k];
				break;
			}
			if (!landPassable)
				continue;
			var distmin = Math.min();
			var baseID = undefined;
			var pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
			for (var base of this.baseManagers)
			{
				if (!base.anchor || !base.anchor.position())
					continue;
				if (base.accessIndex != access)
					continue;
				var dist = API3.SquareVectorDistance(base.anchor.position(), pos);
				if (dist >= distmin)
					continue;
				distmin = dist;
				baseID = base.ID;
			}
			if (!baseID)
				continue;
			this.getBaseByID(baseID).territoryIndices.push(j);
			this.basesMap.map[j] = baseID;
			expansion++;
		}
	}

	this.frontierMap =  m.createFrontierMap(gameState);

	if (!expansion)
		return;
	// We've increased our territory, so we may have some new room to build
	this.stopBuilding.clear();
	// And if sufficient expansion, check if building a new market would improve our present trade routes
	var cellArea = this.territoryMap.cellSize * this.territoryMap.cellSize;
	if (expansion * cellArea > 960)
		this.tradeManager.routeProspection = true;
};

/**
 * returns the base corresponding to baseID
 */
m.HQ.prototype.getBaseByID = function(baseID)
{
	for (let base of this.baseManagers)
		if (base.ID === baseID)
			return base;

	API3.warn("Petra error: no base found with ID " + baseID);
	return undefined;
};

/**
 * returns the number of active (i.e. with one cc) bases
 * TODO should be cached
 */
m.HQ.prototype.numActiveBase = function()
{
	let num = 0;
	for (let base of this.baseManagers)
		if (base.anchor)
			++num;
	return num;
};

// Count gatherers returning resources in the number of gatherers of resourceSupplies
// to prevent the AI always reaffecting idle workers to these resourceSupplies (specially in naval maps).
m.HQ.prototype.assignGatherers = function(gameState)
{
	for (var base of this.baseManagers)
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

// Check that the chosen position is not too near from an invading army
m.HQ.prototype.isDangerousLocation = function(pos)
{
	for (var army of this.defenseManager.armies)
		if (army.foePosition && API3.SquareVectorDistance(army.foePosition, pos) < 12000)
			return true;
	return false;
};

// Some functions are run every turn
// Others once in a while
m.HQ.prototype.update = function(gameState, queues, events)
{
	Engine.ProfileStart("Headquarters update");

	this.territoryMap = m.createTerritoryMap(gameState);

	if (this.Config.debug > 1)
	{
		gameState.getOwnUnits().forEach (function (ent) {
			if (!ent.hasClass("CitizenSoldier") || ent.hasClass("Cavalry"))
				return;
			if (!ent.position())
				return;
			var idlePos = ent.getMetadata(PlayerID, "idlePos");
			if (idlePos === undefined || idlePos[0] !== ent.position()[0] || idlePos[1] !== ent.position()[1])
			{
				ent.setMetadata(PlayerID, "idlePos", ent.position());
				ent.setMetadata(PlayerID, "idleTim", gameState.ai.playedTurn);
				return;
			}
			if (gameState.ai.playedTurn - ent.getMetadata(PlayerID, "idleTim") < 50)
				return;
			API3.warn(" unit idle since " + (gameState.ai.playedTurn-ent.getMetadata(PlayerID, "idleTim")) + " turns");
			m.dumpEntity(ent);
			ent.setMetadata(PlayerID, "idleTim", gameState.ai.playedTurn);
		});
	}

	this.checkEvents(gameState, events, queues);

	this.researchManager.checkPhase(gameState, queues);

	// TODO find a better way to update
	if (this.phaseStarted && gameState.currentPhase() == this.phaseStarted)
	{
		this.phaseStarted = undefined;
		this.updateTerritories(gameState);
	}
	else if (gameState.ai.playedTurn - this.lastTerritoryUpdate > 100)
		this.updateTerritories(gameState);

	if (gameState.getGameType() === "wonder")
		this.buildWonder(gameState, queues);

	if (this.numActiveBase() > 0)
	{
		this.trainMoreWorkers(gameState, queues);

		if (gameState.ai.playedTurn % 2 == 1)
			this.buildMoreHouses(gameState,queues);

		if (gameState.ai.playedTurn % 4 == 2 && !this.saveResources)
			this.buildFarmstead(gameState, queues);

		if (queues.minorTech.length() == 0 && gameState.ai.playedTurn % 5 == 1)
			this.researchManager.update(gameState, queues);
	}

	if (this.numActiveBase() < 1 ||
		(this.Config.difficulty > 0 && gameState.ai.playedTurn % 10 == 7 && gameState.currentPhase() > 1))
		this.checkBaseExpansion(gameState, queues);

	if (gameState.currentPhase() > 1)
	{
		if (!this.saveResources)
		{
			this.buildMarket(gameState, queues);
			this.buildBlacksmith(gameState, queues);
			this.buildTemple(gameState, queues);
		}

		if (this.Config.difficulty > 1)
			this.tradeManager.update(gameState, events, queues);
	}

	this.garrisonManager.update(gameState, events);
	this.defenseManager.update(gameState, events);

	if (!this.saveResources)
		this.constructTrainingBuildings(gameState, queues);

	if (this.Config.difficulty > 0)
		this.buildDefenses(gameState, queues);

	this.assignGatherers(gameState);
	for (let i = 0; i < this.baseManagers.length; ++i)
	{
		this.baseManagers[i].checkEvents(gameState, events, queues);
		if (((i + gameState.ai.playedTurn)%this.baseManagers.length) === 0)
			this.baseManagers[i].update(gameState, queues, events);
	}

	this.navalManager.update(gameState, queues, events);

	if (this.Config.difficulty > 0 && (this.numActiveBase() > 0 || !this.canBuildUnits))
		this.attackManager.update(gameState, queues, events);

	this.diplomacyManager.update(gameState, events);

	Engine.ProfileStop();
};

m.HQ.prototype.Serialize = function()
{
	let properties = {
		"econState": this.econState,
		"phaseStarted": this.phaseStarted,
		"wantedRates": this.wantedRates,
		"currentRates": this.currentRates,
		"lastFailedGather": this.lastFailedGather,
		"femaleRatio": this.femaleRatio,
		"lastTerritoryUpdate": this.lastTerritoryUpdate,
		"stopBuilding": this.stopBuilding,
		"towerStartTime": this.towerStartTime,
		"towerLapseTime": this.towerLapseTime,
		"fortressStartTime": this.fortressStartTime,
		"fortressLapseTime": this.fortressLapseTime,
		"targetNumWorkers": this.targetNumWorkers,
		"bBase": this.bBase,
		"bAdvanced": this.bAdvanced,
		"saveResources": this.saveResources,
		"saveSpace": this.saveSpace,
		"canBuildUnits": this.canBuildUnits,
		"navalMap": this.navalMap,
		"landRegions": this.landRegions,
		"navalRegions": this.navalRegions
	};

	let baseManagers = [];
	for (let base of this.baseManagers)
		baseManagers.push(base.Serialize());

	if (this.Config.debug == -100)
	{
		API3.warn(" HQ serialization ---------------------");
		API3.warn(" properties " + uneval(properties));
		API3.warn(" baseManagers " + uneval(baseManagers));
		API3.warn(" attackManager " + uneval(this.attackManager.Serialize()));
		API3.warn(" defenseManager " + uneval(this.defenseManager.Serialize()));
		API3.warn(" tradeManager " + uneval(this.tradeManager.Serialize()));
		API3.warn(" navalManager " + uneval(this.navalManager.Serialize()));
		API3.warn(" researchManager " + uneval(this.researchManager.Serialize()));
		API3.warn(" diplomacyManager " + uneval(this.diplomacyManager.Serialize()));
		API3.warn(" garrisonManager " + uneval(this.garrisonManager.Serialize()));
	}

	return {
		"properties": properties,

		"baseManagers": baseManagers,
		"attackManager": this.attackManager.Serialize(),
		"defenseManager": this.defenseManager.Serialize(),
		"tradeManager": this.tradeManager.Serialize(),
		"navalManager": this.navalManager.Serialize(),
		"researchManager": this.researchManager.Serialize(),
		"diplomacyManager": this.diplomacyManager.Serialize(),
		"garrisonManager": this.garrisonManager.Serialize(),
	};
};

m.HQ.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.properties)
		this[key] = data.properties[key];

	this.baseManagers = [];
	for (let base of data.baseManagers)
	{
		// the first call to deserialize set the ID base needed by entitycollections
		var newbase = new m.BaseManager(gameState, this.Config);
		newbase.Deserialize(gameState, base);
		newbase.init(gameState);
		newbase.Deserialize(gameState, base);
		this.baseManagers.push(newbase);
	}

	this.attackManager = new m.AttackManager(this.Config);
	this.attackManager.Deserialize(gameState, data.attackManager);
	this.attackManager.init(gameState);
	this.attackManager.Deserialize(gameState, data.attackManager);

	this.defenseManager = new m.DefenseManager(this.Config);
	this.defenseManager.Deserialize(gameState, data.defenseManager);

	this.tradeManager = new m.TradeManager(this.Config);
	this.tradeManager.init(gameState);
	this.tradeManager.Deserialize(gameState, data.tradeManager);

	this.navalManager = new m.NavalManager(this.Config);
	this.navalManager.init(gameState, true);
	this.navalManager.Deserialize(gameState, data.navalManager);

	this.researchManager = new m.ResearchManager(this.Config);
	this.researchManager.Deserialize(data.researchManager);

	this.diplomacyManager = new m.DiplomacyManager(this.Config);
	this.diplomacyManager.Deserialize(data.diplomacyManager);

	this.garrisonManager = new m.GarrisonManager();
	this.garrisonManager.Deserialize(data.garrisonManager);
};

return m;

}(PETRA);
