var PETRA = function(m)
{

/**
 * Headquarters
 * Deal with high level logic for the AI. Most of the interesting stuff gets done here.
 * Some tasks:
 *  -defining RESS needs
 *  -BO decisions.
 *     > training workers
 *     > building stuff (though we'll send that to bases)
 *  -picking strategy (specific manager?)
 *  -diplomacy -> diplomacyManager
 *  -planning attacks -> attackManager
 *  -picking new CC locations.
 */

m.HQ = function(Config)
{
	this.Config = Config;

	this.econState = "growth";	// existing values: growth, townPhasing.
	this.currentPhase = undefined;

	// Cache the rates.
	this.turnCache = {};
	// Some resources objects (will be filled in init)
	this.wantedRates = {};
	this.currentRates = {};
	this.lastFailedGather = {};

	// workers configuration
	this.targetNumWorkers = this.Config.Economy.targetNumWorkers;
	this.supportRatio = this.Config.Economy.supportRatio;

	this.stopBuilding = new Map(); // list of buildings to stop (temporarily) production because no room

	this.fortStartTime = 180;	// wooden defense towers, will start at fortStartTime + towerLapseTime
	this.towerStartTime = 0;	// stone defense towers, will start as soon as available
	this.towerLapseTime = this.Config.Military.towerLapseTime;
	this.fortressStartTime = 0;	// will start as soon as available
	this.fortressLapseTime = this.Config.Military.fortressLapseTime;
	this.extraTowers = Math.round(Math.min(this.Config.difficulty, 3) * this.Config.personality.defensive);
	this.extraFortresses = Math.round(Math.max(Math.min(this.Config.difficulty - 1, 2), 0) * this.Config.personality.defensive);

	this.baseManagers = [];
	this.attackManager = new m.AttackManager(this.Config);
	this.defenseManager = new m.DefenseManager(this.Config);
	this.tradeManager = new m.TradeManager(this.Config);
	this.navalManager = new m.NavalManager(this.Config);
	this.researchManager = new m.ResearchManager(this.Config);
	this.diplomacyManager = new m.DiplomacyManager(this.Config);
	this.garrisonManager = new m.GarrisonManager();
};

/** More initialisation for stuff that needs the gameState */
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

	for (let res of gameState.sharedScript.resourceList)
	{
		this.wantedRates[res] = 0;
		this.currentRates[res] = 0;
	}

	this.treasures = gameState.getEntities().filter(function (ent) {
		let type = ent.resourceSupplyType();
		return type && type.generic === "treasure";
	});
	this.treasures.registerUpdates();
	this.currentPhase = gameState.currentPhase();
	this.decayingStructures = new Set();
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

	this.updateTerritories(gameState);
};

/**
 * returns the sea index linking regions 1 and region 2 (supposed to be different land region)
 * otherwise return undefined
 * for the moment, only the case land-sea-land is supported
 */
m.HQ.prototype.getSeaIndex = function (gameState, index1, index2)
{
	let path = gameState.ai.accessibility.getTrajectToIndex(index1, index2);
	if (path && path.length == 3 && gameState.ai.accessibility.regionType[path[1]] === "water")
		return path[1];

	if (this.Config.debug > 1)
	{
		API3.warn("bad path from " + index1 + " to " + index2 + " ??? " + uneval(path));
		API3.warn(" regionLinks start " + uneval(gameState.ai.accessibility.regionLinks[index1]));
		API3.warn(" regionLinks end   " + uneval(gameState.ai.accessibility.regionLinks[index2]));
	}
	return undefined;
};

m.HQ.prototype.checkEvents = function (gameState, events, queues)
{
	if (events.TerritoriesChanged.length || events.DiplomacyChanged.length)
		this.updateTerritories(gameState);

	if (events.DiplomacyChanged.length)
	{
		gameState.resetAllyStructures();
		gameState.resetEnemyStructures();
	}

	for (let evt of events.Create)
	{
		// Let's check if we have a building set to create a new base.
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || !ent.isOwn(PlayerID))
			continue;

		if (ent.getMetadata(PlayerID, "base") == -1)
		{
			// Okay so let's try to create a new base around this.
			let newbase = new m.BaseManager(gameState, this.Config);
			newbase.init(gameState, "unconstructed");
			newbase.setAnchor(gameState, ent);
			this.baseManagers.push(newbase);
			// Let's get a few units from other bases there to build this.
			let builders = this.bulkPickWorkers(gameState, newbase, 10);
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
			let base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
			let builders = this.bulkPickWorkers(gameState, base, 10);
			if (builders !== false)
			{
				builders.forEach(function (worker) {
					worker.setMetadata(PlayerID, "base", base.ID);
					worker.setMetadata(PlayerID, "subrole", "builder");
					worker.setMetadata(PlayerID, "target-foundation", ent.id());
				});
			}
		}
	}

	for (let evt of events.ConstructionFinished)
	{
		// Let's check if we have a building set to create a new base.
		// TODO: move to the base manager.
		if (evt.newentity)
		{
			if (evt.newentity === evt.entity)  // repaired building
				continue;
			let ent = gameState.getEntityById(evt.newentity);
			if (!ent || !ent.isOwn(PlayerID))
				continue;

			if (ent.getMetadata(PlayerID, "baseAnchor") === true)
			{
				let base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
				if (base.constructing)
					base.constructing = false;
				base.anchor = ent;
				base.anchorId = evt.newentity;
				base.buildings.updateEnt(ent);
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
		}
	}

	for (let evt of events.OwnershipChanged)   // capture events
	{
		if (evt.to !== PlayerID)
			continue;
		let ent = gameState.getEntityById(evt.entity);
		if (!ent)
			continue;
		if (ent.position())
		    ent.setMetadata(PlayerID, "access", gameState.ai.accessibility.getAccessValue(ent.position()));
		if (ent.hasClass("Unit"))
		{
			m.getBestBase(gameState, ent).assignEntity(gameState, ent);
			ent.setMetadata(PlayerID, "role", undefined);
			ent.setMetadata(PlayerID, "subrole", undefined);
			ent.setMetadata(PlayerID, "plan", undefined);
			ent.setMetadata(PlayerID, "PartOfArmy", undefined);
			if (ent.hasClass("Trader"))
			{
				ent.setMetadata(PlayerID, "role", "trader");
				ent.setMetadata(PlayerID, "route", undefined);
			}
			if (ent.hasClass("Worker"))
			{
				ent.setMetadata(PlayerID, "role", "worker");
				ent.setMetadata(PlayerID, "subrole", "idle");
			}
			if (ent.hasClass("Ship"))
				ent.setMetadata(PlayerID, "sea", gameState.ai.accessibility.getAccessValue(ent.position(), true));
			if (!ent.hasClass("Support") && !ent.hasClass("Ship") && ent.attackTypes() !== undefined)
				ent.setMetadata(PlayerID, "plan", -1);
			continue;
		}
		if (ent.hasClass("CivCentre"))   // build a new base around it
		{
			let newbase = new m.BaseManager(gameState, this.Config);
			if (ent.foundationProgress() !== undefined)
				newbase.init(gameState, "unconstructed");
			else
				newbase.init(gameState, "captured");
			newbase.setAnchor(gameState, ent);
			this.baseManagers.push(newbase);
			newbase.assignEntity(gameState, ent);
		}
		else
		{
			// TODO should be reassigned later if a better base is captured
			m.getBestBase(gameState, ent).assignEntity(gameState, ent);
			if (ent.decaying())
			{
				if (ent.isGarrisonHolder() && this.garrisonManager.addDecayingStructure(gameState, evt.entity, true))
					continue;
				if (!this.decayingStructures.has(evt.entity))
					this.decayingStructures.add(evt.entity);
			}
		}
	}

	// deal with the different rally points of training units: the rally point is set when the training starts
	// for the time being, only autogarrison is used

	for (let evt of events.TrainingStarted)
	{
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || !ent.isOwn(PlayerID))
			continue;

		if (!ent._entity.trainingQueue || !ent._entity.trainingQueue.length)
			continue;
		let metadata = ent._entity.trainingQueue[0].metadata;
		if (metadata && metadata.garrisonType)
			ent.setRallyPoint(ent, "garrison");  // trained units will autogarrison
		else
			ent.unsetRallyPoint();
	}

	for (let evt of events.TrainingFinished)
	{
		for (let entId of evt.entities)
		{
			let ent = gameState.getEntityById(entId);
			if (!ent || !ent.isOwn(PlayerID))
				continue;

			if (!ent.position())
			{
				// we are autogarrisoned, check that the holder is registered in the garrisonManager
				let holderId = ent.unitAIOrderData()[0].target;
				let holder = gameState.getEntityById(holderId);
				if (holder)
					this.garrisonManager.registerHolder(gameState, holder);
			}
			else if (ent.getMetadata(PlayerID, "garrisonType"))
			{
				// we were supposed to be autogarrisoned, but this has failed (may-be full)
				ent.setMetadata(PlayerID, "garrisonType", undefined);
			}

			// Check if this unit is no more needed in its attack plan
			// (happen when the training ends after the attack is started or aborted)
			let plan = ent.getMetadata(PlayerID, "plan");
			if (plan !== undefined && plan >= 0)
			{
				let attack = this.attackManager.getPlan(plan);
				if (!attack || attack.state !== "unexecuted")
					ent.setMetadata(PlayerID, "plan", -1);
			}
			// Assign it immediately to something useful to do
			if (ent.getMetadata(PlayerID, "role") === "worker")
			{
				let base;
				if (ent.getMetadata(PlayerID, "base") === undefined)
				{
					base = m.getBestBase(gameState, ent);
					base.assignEntity(gameState, ent);
				}
				else
					base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
				base.reassignIdleWorkers(gameState, [ent]);
				base.workerObject.update(gameState, ent);
			}
			else if (ent.resourceSupplyType() && ent.position())
			{
				let type = ent.resourceSupplyType();
				if (!type.generic)
					continue;
				let dropsites = gameState.getOwnDropsites(type.generic);
				let pos = ent.position();
				let access = gameState.ai.accessibility.getAccessValue(pos);
				let distmin = Math.min();
				let goal;
				for (let dropsite of dropsites.values())
				{
					if (!dropsite.position() || dropsite.getMetadata(PlayerID, "access") !== access)
						continue;
					let dist = API3.SquareVectorDistance(pos, dropsite.position());
					if (dist > distmin)
						continue;
					distmin = dist;
					goal = dropsite.position();
				}
				if (goal)
					ent.moveToRange(goal[0], goal[1]);
			}
		}
	}

	for (let evt of events.TerritoryDecayChanged)
	{
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || !ent.isOwn(PlayerID) || ent.foundationProgress() !== undefined)
			continue;
		if (evt.to)
		{
			if (ent.isGarrisonHolder() && this.garrisonManager.addDecayingStructure(gameState, evt.entity))
				continue;
			if (!this.decayingStructures.has(evt.entity))
				this.decayingStructures.add(evt.entity);
		}
		else if (ent.isGarrisonHolder())
			this.garrisonManager.removeDecayingStructure(evt.entity);
	}

	// then deals with decaying structures
	for (let entId of this.decayingStructures)
	{
		let ent = gameState.getEntityById(entId);
		if (ent && ent.decaying() && ent.isOwn(PlayerID))
		{
			let capture = ent.capturePoints();
			if (!capture)
				continue;
			let captureRatio = capture[PlayerID] / capture.reduce((a, b) => a + b);
			if (captureRatio < 0.50)
				continue;
			let decayToGaia = true;
			for (let i = 1; i < capture.length; ++i)
			{
				if (gameState.isPlayerAlly(i) || !capture[i])
					continue;
				decayToGaia = false;
				break;
			}
			if (decayToGaia)
				continue;
			let ratioMax = 0.70;
			for (let evt of events.Attacked)
			{
				if (ent.id() != evt.target)
					continue;
				ratioMax = 0.90;
				break;
			}
			if (captureRatio > ratioMax)
				continue;
			ent.destroy();
		}
		this.decayingStructures.delete(entId);
	}
};

