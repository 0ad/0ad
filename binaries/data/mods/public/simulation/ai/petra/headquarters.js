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

	this.dockStartTime = this.Config.Economy.dockStartTime * 1000;
	
	this.dockFailed = false;	// sanity check
	this.waterMap = false;	// set by the aegis.js file.
	
	this.econState = "growth";	// existing values: growth, townPhasing.
	this.phaseStarted = undefined;

	// cache the rates.
	this.wantedRates = { "food": 0, "wood": 0, "stone":0, "metal": 0 };
	this.currentRates = { "food": 0, "wood": 0, "stone":0, "metal": 0 };

	// this means we'll have about a big third of women, and thus we can maximize resource gathering rates.
	this.femaleRatio = this.Config.Economy.femaleRatio;

	this.lastTerritoryUpdate = -1;
	this.stopBuilding = []; // list of buildings to stop (temporarily) production because no room

	this.towerStartTime = 0;
	this.towerLapseTime = this.Config.Military.towerLapseTime * 1000;
	this.fortressStartTime = 0;
	this.fortressLapseTime = this.Config.Military.fortressLapseTime * 1000;

	this.baseManagers = {};
	this.attackManager = new m.AttackManager(this.Config);
	this.defenseManager = new m.DefenseManager(this.Config);
	this.tradeManager = new m.TradeManager(this.Config);
	this.navalManager = new m.NavalManager();

	this.boostedSoldiers = undefined;
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

	if (this.baseManagers[1])     // Affects units in the different bases
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
			var x = Math.round(pos[0] / gameState.cellSize);
			var z = Math.round(pos[1] / gameState.cellSize);
			var id = x + width*z;
			for (var i in self.baseManagers)
			{
				if (self.baseManagers[i].territoryIndices.indexOf(id) === -1)
					continue;
				self.baseManagers[i].assignEntity(ent);
				if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
					self.baseManagers[i].assignResourceToDropsite(gameState, ent);
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
		{
			queues.dropsites.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse", { "base": 1 }, newDP.pos));
			queues.minorTech.addItem(new m.ResearchPlan(gameState, "gather_capacity_wheelbarrow"));
		}
	}

	this.attackManager.init(gameState, queues);
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
	
	if (civ in this.Config.buildings.fort)
		this.bFort = this.Config.buildings.fort[civ];
	else
		this.bFort = this.Config.buildings.fort['default'];
	
	for (var i in this.bBase)
		this.bBase[i] = gameState.applyCiv(this.bBase[i]);
	for (var i in this.bAdvanced)
		this.bAdvanced[i] = gameState.applyCiv(this.bAdvanced[i]);
	for (var i in this.bFort)
		this.bFort[i] = gameState.applyCiv(this.bFort[i]);
};

