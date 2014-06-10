var AEGIS = function(m)
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

m.HQ = function(Config) {
	this.Config = Config;
	
	this.targetNumBuilders = this.Config.Economy.targetNumBuilders; // number of workers we want building stuff
	
	this.dockStartTime = this.Config.Economy.dockStartTime * 1000;
	this.techStartTime = this.Config.Economy.techStartTime * 1000;
	
	this.dockFailed = false;	// sanity check
	this.waterMap = false;	// set by the aegis.js file.
	
	this.econState = "growth";	// existing values: growth, townPhasing.

	// tell if we can't gather from a resource type for sanity checks.
	this.outOf = { "food" : false,  "wood" : false,  "stone" : false,  "metal" : false };
	
	this.baseManagers = {};
	
	// cache the rates.
	this.wantedRates = { "food": 0, "wood": 0, "stone":0, "metal": 0 };
	this.currentRates = { "food": 0, "wood": 0, "stone":0, "metal": 0 };
	this.currentRateLastUpdateTime = 0;

	// this means we'll have about a big third of women, and thus we can maximize resource gathering rates.
	this.femaleRatio = this.Config.Economy.femaleRatio;
	
	this.fortressStartTime = 0;
	this.fortressLapseTime = this.Config.Military.fortressLapseTime * 1000;
	this.defenceBuildingTime = this.Config.Military.defenceBuildingTime * 1000;
	this.attackPlansStartTime = this.Config.Military.attackPlansStartTime * 1000;

	this.defenceManager = new m.Defence(this.Config);
	this.navalManager = new m.NavalManager();
	
	this.TotalAttackNumber = 0;
	this.upcomingAttacks = { "CityAttack" : [], "Rush" : [] };
	this.startedAttacks = { "CityAttack" : [], "Rush" : [] };
};

// More initialisation for stuff that needs the gameState
m.HQ.prototype.init = function(gameState, queues){
	// initialize base map. Each pixel is a base ID, or 0 if none
	this.basesMap = new API3.Map(gameState.sharedScript, new Uint8Array(gameState.getMap().data.length));
	this.basesMap.setMaxVal(255);

	if (this.Config.Economy.targetNumWorkers)
		this.targetNumWorkers = this.Config.Economy.targetNumWorkers;
	else if (this.targetNumWorkers === undefined && this.Config.difficulty === 0)
		this.targetNumWorkers = Math.max(1, Math.min(40, Math.floor(gameState.getPopulationMax())));
	else if (this.targetNumWorkers === undefined && this.Config.difficulty === 1)
		this.targetNumWorkers = Math.max(1, Math.min(60, Math.floor(gameState.getPopulationMax())));
	else if (this.targetNumWorkers === undefined)
		this.targetNumWorkers = Math.max(1, Math.min(120,Math.floor(gameState.getPopulationMax()/3.0)));


	// Let's get our initial situation here.
	// TODO: improve on this.
	// TODO: aknowledge bases, assign workers already.
	var ents = gameState.getEntities().filter(API3.Filters.byOwner(PlayerID));
	
	var workersNB = 0;
	var hasScout = false;
	var treasureAmount = { 'food': 0, 'wood': 0, 'stone': 0, 'metal': 0 };
	var hasCC = false;
	
	if (ents.filter(API3.Filters.byClass("CivCentre")).length > 0)
		hasCC = true;
	workersNB = ents.filter(API3.Filters.byClass("Worker")).length;
	if (ents.filter(API3.Filters.byClass("Cavalry")).length > 0)
		hasScout = true;
	
	// TODO: take multiple CCs into account.
	if (hasCC)
	{
		var CC = ents.filter(API3.Filters.byClass("CivCentre")).toEntityArray()[0];
		for (var i in treasureAmount)
			gameState.getResourceSupplies(i).forEach( function (ent) {
				if (ent.resourceSupplyType().generic === "treasure" && API3.SquareVectorDistance(ent.position(), CC.position()) < 5000)
					treasureAmount[i] += ent.resourceSupplyMax();
			});
		this.baseManagers[1] = new m.BaseManager(this.Config);
		this.baseManagers[1].init(gameState);
		this.baseManagers[1].setAnchor(CC);
		this.baseManagers[1].initTerritory(this, gameState);
		this.baseManagers[1].initGatheringFunctions(this, gameState);
		
		if (m.DebugEnabled())
			this.basesMap.dumpIm("basesMap.png");
		var self = this;

		ents.forEach( function (ent) { //}){
			self.baseManagers[1].assignEntity(ent);
		});
	}
	// we now have enough data to decide on a few things.
	
	// TODO: here would be where we pick our initial strategy.
	
	// immediatly build a wood dropsite if possible.
	if (this.baseManagers[1])
	{
		if (gameState.ai.queueManager.getAvailableResources(gameState)["wood"] >= 250)
		{
			var pos = this.baseManagers[1].findBestDropsiteLocation(gameState, "wood");
			if (pos)
			{
				queues.dropsites.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse",{ "base" : 1 }, pos));
				queues.minorTech.addItem(new m.ResearchPlan(gameState, "gather_capacity_wheelbarrow"));
			}
		}
	}
	
	var map = new API3.Map(gameState.sharedScript, gameState.sharedScript.CCResourceMaps["wood"].map);
	if (m.DebugEnabled())
		map.dumpIm("map_CC_Wood.png");
	
	//this.reassignIdleWorkers(gameState);
	
	this.navalManager.init(gameState, queues);
	this.defenceManager.init(gameState);

	// TODO: change that to something dynamic.
	var civ = gameState.playerData.civ;
	
	// load units and buildings from the config files
	
	if (civ in this.Config.buildings.moderate){
		this.bModerate = this.Config.buildings.moderate[civ];
	}else{
		this.bModerate = this.Config.buildings.moderate['default'];
	}
	
	if (civ in this.Config.buildings.advanced){
		this.bAdvanced = this.Config.buildings.advanced[civ];
	}else{
		this.bAdvanced = this.Config.buildings.advanced['default'];
	}
	
	if (civ in this.Config.buildings.fort){
		this.bFort = this.Config.buildings.fort[civ];
	}else{
		this.bFort = this.Config.buildings.fort['default'];
	}
	
	for (var i in this.bAdvanced){
		this.bAdvanced[i] = gameState.applyCiv(this.bAdvanced[i]);
	}
	for (var i in this.bFort){
		this.bFort[i] = gameState.applyCiv(this.bFort[i]);
	}
};