/** Called by the "town phase" research plan once it's started */
m.HQ.prototype.OnTownPhase = function(gameState)
{
	let phaseName = gameState.getTemplate(gameState.townPhase()).name();
	m.chatNewPhase(gameState, phaseName, true);
};

/** Called by the "city phase" research plan once it's started */
m.HQ.prototype.OnCityPhase = function(gameState)
{
	// increase the priority of defense buildings to free this queue for our first fortress
	gameState.ai.queueManager.changePriority("defenseBuilding", 2*this.Config.priorities.defenseBuilding);

	let phaseName = gameState.getTemplate(gameState.cityPhase()).name();
	m.chatNewPhase(gameState, phaseName, true);
};

/** This code trains citizen workers, trying to keep close to a ratio of worker/soldiers */
m.HQ.prototype.trainMoreWorkers = function(gameState, queues)
{
	// default template
	let requirementsDef = [["cost", 1], ["costsResource", 1, "food"]];
	let classesDef = ["Support", "Worker"];
	let templateDef = this.findBestTrainableUnit(gameState, classesDef, requirementsDef);

	// counting the workers that aren't part of a plan
	let numberOfWorkers = 0;   // all workers
	let numberOfSupports = 0;  // only support workers (i.e. non fighting)
	gameState.getOwnUnits().forEach (function (ent) {
		if (ent.getMetadata(PlayerID, "role") === "worker" && ent.getMetadata(PlayerID, "plan") === undefined)
		{
			++numberOfWorkers;
			if (ent.hasClass("Support"))
				++numberOfSupports;
		}
	});
	let numberInTraining = 0;
	gameState.getOwnTrainingFacilities().forEach(function(ent) {
		for (let item of ent.trainingQueue())
		{
			numberInTraining += item.count;
			if (item.metadata && item.metadata.role && item.metadata.role === "worker" && item.metadata.plan === undefined)
			{
				numberOfWorkers += item.count;
				if (item.metadata.support)
					numberOfSupports += item.count;
			}
		}
	});

	// Anticipate the optimal batch size when this queue will start
	// and adapt the batch size of the first and second queued workers to the present population
	// to ease a possible recovery if our population was drastically reduced by an attack
	// (need to go up to second queued as it is accounted in queueManager)
	let size = numberOfWorkers < 12 ? 1 : Math.min(5, Math.ceil(numberOfWorkers / 10));
	if (queues.villager.plans[0])
	{
		queues.villager.plans[0].number = Math.min(queues.villager.plans[0].number, size);
		if (queues.villager.plans[1])
			queues.villager.plans[1].number = Math.min(queues.villager.plans[1].number, size);
	}
	if (queues.citizenSoldier.plans[0])
	{
		queues.citizenSoldier.plans[0].number = Math.min(queues.citizenSoldier.plans[0].number, size);
		if (queues.citizenSoldier.plans[1])
			queues.citizenSoldier.plans[1].number = Math.min(queues.citizenSoldier.plans[1].number, size);
	}

	let numberOfQueuedSupports = queues.villager.countQueuedUnits();
	let numberOfQueuedSoldiers = queues.citizenSoldier.countQueuedUnits();
	let numberQueued = numberOfQueuedSupports + numberOfQueuedSoldiers;
	let numberTotal = numberOfWorkers + numberQueued;

	if (this.saveResources && numberTotal > this.Config.Economy.popForTown + 10)
		return;
	if (numberTotal > this.targetNumWorkers || (numberTotal >= this.Config.Economy.popForTown &&
		gameState.currentPhase() == 1 && !gameState.isResearching(gameState.townPhase())))
		return;
	if (numberQueued > 50 || (numberOfQueuedSupports > 20 && numberOfQueuedSoldiers > 20) || numberInTraining > 15)
		return;

	// Choose whether we want soldiers or support units.
	let supportRatio = gameState.isTemplateDisabled(gameState.applyCiv("structures/{civ}_field")) ? Math.min(this.supportRatio, 0.1) : this.supportRatio;
	let supportMax = supportRatio * this.targetNumWorkers;
	let supportNum = supportMax * Math.atan(numberTotal/supportMax) / 1.570796;

	let template;
	if (numberOfSupports + numberOfQueuedSupports > supportNum)
	{
		let requirements;
		if (numberTotal < 45)
			requirements = [ ["cost", 1], ["speed", 0.5], ["costsResource", 0.5, "stone"], ["costsResource", 0.5, "metal"]];
		else
			requirements = [ ["strength", 1] ];

		let classes = ["CitizenSoldier", "Infantry"];
		let proba = Math.random();
		//  we require at least 30% ranged and 30% melee
		if ( proba < 0.3 )
			classes.push("Ranged");
		else if ( proba < 0.6 )
			classes.push("Melee");

		template = this.findBestTrainableUnit(gameState, classes, requirements);
	}

	// If the template variable is empty, the default unit (Support unit) will be used
	// base "0" means automatic choice of base
	if (!template && templateDef)
		queues.villager.addPlan(new m.TrainingPlan(gameState, templateDef, { "role": "worker", "base": 0, "support": true }, size, size));
	else if (template)
		queues.citizenSoldier.addPlan(new m.TrainingPlan(gameState, template, { "role": "worker", "base": 0 }, size, size));
};