m.HQ.prototype.checkEvents = function (gameState, events, queues)
{
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
			
			if (ent.isOwn(PlayerID))
			{
				if (ent.getMetadata(PlayerID, "baseAnchor") == true)
				{
					var base = ent.getMetadata(PlayerID, "base");
					if (this.baseManagers[base].constructing)
						this.baseManagers[base].constructing = false;
					this.baseManagers[base].anchor = ent;
					this.baseManagers[base].buildings.updateEnt(ent);
					this.updateTerritories(gameState);
				}
				else if (ent.hasTerritoryInfluence())
					this.updateTerritories(gameState);
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
	if (!this.boostedSoldiers)
	{
		if (numTotal > this.targetNumWorkers || (numTotal >= this.Config.Economy.popForTown 
			&& gameState.currentPhase() === 1 && !gameState.isResearching(gameState.townPhase())))
			return;
		if (numQueued > 50 || (numQueuedF > 20 && numQueuedS > 20) || numInTraining > 15)
			return;
	}
	else if (numQueuedS > 20)
			return;

	// default template and size
	var template = gameState.applyCiv("units/{civ}_support_female_citizen");
	var size = Math.min(5, Math.ceil(numTotal / 10));

	// Choose whether we want soldiers instead.
	if ((numFemales+numQueuedF)/numTotal > this.femaleRatio || this.boostedSoldiers)
	{
		if (numTotal < 35)
			template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["cost",1], ["speed",0.5], ["costsResource", 0.5, "stone"], ["costsResource", 0.5, "metal"]]);
		else
			template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["strength",1] ]);
		if (!template && this.boostedSoldiers)
			return;
		if (!template)
			template = gameState.applyCiv("units/{civ}_support_female_citizen");
	}

	// TODO: perhaps assign them a default resource and check the base according to that.
	
	// base "0" means "auto"
	if (template === gameState.applyCiv("units/{civ}_support_female_citizen"))
		queues.villager.addItem(new m.TrainingPlan(gameState, template, { "role": "worker", "base": 0 }, size, size));
	else
		queues.citizenSoldier.addItem(new m.TrainingPlan(gameState, template, { "role": "worker", "base": 0 }, size, size));
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
m.HQ.prototype.tryResearchTechs = function(gameState, queues)
{
	if (gameState.currentPhase() < 2 || queues.minorTech.length() !== 0)
		return;

	var possibilities = gameState.findAvailableTech();
	for (var i = 0; i < possibilities.length; ++i)
	{
		var techName = possibilities[i][0];
		if (techName.indexOf("attack_tower_watch") !== -1 || techName.indexOf("gather_mining_servants") !== -1 ||
			techName.indexOf("gather_mining_shaftmining") !== -1)
		{
			queues.minorTech.addItem(new m.ResearchPlan(gameState, techName));
			return;
		}
	}

	if (gameState.currentPhase() < 3)
		return;

	// remove some tech not yet used by this AI
	for (var i = 0; i < possibilities.length; ++i)
	{
		var techName = possibilities[i][0];
		if (techName.indexOf("heal_rate") !== -1 || techName.indexOf("heal_range") !== -1 ||
			techName.indexOf("heal_temple") !== -1 || techName.indexOf("unlock_females_house") !== -1)
			possibilities.splice(i--, 1);
		// temporary hack for upgrade problem TODO fix that
		else if (techName.slice(0, 12) === "upgrade_rank")
			possibilities.splice(i--, 1);
	}
	if (possibilities.length === 0)
		return;
	// randomly pick one. No worries about pairs in that case.
	var p = Math.floor((Math.random()*possibilities.length));
	queues.minorTech.addItem(new m.ResearchPlan(gameState, possibilities[p][0]));
};


// returns an entity collection of workers through BaseManager.pickBuilders
// TODO: better the choice algo.
m.HQ.prototype.bulkPickWorkers = function(gameState, newBaseID, number)
{
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

m.HQ.prototype.GetTotalResourceLevel = function(gameState)
{
	var total = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	for (var i in this.baseManagers)
		for (var type in total)
			total[type] += this.baseManagers[i].getResourceLevel(gameState, type);

	return total;
};

// returns the current gather rate
// This is not per-se exact, it performs a few adjustments ad-hoc to account for travel distance, stuffs like that.
m.HQ.prototype.GetCurrentGatherRates = function(gameState)
{
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

// If all the CC's are destroyed then build a new one
// TODO: rehabilitate.
m.HQ.prototype.buildNewCC= function(gameState, queues)
{
    var numCCs = gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bBase[0]), true);
	numCCs += queues.civilCentre.length();

	// no use trying to lay foundations that will be destroyed
	if (gameState.defcon() > 2)
		for (var i = numCCs; i < 1; i++) {
			gameState.ai.queueManager.clear();
			this.baseNeed["food"] = 0;
			this.baseNeed["wood"] = 50;
			this.baseNeed["stone"] = 50;
			this.baseNeed["metal"] = 50;
			queues.civilCentre.addItem(new m.ConstructionPlan(gameState, this.bBase[0]));
		}
	return (gameState.countEntitiesByType(gameState.applyCiv(this.bBase[0]), true) == 0 && gameState.currentPhase() > 1);
};

