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

var HQ = function() {
	this.targetNumBuilders = Config.Economy.targetNumBuilders; // number of workers we want building stuff
	
	this.dockStartTime =  Config.Economy.dockStartTime * 1000;
	this.techStartTime = Config.Economy.techStartTime * 1000;
	
	this.dockFailed = false;	// sanity check
	this.waterMap = false;	// set by the aegis.js file.
	
	// tell if we can't gather from a resource type for sanity checks.
	this.outOf = { "food" : false,  "wood" : false,  "stone" : false,  "metal" : false };
	
	this.baseManagers = {};
	
	// this means we'll have about a big third of women, and thus we can maximize resource gathering rates.
	this.femaleRatio = Config.Economy.femaleRatio;
	
	this.fortressStartTime = 0;
	this.fortressLapseTime = Config.Military.fortressLapseTime * 1000;
	this.defenceBuildingTime = Config.Military.defenceBuildingTime * 1000;
	this.attackPlansStartTime = Config.Military.attackPlansStartTime * 1000;
	this.defenceManager = new Defence();
	
	this.navalManager = new NavalManager();
	
	this.TotalAttackNumber = 0;
	this.upcomingAttacks = { "CityAttack" : [] };
	this.startedAttacks = { "CityAttack" : [] };
};

// More initialisation for stuff that needs the gameState
HQ.prototype.init = function(gameState, events, queues){
	// initialize base map. Each pixel is a base ID, or 0 if none
	this.basesMap = new Map(gameState.sharedScript, new Uint8Array(gameState.getMap().data.length));
	this.basesMap.setMaxVal(255);

	if (Config.Economy.targetNumWorkers)
		this.targetNumWorkers = Config.Economy.targetNumWorkers;
	else if (this.targetNumWorkers === undefined)
		this.targetNumWorkers = Math.max(Math.floor(gameState.getPopulationMax()*(0.2 + Math.min(+(Config.difficulty)*0.125,0.3))), 1);


	// Let's get our initial situation here.
	// TODO: improve on this.
	// TODO: aknowledge bases, assign workers already.
	var ents = gameState.getEntities().filter(Filters.byOwner(PlayerID));
	
	var workersNB = 0;
	var hasScout = false;
	var treasureAmount = { 'food': 0, 'wood': 0, 'stone': 0, 'metal': 0 };
	var hasCC = false;
	
	if (ents.filter(Filters.byClass("CivCentre")).length > 0)
		hasCC = true;
	workersNB = ents.filter(Filters.byClass("Worker")).length;
	if (ents.filter(Filters.byClass("Cavalry")).length > 0)
		hasScout = true;
	
	// tODO: take multiple CCs into account.
	if (hasCC)
	{
		var CC = ents.filter(Filters.byClass("CivCentre")).toEntityArray()[0];
		for (i in treasureAmount)
			gameState.getResourceSupplies(i).forEach( function (ent) {
				if (ent.resourceSupplyType().generic === "treasure" && SquareVectorDistance(ent.position(), CC.position()) < 5000)
					treasureAmount[i] += ent.resourceSupplyMax();
			});
		this.baseManagers[1] = new BaseManager();
		this.baseManagers[1].init(gameState, events);
		this.baseManagers[1].setAnchor(CC);
		this.baseManagers[1].initTerritory(this, gameState);
		this.baseManagers[1].initGatheringFunctions(this, gameState);
		
		if (Config.debug)
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
				queues.dropsites.addItem(new ConstructionPlan(gameState, "structures/{civ}_storehouse",{ "base" : 1 }, 0, -1, pos));
				queues.minorTech.addItem(new ResearchPlan(gameState, "gather_capacity_wheelbarrow"));
			}
		}
	}
	
	var map = new Map(gameState.sharedScript, gameState.sharedScript.CCResourceMaps["wood"].map);
	if (Config.debug)
		map.dumpIm("map_CC_Wood.png");
	
	//this.reassignIdleWorkers(gameState);
	
	
	this.navalManager.init(gameState, events, queues);

	// TODO: change that.
	var civ = gameState.playerData.civ;
	
	// load units and buildings from the config files
	
	if (civ in Config.buildings.moderate){
		this.bModerate = Config.buildings.moderate[civ];
	}else{
		this.bModerate = Config.buildings.moderate['default'];
	}
	
	if (civ in Config.buildings.advanced){
		this.bAdvanced = Config.buildings.advanced[civ];
	}else{
		this.bAdvanced = Config.buildings.advanced['default'];
	}
	
	if (civ in Config.buildings.fort){
		this.bFort = Config.buildings.fort[civ];
	}else{
		this.bFort = Config.buildings.fort['default'];
	}
	
	for (var i in this.bAdvanced){
		this.bAdvanced[i] = gameState.applyCiv(this.bAdvanced[i]);
	}
	for (var i in this.bFort){
		this.bFort[i] = gameState.applyCiv(this.bFort[i]);
	}
	
	// TODO: figure out how to make this generic
	for (var i in this.attackManagers){
		this.availableAttacks[i] = new this.attackManagers[i](gameState, this);
	}
	
	var enemies = gameState.getEnemyEntities();
	var filter = Filters.byClassesOr(["CitizenSoldier", "Champion", "Hero", "Siege"]);
	this.enemySoldiers = enemies.filter(filter); // TODO: cope with diplomacy changes
	this.enemySoldiers.registerUpdates();
	
	// each enemy watchers keeps a list of entity collections about the enemy it watches
	// It also keeps track of enemy armies, merging/splitting as needed
	// TODO: remove those.
	this.enemyWatchers = {};
	this.ennWatcherIndex = [];
	for (var i = 1; i <= 8; i++)
		if (PlayerID != i && gameState.isPlayerEnemy(i)) {
			this.enemyWatchers[i] = new enemyWatcher(gameState, i);
			this.ennWatcherIndex.push(i);
			this.defenceManager.enemyArmy[i] = [];
		}
};