/** picks the best template based on parameters and classes */
m.HQ.prototype.findBestTrainableUnit = function(gameState, classes, requirements)
{
	let units;
	if (classes.indexOf("Hero") !== -1)
		units = gameState.findTrainableUnits(classes, []);
	else if (classes.indexOf("Siege") !== -1)	// We do not want siege tower as AI does not know how to use it
		units = gameState.findTrainableUnits(classes, ["SiegeTower"]);
	else						// We do not want hero when not explicitely specified
		units = gameState.findTrainableUnits(classes, ["Hero"]);

	if (units.length === 0)
		return undefined;

	let parameters = requirements.slice();
	let remainingResources = this.getTotalResourceLevel(gameState);    // resources (estimation) still gatherable in our territory
	let availableResources = gameState.ai.queueManager.getAvailableResources(gameState); // available (gathered) resources
	for (let type in remainingResources)
	{
		if (availableResources[type] > 800)
			continue;
		if (remainingResources[type] > 800)
			continue;
		let costsResource = remainingResources[type] > 400 ? 0.6 : 0.2;
		let toAdd = true;
		for (let param of parameters)
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

	units.sort(function(a, b) {
		let aDivParam = 0;
		let bDivParam = 0;
		let aTopParam = 0;
		let bTopParam = 0;
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
		return -aTopParam/(aDivParam+1) + bTopParam/(bDivParam+1);
	});
	return units[0][0];
};

/**
 * returns an entity collection of workers through BaseManager.pickBuilders
 * TODO: when same accessIndex, sort by distance
 */
m.HQ.prototype.bulkPickWorkers = function(gameState, baseRef, number)
{
	let accessIndex = baseRef.accessIndex;
	if (!accessIndex)
		return false;
	// sorting bases by whether they are on the same accessindex or not.
	let baseBest = this.baseManagers.slice().sort(function (a,b) {
		if (a.accessIndex == accessIndex && b.accessIndex != accessIndex)
			return -1;
		else if (b.accessIndex == accessIndex && a.accessIndex != accessIndex)
			return 1;
		return 0;
	});

	let needed = number;
	let workers = new API3.EntityCollection(gameState.sharedScript);
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
	if (!workers.length)
		return false;
	return workers;
};

m.HQ.prototype.getTotalResourceLevel = function(gameState)
{
	let total = {};
	for (let res of gameState.sharedScript.resourceList)
		total[res] = 0;
	for (let base of this.baseManagers)
		for (let res in total)
			total[res] += base.getResourceLevel(gameState, res);

	return total;
};

/**
 * returns the current gather rate
 * This is not per-se exact, it performs a few adjustments ad-hoc to account for travel distance, stuffs like that.
 */
m.HQ.prototype.GetCurrentGatherRates = function(gameState)
{
	if (!this.turnCache.gatherRates)
	{
		for (let res in this.currentRates)
			this.currentRates[res] = 0.5 * this.GetTCResGatherer(res);

		for (let base of this.baseManagers)
			base.getGatherRates(gameState, this.currentRates);

		for (let res in this.currentRates)
		{
			if (this.currentRates[res] < 0)
			{
				if (this.Config.debug > 0)
					API3.warn("Petra: current rate for " + res + " < 0 with " + this.GetTCResGatherer(res) + " moved gatherers");
				this.currentRates[res] = 0;
			}
		}
		this.turnCache.gatherRates = true;
	}

	return this.currentRates;
};

/**
 * Pick the resource which most needs another worker
 * How this works:
 * We get the rates we would want to have to be able to deal with our plans
 * We get our current rates
 * We compare; we pick the one where the discrepancy is highest.
 * Need to balance long-term needs and possible short-term needs.
 */
m.HQ.prototype.pickMostNeededResources = function(gameState)
{
	this.wantedRates = gameState.ai.queueManager.wantedGatherRates(gameState);
	let currentRates = this.GetCurrentGatherRates(gameState);

	let needed = [];
	for (let res in this.wantedRates)
		needed.push({ "type": res, "wanted": this.wantedRates[res], "current": currentRates[res] });

	needed.sort((a, b) => {
		let va = Math.max(0, a.wanted - a.current) / (a.current + 1);
		let vb = Math.max(0, b.wanted - b.current) / (b.current + 1);
		// If they happen to be equal (generally this means "0" aka no need), make it fair.
		if (va === vb)
			return a.current - b.current;
		return vb - va;
	});
	return needed;
};

/**
 * Returns the best position to build a new Civil Centre
 * Whose primary function would be to reach new resources of type "resource".
 */
m.HQ.prototype.findEconomicCCLocation = function(gameState, template, resource, proximity, fromStrategic)
{
	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.

	Engine.ProfileStart("findEconomicCCLocation");

	// obstruction map
	let obstructions = m.createObstructionMap(gameState, 0, template);
	let halfSize = 0;
	if (template.get("Footprint/Square"))
		halfSize = Math.max(+template.get("Footprint/Square/@depth"), +template.get("Footprint/Square/@width")) / 2;
	else if (template.get("Footprint/Circle"))
		halfSize = +template.get("Footprint/Circle/@radius");

	let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
	let dpEnts = gameState.getOwnDropsites().filter(API3.Filters.not(API3.Filters.byClassesOr(["CivCentre", "Elephant"])));
	let ccList = [];
	for (let cc of ccEnts.values())
		ccList.push({"pos": cc.position(), "ally": gameState.isPlayerAlly(cc.owner())});
	let dpList = [];
	for (let dp of dpEnts.values())
		dpList.push({"pos": dp.position()});

	let bestIdx;
	let bestVal;
	let radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);
	let scale = 250 * 250;
	let proxyAccess;
	let nbShips = this.navalManager.transportShips.length;
	if (proximity)	// this is our first base
	{
		// if our first base, ensure room around
		radius = Math.ceil((template.obstructionRadius() + 8) / obstructions.cellSize);
		// scale is the typical scale at which we want to find a location for our first base
		// look for bigger scale if we start from a ship (access < 2) or from a small island
		let cellArea = gameState.getMap().cellSize * gameState.getMap().cellSize;
		proxyAccess = gameState.ai.accessibility.getAccessValue(proximity);
		if (proxyAccess < 2 || cellArea*gameState.ai.accessibility.regionSize[proxyAccess] < 24000)
			scale = 400 * 400;
	}

	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.territoryMap.getOwnerIndex(j) !== 0)
			continue;
		// with enough room around to build the cc
		let i = this.territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;
		// we require that it is accessible
		let index = gameState.ai.accessibility.landPassMap[i];
		if (!this.landRegions[index])
			continue;
		if (proxyAccess && nbShips === 0 && proxyAccess !== index)
			continue;

		let norm = 0.5;   // TODO adjust it, knowing that we will sum 5 maps
		// checking distance to other cc
		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];

		if (proximity)	// this is our first cc, let's do it near our units
			norm /= (1 + API3.SquareVectorDistance(proximity, pos) / scale);
		else
		{
			let minDist = Math.min();

			for (let cc of ccList)
			{
				let dist = API3.SquareVectorDistance(cc.pos, pos);
				if (dist < 14000)    // Reject if too near from any cc
				{
					norm = 0;
					break;
				}
				if (!cc.ally)
					continue;
				if (dist < 40000)    // Reject if too near from an allied cc
				{
					norm = 0;
					break;
				}
				if (dist < 62000)   // Disfavor if quite near an allied cc
					norm *= 0.5;
				if (dist < minDist)
					minDist = dist;
			}
			if (norm === 0)
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

			for (let dp of dpList)
			{
				let dist = API3.SquareVectorDistance(dp.pos, pos);
				if (dist < 3600)
				{
					norm = 0;
					break;
				}
				else if (dist < 6400)
					norm *= 0.5;
			}
			if (norm === 0)
				continue;
		}

		if (this.borderMap.map[j] > 0)	// disfavor the borders of the map
			norm *= 0.5;

		let val = 2*gameState.sharedScript.ccResourceMaps[resource].map[j] +
			    gameState.sharedScript.ccResourceMaps.wood.map[j] +
			    gameState.sharedScript.ccResourceMaps.stone.map[j] +
			    gameState.sharedScript.ccResourceMaps.metal.map[j];
		val *= norm;

		if (bestVal !== undefined && val < bestVal)
			continue;
		if (this.isDangerousLocation(gameState, pos, halfSize))
			continue;
		bestVal = val;
		bestIdx = i;
	}

	Engine.ProfileStop();

	let cut = 60;
	if (fromStrategic || proximity)  // be less restrictive
		cut = 30;
	if (this.Config.debug > 1)
		API3.warn("we have found a base for " + resource + " with best (cut=" + cut + ") = " + bestVal);
	// not good enough.
	if (bestVal < cut)
		return false;

	let x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	let z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;

	// Define a minimal number of wanted ships in the seas reaching this new base
	let indexIdx = gameState.ai.accessibility.landPassMap[bestIdx];
	for (let base of this.baseManagers)
	{
		if (!base.anchor || base.accessIndex === indexIdx)
			continue;
		let sea = this.getSeaIndex(gameState, base.accessIndex, indexIdx);
		if (sea !== undefined)
			this.navalManager.setMinimalTransportShips(gameState, sea, 1);
	}

	return [x, z];
};