// Returns the best position to build a new Civil Centre
// Whose primary function would be to reach new resources of type "resource".
m.HQ.prototype.findEconomicCCLocation = function(gameState, resource)
{	
	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.

	// create an empty map
	var locateMap = new API3.Map(gameState.sharedScript);
	locateMap.setMaxVal(255);
	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0);
	obstructions.expandInfluences();

	var ccEnts = gameState.getEntities().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	var dpEnts = gameState.getOwnDropsites().toEntityArray();

	for (var j = 0; j < locateMap.length; ++j)
	{
		// We check for our other CCs: the distance must not be too big. Anything bigger will result in scrapping.
		// This ensures territorial continuity.
		
		var norm = 0.5;   // TODO adjust it, knowing that we will sum 5 maps
		if (this.territoryMap.getOwnerIndex(j) !== 0 || this.borderMap.map[j] === 2)
		{
			norm = 0;
			continue;
		}
		else if (this.borderMap.map[j] === 1)	// disfavor the borders of the map
			norm *= 0.5;

		var pos = [j%locateMap.width+0.5, Math.floor(j/locateMap.width)+0.5];
		pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
		// We require that it is accessible from our starting position
		// TODO modify when naval maps
		if (gameState.ai.accessibility.getAccessValue(pos) !== gameState.ai.myIndex)
		{
			norm = 0;
			continue;
		}

		// checking distance to other cc
		var minDist = Math.min();
		for each (var cc in ccEnts)
		{
			var ccPos = cc.position();
			var dist = API3.SquareVectorDistance(ccPos, pos);
			if (dist < 14000)    // Reject if too near from any cc
			{
				norm = 0
				break;
			}
			if (!gameState.isPlayerAlly(cc.owner()))
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
		if (minDist > 170000 && !this.waterMap)   // Reject if too far from any allied cc (-> not connected)
			continue;
		else if (minDist > 130000)                // Disfavor if quite far from any allied cc
			norm *= 0.5;

		for each (var dp in dpEnts)
		{
			if (dp.hasClass("Elephant"))
				continue;
			var dpPos = dp.position();
			var dist = API3.SquareVectorDistance(dpPos, pos);
			if (dist < 3600)
			{
				norm = 0;
				continue;
			}
			else if (dist < 6400)
				norm *= 0.5;
		}
		if (norm == 0)
			continue;
		
		var val = 2*gameState.sharedScript.CCResourceMaps[resource].map[j]
			+ gameState.sharedScript.CCResourceMaps["wood"].map[j]
			+ gameState.sharedScript.CCResourceMaps["stone"].map[j]
			+ gameState.sharedScript.CCResourceMaps["metal"].map[j];
		val *= norm;
		if (val > 255)
			val = 255;
		locateMap.map[j] = val;
	}
	
	
	var best = locateMap.findBestTile(6, obstructions);
	var bestIdx = best[0];

	if (m.DebugEnabled())
	{
		gameState.sharedScript.CCResourceMaps["wood"].dumpIm("woodMap.png", 300);
		gameState.sharedScript.CCResourceMaps["stone"].dumpIm("stoneMap.png", 300);
		gameState.sharedScript.CCResourceMaps["metal"].dumpIm("metalMap.png", 300);
		locateMap.dumpIm("cc_placement_base_" + best[1] + ".png",300);
		obstructions.dumpIm("cc_placement_base_" + best[1] + "_obs.png", 20);
	}

	if (this.Config.debug)
		warn("on a trouve une base avec best (cut=60) = " + best[1]);
	// not good enough.
	if (best[1] < 60)
		return false;
	
	var bestIdx = best[0];
	var x = ((bestIdx % locateMap.width) + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx / locateMap.width) + 0.5) * gameState.cellSize;
	if (this.Config.debug)
		warn(" avec accessIndex " + gameState.ai.myIndex + " new " + gameState.ai.accessibility.getAccessValue([x,z]));
	return [x,z];
};