m.HQ.prototype.checkEvents = function (gameState, events, queues) {
	// TODO: probably check stuffs like a base destruction.
	var CreateEvents = events["Create"];
	var ConstructionEvents = events["ConstructionFinished"];
	for (var i in CreateEvents)
	{
		var evt = CreateEvents[i];
		// Let's check if we have a building set to create a new base.
		if (evt && evt.entity)
		{
			var ent = gameState.getEntityById(evt.entity);
			
			if (ent === undefined)
				continue; // happens when this message is right before a "Destroy" one for the same entity.
			
			if (ent.isOwn(PlayerID) && ent.getMetadata(PlayerID, "base") === -1)
			{
				// Okay so let's try to create a new base around this.
				var bID = m.playerGlobals[PlayerID].uniqueIDBases;
				this.baseManagers[bID] = new m.BaseManager(this.Config);
				this.baseManagers[bID].init(gameState, events, true);
				this.baseManagers[bID].setAnchor(ent);
				this.baseManagers[bID].initTerritory(this, gameState);
				
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
	}
	for (var i in ConstructionEvents)
	{
		var evt = ConstructionEvents[i];
		// Let's check if we have a building set to create a new base.
		// TODO: move to the base manager.
		if (evt.newentity)
		{
			var ent = gameState.getEntityById(evt.newentity);

			if (ent === undefined)
				continue; // happens when this message is right before a "Destroy" one for the same entity.
			
			if (ent.isOwn(PlayerID) && ent.getMetadata(PlayerID, "baseAnchor") == true)
			{
				var base = ent.getMetadata(PlayerID, "base");
				if (this.baseManagers[base].constructing)
				{
					this.baseManagers[base].constructing = false;
					this.baseManagers[base].initGatheringFunctions(this, gameState);
				}
			}
		}
	}
};

// Called by the "town phase" research plan once it's started
m.HQ.prototype.OnTownPhase = function(gameState)
{
	if (this.Config.difficulty >= 2 && this.femaleRatio !== 0.4)
	{
		this.femaleRatio = 0.4;
		gameState.ai.queues["villager"].empty();
		gameState.ai.queues["citizenSoldier"].empty();
		for (var i in this.baseManagers)
		{
			if (this.baseManagers[i].willGather["wood"] === 2)
				this.baseManagers[i].willGather["wood"] = 1;	// retry.
			if (this.baseManagers[i].willGather["stone"] === 2)
				this.baseManagers[i].willGather["stone"] = 1;	// retry.
			if (this.baseManagers[i].willGather["metal"] === 2)
				this.baseManagers[i].willGather["metal"] = 1;	// retry.
		}
	}
}

// This code trains females and citizen workers, trying to keep close to a ratio of females/CS
// TODO: this should choose a base depending on which base need workers
// TODO: also there are several things that could be greatly improved here.
m.HQ.prototype.trainMoreWorkers = function(gameState, queues)
{
	// Get some data.
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
	var numQueuedF = queues.villager.countQueuedUnits();
	var numQueuedS = queues.citizenSoldier.countQueuedUnits();
	var numQueued = numQueuedS + numQueuedF;
	var numTotal = numWorkers + numQueued;

	// If we have too few, train more
	// should plan enough to always have females…
	// TODO: 15 here should be changed to something more sensible, such as nb of producing buildings.
	if (numTotal > this.targetNumWorkers || numQueued > 50 || (numQueuedF > 20 && numQueuedS > 20) || numInTraining > 15)
		return;

	if (numTotal >= this.Config.Economy.villagePopCap && gameState.currentPhase() === 1 && !gameState.isResearching(gameState.townPhase()))
		return;

	// default template and size
	var template = gameState.applyCiv("units/{civ}_support_female_citizen");
	var size = Math.min(5, Math.max(Math.ceil(numTotal / 10), 1));

	// Choose whether we want soldiers instead.
	// TODO: we might want to adjust our female ratio.
	if ((numFemales+numQueuedF)/numTotal > this.femaleRatio && numQueuedS < 20) {
		if (numTotal < 35)
			template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["cost",1], ["speed",0.5], ["costsResource", 0.5, "stone"], ["costsResource", 0.5, "metal"]]);
		else
			template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["strength",1] ]);

		if (!template)
			template = gameState.applyCiv("units/{civ}_support_female_citizen");
		else
			size = Math.min(5, Math.ceil(numTotal / 12));
	}

	// TODO: improve that logic.
	/*
	if (numFemales/numWorkers > this.femaleRatio && numQueuedS > 0 && numWorkers > 25)
		queues.villager.paused = true;
	else
		queues.villager.paused = false;
	*/

	// TODO: perhaps assign them a default resource and check the base according to that.
	
	// base "0" means "auto"
	if (template === gameState.applyCiv("units/{civ}_support_female_citizen"))
		queues.villager.addItem(new m.TrainingPlan(gameState, template, { "role" : "worker", "base" : 0 }, size, size ));
	else
		queues.citizenSoldier.addItem(new m.TrainingPlan(gameState, template, { "role" : "worker", "base" : 0 }, size, size));
};