/**
 * Returns the best position to build a new Civil Centre
 * Whose primary function would be to assure territorial continuity with our allies
 */
m.HQ.prototype.findStrategicCCLocation = function(gameState, template)
{
	// This builds a map. The procedure is fairly simple.
	// We minimize the Sum((dist-300)**2) where the sum is on the three nearest allied CC
	// with the constraints that all CC have dist > 200 and at least one have dist < 400
	// This needs at least 2 CC. Otherwise, go back to economic CC.

	let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
	let ccList = [];
	let numAllyCC = 0;
	for (let cc of ccEnts.values())
	{
		let ally = gameState.isPlayerAlly(cc.owner());
		ccList.push({"pos": cc.position(), "ally": ally});
		if (ally)
			++numAllyCC;
	}
	if (numAllyCC < 2)
		return this.findEconomicCCLocation(gameState, template, "wood", undefined, true);

	Engine.ProfileStart("findStrategicCCLocation");

	// obstruction map
	let obstructions = m.createObstructionMap(gameState, 0, template);
	let halfSize = 0;
	if (template.get("Footprint/Square"))
		halfSize = Math.max(+template.get("Footprint/Square/@depth"), +template.get("Footprint/Square/@width")) / 2;
	else if (template.get("Footprint/Circle"))
		halfSize = +template.get("Footprint/Circle/@radius");

	let bestIdx;
	let bestVal;
	let radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);

	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;
	let currentVal, delta;
	let distcc0, distcc1, distcc2;

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.territoryMap.getOwnerIndex(j) !== 0)
			continue;
		// with enough room around to build the cc
		let i = this.territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;
		// we require that it is accessible
		let index = gameState.ai.accessibility.landPassMap[i];
		if (!this.landRegions[index])
			continue;

		// checking distances to other cc
		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		let minDist = Math.min();
		distcc0 = undefined;

		for (let cc of ccList)
		{
			let dist = API3.SquareVectorDistance(cc.pos, pos);
			if (dist < 14000)    // Reject if too near from any cc
			{
				minDist = 0;
				break;
			}
			if (!cc.ally)
				continue;
			if (dist < 62000)    // Reject if quite near from ally cc
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
		if (this.isDangerousLocation(gameState, pos, halfSize))
			continue;
		bestVal = currentVal;
		bestIdx = i;
	}

	if (this.Config.debug > 1)
		API3.warn("We've found a strategic base with bestVal = " + bestVal);

	Engine.ProfileStop();

	if (bestVal === undefined)
		return undefined;

	let x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	let z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;

	// Define a minimal number of wanted ships in the seas reaching this new base
	let indexIdx = gameState.ai.accessibility.landPassMap[bestIdx];
	for (let base of this.baseManagers)
	{
		if (!base.anchor || base.accessIndex === indexIdx)
			continue;
		let sea = this.getSeaIndex(gameState, base.accessIndex, indexIdx);
		if (sea !== undefined)
			this.navalManager.setMinimalTransportShips(gameState, sea, 1);
	}

	return [x, z];
};

/**
 * Returns the best position to build a new market: if the allies already have a market, build it as far as possible
 * from it, although not in our border to be able to defend it easily. If no allied market, our second market will
 * follow the same logic
 * TODO check that it is on same accessIndex
 */
m.HQ.prototype.findMarketLocation = function(gameState, template)
{
	let markets = gameState.updatingCollection("ExclusiveAllyMarkets", API3.Filters.byClass("Market"), gameState.getExclusiveAllyEntities()).toEntityArray();
	if (!markets.length)
		markets = gameState.updatingCollection("OwnMarkets", API3.Filters.byClass("Market"), gameState.getOwnStructures()).toEntityArray();

	if (!markets.length)	// this is the first market. For the time being, place it arbitrarily by the ConstructionPlan
		return [-1, -1, -1, 0];

	// obstruction map
	let obstructions = m.createObstructionMap(gameState, 0, template);
	let halfSize = 0;
	if (template.get("Footprint/Square"))
		halfSize = Math.max(+template.get("Footprint/Square/@depth"), +template.get("Footprint/Square/@width")) / 2;
	else if (template.get("Footprint/Circle"))
		halfSize = +template.get("Footprint/Circle/@radius");

	let bestIdx;
	let bestJdx;
	let bestVal;
	let radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);
	let isNavalMarket = template.hasClass("NavalMarket");

	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;

	let traderTemplatesGains = gameState.getTraderTemplatesGains();

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		// do not try on the border of our territory
		if (this.frontierMap.map[j] === 2)
			continue;
		if (this.basesMap.map[j] === 0)   // only in our territory
			continue;
		// with enough room around to build the cc
		let i = this.territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;
		let index = gameState.ai.accessibility.landPassMap[i];
		if (!this.landRegions[index])
			continue;
		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		// checking distances to other markets
		let maxVal = 0;
		let gainMultiplier;
		for (let market of markets)
		{
			if (isNavalMarket && market.hasClass("NavalMarket"))
			{
				if (this.navalManager.getDockIndex(gameState, market, true) !== gameState.ai.accessibility.getAccessValue(pos, true))
					continue;
				gainMultiplier = traderTemplatesGains.navalGainMultiplier;
			}
			else if (gameState.ai.accessibility.getAccessValue(market.position()) === index)
				gainMultiplier = traderTemplatesGains.landGainMultiplier;
			else
				continue;
			if (!gainMultiplier)
				continue;
			let val = API3.SquareVectorDistance(market.position(), pos) * gainMultiplier;
			if (val > maxVal)
				maxVal = val;
		}
		if (maxVal === 0)
			continue;
		if (bestVal !== undefined && maxVal < bestVal)
			continue;
		if (this.isDangerousLocation(gameState, pos, halfSize))
			continue;
		bestVal = maxVal;
		bestIdx = i;
		bestJdx = j;
	}

	if (this.Config.debug > 1)
		API3.warn("We found a market position with bestVal = " + bestVal);

	if (bestVal === undefined)  // no constraints. For the time being, place it arbitrarily by the ConstructionPlan
		return [-1, -1, -1, 0];
	let expectedGain = Math.round(bestVal / 10000);
	if (this.Config.debug > 1)
		API3.warn("this would give a trading gain of " + expectedGain);
	// do not keep it if gain is too small, except if this is our first BarterMarket
	if (expectedGain < this.tradeManager.minimalGain ||
		(expectedGain < 8 && (!template.hasClass("BarterMarket") || gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities())))
		return false;

	let x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	let z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;
	return [x, z, this.basesMap.map[bestJdx], expectedGain];
};

/**
 * Returns the best position to build defensive buildings (fortress and towers)
 * Whose primary function is to defend our borders
 */