// Returns the best position to build a new Civil Centre
// Whose primary function would be to assure territorial continuity with our allies
m.HQ.prototype.findStrategicCCLocation = function(gameState)
{	
	// This builds a map. The procedure is fairly simple.
	// We minimize the Sum((dist-300)**2) where the sum is on all allied CC
	// with the constraints that all CC have dist > 200 and at least one have dist < 400
	// This needs at least 2 CC. Otherwise, go back to economic CC.

	// TODO add CC foundations (needed for allied)
	var ccEnts = gameState.getEntities().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	var numAllyCC = 0;
	for each (var cc in ccEnts)
		if (gameState.isPlayerAlly(cc.owner()))
			numAllyCC += 1;
	if (numAllyCC < 2)
		return this.findEconomicCCLocation(gameState, "wood");

	// obstruction map
	var obstructions = m.createObstructionMap(gameState, 0);
	obstructions.expandInfluences();

	var map = {};
	var width = this.territoryMap.width;

	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.territoryMap.getOwnerIndex(j) !== 0 || this.borderMap.map[j] === 2)
			continue;

		var ix = j%width;
		var iy = Math.floor(j/width);
		var pos = [ix+0.5, iy+0.5];
		pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
		// We require that it is accessible from our starting position
		// TODO modify when naval maps
		if (gameState.ai.accessibility.getAccessValue(pos) !== gameState.ai.myIndex)
			continue;

		// checking distances to other cc
		var minDist = Math.min();
		var sumDelta = 0;
		for each (var cc in ccEnts)
		{
			var ccPos = cc.position();
			var dist = API3.SquareVectorDistance(ccPos, pos);
			if (dist < 14000)    // Reject if too near from any cc
			{
				minDist = 0;
				break;
			}
			if (!gameState.isPlayerAlly(cc.owner()))
				continue;
			if (dist < 40000)    // Reject if quite near from ally cc
			{
				minDist = 0;
				break;
			}
			var delta = Math.sqrt(dist) - 300;
			if (cc.owner === PlayerID)     // small preference territory continuity with our territory
				delta = 1.05*delta;    // rather than ally one
			sumDelta += delta*delta;
			if (dist < minDist)
				minDist = dist;
		}
		if (minDist < 1 || minDist > 160000)
			continue;
		
		map[j] = 10 + sumDelta;
		// disfavor border of the map
		if (this.borderMap.map[j] === 1)
			map[j] = map[j] + 10000;
	}

	var bestIdx = undefined;
	var bestVal = undefined;
	var radius = 6;
	for (var i in map)
	{
		if (obstructions.map[+i] <= radius)
			continue;
		var v = map[i];
		if (bestVal !== undefined && v > bestVal)
			continue;
		bestVal = v;
		bestIdx = i;
	}

	if (this.Config.debug)
		warn("on a trouve une base strategic avec bestVal = " + bestVal);	

	if (bestVal === undefined)
		return undefined;

	var x = (bestIdx%width + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx/width) + 0.5) * gameState.cellSize;
	if (this.Config.debug)
		warn(" avec accessIndex " + gameState.ai.myIndex + " new " + gameState.ai.accessibility.getAccessValue([x,z]));
	return [x,z];
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
	var obstructions = m.createObstructionMap(gameState, 0);
	obstructions.expandInfluences();

	var map = {};
	var width = this.territoryMap.width;
	for (var j = 0; j < this.territoryMap.length; ++j)
	{
		// do not try if well inside or outside territory
		if (this.frontierMap.map[j] === 0)
			continue
		if (this.frontierMap.map[j] === 1 && template.hasClass("Tower"))
			continue;
		if (this.basesMap.map[j] === 0)   // inaccessible cell
			continue;

		var ix = j%width;
		var iy = Math.floor(j/width);
		var pos = [ix+0.5, iy+0.5];
		pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
		// checking distances to other structures
		var minDist = Math.min();
		for each (var str in enemyStructures)
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

		for each (var str in ownStructures)
		{
			if ((template.hasClass("Tower") && str.hasClass("Tower")) || (template.hasClass("Fortress") && str.hasClass("Fortress")))
			{
				var strPos = str.position();
				if (!strPos)
					continue;
				var dist = API3.SquareVectorDistance(strPos, pos);
				if (dist < 4225) //  TODO check on true buildrestrictions instead of this 65*65 
				{
					minDist = -1;
					break;
				}
			}
		}
		if (minDist < 0)
			continue;
		
		map[j] = minDist;
	}

	var bestIdx = undefined;
	var bestVal = undefined;
	if (template.hasClass("Fortress"))
		var radius = Math.floor(template.obstructionRadius() / gameState.cellSize) + 2;
	else
		var radius = Math.ceil(template.obstructionRadius() / gameState.cellSize);

	for (var j in map)
	{
		if (obstructions.map[+j] <= radius)
			continue;
		var v = map[j];
		if (bestVal !== undefined && v > bestVal)
			continue;
		bestVal = v;
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
	// add the health regeneration to the research we want.
	if (!gameState.isResearched("health_regen_units") && !gameState.isResearching("health_regen_units"))
		queues.minorTech.addItem(new m.ResearchPlan(gameState, "health_regen_units"));
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
	// Wait to have at least one house before the farmstead
	if (gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_house"), true) == 0)
		return;
	if (queues.economicBuilding.countQueuedUnitsWithClass("DropsiteFood") > 0)
		return;
	if (!this.canBuild(gameState, "structures/{civ}_farmstead"))
		return;

	queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_farmstead"));
	// add the farming plough to the research we want.
	if (!gameState.isResearched("gather_farming_plows") && !gameState.isResearching("gather_farming_plows"))
		queues.minorTech.addItem(new m.ResearchPlan(gameState, "gather_farming_plows"));
};

// TODO: generic this, probably per-base
m.HQ.prototype.buildDock = function(gameState, queues)
{
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
			if (tp !== "" && this.canBuild(gameState, tp))
			{
				var remaining = this.navalManager.getUnconnectedSeas(gameState, this.baseManagers[1].accessIndex);
				queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, tp, { "base": 1, "sea": remaining[0] }));
			}
		}
	}
};