HQ.prototype.checkEvents = function (gameState, events, queues) {
	for (i in events)
	{
		if (events[i].type == "Destroy")
		{
			// TODO: probably check stuffs like a base destruction.
		} else if (events[i].type == "Create")
		{
			var evt = events[i];
			// Let's check if we have a building set to create a new base.
			if (evt.msg && evt.msg.entity)
			{
				var ent = gameState.getEntityById(evt.msg.entity);
				
				if (ent === undefined)
					continue; // happens when this message is right before a "Destroy" one for the same entity.

				if (ent.isOwn(PlayerID) && ent.getMetadata(PlayerID, "base") === -1)
				{
					// Okay so let's try to create a new base around this.
					var bID = uniqueIDBases;
					this.baseManagers[bID] = new BaseManager();
					this.baseManagers[bID].init(gameState, events, true);
					this.baseManagers[bID].setAnchor(ent);
					this.baseManagers[bID].initTerritory(this, gameState);
					
					// Let's get a few units out there to build this.
					// TODO: select the best base, or use multiple bases.
					var builders = this.bulkPickWorkers(gameState, bID, 10);
					builders.forEach(function (worker) {
						worker.setMetadata(PlayerID, "base", bID);
						worker.setMetadata(PlayerID, "subrole", "builder");
						worker.setMetadata(PlayerID, "target-foundation", ent.id());
					});
				}
			}
		} else if (events[i].type == "ConstructionFinished")
		{
			var evt = events[i];
			// Let's check if we have a building set to create a new base.
			// TODO: move to the base manager.
			if (evt.msg && evt.msg.newentity)
			{
				var ent = gameState.getEntityById(evt.msg.newentity);
				
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
	}
};

// okay, so here we'll create both females and male workers.
// We'll try to keep close to the "ratio" defined atop.
// Choice of citizen soldier is a bit messy.
// Before having 100 workers it focuses on speed, cost, and won't choose units that cost stone/metal
// After 100 it just picks the strongest;
// TODO: This should probably be changed to favor a more mixed approach for better defense.
//		(or even to adapt based on estimated enemy strategy).
// TODO: this should probably set which base it wants them in.
HQ.prototype.trainMoreWorkers = function(gameState, queues) {
	// Count the workers in the world and in progress
	var numFemales = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("units/{civ}_support_female_citizen"));
	numFemales += queues.villager.countQueuedUnitsWithClass("Support");

	// counting the workers that aren't part of a plan
	var numWorkers = 0;
	gameState.getOwnEntities().forEach (function (ent) {
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
	var numQueued = queues.villager.countQueuedUnits() + queues.citizenSoldier.countQueuedUnits();
	var numTotal = numWorkers + numQueued;

	// If we have too few, train more
	// should plan enough to always have females…
	// TODO: 15 here should be changed to something more sensible, such as nb of producing buildings.
	if (numTotal < this.targetNumWorkers && numQueued < 50 && (queues.villager.length() + queues.citizenSoldier.length()) < 120 && numInTraining < 15) {
		var template = gameState.applyCiv("units/{civ}_support_female_citizen");
		
		var size = Math.min(5, Math.ceil(numTotal / 10));

		if (numFemales/numTotal > this.femaleRatio && (numTotal > 20 || (this.fastStart && numTotal > 10))) {
			if (numTotal < 100)
				template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["cost",1], ["speed",0.5], ["costsResource", 0.5, "stone"], ["costsResource", 0.5, "metal"]]);
			else
				template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["strength",1] ]);
			if (!template)
				template = gameState.applyCiv("units/{civ}_support_female_citizen");
			if (gameState.currentPhase() === 1)
				size = 2;
		}
		
		if (numFemales/numTotal > this.femaleRatio * 1.3)
			queues.villager.paused = true;
		else if ((numFemales/numTotal < this.femaleRatio * 1.1) || gameState.ai.queueManager.getAvailableResources(gameState)["food"] > 250)
			queues.villager.paused = false;
		
		// TODO: perhaps assign them a default resource and check the base according to that.
		
		// base "0" means "auto"
		if (template === gameState.applyCiv("units/{civ}_support_female_citizen"))
			queues.villager.addItem(new TrainingPlan(gameState, template, { "role" : "worker", "base" : 0 }, size, 0, -1, size ));
		else
			queues.citizenSoldier.addItem(new TrainingPlan(gameState, template, { "role" : "worker", "base" : 0 }, size, 0, -1, size));
	}
};