m.HQ.prototype.findDefensiveLocation = function(gameState, template)
{
	// We take the point in our territory which is the nearest to any enemy cc
	// but requiring a minimal distance with our other defensive structures
	// and not in range of any enemy defensive structure to avoid building under fire.

	let ownStructures = gameState.getOwnStructures().filter(API3.Filters.byClassesOr(["Fortress", "Tower"])).toEntityArray();
	let enemyStructures = gameState.getEnemyStructures().filter(API3.Filters.not(API3.Filters.byOwner(0))).
		filter(API3.Filters.byClassesOr(["CivCentre", "Fortress", "Tower"]));
	if (!enemyStructures.hasEntities())	// we may be in cease fire mode, build defense against neutrals
	{
		enemyStructures = gameState.getNeutralStructures().filter(API3.Filters.not(API3.Filters.byOwner(0))).
			filter(API3.Filters.byClassesOr(["CivCentre", "Fortress", "Tower"]));
		if (!enemyStructures.hasEntities())
			return undefined;
	}
	enemyStructures = enemyStructures.toEntityArray();

	let wonderMode = gameState.getGameType() === "wonder";
	let wonderDistmin;
	let wonders;
	if (wonderMode)
	{
		wonders = gameState.getOwnStructures().filter(API3.Filters.byClass("Wonder")).toEntityArray();
		wonderMode = wonders.length !== 0;
		if (wonderMode)
			wonderDistmin = (50 + wonders[0].footprintRadius()) * (50 + wonders[0].footprintRadius());
	}

	// obstruction map
	let obstructions = m.createObstructionMap(gameState, 0, template);
	let halfSize = 0;
	if (template.get("Footprint/Square"))
		halfSize = Math.max(+template.get("Footprint/Square/@depth"), +template.get("Footprint/Square/@width")) / 2;
	else if (template.get("Footprint/Circle"))
		halfSize = +template.get("Footprint/Circle/@radius");

	let bestIdx;
	let bestJdx;
	let bestVal;
	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;

	let isTower = template.hasClass("Tower");
	let isFortress = template.hasClass("Fortress");
	let radius;
	if (isFortress)
		radius = Math.floor((template.obstructionRadius() + 8) / obstructions.cellSize);
	else
		radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (!wonderMode)
		{
			// do not try if well inside or outside territory
			if (this.frontierMap.map[j] === 0)
				continue;
			if (this.frontierMap.map[j] === 1 && isTower)
				continue;
		}
		if (this.basesMap.map[j] === 0)   // inaccessible cell
			continue;
		// with enough room around to build the cc
		let i = this.territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;

		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		// checking distances to other structures
		let minDist = Math.min();

		let dista = 0;
		if (wonderMode)
		{
			dista = API3.SquareVectorDistance(wonders[0].position(), pos);
			if (dista < wonderDistmin)
				continue;
			dista *= 200;   // empirical factor (TODO should depend on map size) to stay near the wonder
		}

		for (let str of enemyStructures)
		{
			if (str.foundationProgress() !== undefined)
				continue;
			let strPos = str.position();
			if (!strPos)
				continue;
			let dist = API3.SquareVectorDistance(strPos, pos);
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

		let cutDist = 900;  //  30*30   TODO maybe increase it
		for (let str of ownStructures)
		{
			let strPos = str.position();
			if (!strPos)
				continue;
			if (API3.SquareVectorDistance(strPos, pos) < cutDist)
			{
				minDist = -1;
				break;
			}
		}
		if (minDist < 0 || minDist === Math.min())
			continue;
		if (bestVal !== undefined && minDist > bestVal)
			continue;
		if (this.isDangerousLocation(gameState, pos, halfSize))
			continue;
		bestVal = minDist;
		bestIdx = i;
		bestJdx = j;
	}

	if (bestVal === undefined)
		return undefined;

	let x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	let z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;
	return [x, z, this.basesMap.map[bestJdx]];
};

m.HQ.prototype.buildTemple = function(gameState, queues)
{
	// at least one market (which have the same queue) should be build before any temple
	if (gameState.currentPhase() < 3 || queues.economicBuilding.hasQueuedUnits() ||
		gameState.getOwnEntitiesByClass("Temple", true).hasEntities() ||
		!gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities())
		return;
	if (!this.canBuild(gameState, "structures/{civ}_temple"))
		return;
	queues.economicBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_temple"));
};

m.HQ.prototype.buildMarket = function(gameState, queues)
{
	if (gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities() ||
		!this.canBuild(gameState, "structures/{civ}_market"))
		return;

	if (queues.economicBuilding.hasQueuedUnitsWithClass("BarterMarket"))
	{
		if (!this.navalMap && !queues.economicBuilding.paused)
		{
			// Put available resources in this market when not a naval map
			let queueManager = gameState.ai.queueManager;
			let cost = queues.economicBuilding.plans[0].getCost();
			queueManager.setAccounts(gameState, cost, "economicBuilding");
			if (!queueManager.canAfford("economicBuilding", cost))
			{
				for (let q in queueManager.queues)
				{
					if (q === "economicBuilding")
						continue;
					queueManager.transferAccounts(cost, q, "economicBuilding");
					if (queueManager.canAfford("economicBuilding", cost))
						break;
				}
			}
		}
		return;
	}
	if (gameState.getPopulation() < this.Config.Economy.popForMarket)
		return;

	gameState.ai.queueManager.changePriority("economicBuilding", 3*this.Config.priorities.economicBuilding);
	let plan = new m.ConstructionPlan(gameState, "structures/{civ}_market");
	plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("economicBuilding", gameState.ai.Config.priorities.economicBuilding); };
	queues.economicBuilding.addPlan(plan);
};

/** Build a farmstead */
m.HQ.prototype.buildFarmstead = function(gameState, queues)
{
	// Only build one farmstead for the time being ("DropsiteFood" does not refer to CCs)
	if (gameState.getOwnEntitiesByClass("Farmstead", true).hasEntities())
		return;
	// Wait to have at least one dropsite and house before the farmstead
	if (!gameState.getOwnEntitiesByClass("Storehouse", true).hasEntities())
		return;
	if (!gameState.getOwnEntitiesByClass("House", true).hasEntities())
		return;
	if (queues.economicBuilding.hasQueuedUnitsWithClass("DropsiteFood"))
		return;
	if (!this.canBuild(gameState, "structures/{civ}_farmstead"))
		return;

	queues.economicBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_farmstead"));
};

/** Build a corral, and train animals there */
m.HQ.prototype.manageCorral = function(gameState, queues)
{
	if (queues.corral.hasQueuedUnits())
		return;

	let nCorral = gameState.getOwnEntitiesByClass("Corral", true).length;
	if (nCorral === 0 ||
		(gameState.isTemplateDisabled(gameState.applyCiv("structures/{civ}_field")) &&
		 nCorral < gameState.currentPhase() && gameState.getPopulation() > 30*nCorral))
	{
		if (this.canBuild(gameState, "structures/{civ}_corral"))
		{
			queues.corral.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_corral"));
			return;
		}
		if (nCorral === 0) return;
	}

	// And train some animals
	for (let corral of gameState.getOwnEntitiesByClass("Corral", true).values())
	{
		if (corral.foundationProgress() !== undefined)
			continue;
		let trainables = corral.trainableEntities();
		for (let trainable of trainables)
		{
			if (gameState.isTemplateDisabled(trainable))
				continue;
			let template = gameState.getTemplate(trainable);
			if (!template || !template.isHuntable())
				continue;
			let count = gameState.countEntitiesByType(trainable, true);
			for (let item of corral.trainingQueue())
				count += item.count;
			if (count > nCorral)
				continue;
			queues.corral.addPlan(new m.TrainingPlan(gameState, trainable, { "trainer": corral.id() }));
			return;
		}
	}
};

/**
 * build more houses if needed.
 * kinda ugly, lots of special cases to both build enough houses but not tooo many…
 */