// Try to barter unneeded resources for needed resources.
// once per turn because the info doesn't update between a turn and fixing isn't worth it.
m.HQ.prototype.tryBartering = function(gameState)
{
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
		for each (var buy in elemCost.types)
		{
			if (available[buy] >= 0)
				continue;	// don't care if we still have available resource
			var need = elemCost[buy] - account[buy];
			if (need <= 0 || 50*rates[buy] > need)	// don't care if we don't need resource or our rate is good enough
				continue;
			
			if (buy == "food" && need < 400)
				continue;

			// pick the best resource to barter.
			var bestToBarter = undefined;
			var bestRate = 0;
			for each (var sell in elemCost.types)
			{
				if (sell === buy)
					continue;
				// I wanna keep some
				if (available[sell] < 130 + need)
					continue;
				var barterRate = getBarterRate(prices, buy, sell);
				if (barterRate > bestRate)
				{
					bestRate = barterRate;
					bestToBarter = otherRess;
				}
			}
			if (bestToBarter !== undefined && bestRate > 10)
			{
				markets[0].barter(buy, sell, 100);
				if (this.Config.debug > 0)
					warn("Snipe bartered " + sell +" for " + buy + ", value 100" + " with barterRate " + bestRate);
				return true;
			}
		}
	}
	// now barter for big needs.
	var needs = gameState.ai.queueManager.currentNeeds(gameState);
	for each (var buy in  needs.types)
	{
		if (needs[buy] == 0 || needs[buy] < rates[buy]*30) // check if our rate allows to gather it fast enough
			continue;

		// pick the best resource to barter.
		var bestToSell = undefined;
		var bestRate = 0;
		for each (var sell in needs.types)
		{
			if (sell === buy)
				continue;
			if (needs[sell] > 0 || available[sell] < 500)    // do not sell if we need it or do not have enough buffer
				continue;

			var barterRateMin = 70;
			if (available > 1000)
				barterRateMin = 50;
			if (sell === "food")
				barterRateMin -= 40;
			else if (buy === "food")
				barterRateMin += 10;

			var barterRate = getBarterRate(prices, buy, sell);
			if (barterRate > bestRate && barterRate > barterRateMin)
			{
				bestRate = barterRate;
				bestToSell = sell;
			}
		}

		if (bestToSell !== undefined)
		{
			markets[0].barter(buy, bestToSell, 100);
			if (this.Config.debug > 0)
				warn("Gross bartered: sold " + bestToSell +" for " + buy + " >> need sell " + needs[bestToSell]
					 + " buy " + needs[buy] + " rate " + rates[buy] + " available sell " + available[bestToSell]
					 + " buy " + available[buy] + " barterRate " + bestRate);
			return true;
		}
	}
	return false;
};

// Try to setup trade routes  TODO complete it
// TODO use also docks (should be counted in Class("Market"), but may be build one when necessary
m.HQ.prototype.buildTradeRoute = function(gameState, queues)
{
	var market1 = gameState.getOwnStructures().filter(API3.Filters.and(API3.Filters.byClass("Market"), API3.Filters.not(API3.Filters.isFoundation()))).toEntityArray();
	var market2 = gameState.getAllyEntities().filter(API3.Filters.and(API3.Filters.byClass("Market"), API3.Filters.not(API3.Filters.isFoundation()))).toEntityArray();
	if (market1.length < 1)  // We have to wait  ... the first market will be built when needed conditions are satisfied 
		return false;

	var needed = 2;
	if (market2.length > 0)
		var needed = 1;
	if (market1.length < needed)
	{
		// TODO what to do if market1 is invalid ??? should not happen
		if (!market1[0] || !market1[0].position())
			return false;
		// We require at least two finished bases
		if (!this.baseManagers[2] || this.baseManagers[2].constructing)
			return false;
		if (queues.economicBuilding.countQueuedUnitsWithClass("Market") > 0 ||
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) >= needed)
			return false;
		if (!this.canBuild(gameState, "structures/{civ}_market"))
			return false;
		// We have to build a second market ... try to put it as far as possible from the first one
		// TODO improve, for the time being, we affect it to the farthest base
		var marketBase =  market1[0].getMetadata(PlayerID, "base");
		var distmax = -1;
		var base = -1;
		for (var i in this.baseManagers)
		{
			if (marketBase === +i)
				continue;
			if (!this.baseManagers[i].anchor || !this.baseManagers[i].anchor.position())
				continue;
			var dist = API3.SquareVectorDistance(market1[0].position(), this.baseManagers[i].anchor.position());
			if (dist < distmax)
				continue;
			distmax = dist;
			base = +i;
		}
		if (distmax > 0)
		{
			if (this.Config.debug > 1)
				warn(" a second market will be built in base " + base);
			// TODO build also docks when better
			queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_market", { "base": base }));
		}
		return false;
	}

	if (market2.length === 0)
		market2 = market1;
	var distmax = -1;
	var imax = -1;
	var jmax = -1;
	for each (var m1 in market1)
	{
		if (!m1.position())
			continue;
		for each (var m2 in market2)
		{
			if (m1.id() === m2.id())
				continue;
			if (!m2.position())
				continue;
			var dist = API3.SquareVectorDistance(m1.position(), m2.position());
			if (dist < distmax)
				continue;
			distmax = dist;
			this.tradeManager.setTradeRoute(m1, m2);
		}
	}
	if (distmax < 0)
	{
		if (this.Config.debug)
			warn("no trade route possible");
		return false;
	}
	if (this.Config.debug)
		warn("one trade route set");
	return true;
};