// picks the best template based on parameters and classes
m.HQ.prototype.findBestTrainableUnit = function(gameState, classes, parameters) {
	var units = gameState.findTrainableUnits(classes);
	
	if (units.length === 0)
		return undefined;
	
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

// Tries to research any available tech
// Only one at once. Also does military tech (selection is completely random atm)
// TODO: Lots, lots, lots here.
m.HQ.prototype.tryResearchTechs = function(gameState, queues) {
	if (queues.minorTech.length() === 0)
	{
		var possibilities = gameState.findAvailableTech();
		if (possibilities.length === 0)
			return;
		// randomly pick one. No worries about pairs in that case.
		var p = Math.floor((Math.random()*possibilities.length));
		queues.minorTech.addItem(new m.ResearchPlan(gameState, possibilities[p][0]));
	}
}

// We're given a worker and a resource type
// We'll assign the worker for the best base for that resource type.
// TODO: improve choice alogrithm
m.HQ.prototype.switchWorkerBase = function(gameState, worker, type) {
	var bestBase = 0;
	var bestBaseState = -1;

	for (var i in this.baseManagers)
	{
		if (this.baseManagers[i].willGather[type] === 1 || (this.baseManagers[i].willGather[type] === 2 && bestBaseState !== 1))
		{
			if (this.baseManagers[i].accessIndex === this.baseManagers[worker.getMetadata(PlayerID,"base")].accessIndex
				|| this.navalManager.canReach(gameState, this.baseManagers[i].accessIndex, this.baseManagers[worker.getMetadata(PlayerID,"base")].accessIndex))
			{
				bestBaseState = this.baseManagers[i].willGather[type]
				bestBase = i;
				break;
			}
		}
	}
	if (bestBase && bestBase !== worker.getMetadata(PlayerID,"base"))
	{
		worker.setMetadata(PlayerID,"base",bestBase);
		return true;
	} else {
		return false;
	}
};

// returns an entity collection of workers through BaseManager.pickBuilders
// TODO: better the choice algo.
m.HQ.prototype.bulkPickWorkers = function(gameState, newBaseID, number) {
	var accessIndex = this.baseManagers[newBaseID].accessIndex;
	if (!accessIndex)
		return false;
	// sorting bases by whether they are on the same accessindex or not.
	var baseBest = m.AssocArraytoArray(this.baseManagers).sort(function (a,b) {
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

// returns the current gather rate
// This is not per-se exact, it performs a few adjustments ad-hoc to account for travel distance, stuffs like that.
m.HQ.prototype.GetCurrentGatherRates = function(gameState) {
	var self = this;

//	if (gameState.getTimeElapsed() - this.currentRateLastUpdateTime < 10000 && this.currentRateLastUpdateTime !== 0 && gameState.ai.playedTurn > 3)
//		return this.currentRates;
	
	this.currentRateLastUpdateTime = gameState.getTimeElapsed();

	for (var type in this.wantedRates)
		this.currentRates[type] = 0;
	
	for (var i in this.baseManagers)
		this.baseManagers[i].getGatherRates(gameState, this.currentRates);

	return this.currentRates;
};


/* Pick the resource which most needs another worker
 * How this works:
 * We get the rates we would want to have to be able to deal with our plans
 * We get our current rates
 * We compare; we pick the one where the discrepancy is highest.
 * Need to balance long-term needs and possible short-term needs.
 */
m.HQ.prototype.pickMostNeededResources = function(gameState) {
	var self = this;
	
	this.wantedRates = gameState.ai.queueManager.wantedGatherRates(gameState);

	var currentRates = {};
	for (var type in this.wantedRates)
		currentRates[type] = 0;
	currentRates = this.GetCurrentGatherRates(gameState);

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

// If all the CC's are destroyed then build a new one
// TODO: rehabilitate.
m.HQ.prototype.buildNewCC= function(gameState, queues) {
	var numCCs = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_civil_centre"), true);
	numCCs += queues.civilCentre.length();

	// no use trying to lay foundations that will be destroyed
	if (gameState.defcon() > 2)
		for (var i = numCCs; i < 1; i++) {
			gameState.ai.queueManager.clear();
			this.baseNeed["food"] = 0;
			this.baseNeed["wood"] = 50;
			this.baseNeed["stone"] = 50;
			this.baseNeed["metal"] = 50;
			queues.civilCentre.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_civil_centre"));
		}
	return (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_civil_centre"), true) == 0 && gameState.currentPhase() > 1);
};

// Returns the best position to build a new Civil Centre
// Whose primary function would be to reach new resources of type "resource".
m.HQ.prototype.findBestEcoCCLocation = function(gameState, resource){
	
	var CCPlate = gameState.getTemplate("structures/{civ}_civil_centre");

	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.
	
	var obstructions = m.createObstructionMap(gameState, 0);
	obstructions.expandInfluences();

	// copy the resource map as initialization.
	var friendlyTiles = new API3.Map(gameState.sharedScript, gameState.sharedScript.CCResourceMaps[resource].map, true);
	friendlyTiles.setMaxVal(255);
	var ents = gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	var eEnts = gameState.getEnemyStructures().filter(API3.Filters.byClass("CivCentre")).toEntityArray();

	var dps = gameState.getOwnDropsites().toEntityArray();

	for (var j = 0; j < friendlyTiles.length; ++j)
	{
		// We check for our other CCs: the distance must not be too big. Anything bigger will result in scrapping.
		// This ensures territorial continuity.
		// TODO: maybe whenever I get around to implement multi-base support (details below, requires being part of the team. If you're not, ask wraitii directly by PM).
		// (see www.wildfiregames.com/forum/index.php?showtopic=16702&#entry255631 )
		// TODO: figure out what I was trying to say above.
		
		var canBuild = true;
		var canBuild2 = false;

		var pos = [j%friendlyTiles.width+0.5, Math.floor(j/friendlyTiles.width)+0.5];

		for (var i in ents)
		{
			var entPos = ents[i].position();
			entPos = [entPos[0]/4.0,entPos[1]/4.0];
			
			var dist = API3.SquareVectorDistance(entPos, pos);
			if (dist < 3500 || dist > 7900)
				friendlyTiles.map[j] /= 2.0;
			if (dist < 2120)
			{
				canBuild = false;
				continue;
			} else if (dist < 9200 || this.waterMap)
				canBuild2 = true;
		}
		// checking for bases.
		if (this.basesMap.map[j] !== 0)
			canBuild = false;
		
		if (!canBuild2)
			canBuild = false;
		if (canBuild)
		{
			// Checking for enemy CCs
			for (var i in eEnts)
			{
				var entPos = eEnts[i].position();
				entPos = [entPos[0]/4.0,entPos[1]/4.0];
				// 7100 works well as a limit.
				if (API3.SquareVectorDistance(entPos, pos) < 2500)
				{
					canBuild = false;
					continue;
				}
			}
		}
		if (!canBuild)
		{
			friendlyTiles.map[j] = 0;
			continue;
		}

		for (var i in dps)
		{
			var dpPos = dps[i].position();
			if (dpPos === undefined)
			{
				// Probably a mauryan elephant, skip
				continue;
			}
			dpPos = [dpPos[0]/4.0,dpPos[1]/4.0];
			var dist = API3.SquareVectorDistance(dpPos, pos);
			if (dist < 600)
			{
				friendlyTiles.map[j] = 0;
				continue;
			} else if (dist < 1500)
				friendlyTiles.map[j] /= 2.0;
		}

		friendlyTiles.map[j] *= 1.5;
		
		for (var i in gameState.sharedScript.CCResourceMaps)
			if (friendlyTiles.map[j] !== 0 && i !== "food")
			{
				var val = friendlyTiles.map[j] + gameState.sharedScript.CCResourceMaps[i].map[j];
				if (val < 255)
					friendlyTiles.map[j] = val;
				else
					friendlyTiles.map[j] = 255;
			}
	}
	
	
	var best = friendlyTiles.findBestTile(6, obstructions);
	var bestIdx = best[0];

	if (m.DebugEnabled())
	{
		friendlyTiles.map[bestIdx] = 270;
		friendlyTiles.dumpIm("cc_placement_base_" + gameState.getTimeElapsed() + "_" + resource + "_" + best[1] + ".png",301);
		//obstructions.dumpIm("cc_placement_base_" + gameState.getTimeElapsed() + "_" + resource + "_" + best[1] + "_obs.png", 20);
	}
	
	// not good enough.
	if (best[1] < 60)
		return false;
	
	var x = ((bestIdx % friendlyTiles.width) + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * gameState.cellSize;

	m.debug ("Best for value " + best[1] + " at " + uneval([x,z]));

	return [x,z];
};

m.HQ.prototype.buildTemple = function(gameState, queues){
	if (gameState.currentPhase() >= 2 ) {
		if (queues.economicBuilding.countQueuedUnits() === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_temple"), true) === 0){
			queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_temple", { "base" : 1 }));
		}
	}
};

m.HQ.prototype.buildMarket = function(gameState, queues){
	if (gameState.getPopulation() > this.Config.Economy.popForMarket && gameState.currentPhase() >= 2 ) {
		if (queues.economicBuilding.countQueuedUnitsWithClass("BarterMarket") === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) === 0){
			//only ever build one storehouse/CC/market at a time
			queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_market", { "base" : 1 }));
		}
	}
};

// Build a farmstead to go to town phase faster and prepare for research. Only really active on higher diff mode.
m.HQ.prototype.buildFarmstead = function(gameState, queues){
	if (gameState.getPopulation() > this.Config.Economy.popForFarmstead
		&& (gameState.currentPhase() > 1 || gameState.isResearching(gameState.townPhase()))) {
		// achtung: "DropsiteFood" does not refer to CCs.
		if (queues.economicBuilding.countQueuedUnitsWithClass("DropsiteFood") === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_farmstead"), true) === 0){
			//only ever build one storehouse/CC/market at a time
			queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_farmstead", { "base" : 1 }));
			// add the farming plough to the research we want.
			queues.minorTech.addItem(new m.ResearchPlan(gameState, "gather_farming_plows"));
		}
	}
};

// TODO: generic this, probably per-base
m.HQ.prototype.buildDock = function(gameState, queues){
	if (!this.waterMap || this.dockFailed)
		return;
	if (gameState.getTimeElapsed() > this.dockStartTime) {
		if (queues.economicBuilding.countQueuedUnitsWithClass("NavalMarket") === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_dock"), true) === 0) {
			var tp = ""
			if (gameState.civ() == "cart" && gameState.currentPhase() > 1)
				tp = "structures/{civ}_super_dock";
			else if (gameState.civ() !== "cart")
				tp = "structures/{civ}_dock";
			if (tp !== "")
			{
				var remaining = this.navalManager.getUnconnectedSeas(gameState, this.baseManagers[1].accessIndex);
				queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, tp, { "base" : 1, "sea" : remaining[0] }));
			}
		}
	}
};

// Try to barter unneeded resources for needed resources.
// once per turn because the info doesn't update between a turn and fixing isn't worth it.
m.HQ.prototype.tryBartering = function(gameState) {
	var markets = gameState.getOwnEntitiesByType(gameState.applyCiv("structures/{civ}_market"), true).toEntityArray();

	if (markets.length === 0)
		return false;

	// Available resources after account substraction
	var available = gameState.ai.queueManager.getAvailableResources(gameState);

	var rates = this.GetCurrentGatherRates(gameState)

	var prices = gameState.getBarterPrices();
	// calculates conversion rates
	var getBarterRate = function (prices,buy,sell) { return Math.round(100 * prices["sell"][sell] / prices["buy"][buy]); };

	// loop through each queues checking if we could barter and finish a queue quickly.
	for (var j in gameState.ai.queues)
	{
		var queue = gameState.ai.queues[j];
		if (queue.paused || queue.length() === 0)
			continue;

		var account = gameState.ai.queueManager.accounts[j];
		var elem = queue.queue[0];
		var elemCost = elem.getCost();
		for each (var ress in elemCost.types)
		{
			if (available[ress] >= 0)
				continue;	// don't care if we still have available resources or our rate is good enough
			var need = elemCost[ress] - account[ress];
			if (need <= 0 || rates[ress] >= need/50)	// don't care if we don't need resources for our first item
				continue;
			
			if (ress == "food" && need < 400)
				continue;

			// pick the best resource to barter.
			var bestToBarter = "";
			var bestRate = 0;
			for each (var otherRess in elemCost.types)
			{
				if (ress === otherRess)
					continue;
				// I wanna keep some
				if (available[otherRess] < 130 + need)
					return false;
				var barterRate = getBarterRate(prices, ress, otherRess);
				if (barterRate > bestRate)
				{
					bestRate = barterRate;
					bestToBarter = otherRess;
				}
			}
			if (bestToBarter !== "")
			{
				markets[0].barter(buy,sell,100);
				m.debug ("Snipe bartered " + sell +" for " + buy + ", value 100");
				return true;
			}
		}
	}
	// now barter for big needs.
	var needs = gameState.ai.queueManager.currentNeeds(gameState);
	for each (var sell in needs.types) {
		for each (var buy in needs.types) {
			if (buy != sell && needs[sell] <= 0 && available[sell] > 500) {	// if we don't need it and have a buffer
				if (needs[buy] > rates[buy]*80) {	// if we need that other resource terribly.
					markets[0].barter(buy,sell,100);
					m.debug ("Gross bartered " +sell +" for " + buy + ", value 100");
					return true;
				}
			}
		}
	}
	return false;
};

// build more houses if needed.
// kinda ugly, lots of special cases to both build enough houses but not tooo many…
m.HQ.prototype.buildMoreHouses = function(gameState,queues) {

	if (gameState.getPopulationLimit() < gameState.getPopulationMax()) {
		var numPlanned = queues.house.length();
		if (numPlanned < 3 || (numPlanned < 5 && gameState.getPopulation() > 80))
		{
			var plan = new m.ConstructionPlan(gameState, "structures/{civ}_house", { "base" : 1 });
			// make the difficulty available to the isGo function without having to pass it as argument
			var difficulty = this.Config.difficulty;
			// change the starting condition to "less than 15 slots left".
			plan.isGo = function (gameState) {
				var HouseNb = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_house"), true);

				var freeSlots = 0;
				if (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber" ||
					gameState.civ() == "maur" || gameState.civ() == "ptol")
					freeSlots = gameState.getPopulationLimit() + HouseNb*5 - gameState.getPopulation();
				else
					freeSlots = gameState.getPopulationLimit() + HouseNb*10 - gameState.getPopulation();
				if (gameState.getPopulation() > 55 && difficulty > 1)
					return (freeSlots <= 21);
				else if (gameState.getPopulation() >= 30 && difficulty !== 0)
					return (freeSlots <= 15);
				else
					return (freeSlots <= 10);
			}
			queues.house.addItem(plan);
		}
		if (numPlanned > 0 && this.econState == "townPhasing")
		{
			var houseQueue = queues.house.queue;
			var count = gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length;
			count += queues.militaryBuilding.length();	// barracks
			for (var i = 0; i < numPlanned; ++i)
			{
				if (houseQueue[i].isGo(gameState))
					++count;
				else if (count < 5)
				{
					houseQueue[i].isGo = function () { return true; }
					++count;
				}
			}
		}
	}
};

// checks if we have bases for all resource types (bar food for now) or if we need to expand.
m.HQ.prototype.checkBasesRessLevel = function(gameState,queues) {
	if (gameState.currentPhase() === 1 && !gameState.isResearching(gameState.townPhase()))
		return;
	var count = { "food" : 0, "wood" : 0, "stone" : 0, "metal" : 0 }
	var capacity = { "wood" : 0, "stone" : 0, "metal" : 0 }
	var need = { "food": false, "wood" : true, "stone" : true, "metal" : true };
	var posss = [];

	for (var i in this.baseManagers)
	{
		var base = this.baseManagers[i];
		for (var type in count)
		{
			if (type == "food")
			{
				count[type] = 1;
				capacity[type] = 20000;
				need[type] = (base.willGather[type] !== 1);
				continue;
			}
			if (base.getResourceLevel(gameState, type, "dropsites") > 4000*Math.max(this.Config.difficulty,2))
				count[type]++;
			capacity[type] += base.getWorkerCapacity(gameState, type, true);
			if (base.willGather[type] === 1)
				need[type] = false;
		}
	}
	for (var type in count)
	{
		if (count[type] === 0 || need[type]
			|| capacity[type] < gameState.getOwnUnits().filter(API3.Filters.and(API3.Filters.byMetadata(PlayerID, "subrole", "gatherer"), API3.Filters.byMetadata(PlayerID, "gather-type", type))).length * 1.05)
		{
			// plan a new base.
			if (gameState.countFoundationsByType(gameState.applyCiv("structures/{civ}_civil_centre"), true) === 0 && queues.civilCentre.length() === 0) {	
				// In endgame when the whole map is claimed by players, we won't find a spot for a new CC.
				// findBestEcoCCLocation needs to search the whole map for a good spot and is currently way too slow, so we wait quite long
				// until we check again. The "PlayerID * 10" part is to distribute the load across multiple turns. 
				// TODO: this is a workaround. the current solution is bad for various reasons:
				// 1. findBestEcoCCLocation could be much more efficient for the case when nearly all territory is occupied.
				// 2. It doesn't make sense to check the whole map for a good spot for all resource types if we could see in the beginning
				// that no free territory is available to build a CC.
				// 3. Trying to build a new CC should not only be triggered by the need for more resources.
				// Opportunity (having destroyed an enemy CC and some soldiers standing around) is also a good reason for a new CC.
				// 4. Last but not least it causes the AI to react slowly when new territory becomes available.
				if (this.outOf[type] && gameState.ai.playedTurn % 100 !== PlayerID * 10)
					continue;

				var pos = this.findBestEcoCCLocation(gameState, type);
				if (!pos)
				{
					// Okay so we'll set us as out of this.
					this.outOf[type] = true;
				} else {
					//warn ("planning new base");
					// base "-1" means new base.
					queues.civilCentre.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_civil_centre",{ "base" : -1 }, pos));
				}
			}
		}
	}
};

// Deals with building fortresses and towers.
// Currently build towers next to every useful dropsites.
// TODO: Fortresses are placed randomly atm.
m.HQ.prototype.buildDefences = function(gameState, queues){
	
	var workersNumber = gameState.getOwnEntitiesByRole("worker", true).filter(API3.Filters.not(API3.Filters.byHasMetadata(PlayerID,"plan"))).length;
	
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv('structures/{civ}_defense_tower'), true)
		+ queues.defenceBuilding.length() < gameState.getEntityLimits()["DefenseTower"] && queues.defenceBuilding.length() === 0 && gameState.currentPhase() > 1) {
		for (var i in this.baseManagers)
		{
			for (var j in this.baseManagers[i].dropsites)
			{
				var amnts = this.baseManagers[i].dropsites[j];
				var dpEnt = gameState.getEntityById(j);
				if (dpEnt !== undefined && dpEnt.getMetadata(PlayerID, "defenseTower") !== true)
					if (amnts["wood"] || amnts["metal"] || amnts["stone"])
					{
						var position = dpEnt.position();
						if (position) {
							queues.defenceBuilding.addItem(new m.ConstructionPlan(gameState, 'structures/{civ}_defense_tower', { "base" : i }, position));
						}
						dpEnt.setMetadata(PlayerID, "defenseTower", true);
					}
			}
		}
	}
	
	var numFortresses = 0;
	for (var i in this.bFort){
		numFortresses += gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bFort[i]), true);
	}
	
	if (queues.defenceBuilding.length() < 1 && (gameState.currentPhase() > 2 || gameState.isResearching("phase_city_generic")))
	{
		if (workersNumber >= 80 && gameState.getTimeElapsed() > numFortresses * this.fortressLapseTime + this.fortressStartTime)
		{
			if (!this.fortressStartTime)
				this.fortressStartTime = gameState.getTimeElapsed();
			queues.defenceBuilding.addItem(new m.ConstructionPlan(gameState, this.bFort[0], { "base" : 1 }));
			m.debug ("Building a fortress");
		}
	}
	if (gameState.countEntitiesByType(gameState.applyCiv(this.bFort[i]), true) >= 1) {
		// let's add a siege building plan to the current attack plan if there is none currently.
		if (this.upcomingAttacks["CityAttack"].length !== 0)
		{
			var attack = this.upcomingAttacks["CityAttack"][0];
			if (!attack.unitStat["Siege"])
			{
				// no minsize as we don't want the plan to fail at the last minute though.
				var stat = { "priority" : 1.1, "minSize" : 0, "targetSize" : 4, "batchSize" : 2, "classes" : ["Siege"],
					"interests" : [ ["siegeStrength", 3], ["cost",1] ]  ,"templates" : [] };
				if (gameState.civ() == "cart" || gameState.civ() == "maur")
					stat["classes"] = ["Elephant"];
				attack.addBuildOrder(gameState, "Siege", stat, true);
			}
		}
	}
};