m.HQ.prototype.buildMoreHouses = function(gameState,queues)
{
	if (gameState.getPopulationMax() <= gameState.getPopulationLimit())
		return;

	let numPlanned = queues.house.length();
	if (numPlanned < 3 || (numPlanned < 5 && gameState.getPopulation() > 80))
	{
		let plan = new m.ConstructionPlan(gameState, "structures/{civ}_house");
		// change the starting condition according to the situation.
		plan.isGo = function (gameState) {
			if (!gameState.ai.HQ.canBuild(gameState, "structures/{civ}_house"))
				return false;
			if (gameState.getPopulationMax() <= gameState.getPopulationLimit())
				return false;
			let freeSlots = gameState.getPopulationLimit() - gameState.getPopulation();
			for (let ent of gameState.getOwnFoundations().values())
				freeSlots += ent.getPopulationBonus();

			if (gameState.ai.HQ.saveResources)
				return freeSlots <= 10;
			else if (gameState.getPopulation() > 55)
				return freeSlots <= 21;
			else if (gameState.getPopulation() > 30)
				return freeSlots <= 15;
			return freeSlots <= 10;
		};
		queues.house.addPlan(plan);
	}

	if (numPlanned > 0 && this.econState == "townPhasing" && gameState.getPhaseRequirements(2))
	{
		let requirements = gameState.getPhaseRequirements(2);
		let count = gameState.getOwnStructures().filter(API3.Filters.byClass(requirements["class"])).length;
		if (requirements && count < requirements.number && this.stopBuilding.has(gameState.applyCiv("structures/{civ}_house")))
		{
			if (this.Config.debug > 1)
				API3.warn("no room to place a house ... try to be less restrictive");
			this.stopBuilding.delete(gameState.applyCiv("structures/{civ}_house"));
			this.requireHouses = true;
		}
		let houseQueue = queues.house.plans;
		for (let i = 0; i < numPlanned; ++i)
		{
			if (houseQueue[i].isGo(gameState))
				++count;
			else if (count < requirements.number)
			{
				houseQueue[i].isGo = function () { return true; };
				++count;
			}
		}
	}

	if (this.requireHouses)
	{
		let requirements = gameState.getPhaseRequirements(2);
		if (gameState.getOwnStructures().filter(API3.Filters.byClass(requirements["class"])).length >= requirements.number)
			this.requireHouses = undefined;
	}

	// When population limit too tight
	//    - if no room to build, try to improve with technology
	//    - otherwise increase temporarily the priority of houses
	let house = gameState.applyCiv("structures/{civ}_house");
	let HouseNb = gameState.getOwnFoundations().filter(API3.Filters.byClass("House")).length;
	let popBonus = gameState.getTemplate(house).getPopulationBonus();
	let freeSlots = gameState.getPopulationLimit() + HouseNb*popBonus - gameState.getPopulation();
	let priority;
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
				priority = 2*this.Config.priorities.house;
			}
		}
		else
			priority = 2*this.Config.priorities.house;
	}
	else
		priority = this.Config.priorities.house;
	if (priority && priority != gameState.ai.queueManager.getPriority("house"))
		gameState.ai.queueManager.changePriority("house", priority);
};

/** checks the status of the territory expansion. If no new economic bases created, build some strategic ones. */
m.HQ.prototype.checkBaseExpansion = function(gameState, queues)
{
	if (queues.civilCentre.hasQueuedUnits())
		return;
	// first build one cc if all have been destroyed
	let activeBases = this.numActiveBase();
	if (activeBases === 0)
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
	// then expand if we have lots of units (threshold depending on the aggressivity value)
	let numUnits = gameState.getOwnUnits().length;
	let numvar = 10 * (1 - this.Config.personality.aggressive);
	if (numUnits > activeBases * (65 + numvar + (10 + numvar)*(activeBases-1)) || (this.saveResources && numUnits > 50))
	{
		if (this.Config.debug > 2)
			API3.warn("try to build a new base because of population " + numUnits + " for " + activeBases + " CCs");
		this.buildNewBase(gameState, queues);
	}
};

m.HQ.prototype.buildNewBase = function(gameState, queues, resource)
{
	if (this.numActiveBase() > 0 && gameState.currentPhase() == 1 && !gameState.isResearching(gameState.townPhase()))
		return false;
	if (gameState.getOwnFoundations().filter(API3.Filters.byClass("CivCentre")).hasEntities() || queues.civilCentre.hasQueuedUnits())
		return false;
	let template = this.numActiveBase() > 0 ? this.bBase[0] : gameState.applyCiv("structures/{civ}_civil_centre");
	if (!this.canBuild(gameState, template))
		return false;

	// base "-1" means new base.
	if (this.Config.debug > 1)
		API3.warn("new base planned with resource " + resource);
	queues.civilCentre.addPlan(new m.ConstructionPlan(gameState, template, { "base": -1, "resource": resource }));
	return true;
};

/** Deals with building fortresses and towers along our border with enemies. */
m.HQ.prototype.buildDefenses = function(gameState, queues)
{
	if (this.saveResources || queues.defenseBuilding.hasQueuedUnits())
		return;

	if (gameState.currentPhase() > 2 || gameState.isResearching(gameState.cityPhase()))
	{
		// try to build fortresses
		if (this.canBuild(gameState, "structures/{civ}_fortress"))
		{
			let numFortresses = gameState.getOwnEntitiesByClass("Fortress", true).length;
			if ((!numFortresses || gameState.ai.elapsedTime > (1 + 0.10*numFortresses)*this.fortressLapseTime + this.fortressStartTime) &&
				numFortresses < this.numActiveBase() + 1 + this.extraFortresses &&
				gameState.getOwnFoundationsByClass("Fortress").length < 2)
			{
				this.fortressStartTime = gameState.ai.elapsedTime;
				if (!numFortresses)
					gameState.ai.queueManager.changePriority("defenseBuilding", 2*this.Config.priorities.defenseBuilding);
				let plan = new m.ConstructionPlan(gameState, "structures/{civ}_fortress");
				plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("defenseBuilding", gameState.ai.Config.priorities.defenseBuilding); };
				queues.defenseBuilding.addPlan(plan);
				return;
			}
		}
	}

	if (this.Config.Military.numWoodenTowers && gameState.currentPhase() < 2 && this.canBuild(gameState, "structures/{civ}_wooden_tower"))
	{
		let numTowers = gameState.getOwnEntitiesByClass("Tower", true).length;	// we count all towers, including wall towers
		if (numTowers < this.Config.Military.numWoodenTowers && gameState.ai.elapsedTime > this.towerLapseTime + this.fortStartTime)
		{
			this.fortStartTime = gameState.ai.elapsedTime;
			queues.defenseBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_wooden_tower"));
		}
		return;
	}

	if (gameState.currentPhase() < 2 || !this.canBuild(gameState, "structures/{civ}_defense_tower"))
		return;

	let numTowers = gameState.getOwnEntitiesByClass("DefenseTower", true).filter(API3.Filters.byClass("Town")).length;
	if ((!numTowers || gameState.ai.elapsedTime > (1 + 0.10*numTowers)*this.towerLapseTime + this.towerStartTime) &&
		numTowers < 2 * this.numActiveBase() + 3 + this.extraTowers &&
		gameState.getOwnFoundationsByClass("DefenseTower").length < 3)
	{
		this.towerStartTime = gameState.ai.elapsedTime;
		queues.defenseBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_defense_tower"));
	}
};

m.HQ.prototype.buildBlacksmith = function(gameState, queues)
{
	if (gameState.getPopulation() < this.Config.Military.popForBlacksmith ||
		queues.militaryBuilding.hasQueuedUnits() || gameState.getOwnEntitiesByClass("Blacksmith", true).length)
		return;
	// build a market before the blacksmith
	if (!gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities())
		return;

	if (this.canBuild(gameState, "structures/{civ}_blacksmith"))
		queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_blacksmith"));
};

m.HQ.prototype.buildWonder = function(gameState, queues)
{
	if (queues.wonder && queues.wonder.hasQueuedUnits())
		return;
	if (gameState.getOwnEntitiesByClass("Wonder", true).hasEntities())
		return;
	if (!this.canBuild(gameState, "structures/{civ}_wonder"))
		return;

	if (!queues.wonder)
		gameState.ai.queueManager.addQueue("wonder", 1000);
	queues.wonder.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_wonder"));
};

/**
 * Deals with constructing military buildings (barracks, stables…)
 * They are mostly defined by Config.js. This is unreliable since changes could be done easily.
 */