// build more houses if needed.
// kinda ugly, lots of special cases to both build enough houses but not tooo many…
m.HQ.prototype.buildMoreHouses = function(gameState,queues)
{
	if (gameState.getPopulationMax() < gameState.getPopulationLimit())
	{
		var numPlanned = queues.house.length();
		if (numPlanned)
			warn(" ########  Houses planned while already max pop !! remove them from queue");
	}
	if (gameState.getPopulationMax() < gameState.getPopulationLimit())
		return;

	var numPlanned = queues.house.length();
	if (numPlanned < 3 || (numPlanned < 5 && gameState.getPopulation() > 80))
	{
		var plan = new m.ConstructionPlan(gameState, "structures/{civ}_house");
		// make the difficulty available to the isGo function without having to pass it as argument
		var difficulty = this.Config.difficulty;
		var self = this;
		// change the starting condition to "less than 15 slots left".
		plan.isGo = function (gameState) {
			if (!self.canBuild(gameState, "structures/{civ}_house"))
				return false;
			var HouseNb = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_house"), true);

			var freeSlots = 0;
			// TODO get this info from PopulationBonus of houses
			if (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber" ||
				gameState.civ() == "maur" || gameState.civ() == "ptol")
				var popBonus = 5;
			else
				var popBonus = 10;
			freeSlots = gameState.getPopulationLimit() + HouseNb*popBonus - gameState.getPopulation();
			if (gameState.getPopulation() > 55 && difficulty > 1)
				return (freeSlots <= 21);
			else if (gameState.getPopulation() >= 30 && difficulty !== 0)
				return (freeSlots <= 15);
			else
				return (freeSlots <= 10);
		};
		queues.house.addItem(plan);
	}

	if (numPlanned > 0 && this.econState == "townPhasing")
	{
		var houseQueue = queues.house.queue;
		var count = gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length;
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
};

// checks the status of the territory expansion. If no new economic bases created, build some strategic ones.
m.HQ.prototype.checkBaseExpansion = function(gameState,queues)
{
	var numUnits = 	gameState.getOwnUnits().length;
	var numCCs = gameState.countEntitiesByType(gameState.applyCiv(this.bBase[0]), true);
	if (Math.floor(numUnits/60) >= numCCs)
		this.buildNewBase(gameState, queues);
};

m.HQ.prototype.buildNewBase = function(gameState, queues, type)
{
	if (gameState.currentPhase() === 1 && !gameState.isResearching(gameState.townPhase()))
		return false;
	if (gameState.countFoundationsByType(gameState.applyCiv(this.bBase[0]), true) !== 0 || queues.civilCentre.length() !== 0)
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
	if (gameState.currentPhase() > 2 || gameState.isResearching(gameState.cityPhase()))
	{
		// try to build fortresses
		if (queues.defenseBuilding.length() === 0 && this.canBuild(gameState, this.bFort[0]))
		{
			var numFortresses = 0;
			for (var i in this.bFort)
				numFortresses += gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bFort[i]), true);

			if (gameState.getTimeElapsed() > (1 + 0.05*numFortresses)*this.fortressLapseTime + this.fortressStartTime)
			{
				this.fortressStartTime = gameState.getTimeElapsed();
				// TODO should affect it to the right base
				queues.defenseBuilding.addItem(new m.ConstructionPlan(gameState, this.bFort[0]));
			}
		}

		// let's add a siege building plan to the current attack plan if there is none currently.
		var numSiegeBuilder = 0;
		if (gameState.civ() === "mace")
			numSiegeBuilder = gameState.countEntitiesByType(gameState.applyCiv("siege_workshop"), true);
		else
			for (var i in this.bFort)
				numSiegeBuilder += gameState.countEntitiesByType(gameState.applyCiv(this.bFort[i]), true);

		if (numSiegeBuilder > 0)
		{
			if (this.attackManager.upcomingAttacks["CityAttack"].length !== 0)
			{
				var attack = this.attackManager.upcomingAttacks["CityAttack"][0];
				if (!attack.unitStat["Siege"])
					attack.addSiegeUnits(gameState);
			}
		}
	}

	if (gameState.currentPhase() < 2 
		|| queues.defenseBuilding.length() !== 0 
		|| !this.canBuild(gameState, "structures/{civ}_defense_tower"))
		return;	

	var numTowers = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_defense_tower"), true);
	if (gameState.getTimeElapsed() > (1 + 0.05*numTowers)*this.towerLapseTime + this.towerStartTime)
	{
		this.towerStartTime = gameState.getTimeElapsed();
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

	if (this.canBuild(gameState, "structures/{civ}_blacksmith"))
		queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_blacksmith"));
};