m.HQ.prototype.buildBlacksmith = function(gameState, queues){
	if (gameState.getTimeElapsed() > this.Config.Military.timeForBlacksmith*1000) {
		if (queues.militaryBuilding.length() === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_blacksmith"), true) === 0) {
			var tp = gameState.getTemplate(gameState.applyCiv("structures/{civ}_blacksmith"));
			if (tp.available(gameState))
				queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_blacksmith", { "base" : 1 }));
		}
	}
};

// Deals with constructing military buildings (barracks, stables…)
// They are mostly defined by Config.js. This is unreliable since changes could be done easily.
// TODO: We need to determine these dynamically. Also doesn't build fortresses since the above function does that.
// TODO: building placement is bad. Choice of buildings is also fairly dumb.
m.HQ.prototype.constructTrainingBuildings = function(gameState, queues) {
	Engine.ProfileStart("Build buildings");
	var workersNumber = gameState.getOwnEntitiesByRole("worker", true).filter(API3.Filters.not(API3.Filters.byHasMetadata(PlayerID, "plan"))).length;

	var barrackNb = gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0]), true);

	// first barracks.
	if (workersNumber > this.Config.Military.popForBarracks1 || (this.econState == "townPhasing" && gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length < 5)) {
		if (barrackNb + queues.militaryBuilding.length() < 1) {
			m.debug ("Trying to build barracks");
			var plan = new m.ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 });
			plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("militaryBuilding", 130); };
			queues.militaryBuilding.addItem(plan);
		}
	}

	// second barracks.
	if (barrackNb < 2 && workersNumber > this.Config.Military.popForBarracks2)
		if (queues.militaryBuilding.length() < 1)
			queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
	
	// third barracks (optional 4th/5th for some civs as they rely on barracks more.)
	if (barrackNb === 2 && barrackNb + queues.militaryBuilding.length() < 3 && workersNumber > 125)
		if (queues.militaryBuilding.length() === 0)
		{
			queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
			if (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber") {
				queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
				queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
			}
		}

	//build advanced military buildings
	if (workersNumber >= this.Config.Military.popForBarracks2 - 15 && gameState.currentPhase() > 2){
		if (queues.militaryBuilding.length() === 0){
			var inConst = 0;
			for (var i in this.bAdvanced)
				inConst += gameState.countFoundationsByType(gameState.applyCiv(this.bAdvanced[i]));
			if (inConst == 0 && this.bAdvanced && this.bAdvanced.length !== 0) {
				var i = Math.floor(Math.random() * this.bAdvanced.length);
				if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i]), true) < 1){
					queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bAdvanced[i], { "base" : 1 }));
				}
			}
		}
	}
	// build second advanced building except for some civs.
	if (gameState.civ() !== "gaul" && gameState.civ() !== "brit" && gameState.civ() !== "iber" &&
		workersNumber > 130 && gameState.currentPhase() > 2)
	{
		var inConst = 0;
		for (var i in this.bAdvanced)
			inConst += gameState.countEntitiesByType(gameState.applyCiv(this.bAdvanced[i]), true);
		if (inConst == 1) {
			var i = Math.floor(Math.random() * this.bAdvanced.length);
			if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i]), true) < 1){
				queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bAdvanced[i], { "base" : 1 }));
				queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bAdvanced[i], { "base" : 1 }));
			}
		}
	}
	
	Engine.ProfileStop();
};