m.HQ.prototype.constructTrainingBuildings = function(gameState, queues)
{
	if (!queues.militaryBuilding.hasQueuedUnits() && this.canBuild(gameState, "structures/{civ}_barracks"))
	{
		let barrackNb = gameState.getOwnEntitiesByClass("Barracks", true).length;
		// first barracks.
		if (!barrackNb && (gameState.getPopulation() > this.Config.Military.popForBarracks1 ||
			(this.econState == "townPhasing" && gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length < 5)))
		{
			gameState.ai.queueManager.changePriority("militaryBuilding", 2*this.Config.priorities.militaryBuilding);
			let preferredBase = this.findBestBaseForMilitary(gameState);
			let plan = new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase });
			plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("militaryBuilding", gameState.ai.Config.priorities.militaryBuilding); };
			queues.militaryBuilding.addPlan(plan);
		}
		// second barracks, then 3rd barrack, and optional 4th for some civs as they rely on barracks more.
		else if (barrackNb == 1 && gameState.getPopulation() > this.Config.Military.popForBarracks2)
		{
			let preferredBase = this.findBestBaseForMilitary(gameState);
			queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase }));
		}
		else if (barrackNb == 2 && gameState.getPopulation() > this.Config.Military.popForBarracks2 + 20)
		{
			let preferredBase = this.findBestBaseForMilitary(gameState);
			queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase }));
		}
		else if (barrackNb == 3 && gameState.getPopulation() > this.Config.Military.popForBarracks2 + 50 &&
			(gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber"))
		{
			let preferredBase = this.findBestBaseForMilitary(gameState);
			queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_barracks", { "preferredBase": preferredBase }));
		}
	}

	//build advanced military buildings
	if (gameState.currentPhase() > 2 && gameState.getPopulation() > 80 && !queues.militaryBuilding.hasQueuedUnits() && this.bAdvanced.length !== 0)
	{
		let nAdvanced = 0;
		for (let advanced of this.bAdvanced)
			nAdvanced += gameState.countEntitiesAndQueuedByType(advanced, true);

		if (!nAdvanced || (nAdvanced < this.bAdvanced.length && gameState.getPopulation() > 110))
		{
			for (let advanced of this.bAdvanced)
			{
				if (gameState.countEntitiesAndQueuedByType(advanced, true) > 0 || !this.canBuild(gameState, advanced))
					continue;
				let template = gameState.getTemplate(advanced);
				if (!template)
					continue;
				if (template.hasDefensiveFire() || template.trainableEntities())
				{
					let preferredBase = this.findBestBaseForMilitary(gameState);
					queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, advanced, { "preferredBase": preferredBase }));
				}
				else	// not a military building, but still use this queue
					queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, advanced));
				break;
			}
		}
	}
};

/**
 *  Construct military building in bases nearest to the ennemies  TODO revisit as the nearest one may not be accessible
 */