// Deals with constructing military buildings (barracks, stables…)
// They are mostly defined by Config.js. This is unreliable since changes could be done easily.
// TODO: We need to determine these dynamically. Also doesn't build fortresses since the above function does that.
// TODO: building placement is bad. Choice of buildings is also fairly dumb.
m.HQ.prototype.constructTrainingBuildings = function(gameState, queues)
{
	var workersNumber = gameState.getOwnEntitiesByRole("worker", true).filter(API3.Filters.not(API3.Filters.byHasMetadata(PlayerID, "plan"))).length;

	var barrackNb = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_barracks"), true);
	var bestBase = this.findBestBaseForMilitary(gameState);

	if (this.canBuild(gameState, "structures/{civ}_barracks"))
	{
		// first barracks.
		if (workersNumber > this.Config.Military.popForBarracks1 || (this.econState == "townPhasing" && gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length < 5))
		{
			if (barrackNb + queues.militaryBuilding.length() < 1)
			{
				var plan = new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "base" : bestBase });
				plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("militaryBuilding", 130); };
				queues.militaryBuilding.addItem(plan);
			}
		}

		// second barracks.
		if (barrackNb < 2 && workersNumber > this.Config.Military.popForBarracks2)
			if (queues.militaryBuilding.length() < 1)
				queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "base" : bestBase }));

		// third barracks (optional 4th/5th for some civs as they rely on barracks more.)
		if (barrackNb === 2 && barrackNb + queues.militaryBuilding.length() < 3 && workersNumber > 125)
			if (queues.militaryBuilding.length() === 0)
			{
				queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "base" : bestBase }));
				if (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber")
				{
					queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "base" : bestBase }));
					queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "base" : bestBase }));
				}
			}
	}

	//build advanced military buildings
	if (workersNumber >= this.Config.Military.popForBarracks2 - 15 && gameState.currentPhase() > 2){
		if (queues.militaryBuilding.length() === 0)
		{
			var inConst = 0;
			for (var i in this.bAdvanced)
				inConst += gameState.countFoundationsByType(gameState.applyCiv(this.bAdvanced[i]));
			if (inConst == 0 && this.bAdvanced && this.bAdvanced.length !== 0)
			{
				var i = Math.floor(Math.random() * this.bAdvanced.length);
				if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i]), true) < 1 &&
					this.canBuild(gameState, this.bAdvanced[i]))
					queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bAdvanced[i], { "base" : bestBase }));
			}
		}
	}
	// build second advanced building except for some civs.
	if (gameState.currentPhase() > 2 && gameState.civ() !== "gaul" && gameState.civ() !== "brit" && gameState.civ() !== "iber" && workersNumber > 130)
	{
		var Const = 0;
		for (var i in this.bAdvanced)
			Const += gameState.countEntitiesByType(gameState.applyCiv(this.bAdvanced[i]), true);
		if (inConst == 1)
		{
			var i = Math.floor(Math.random() * this.bAdvanced.length);
			if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i]), true) < 1 &&
				this.canBuild(gameState, this.bAdvanced[i]))
				queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, this.bAdvanced[i], { "base" : bestBase }));
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
	for each (var cc in ccEnts)
	{
		if (cc.owner() != PlayerID)
			continue;
		for each (var cce in ccEnts)
		{
			if (gameState.isPlayerAlly(cce.owner()))
				continue;
			var dist = API3.SquareVectorDistance(cc.position(), cce.position());
			if (dist < distMin)
			{
			    bestBase = cc.getMetadata(PlayerID, "base");
			    distMin = dist;
			}
		}
	}
	return bestBase;
};