// TODO: use pop(). Currently unused as this is too gameable.
m.HQ.prototype.garrisonAllFemales = function(gameState) {
	var buildings = gameState.getOwnStructures().filter(API3.Filters.byCanGarrison()).toEntityArray();
	var females = gameState.getOwnUnits().filter(API3.Filters.byClass("Support"));
	
	var cache = {};
	
	females.forEach( function (ent) {
		for (var i in buildings)
		{
			if (ent.position())
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

m.HQ.prototype.pausePlan = function(gameState, planName) {
	for (var attackType in this.upcomingAttacks) {
		for (var i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, true);
		}
	}
	for (var attackType in this.startedAttacks) {
		for (var i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, true);
		}
	}
}
m.HQ.prototype.unpausePlan = function(gameState, planName) {
	for (var attackType in this.upcomingAttacks) {
		for (var i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, false);
		}
	}
	for (var attackType in this.startedAttacks) {
		for (var i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, false);
		}
	}
}
m.HQ.prototype.pauseAllPlans = function(gameState) {
	for (var attackType in this.upcomingAttacks) {
		for (var i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			attack.setPaused(gameState, true);
		}
	}
	for (var attackType in this.startedAttacks) {
		for (var i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			attack.setPaused(gameState, true);
		}
	}
}
m.HQ.prototype.unpauseAllPlans = function(gameState) {
	for (var attackType in this.upcomingAttacks) {
		for (var i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			attack.setPaused(gameState, false);
		}
	}
	for (var attackType in this.startedAttacks) {
		for (var i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			attack.setPaused(gameState, false);
		}
	}
}