// picks the best template based on parameters and classes
HQ.prototype.findBestTrainableUnit = function(gameState, classes, parameters) {
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
				aTopParam += getMaxStrength(a[1]) * param[1];
				bTopParam += getMaxStrength(b[1]) * param[1];
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
		}
		return -(aTopParam/(aDivParam+1)) + (bTopParam/(bDivParam+1));
	});
	return units[0][0];
};

// picks the best template based on parameters and classes
HQ.prototype.findBestTrainableSoldier = function(gameState, classes, parameters) {
	var units = gameState.findTrainableUnits(classes);
	
	if (units.length === 0)
		return undefined;
	
	
	units.sort(function(a, b) { //}) {
		var aDivParam = 0, bDivParam = 0;
		var aTopParam = 0, bTopParam = 0;
		for (var i in parameters) {
			var param = parameters[i];
			
			if (param[0] == "base") {
				aTopParam = param[1];
				bTopParam = param[1];
			}
			if (param[0] == "strength") {
				aTopParam += getMaxStrength(a[1]) * param[1];
				bTopParam += getMaxStrength(b[1]) * param[1];
			}
			if (param[0] == "siegeStrength") {
				aTopParam += getMaxStrength(a[1], "Structure") * param[1];
				bTopParam += getMaxStrength(b[1], "Structure") * param[1];
			}
			if (param[0] == "speed") {
				aTopParam += a[1].walkSpeed() * param[1];
				bTopParam += b[1].walkSpeed() * param[1];
			}
			
			if (param[0] == "cost") {
				aDivParam += a[1].costSum() * param[1];
				bDivParam += b[1].costSum() * param[1];
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
HQ.prototype.tryResearchTechs = function(gameState, queues) {
	if (queues.minorTech.length() === 0)
	{
		var possibilities = gameState.findAvailableTech();
		if (possibilities.length === 0)
			return;
		// randomly pick one. No worries about pairs in that case.
		var p = Math.floor((Math.random()*possibilities.length));
		queues.minorTech.addItem(new ResearchPlan(gameState, possibilities[p][0]));
	}
}

// We're given a worker and a resource type
// We'll assign the worker for the best base for that resource type.
// TODO: improve choice alogrithm
HQ.prototype.switchWorkerBase = function(gameState, worker, type) {
	var bestBase = 0;
	for (var i in this.baseManagers)
	{
		if (this.baseManagers[i].willGather[type] >= 1)
		{
			if (this.baseManagers[i].accessIndex === this.baseManagers[worker.getMetadata(PlayerID,"base")].accessIndex
				|| this.navalManager.canReach(gameState, this.baseManagers[i].accessIndex, this.baseManagers[worker.getMetadata(PlayerID,"base")].accessIndex))
			{
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
// TODO: also can't get over multiple bases right now.
HQ.prototype.bulkPickWorkers = function(gameState, newBaseID, number) {
	var accessIndex = this.baseManagers[newBaseID].accessIndex;
	if (!accessIndex)
		return false;
	// sorting bases by whether they are on the same accessindex or not.
	var baseBest = AssocArraytoArray(this.baseManagers).sort(function (a,b) {
		if (a.accessIndex === accessIndex && b.accessIndex !== accessIndex)
			return -1;
		else if (b.accessIndex === accessIndex && a.accessIndex !== accessIndex)
			return 1;
		return 0;
	});
	for (i in baseBest)
	{
		if (baseBest[i].workers.length > number)
		{
			return baseBest[i].pickBuilders(gameState,number);
		}
	}
	return false;
}

// returns the current gather rate
// This is not per-se exact, it performs a few adjustments ad-hoc to account for travel distance, stuffs like that.
HQ.prototype.GetCurrentGatherRates = function(gameState) {
	var self = this;

	var currentRates = {};
	for (var type in this.wantedRates)
		currentRates[type] = 0;
	
	for (i in this.baseManagers)
		this.baseManagers[i].getGatherRates(gameState, currentRates);

	return currentRates;
};


// Pick the resource which most needs another worker
HQ.prototype.pickMostNeededResources = function(gameState) {
	var self = this;
	
	this.wantedRates = gameState.ai.queueManager.wantedGatherRates(gameState);

	var currentRates = {};
	for (var type in this.wantedRates)
		currentRates[type] = 0;
	
	for (i in this.baseManagers)
	{
		var base = this.baseManagers[i];
		for (var type in this.wantedRates)
		{
			if (gameState.turnCache["gathererAssignementCache-" + type])
				currentRates[type] += gameState.turnCache["gathererAssignementCache-" + type];

			base.gatherersByType(gameState,type).forEach (function (ent) { //}){
				var worker = ent.getMetadata(PlayerID, "worker-object");
				if (worker)
					currentRates[type] += worker.getGatherRate(gameState);
			});
		}
	}
	
	// let's get our ideal number.
	
	var types = Object.keys(this.wantedRates);

	types.sort(function(a, b) {
		var va = (Math.max(0,self.wantedRates[a] - currentRates[a]))/ (currentRates[a]+1);
		var vb = (Math.max(0,self.wantedRates[b] - currentRates[b]))/ (currentRates[b]+1);
		
		// If they happen to be equal (generally this means "0" aka no need), make it equitable.
		if (va === vb)
			return (self.wantedRates[b]/(currentRates[b]+1)) - (self.wantedRates[a]/(currentRates[a]+1));
		return vb-va;
	});
	return types;
};

// If all the CC's are destroyed then build a new one
// TODO: rehabilitate.
HQ.prototype.buildNewCC= function(gameState, queues) {
	var numCCs = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_civil_centre"));
	numCCs += queues.civilCentre.length();

	// no use trying to lay foundations that will be destroyed
	if (gameState.defcon() > 2)
		for ( var i = numCCs; i < 1; i++) {
			gameState.ai.queueManager.clear();
			this.baseNeed["food"] = 0;
			this.baseNeed["wood"] = 50;
			this.baseNeed["stone"] = 50;
			this.baseNeed["metal"] = 50;
			queues.civilCentre.addItem(new ConstructionPlan(gameState, "structures/{civ}_civil_centre"));
		}
	return (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_civil_centre"), true) == 0 && gameState.currentPhase() > 1);
};

// Returns the best position to build a new Civil Centre
// Whose primary function would be to reach new resources of type "resource".
HQ.prototype.findBestEcoCCLocation = function(gameState, resource){
	
	var CCPlate = gameState.getTemplate("structures/{civ}_civil_centre");

	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.
	
	var territory = Map.createTerritoryMap(gameState);
	
	var obstructions = Map.createObstructionMap(gameState, 0);
	obstructions.expandInfluences();

	// copy the resource map as initialization.
	var friendlyTiles = new Map(gameState.sharedScript, gameState.sharedScript.CCResourceMaps[resource].map, true);
	friendlyTiles.setMaxVal(255);
	var ents = gameState.getOwnEntities().filter(Filters.byClass("CivCentre")).toEntityArray();
	var eEnts = gameState.getEnemyEntities().filter(Filters.byClass("CivCentre")).toEntityArray();

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
			
			var dist = SquareVectorDistance(entPos, pos);
			if (dist < 2120)
			{
				canBuild = false;
				continue;
			} else if (dist < 8000 || this.waterMap)
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
				if (SquareVectorDistance(entPos, pos) < 2500)
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
			if (SquareVectorDistance(dpPos, pos) < 100)
			{
				friendlyTiles.map[j] = 0;
				continue;
			} else if (SquareVectorDistance(dpPos, pos) < 400)
				friendlyTiles.map[j] /= 2;
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

	if (Config.debug)
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

	debug ("Best for value " + best[1] + " at " + uneval([x,z]));

	return [x,z];
};

HQ.prototype.buildTemple = function(gameState, queues){
	if (gameState.currentPhase() >= 2 ) {
		if (queues.economicBuilding.countQueuedUnits() === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_temple")) === 0){
			queues.economicBuilding.addItem(new ConstructionPlan(gameState, "structures/{civ}_temple", { "base" : 1 }));
		}
	}
};

HQ.prototype.buildMarket = function(gameState, queues){
	if (gameState.getPopulation() > Config.Economy.popForMarket && gameState.currentPhase() >= 2 ) {
		if (queues.economicBuilding.countQueuedUnitsWithClass("BarterMarket") === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market")) === 0){
			//only ever build one storehouse/CC/market at a time
			queues.economicBuilding.addItem(new ConstructionPlan(gameState, "structures/{civ}_market", { "base" : 1 }));
		}
	}
};

// Build a farmstead to go to town phase faster and prepare for research. Only really active on higher diff mode.
HQ.prototype.buildFarmstead = function(gameState, queues){
	if (gameState.getPopulation() > Config.Economy.popForFarmstead) {
		// achtung: "DropsiteFood" does not refer to CCs.
		if (queues.economicBuilding.countQueuedUnitsWithClass("DropsiteFood") === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_farmstead")) === 0){
			//only ever build one storehouse/CC/market at a time
			queues.economicBuilding.addItem(new ConstructionPlan(gameState, "structures/{civ}_farmstead", { "base" : 1 }));
		}
	}
};

// TODO: generic this, probably per-base
HQ.prototype.buildDock = function(gameState, queues){
	if (!this.waterMap || this.dockFailed)
		return;
	if (gameState.getTimeElapsed() > this.dockStartTime) {
		if (queues.economicBuilding.countQueuedUnitsWithClass("NavalMarket") === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_dock")) === 0) {
			var tp = ""
			if (gameState.civ() == "cart" && gameState.currentPhase() > 1)
				tp = "structures/{civ}_super_dock";
			else if (gameState.civ() !== "cart")
				tp = "structures/{civ}_dock";
			if (tp !== "")
			{
				var remaining = this.navalManager.getUnconnectedSeas(gameState, this.baseManagers[1].accessIndex);
				queues.economicBuilding.addItem(new ConstructionPlan(gameState, tp, { "base" : 1, "sea" : remaining[0] }));
			}
		}
	}
};

// if Aegis has resources it doesn't need, it'll try to barter it for resources it needs
// once per turn because the info doesn't update between a turn and I don't want to fix it.
// Not sure how efficient it is but it seems to be sane, at least.
HQ.prototype.tryBartering = function(gameState){
	var done = false;
	if (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_market"), true) >= 1) {
		
		var needs = gameState.ai.queueManager.futureNeeds(gameState);
		var ress = gameState.ai.queueManager.getAvailableResources(gameState);
		
		for (var sell in needs) {
			for (var buy in needs) {
				if (!done && buy != sell && needs[sell] <= 0 && ress[sell] > 400) {	// if we don't need it and have a buffer
					if ( (ress[buy] < 400) || needs[buy] > 0) {	// if we need that other resource/ have too little of it
						var markets = gameState.getOwnEntitiesByType(gameState.applyCiv("structures/{civ}_market"), true).toEntityArray();
						markets[0].barter(buy,sell,100);
						//debug ("bartered " +sell +" for " + buy + ", value 100");
						done = true;
					}
				}
			}
		}
	}
};

// build more houses if needed.
// kinda ugly, lots of special cases to both build enough houses but not tooo many…
HQ.prototype.buildMoreHouses = function(gameState,queues) {

	if (gameState.getPopulationLimit() < gameState.getPopulationMax()) {

		var numPlanned = queues.house.length();

		if (numPlanned < 3 || (numPlanned < 5 && gameState.getPopulation() > 80))
		{
			var plan = new ConstructionPlan(gameState, "structures/{civ}_house", { "base" : 1 });
			// change the starting condition to "less than 15 slots left".
			plan.isGo = function (gameState) {
				var HouseNb = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_house"), true);

				var freeSlots = 0;
				if (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber")
					freeSlots = gameState.getPopulationLimit() + HouseNb*5 - gameState.getPopulation();
				else
					freeSlots = gameState.getPopulationLimit() + HouseNb*10 - gameState.getPopulation();
				if (gameState.getPopulation() > 55 && Config.difficulty > 1)
					return (freeSlots <= 21);
				else if (gameState.getPopulation() >= 20 && Config.difficulty !== 0)
					return (freeSlots <= 16);
				else
					return (freeSlots <= 10);
			}
			queues.house.addItem(plan);
		}
	}
};

// checks if we have bases for all resource types (bar food for now) or if we need to expand.
HQ.prototype.checkBasesRessLevel = function(gameState,queues) {
	if (gameState.currentPhase() === 1 && !gameState.isResearching(gameState.townPhase()))
		return;
	var count = { "wood" : 0, "stone" : 0, "metal" : 0 }
	var capacity = { "wood" : 0, "stone" : 0, "metal" : 0 }
	var need = { "wood" : true, "stone" : true, "metal" : true };
	var posss = [];
	for (i in this.baseManagers)
	{
		var base = this.baseManagers[i];
		for (type in count)
		{
			if (base.getResourceLevel(gameState, type, "all") > 1500*Math.max(Config.difficulty,2))
				count[type]++;
			capacity[type] += base.getWorkerCapacity(gameState, type);
			if (base.willGather[type] !== 2)
				need[type] = false;
		}
	}
	for (type in count)
	{
		if (count[type] === 0 || need[type]
			|| capacity[type] < gameState.getOwnEntities().filter(Filters.and(Filters.byMetadata(PlayerID, "subrole", "gatherer"), Filters.byMetadata(PlayerID, "gather-type", type))).length * 1.05)
		{
			// plan a new base.
			if (gameState.countFoundationsWithType(gameState.applyCiv("structures/{civ}_civil_centre")) === 0 && queues.civilCentre.length() === 0) {
				if (this.outOf[type] && gameState.ai.playedTurn % 10 !== 0)
					continue;
				var pos = this.findBestEcoCCLocation(gameState, type);
				if (!pos)
				{
					// Okay so we'll set us as out of this.
					this.outOf[type] = true;
				} else {
					// base "-1" means new base.
					queues.civilCentre.addItem(new ConstructionPlan(gameState, "structures/{civ}_civil_centre",{ "base" : -1 }, 0, -1, pos));
				}
			}
		}
	}
};

// Deals with building fortresses and towers.
// Currently build towers next to every useful dropsites.
// TODO: Fortresses are placed randomly atm.
HQ.prototype.buildDefences = function(gameState, queues){
	
	var workersNumber = gameState.getOwnEntitiesByRole("worker").filter(Filters.not(Filters.byHasMetadata(PlayerID,"plan"))).length;
	
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv('structures/{civ}_defense_tower'))
		+ queues.defenceBuilding.length() < gameState.getEntityLimits()["DefenseTower"] && queues.defenceBuilding.length() < 4 && gameState.currentPhase() > 1) {
		for (i in this.baseManagers)
		{
			for (j in this.baseManagers[i].dropsites)
			{
				var amnts = this.baseManagers[i].dropsites[j];
				var dpEnt = gameState.getEntityById(j);
				if (dpEnt !== undefined && dpEnt.getMetadata(PlayerID, "defenseTower") !== true)
					if (amnts["wood"] || amnts["metal"] || amnts["stone"])
					{
						var position = dpEnt.position();
						if (position) {
							queues.defenceBuilding.addItem(new ConstructionPlan(gameState, 'structures/{civ}_defense_tower', { "base" : i }, 0 , -1, position));
						}
						dpEnt.setMetadata(PlayerID, "defenseTower", true);
					}
			}
		}
	}
	
	var numFortresses = 0;
	for (var i in this.bFort){
		numFortresses += gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bFort[i]));
	}
	
	if (queues.defenceBuilding.length() < 1 && (gameState.currentPhase() > 2 || gameState.isResearching("phase_city_generic")))
	{
		if (workersNumber >= 80 && gameState.getTimeElapsed() > numFortresses * this.fortressLapseTime + this.fortressStartTime)
		{
			if (!this.fortressStartTime)
				this.fortressStartTime = gameState.getTimeElapsed();
			queues.defenceBuilding.addItem(new ConstructionPlan(gameState, this.bFort[0], { "base" : 1 }));
			debug ("Building a fortress");
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

HQ.prototype.buildBlacksmith = function(gameState, queues){
	if (gameState.getTimeElapsed() > Config.Military.timeForBlacksmith*1000) {
		if (queues.militaryBuilding.length() === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_blacksmith")) === 0) {
			var tp = gameState.getTemplate(gameState.applyCiv("structures/{civ}_blacksmith"));
			if (tp.available(gameState))
				queues.militaryBuilding.addItem(new ConstructionPlan(gameState, "structures/{civ}_blacksmith", { "base" : 1 }));
		}
	}
};

// Deals with constructing military buildings (barracks, stables…)
// They are mostly defined by Config.js. This is unreliable since changes could be done easily.
// TODO: We need to determine these dynamically. Also doesn't build fortresses since the above function does that.
// TODO: building placement is bad. Choice of buildings is also fairly dumb.
HQ.prototype.constructTrainingBuildings = function(gameState, queues) {
	Engine.ProfileStart("Build buildings");
	var workersNumber = gameState.getOwnEntitiesByRole("worker").filter(Filters.not(Filters.byHasMetadata(PlayerID, "plan"))).length;

	if (workersNumber > Config.Military.popForBarracks1) {
		if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0])) + queues.militaryBuilding.length() < 1) {
			debug ("Trying to build barracks");
			queues.militaryBuilding.addItem(new ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
		}
	}
	
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0])) < 2 && workersNumber > Config.Military.popForBarracks2)
		if (queues.militaryBuilding.length() < 1)
			queues.militaryBuilding.addItem(new ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
	
	if (gameState.countEntitiesByType(gameState.applyCiv(this.bModerate[0]), true) === 2 && gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0])) < 3 && workersNumber > 125)
		if (queues.militaryBuilding.length() < 1)
		{
			queues.militaryBuilding.addItem(new ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
			if (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber") {
				queues.militaryBuilding.addItem(new ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
				queues.militaryBuilding.addItem(new ConstructionPlan(gameState, this.bModerate[0], { "base" : 1 }));
			}
		}
	//build advanced military buildings
	if (workersNumber >= Config.Military.popForBarracks2 - 15 && gameState.currentPhase() > 2){
		if (queues.militaryBuilding.length() === 0){
			var inConst = 0;
			for (var i in this.bAdvanced)
				inConst += gameState.countFoundationsWithType(gameState.applyCiv(this.bAdvanced[i]));
			if (inConst == 0 && this.bAdvanced && this.bAdvanced.length !== 0) {
				var i = Math.floor(Math.random() * this.bAdvanced.length);
				if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i])) < 1){
					queues.militaryBuilding.addItem(new ConstructionPlan(gameState, this.bAdvanced[i], { "base" : 1 }));
				}
			}
		}
	}
	if (gameState.civ() !== "gaul" && gameState.civ() !== "brit" && gameState.civ() !== "iber" &&
		workersNumber > 130 && gameState.currentPhase() > 2)
	{
		var Const = 0;
		for (var i in this.bAdvanced)
			Const += gameState.countEntitiesByType(gameState.applyCiv(this.bAdvanced[i]), true);
		if (inConst == 1) {
			var i = Math.floor(Math.random() * this.bAdvanced.length);
			if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i])) < 1){
				queues.militaryBuilding.addItem(new ConstructionPlan(gameState, this.bAdvanced[i], { "base" : 1 }));
				queues.militaryBuilding.addItem(new ConstructionPlan(gameState, this.bAdvanced[i], { "base" : 1 }));
			}
		}
	}
	
	Engine.ProfileStop();
};