m.HQ.prototype.boostSoldiers = function(gameState, val, emergency)
{
	if (this.boostedSoldiers && this.boostedSoldiers >= val)
		return;
	if (!this.boostedSoldiers)
		this.nominalSoldierPriority = this.Config.priorities.citizenSoldier;
	this.boostedSoldiers = val;
	gameState.ai.queueManager.changePriority("citizenSoldier", val);
	if (!emergency)
		return;

	//  Emergency: reset accounts from all other queues
	for (var p in gameState.ai.queueManager.queues)
		if (p != "citizenSoldier")
			gameState.ai.queueManager.accounts[p].reset();
};

m.HQ.prototype.unboostSoldiers = function(gameState)
{
	if (!this.boostedSoldiers)
		return;
	gameState.ai.queueManager.changePriority("citizenSoldier", this.nominalSoldierPriority);
	this.boostedSoldiers = undefined;
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
			var distmin = Math.min();
			var baseID = undefined;
			var ix = j%width;
			var iy = Math.floor(j/width);

			var pos = [ix+0.5, iy+0.5];
			pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
			for each (var base in this.baseManagers)
			{
				if (!base.anchor || !base.anchor.position())
					continue;
				if (base.accessIndex !== gameState.ai.accessibility.getAccessValue(pos))
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
	if (this.Config.debug > 1)
		warn(" buildings stopped " + uneval(this.stopBuilding));
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

// Some functions are run every turn
// Others once in a while
m.HQ.prototype.update = function(gameState, queues, events)
{
	Engine.ProfileStart("Headquarters update");
	
	this.territoryMap = m.createTerritoryMap(gameState);

	if (this.Config.debug > 0)
	{
		gameState.getOwnUnits().forEach (function (ent) {
		    return;
			if (!ent.isIdle())
			{
				ent.setMetadata(PlayerID, "lastIdle", undefined);
				return;
			}
			if (ent.hasClass("Animal"))
				return;
			if (!ent.getMetadata(PlayerID, "lastIdle"))
			{
				ent.setMetadata(PlayerID, "lastIdle", gameState.ai.playedTurn);
				return;
			}
			if (gameState.ai.playedTurn - ent.getMetadata(PlayerID, "lastIdle") < 20)
				return;
			warn(" unit idle since " + (gameState.ai.playedTurn-ent.getMetadata(PlayerID, "lastIdle")) + " turns");
			warn(" >>> base " + ent.getMetadata(PlayerID, "base"));
			warn(" >>> role " + ent.getMetadata(PlayerID, "role"));
			warn(" >>> subrole " + ent.getMetadata(PlayerID, "subrole"));
			warn(" >>> gather-type " + ent.getMetadata(PlayerID, "gather-type"));
			warn(" >>> target-foundation " + ent.getMetadata(PlayerID, "target-foundation"));
			warn(" >>> PartOfArmy " + ent.getMetadata(PlayerID, "PartOfArmy"));
			warn(" >>> plan " + ent.getMetadata(PlayerID, "plan"));
			ent.setMetadata(PlayerID, "lastIdle", gameState.ai.playedTurn);
		});
	}

	this.checkEvents(gameState,events,queues);

	// TODO find a better way to update
	if (this.phaseStarted && gameState.currentPhase() === this.phaseStarted)
	{
		this.phaseStarted = undefined;
		this.updateTerritories(gameState);
	}
	else if (gameState.ai.playedTurn - this.lastTerritoryUpdate > 100)
		this.updateTerritories(gameState);

	this.trainMoreWorkers(gameState, queues);
	
	// sandbox doesn't expand.
	if (this.Config.difficulty !== 0 && gameState.ai.playedTurn % 10 === 7)
		this.checkBaseExpansion(gameState, queues);

	if (gameState.ai.playedTurn % 2 === 0)
		this.buildMoreHouses(gameState,queues);
	else
		this.buildFarmstead(gameState, queues);

	if (this.waterMap)
		this.buildDock(gameState, queues);

	if (queues.minorTech.length() === 0 && gameState.ai.playedTurn % 5 === 1)
		this.tryResearchTechs(gameState,queues);

	if (gameState.currentPhase() > 1)
	{
		this.buildMarket(gameState, queues);
		this.buildBlacksmith(gameState, queues);
		this.buildTemple(gameState, queues);

		if (this.Config.difficulty > 1)
		{
			this.tryBartering(gameState);
			if (!this.tradeManager.hasTradeRoute() && gameState.ai.playedTurn % 5 === 2)
				this.buildTradeRoute(gameState, queues);
			this.tradeManager.update(gameState, queues);
		}
	}

	this.defenseManager.update(gameState, events);

	this.constructTrainingBuildings(gameState, queues);

	if (this.Config.difficulty > 0)
		this.buildDefenses(gameState, queues);

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