// Some functions are run every turn
// Others once in a while
m.HQ.prototype.update = function(gameState, queues, events) {	
	Engine.ProfileStart("Headquarters update");
	
	this.checkEvents(gameState,events,queues);
	//this.buildMoreHouses(gameState);

	this.trainMoreWorkers(gameState, queues);
	
	// sandbox doesn't expand.
	if (this.Config.difficulty !== 0)
		this.checkBasesRessLevel(gameState, queues);

	this.buildMoreHouses(gameState,queues);

	if (gameState.getTimeElapsed() > this.techStartTime && gameState.currentPhase() > 2 )
		this.tryResearchTechs(gameState,queues);
	
	if (this.Config.difficulty > 1)
		this.tryBartering(gameState);
	
	this.buildFarmstead(gameState, queues);
	this.buildMarket(gameState, queues);
	// Deactivated: the temple had no useful purpose for the AI now.
	//if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) === 1)
	//	this.buildTemple(gameState, queues);
	this.buildDock(gameState, queues);	// not if not a water map.
	
	Engine.ProfileStart("Constructing military buildings and building defences");
	this.constructTrainingBuildings(gameState, queues);
	
	this.buildBlacksmith(gameState, queues);

	if(gameState.getTimeElapsed() > this.defenceBuildingTime)
		this.buildDefences(gameState, queues);
	Engine.ProfileStop();

	for (var i in this.baseManagers)
	{
		this.baseManagers[i].checkEvents(gameState, events, queues)
		if ( ( (+i + gameState.ai.playedTurn) % (m.playerGlobals[PlayerID].uniqueIDBases - 1)) === 0)
			this.baseManagers[i].update(gameState, queues, events);
	}

	this.navalManager.update(gameState, queues, events);
	
	this.defenceManager.update(gameState, events, this);

	Engine.ProfileStart("Looping through attack plans");
	
	// TODO: bump this into a function.
	// TODO: implement some form of check before starting a new attack plans. Sometimes it is not the priority.
	if (1) {
		for (var attackType in this.upcomingAttacks) {
			for (var i = 0;i < this.upcomingAttacks[attackType].length; ++i) {
				
				var attack = this.upcomingAttacks[attackType][i];
				
				// okay so we'll get the support plan
				if (!attack.isStarted()) {
					var updateStep = attack.updatePreparation(gameState, this,events);
					
					// now we're gonna check if the preparation time is over
					if (updateStep === 1 || attack.isPaused() ) {
						// just chillin'
					} else if (updateStep === 0 || updateStep === 3) {
						m.debug ("Military Manager: " +attack.getType() +" plan " +attack.getName() +" aborted.");
						if (updateStep === 3) {
							this.attackPlansEncounteredWater = true;
							m.debug("No attack path found. Aborting.");
						}
						attack.Abort(gameState, this);
						this.upcomingAttacks[attackType].splice(i--,1);
					} else if (updateStep === 2) {
						var chatText = "I am launching an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
						if (Math.random() < 0.2)
							chatText = "Attacking " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
						else if (Math.random() < 0.3)
							chatText = "I have sent an army against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
						else if (Math.random() < 0.3)
							chatText = "I'm starting an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
						gameState.ai.chatTeam(chatText);
						
						m.debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
						attack.StartAttack(gameState,this);
						this.startedAttacks[attackType].push(attack);
						this.upcomingAttacks[attackType].splice(i--,1);
					}
				} else {
					var chatText = "I am launching an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
					if (Math.random() < 0.2)
						chatText = "Attacking " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
					else if (Math.random() < 0.3)
						chatText = "I have sent an army against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
					else if (Math.random() < 0.3)
						chatText = "I'm starting an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
					gameState.ai.chatTeam(chatText);
					
					m.debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
					this.startedAttacks[attackType].push(attack);
					this.upcomingAttacks[attackType].splice(i--,1);
				}
			}
		}
	}
	for (var attackType in this.startedAttacks) {
		for (var i = 0; i < this.startedAttacks[attackType].length; ++i) {
			var attack = this.startedAttacks[attackType][i];
			// okay so then we'll update the attack.
			if (!attack.isPaused())
			{
				var remaining = attack.update(gameState,this,events);
				if (remaining == 0 || remaining == undefined) {
					m.debug ("Military Manager: " +attack.getType() +" plan " +attack.getName() +" is now finished.");
					attack.Abort(gameState);
					this.startedAttacks[attackType].splice(i--,1);
				}
			}
		}
	}
	
	// creating plans after updating because an aborted plan might be reused in that case.

	// TODO: remove the limitation to attacks when on water maps.
	if (!this.waterMap && !this.attackPlansEncounteredWater)
	{
		if (gameState.ai.aggressiveness > 0.75 && gameState.countEntitiesByType(gameState.applyCiv(this.bModerate[0]), true) >= 1
			&& gameState.getTimeElapsed() > this.attackPlansStartTime && gameState.getTimeElapsed() < 360000)
		{
			if (this.upcomingAttacks["Rush"].length === 0)
			{ 
				// we have a barracks and we want to rush, rush.
				var AttackPlan = new m.CityAttack(gameState, this, this.Config, this.TotalAttackNumber, -1, "Rush");
				if (!AttackPlan.failed)
				{
					m.debug ("Headquarters: Rushing plan " +this.TotalAttackNumber);
					this.TotalAttackNumber++;
					this.upcomingAttacks["Rush"].push(AttackPlan);
				}
			}
		}
		// if we have a barracks, there's no water, we're at age >= 1 and we've decided to attack.
		else if (gameState.countEntitiesByType(gameState.applyCiv(this.bModerate[0]), true) >= 1 && !this.attackPlansEncounteredWater
			&& gameState.getTimeElapsed() > this.attackPlansStartTime && (gameState.currentPhase() > 1 || gameState.isResearching(gameState.townPhase()))) {
			if (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_dock"), true) === 0 && this.waterMap)
			{
				// wait till we get a dock.
			} else if (this.upcomingAttacks["CityAttack"].length === 0) {
				// basically only the first plan, really.
				var Lalala = undefined;
				if (gameState.getTimeElapsed() < 12*60000)
					Lalala = new m.CityAttack(gameState, this, this.Config, this.TotalAttackNumber, -1);
				else if (this.Config.difficulty !== 0)
					Lalala = new m.CityAttack(gameState, this, this.Config, this.TotalAttackNumber, -1, "superSized");

				if (Lalala.failed)
					this.attackPlansEncounteredWater = true; // hack
				else {
					m.debug ("Military Manager: Creating the plan " +this.TotalAttackNumber);
					this.TotalAttackNumber++;
					this.upcomingAttacks["CityAttack"].push(Lalala);
				}
			}
		}
	}
	
	/*
	 // very old relic. This should be reimplemented someday so the code stays here.
	 
	 if (this.HarassRaiding && this.preparingRaidNumber + this.startedRaidNumber < 1 && gameState.getTimeElapsed() < 780000) {
	 var Lalala = new m.CityAttack(gameState, this,this.totalStartedAttackNumber, -1, "harass_raid");
	 if (!Lalala.createSupportPlans(gameState, this, )) {
	 m.debug ("Military Manager: harrassing plan not a valid option");
	 this.HarassRaiding = false;
	 } else {
	 m.debug ("Military Manager: Creating the harass raid plan " +this.totalStartedAttackNumber);
	 
	 this.totalStartedAttackNumber++;
	 this.preparingRaidNumber++;
	 this.currentAttacks.push(Lalala);
	 }
	 }
	 */
	Engine.ProfileStop();

	/*
	Engine.ProfileStop();
			
	Engine.ProfileStart("Build new Dropsites");
	this.buildDropsites(gameState, queues);
	Engine.ProfileStop();

	if (this.Config.difficulty !== 0)
		this.tryBartering(gameState);
		
	this.buildFarmstead(gameState, queues);
	this.buildMarket(gameState, queues);
	// Deactivated: the temple had no useful purpose for the AI now.
	//if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true === 1)
	//	this.buildTemple(gameState, queues);
	this.buildDock(gameState, queues);	// not if not a water map.
*/
	Engine.ProfileStop();	// Heaquarters update
};

return m;

}(AEGIS);