// TODO: use pop(). Currently unused as this is too gameable.
HQ.prototype.garrisonAllFemales = function(gameState) {
	var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).toEntityArray();
	var females = gameState.getOwnEntities().filter(Filters.byClass("Support"));
	
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
HQ.prototype.ungarrisonAll = function(gameState) {
	this.hasGarrisonedFemales = false;
	var buildings = gameState.getOwnEntities().filter(Filters.and(Filters.byClass("Structure"),Filters.byCanGarrison())).toEntityArray();
	buildings.forEach( function (struct) {
		if (struct.garrisoned() && struct.garrisoned().length)
			struct.unloadAll();
		});
};

HQ.prototype.pausePlan = function(gameState, planName) {
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
HQ.prototype.unpausePlan = function(gameState, planName) {
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
HQ.prototype.pauseAllPlans = function(gameState) {
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
HQ.prototype.unpauseAllPlans = function(gameState) {
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
HQ.prototype.update = function(gameState, queues, events) {
	Engine.ProfileStart("Headquarters update");
	
	this.checkEvents(gameState,events,queues);
	//this.buildMoreHouses(gameState);

	//Engine.ProfileStart("Train workers and build farms, houses. Research techs.");
	this.trainMoreWorkers(gameState, queues);
	
	// sandbox doesn't expand.
	if (Config.difficulty !== 0)
		this.checkBasesRessLevel(gameState, queues);

	this.buildMoreHouses(gameState,queues);

	if (gameState.getTimeElapsed() > this.techStartTime && gameState.currentPhase() > 2)
		this.tryResearchTechs(gameState,queues);
	
	if (Config.difficulty > 1)
		this.tryBartering(gameState);
	
	this.buildFarmstead(gameState, queues);
	this.buildMarket(gameState, queues);
	// Deactivated: the temple had no useful purpose for the AI now.
	//if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market")) === 1)
	//	this.buildTemple(gameState, queues);
	this.buildDock(gameState, queues);	// not if not a water map.
	
	Engine.ProfileStart("Constructing military buildings and building defences");
	this.constructTrainingBuildings(gameState, queues);
	
	this.buildBlacksmith(gameState, queues);

	if(gameState.getTimeElapsed() > this.defenceBuildingTime)
		this.buildDefences(gameState, queues);
	Engine.ProfileStop();

	for (i in this.baseManagers)
	{
		this.baseManagers[i].checkEvents(gameState, events, queues)
		if ( ( (+i + gameState.ai.playedTurn) % (uniqueIDBases - 1)) === 0)
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
						debug ("Military Manager: " +attack.getType() +" plan " +attack.getName() +" aborted.");
						if (updateStep === 3) {
							this.attackPlansEncounteredWater = true;
							debug("No attack path found. Aborting.");
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
						
						debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
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
					
					debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
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
					debug ("Military Manager: " +attack.getType() +" plan " +attack.getName() +" is now finished.");
					attack.Abort(gameState);
					this.startedAttacks[attackType].splice(i--,1);
				}
			}
		}
	}
	
	// TODO: remove the limitation to attacks when on water maps.
	
	// Note: these indications of "rush" are currently unused.
	if (gameState.ai.strategy === "rush" && this.startedAttacks["CityAttack"].length !== 0) {
		// and then we revert.
		gameState.ai.strategy = "normal";
		Config.Economy.femaleRatio = 0.4;
		gameState.ai.modules.economy.targetNumWorkers = Math.max(Math.floor(gameState.getPopulationMax()*0.55), 1);
	} else if (gameState.ai.strategy === "rush" && this.upcomingAttacks["CityAttack"].length === 0)
	{
		Lalala = new CityAttack(gameState, this,this.TotalAttackNumber, -1, "rush")
		this.TotalAttackNumber++;
		this.upcomingAttacks["CityAttack"].push(Lalala);
		debug ("Starting a little something");
	} else if (gameState.ai.strategy !== "rush" && !this.waterMap)
	{
		// creating plans after updating because an aborted plan might be reused in that case.
		if (gameState.countEntitiesByType(gameState.applyCiv(this.bModerate[0]), true) >= 1 && !this.attackPlansEncounteredWater
			&& gameState.getTimeElapsed() > this.attackPlansStartTime && gameState.currentPhase() > 1) {
			if (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_dock"), true) === 0 && this.waterMap)
			{
				// wait till we get a dock.
			} else {
				// basically only the first plan, really.
				if (this.upcomingAttacks["CityAttack"].length == 0 && gameState.getTimeElapsed() < 12*60000) {
					var Lalala = new CityAttack(gameState, this,this.TotalAttackNumber, -1);
					if (Lalala.failed)
					{
						this.attackPlansEncounteredWater = true; // hack
					} else {
						debug ("Military Manager: Creating the plan " +this.TotalAttackNumber);
						this.TotalAttackNumber++;
						this.upcomingAttacks["CityAttack"].push(Lalala);
					}
				} else if (this.upcomingAttacks["CityAttack"].length == 0 && Config.difficulty !== 0) {
					var Lalala = new CityAttack(gameState, this,this.TotalAttackNumber, -1, "superSized");
					if (Lalala.failed)
					{
						this.attackPlansEncounteredWater = true; // hack
					} else {
						debug ("Military Manager: Creating the super sized plan " +this.TotalAttackNumber);
						this.TotalAttackNumber++;
						this.upcomingAttacks["CityAttack"].push(Lalala);
					}
				}
			}
		}
	}
	
	/*
	 // very old relic. This should be reimplemented someday so the code stays here.
	 
	 if (this.HarassRaiding && this.preparingRaidNumber + this.startedRaidNumber < 1 && gameState.getTimeElapsed() < 780000) {
	 var Lalala = new CityAttack(gameState, this,this.totalStartedAttackNumber, -1, "harass_raid");
	 if (!Lalala.createSupportPlans(gameState, this, )) {
	 debug ("Military Manager: harrassing plan not a valid option");
	 this.HarassRaiding = false;
	 } else {
	 debug ("Military Manager: Creating the harass raid plan " +this.totalStartedAttackNumber);
	 
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

	if (Config.difficulty !== 0)
		this.tryBartering(gameState);
		
	this.buildFarmstead(gameState, queues);
	this.buildMarket(gameState, queues);
	// Deactivated: the temple had no useful purpose for the AI now.
	//if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market")) === 1)
	//	this.buildTemple(gameState, queues);
	this.buildDock(gameState, queues);	// not if not a water map.
*/
	Engine.ProfileStop();	// Heaquarters update
};