m.HQ.prototype.findBestBaseForMilitary = function(gameState)
{
	let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre")).toEntityArray();
	let bestBase = 1;
	let distMin = Math.min();
	for (let cce of ccEnts)
	{
		if (gameState.isPlayerAlly(cce.owner()))
			continue;
		for (let cc of ccEnts)
		{
			if (cc.owner() != PlayerID)
				continue;
			let dist = API3.SquareVectorDistance(cc.position(), cce.position());
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
	if (gameState.ai.queues.emergency.hasQueuedUnits())
		return false;

	let civ = gameState.civ();
	// find nearest base anchor
	let distcut = 20000;
	let nearestAnchor;
	let distmin;
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
			if (!base.anchor.trainableEntities(civ))	// base still in construction
				continue;
			let queue = base.anchor._entity.trainingQueue;
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
	let numGarrisoned = this.garrisonManager.numberOfGarrisonedUnits(nearestAnchor);
	if (nearestAnchor._entity.trainingQueue)
	{
		for (let item of nearestAnchor._entity.trainingQueue)
		{
			if (item.metadata && item.metadata.garrisonType)
				numGarrisoned += item.count;
			else if (!item.progress && (!item.metadata || !item.metadata.trainer))
				nearestAnchor.stopProduction(item.id);
		}
	}
	let autogarrison = numGarrisoned < nearestAnchor.garrisonMax() &&
	                   nearestAnchor.hitpoints() > nearestAnchor.garrisonEjectHealth() * nearestAnchor.maxHitpoints();
	let rangedWanted = Math.random() > 0.5 && autogarrison;

	let total = gameState.getResources();
	let templateFound;
	let trainables = nearestAnchor.trainableEntities(civ);
	let garrisonArrowClasses = nearestAnchor.getGarrisonArrowClasses();
	for (let trainable of trainables)
	{
		if (gameState.isTemplateDisabled(trainable))
			continue;
		let template = gameState.getTemplate(trainable);
		if (!template || !template.hasClass("Infantry") || !template.hasClass("CitizenSoldier"))
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
	if (!queueManager.canAfford("emergency", cost))
	{
		for (let q in queueManager.queues)
		{
			if (q === "emergency")
				continue;
			queueManager.transferAccounts(cost, q, "emergency");
			if (queueManager.canAfford("emergency", cost))
				break;
		}
	}
	let metadata = { "role": "worker", "base": nearestAnchor.getMetadata(PlayerID, "base"), "plan": -1, "trainer": nearestAnchor.id() };
	if (autogarrison)
		metadata.garrisonType = "protection";
	gameState.ai.queues.emergency.addPlan(new m.TrainingPlan(gameState, templateFound[0], metadata, 1, 1));
	return true;
};

m.HQ.prototype.canBuild = function(gameState, structure)
{
	let type = gameState.applyCiv(structure);
	// available room to build it
	if (this.stopBuilding.has(type))
	{
		if (this.stopBuilding.get(type) > gameState.ai.elapsedTime)
			return false;
		this.stopBuilding.delete(type);
	}

	if (gameState.isTemplateDisabled(type))
	{
		this.stopBuilding.set(type, Infinity);
		return false;
	}

	let template = gameState.getTemplate(type);
	if (!template)
	{
		this.stopBuilding.set(type, Infinity);
		if (this.Config.debug > 0)
			API3.warn("Petra error: trying to build " + structure + " for civ " + gameState.civ() + " but no template found.");
	}
	if (!template || !template.available(gameState))
		return false;

	if (!gameState.findBuilder(type))
	{
		this.stopBuild(gameState, type, 120);
		return false;
	}

	if (this.numActiveBase() < 1)
	{
		// if no base, check that we can build outside our territory
		let buildTerritories = template.buildTerritories();
		if (buildTerritories && (!buildTerritories.length || (buildTerritories.length === 1 && buildTerritories[0] === "own")))
		{
			this.stopBuilding.set(type, gameState.ai.elapsedTime + 180);
			return false;
		}
	}

	// build limits
	let limits = gameState.getEntityLimits();
	let category = template.buildCategory();
	if (category && limits[category] !== undefined && gameState.getEntityCounts()[category] >= limits[category])
		return false;

	return true;
};

m.HQ.prototype.stopBuild = function(gameState, structure, time=180)
{
	let type = gameState.applyCiv(structure);
	if (this.stopBuilding.has(type))
		this.stopBuilding.set(type, Math.max(this.stopBuilding.get(type), gameState.ai.elapsedTime + time));
	else
		this.stopBuilding.set(type, gameState.ai.elapsedTime + time);
};

m.HQ.prototype.restartBuild = function(gameState, structure)
{
	let type = gameState.applyCiv(structure);
	if (this.stopBuilding.has(type))
		this.stopBuilding.delete(type);
};

m.HQ.prototype.updateTerritories = function(gameState)
{
	let passabilityMap = gameState.getMap();
	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;
	let expansion = 0;
	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.borderMap.map[j] > 1)
			continue;
		if (this.territoryMap.getOwnerIndex(j) != PlayerID)
		{
			if (this.basesMap.map[j] === 0)
				continue;
			let base = this.getBaseByID(this.basesMap.map[j]);
			let index = base.territoryIndices.indexOf(j);
			if (index == -1)
			{
				API3.warn(" problem in headquarters::updateTerritories for base " + this.basesMap.map[j]);
				continue;
			}
			base.territoryIndices.splice(index, 1);
			this.basesMap.map[j] = 0;
		}
		else if (this.basesMap.map[j] === 0)
		{
			let landPassable = false;
			let ind = API3.getMapIndices(j, this.territoryMap, passabilityMap);
			let access;
			for (let k of ind)
			{
				if (!this.landRegions[gameState.ai.accessibility.landPassMap[k]])
					continue;
				landPassable = true;
				access = gameState.ai.accessibility.landPassMap[k];
				break;
			}
			if (!landPassable)
				continue;
			let distmin = Math.min();
			let baseID;
			let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
			for (let base of this.baseManagers)
			{
				if (!base.anchor || !base.anchor.position())
					continue;
				if (base.accessIndex != access)
					continue;
				let dist = API3.SquareVectorDistance(base.anchor.position(), pos);
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

	this.frontierMap = m.createFrontierMap(gameState);

	if (!expansion)
		return;
	// We've increased our territory, so we may have some new room to build
	this.stopBuilding.clear();
	// And if sufficient expansion, check if building a new market would improve our present trade routes
	let cellArea = this.territoryMap.cellSize * this.territoryMap.cellSize;
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
 */
m.HQ.prototype.numActiveBase = function()
{
	if (!this.turnCache.activeBase)
	{
		let num = 0;
		for (let base of this.baseManagers)
			if (base.anchor)
				++num;
		this.turnCache.activeBase = num;
	}
	return this.turnCache.activeBase;
};

m.HQ.prototype.resetActiveBase = function()
{
	this.turnCache.activeBase = undefined;
};

/**
 * Count gatherers returning resources in the number of gatherers of resourceSupplies
 * to prevent the AI always reaffecting idle workers to these resourceSupplies (specially in naval maps).
 */
m.HQ.prototype.assignGatherers = function()
{
	for (let base of this.baseManagers)
	{
		for (let worker of base.workers.values())
		{
			if (worker.unitAIState().split(".")[1] !== "RETURNRESOURCE")
				continue;
			let orders = worker.unitAIOrderData();
			if (orders.length < 2 || !orders[1].target || orders[1].target !== worker.getMetadata(PlayerID, "supply"))
				continue;
			this.AddTCGatherer(orders[1].target);
		}
	}
};

m.HQ.prototype.isDangerousLocation = function(gameState, pos, radius)
{
	return this.isNearInvadingArmy(pos) || this.isUnderEnemyFire(gameState, pos, radius);
};

/** Check that the chosen position is not too near from an invading army */
m.HQ.prototype.isNearInvadingArmy = function(pos)
{
	for (let army of this.defenseManager.armies)
		if (army.foePosition && API3.SquareVectorDistance(army.foePosition, pos) < 12000)
			return true;
	return false;
};

m.HQ.prototype.isUnderEnemyFire = function(gameState, pos, radius = 0)
{
	if (!this.turnCache.firingStructures)
		this.turnCache.firingStructures = gameState.updatingCollection("FiringStructures", API3.Filters.hasDefensiveFire(), gameState.getEnemyStructures());
	for (let ent of this.turnCache.firingStructures.values())
	{
		let range = radius + ent.attackRange("Ranged").max;
		if (API3.SquareVectorDistance(ent.position(), pos) < range*range)
			return true;
	}
	return false;
};


/** Some functions that register that we assigned a gatherer to a resource this turn */

/** add a gatherer to the turn cache for this supply. */
m.HQ.prototype.AddTCGatherer = function(supplyID)
{
	if (this.turnCache.resourceGatherer && this.turnCache.resourceGatherer[supplyID] !== undefined)
		++this.turnCache.resourceGatherer[supplyID];
	else
	{
		if (!this.turnCache.resourceGatherer)
			this.turnCache.resourceGatherer = {};
		this.turnCache.resourceGatherer[supplyID] = 1;
	}
};

/** remove a gatherer to the turn cache for this supply. */
m.HQ.prototype.RemoveTCGatherer = function(supplyID)
{
	if (this.turnCache.resourceGatherer && this.turnCache.resourceGatherer[supplyID])
		--this.turnCache.resourceGatherer[supplyID];
	else
	{
		if (!this.turnCache.resourceGatherer)
			this.turnCache.resourceGatherer = {};
		this.turnCache.resourceGatherer[supplyID] = -1;
	}
};

m.HQ.prototype.GetTCGatherer = function(supplyID)
{
	if (this.turnCache.resourceGatherer && this.turnCache.resourceGatherer[supplyID])
		return this.turnCache.resourceGatherer[supplyID];

	return 0;
};

/** The next two are to register that we assigned a gatherer to a resource this turn. */
m.HQ.prototype.AddTCResGatherer = function(resource)
{
	if (this.turnCache["resourceGatherer-" + resource])
		++this.turnCache["resourceGatherer-" + resource];
	else
		this.turnCache["resourceGatherer-" + resource] = 1;
	this.turnCache.gatherRates = false;
};

m.HQ.prototype.GetTCResGatherer = function(resource)
{
	if (this.turnCache["resourceGatherer-" + resource])
		return this.turnCache["resourceGatherer-" + resource];

	return 0;
};

/**
 * Some functions are run every turn
 * Others once in a while
 */
m.HQ.prototype.update = function(gameState, queues, events)
{
	Engine.ProfileStart("Headquarters update");
	this.turnCache = {};
	this.territoryMap = m.createTerritoryMap(gameState);

	if (this.Config.debug > 1)
	{
		gameState.getOwnUnits().forEach (function (ent) {
			if (!ent.position())
				return;
			m.dumpEntity(ent);
		});
	}

	this.checkEvents(gameState, events, queues);

	this.researchManager.checkPhase(gameState, queues);

	// TODO find a better way to update
	if (this.currentPhase != gameState.currentPhase())
	{
		this.currentPhase = gameState.currentPhase();
		let phaseName = "Unknown Phase";
		if (this.currentPhase == 2)
			phaseName = gameState.getTemplate(gameState.townPhase()).name();
		else if (this.currentPhase == 3)
			phaseName = gameState.getTemplate(gameState.cityPhase()).name();

		m.chatNewPhase(gameState, phaseName, false);
	}

	if (gameState.getGameType() === "wonder")
		this.buildWonder(gameState, queues);

	if (this.numActiveBase() > 0)
	{
		this.trainMoreWorkers(gameState, queues);

		if (gameState.ai.playedTurn % 2 == 1)
			this.buildMoreHouses(gameState,queues);

		if (!this.saveResources && gameState.ai.playedTurn % 4 == 2)
			this.buildFarmstead(gameState, queues);

		if (this.needCorral && gameState.ai.playedTurn % 4 == 3)
			this.manageCorral(gameState, queues);

		if (!queues.minorTech.hasQueuedUnits() && gameState.ai.playedTurn % 5 == 1)
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

	this.assignGatherers();
	for (let i = 0; i < this.baseManagers.length; ++i)
	{
		this.baseManagers[i].checkEvents(gameState, events, queues);
		if ((i + gameState.ai.playedTurn)%this.baseManagers.length === 0)
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
		"currentPhase": this.currentPhase,
		"wantedRates": this.wantedRates,
		"currentRates": this.currentRates,
		"lastFailedGather": this.lastFailedGather,
		"supportRatio": this.supportRatio,
		"targetNumWorkers": this.targetNumWorkers,
		"stopBuilding": this.stopBuilding,
		"fortStartTime": this.fortStartTime,
		"towerStartTime": this.towerStartTime,
		"fortressStartTime": this.fortressStartTime,
		"bBase": this.bBase,
		"bAdvanced": this.bAdvanced,
		"saveResources": this.saveResources,
		"saveSpace": this.saveSpace,
		"needCorral": this.needCorral,
		"needFarm": this.needFarm,
		"needFish": this.needFish,
		"canBuildUnits": this.canBuildUnits,
		"navalMap": this.navalMap,
		"landRegions": this.landRegions,
		"navalRegions": this.navalRegions,
		"decayingStructures": this.decayingStructures
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
		let newbase = new m.BaseManager(gameState, this.Config);
		newbase.Deserialize(gameState, base);
		newbase.init(gameState);
		newbase.Deserialize(gameState, base);
		this.baseManagers.push(newbase);
	}

	this.navalManager = new m.NavalManager(this.Config);
	this.navalManager.init(gameState, true);
	this.navalManager.Deserialize(gameState, data.navalManager);

	this.attackManager = new m.AttackManager(this.Config);
	this.attackManager.Deserialize(gameState, data.attackManager);
	this.attackManager.init(gameState);
	this.attackManager.Deserialize(gameState, data.attackManager);

	this.defenseManager = new m.DefenseManager(this.Config);
	this.defenseManager.Deserialize(gameState, data.defenseManager);

	this.tradeManager = new m.TradeManager(this.Config);
	this.tradeManager.init(gameState);
	this.tradeManager.Deserialize(gameState, data.tradeManager);

	this.researchManager = new m.ResearchManager(this.Config);
	this.researchManager.Deserialize(data.researchManager);

	this.diplomacyManager = new m.DiplomacyManager(this.Config);
	this.diplomacyManager.Deserialize(data.diplomacyManager);

	this.garrisonManager = new m.GarrisonManager();
	this.garrisonManager.Deserialize(data.garrisonManager);
};

return m;

}(PETRA);
