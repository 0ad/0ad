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
	this.phasing = 0;	// existing values: 0 means no, i > 0 means phasing towards phase i

	// Cache various quantities.
	this.turnCache = {};
	this.lastFailedGather = {};

	this.firstBaseConfig = false;
	this.currentBase = 0;	// Only one base (from baseManager) is run every turn.

	// Workers configuration
	this.targetNumWorkers = this.Config.Economy.targetNumWorkers;
	this.supportRatio = this.Config.Economy.supportRatio;

	this.fortStartTime = 180;	// sentry defense towers, will start at fortStartTime + towerLapseTime
	this.towerStartTime = 0;	// stone defense towers, will start as soon as available
	this.towerLapseTime = this.Config.Military.towerLapseTime;
	this.fortressStartTime = 0;	// will start as soon as available
	this.fortressLapseTime = this.Config.Military.fortressLapseTime;
	this.extraTowers = Math.round(Math.min(this.Config.difficulty, 3) * this.Config.personality.defensive);
	this.extraFortresses = Math.round(Math.max(Math.min(this.Config.difficulty - 1, 2), 0) * this.Config.personality.defensive);

	this.baseManagers = [];
	this.attackManager = new m.AttackManager(this.Config);
	this.buildManager = new m.BuildManager();
	this.defenseManager = new m.DefenseManager(this.Config);
	this.tradeManager = new m.TradeManager(this.Config);
	this.navalManager = new m.NavalManager(this.Config);
	this.researchManager = new m.ResearchManager(this.Config);
	this.diplomacyManager = new m.DiplomacyManager(this.Config);
	this.garrisonManager = new m.GarrisonManager(this.Config);
	this.victoryManager = new m.VictoryManager(this.Config);

	this.capturableTargets = new Map();
	this.capturableTargetsTime = 0;
};

/** More initialisation for stuff that needs the gameState */
m.HQ.prototype.init = function(gameState, queues)
{
	this.territoryMap = m.createTerritoryMap(gameState);
	// initialize base map. Each pixel is a base ID, or 0 if not or not accessible
	this.basesMap = new API3.Map(gameState.sharedScript, "territory");
	// create borderMap: flag cells on the border of the map
	// then this map will be completed with our frontier in updateTerritories
	this.borderMap = m.createBorderMap(gameState);
	// list of allowed regions
	this.landRegions = {};
	// try to determine if we have a water map
	this.navalMap = false;
	this.navalRegions = {};

	this.treasures = gameState.getEntities().filter(ent => {
		let type = ent.resourceSupplyType();
		return type && type.generic == "treasure";
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
		if (!ent.resourceDropsiteTypes() || !ent.hasClass("Structure"))
			continue;
		// Entities which have been built or have changed ownership after the last AI turn have no base.
		// they will be dealt with in the next checkEvents
		let baseID = ent.getMetadata(PlayerID, "base");
		if (baseID === undefined)
			continue;
		let base = this.getBaseByID(baseID);
		base.assignResourceToDropsite(gameState, ent);
	}

	this.updateTerritories(gameState);
};

/**
 * Create a new base in the baseManager:
 * If an existing one without anchor already exist, use it.
 * Otherwise create a new one.
 * TODO when buildings, criteria should depend on distance
 * allowedType: undefined       => new base with an anchor
 *              "unconstructed" => new base with a foundation anchor
 *              "captured"      => captured base with an anchor
 *              "anchorless"    => anchorless base, currently with dock
 */
m.HQ.prototype.createBase = function(gameState, ent, type)
{
	let access = m.getLandAccess(gameState, ent);
	let newbase;
	for (let base of this.baseManagers)
	{
		if (base.accessIndex != access)
			continue;
		if (type != "anchorless" && base.anchor)
			continue;
		if (type != "anchorless")
		{
			// TODO we keep the fisrt one, we should rather use the nearest if buildings
			// and possibly also cut on distance
			newbase = base;
			break;
		}
		else
		{
			// TODO here also test on distance instead of first
			if (newbase && !base.anchor)
				continue;
			newbase = base;
			if (newbase.anchor)
				break;
		}
	}

	if (this.Config.debug > 0)
	{
		API3.warn(" ----------------------------------------------------------");
		API3.warn(" HQ createBase entrance avec access " + access + " and type " + type);
		API3.warn(" with access " + uneval(this.baseManagers.map(base => base.accessIndex)) +
			  " and base nbr " + uneval(this.baseManagers.map(base => base.ID)) +
			  " and anchor " + uneval(this.baseManagers.map(base => !!base.anchor)));
	}

	if (!newbase)
	{
		newbase = new m.BaseManager(gameState, this.Config);
		newbase.init(gameState, type);
		this.baseManagers.push(newbase);
	}
	else
		newbase.reset(type);

	if (type != "anchorless")
		newbase.setAnchor(gameState, ent);
	else
		newbase.setAnchorlessEntity(gameState, ent);

	return newbase;
};

/**
 * returns the sea index linking regions 1 and region 2 (supposed to be different land region)
 * otherwise return undefined
 * for the moment, only the case land-sea-land is supported
 */
m.HQ.prototype.getSeaBetweenIndices = function(gameState, index1, index2)
{
	let path = gameState.ai.accessibility.getTrajectToIndex(index1, index2);
	if (path && path.length == 3 && gameState.ai.accessibility.regionType[path[1]] == "water")
		return path[1];

	if (this.Config.debug > 1)
	{
		API3.warn("bad path from " + index1 + " to " + index2 + " ??? " + uneval(path));
		API3.warn(" regionLinks start " + uneval(gameState.ai.accessibility.regionLinks[index1]));
		API3.warn(" regionLinks end   " + uneval(gameState.ai.accessibility.regionLinks[index2]));
	}
	return undefined;
};

/** TODO check if the new anchorless bases should be added to addBase */
m.HQ.prototype.checkEvents = function(gameState, events)
{
	let addBase = false;

	this.buildManager.checkEvents(gameState, events);

	if (events.TerritoriesChanged.length || events.DiplomacyChanged.length)
		this.updateTerritories(gameState);

	for (let evt of events.DiplomacyChanged)
	{
		if (evt.player != PlayerID && evt.otherPlayer != PlayerID)
			continue;
		// Reset the entities collections which depend on diplomacy
		gameState.resetOnDiplomacyChanged();
		break;
	}

	for (let evt of events.Destroy)
	{
		// Let's check we haven't lost an important building here.
		if (evt && !evt.SuccessfulFoundation && evt.entityObj && evt.metadata && evt.metadata[PlayerID] &&
			evt.metadata[PlayerID].base)
		{
			let ent = evt.entityObj;
			if (ent.owner() != PlayerID)
				continue;
			// A new base foundation was created and destroyed on the same (AI) turn
			if (evt.metadata[PlayerID].base == -1 || evt.metadata[PlayerID].base == -2)
				continue;
			let base = this.getBaseByID(evt.metadata[PlayerID].base);
			if (ent.resourceDropsiteTypes() && ent.hasClass("Structure"))
				base.removeDropsite(gameState, ent);
			if (evt.metadata[PlayerID].baseAnchor && evt.metadata[PlayerID].baseAnchor === true)
				base.anchorLost(gameState, ent);
		}
	}

	for (let evt of events.EntityRenamed)
	{
		let ent = gameState.getEntityById(evt.newentity);
		if (!ent || ent.owner() != PlayerID || ent.getMetadata(PlayerID, "base") === undefined)
			continue;
		let base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
		if (!base.anchorId || base.anchorId != evt.entity)
			continue;
		base.anchorId = evt.newentity;
		base.anchor = ent;
	}

	for (let evt of events.Create)
	{
		// Let's check if we have a valuable foundation needing builders quickly
		// (normal foundations are taken care in baseManager.assignToFoundations)
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || ent.owner() != PlayerID || ent.foundationProgress() === undefined)
			continue;

		if (ent.getMetadata(PlayerID, "base") == -1)	// Standard base around a cc
		{
			// Okay so let's try to create a new base around this.
			let newbase = this.createBase(gameState, ent, "unconstructed");
			// Let's get a few units from other bases there to build this.
			let builders = this.bulkPickWorkers(gameState, newbase, 10);
			if (builders !== false)
			{
				builders.forEach(worker => {
					worker.setMetadata(PlayerID, "base", newbase.ID);
					worker.setMetadata(PlayerID, "subrole", "builder");
					worker.setMetadata(PlayerID, "target-foundation", ent.id());
				});
			}
		}
		else if (ent.getMetadata(PlayerID, "base") == -2)	// anchorless base around a dock
		{
			let newbase = this.createBase(gameState, ent, "anchorless");
			// Let's get a few units from other bases there to build this.
			let builders = this.bulkPickWorkers(gameState, newbase, 4);
			if (builders != false)
			{
				builders.forEach(worker => {
					worker.setMetadata(PlayerID, "base", newbase.ID);
					worker.setMetadata(PlayerID, "subrole", "builder");
					worker.setMetadata(PlayerID, "target-foundation", ent.id());
				});
			}
		}
	}

	for (let evt of events.ConstructionFinished)
	{
		if (evt.newentity == evt.entity)  // repaired building
			continue;
		let ent = gameState.getEntityById(evt.newentity);
		if (!ent || ent.owner() != PlayerID)
			continue;
		if (ent.hasClass("BarterMarket") && this.maxFields)
			this.maxFields = false;
		if (ent.getMetadata(PlayerID, "base") === undefined)
			continue;
		let base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
		base.buildings.updateEnt(ent);
		if (ent.resourceDropsiteTypes())
			base.assignResourceToDropsite(gameState, ent);

		if (ent.getMetadata(PlayerID, "baseAnchor") === true)
		{
			if (base.constructing)
				base.constructing = false;
			addBase = true;
		}
	}

	for (let evt of events.OwnershipChanged)   // capture events
	{
		if (evt.from == PlayerID)
		{
			let ent = gameState.getEntityById(evt.entity);
			if (!ent || ent.getMetadata(PlayerID, "base") === undefined)
				continue;
			let base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
			if (ent.resourceDropsiteTypes() && ent.hasClass("Structure"))
				base.removeDropsite(gameState, ent);
			if (ent.getMetadata(PlayerID, "baseAnchor") === true)
				base.anchorLost(gameState, ent);
		}

		if (evt.to != PlayerID)
			continue;
		let ent = gameState.getEntityById(evt.entity);
		if (!ent)
			continue;
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
				m.setSeaAccess(gameState, ent);
			if (!ent.hasClass("Support") && !ent.hasClass("Ship") && ent.attackTypes() !== undefined)
				ent.setMetadata(PlayerID, "plan", -1);
			continue;
		}
		if (ent.hasClass("CivCentre"))   // build a new base around it
		{
			let newbase;
			if (ent.foundationProgress() !== undefined)
				newbase = this.createBase(gameState, ent, "unconstructed");
			else
			{
				newbase = this.createBase(gameState, ent, "captured");
				addBase = true;
			}
			newbase.assignEntity(gameState, ent);
		}
		else
		{
			let base;
			// If dropsite on new island, create a base around it
			if (!ent.decaying() && ent.resourceDropsiteTypes())
				base = this.createBase(gameState, ent, "anchorless");
			else
				base = m.getBestBase(gameState, ent) || this.baseManagers[0];
			base.assignEntity(gameState, ent);
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
				if (!attack || attack.state != "unexecuted")
					ent.setMetadata(PlayerID, "plan", -1);
			}
			// Assign it immediately to something useful to do
			if (ent.getMetadata(PlayerID, "role") == "worker")
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
				let access = m.getLandAccess(gameState, ent);
				let distmin = Math.min();
				let goal;
				for (let dropsite of dropsites.values())
				{
					if (!dropsite.position() || m.getLandAccess(gameState, dropsite) != access)
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

	if (addBase)
	{
		if (!this.firstBaseConfig)
		{
			// This is our first base, let us configure our starting resources
			this.configFirstBase(gameState);
		}
		else
		{
			// Let us hope this new base will fix our possible resource shortage
			this.saveResources = undefined;
			this.saveSpace = undefined;
			this.maxFields = false;
		}
	}

	// Then deals with decaying structures: destroy them if being lost to enemy (except in easier difficulties)
	if (this.Config.difficulty < 2)
		return;
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
			let ratioMax = 0.70 + randFloat(0., 0.1);
			for (let evt of events.Attacked)
			{
				if (ent.id() != evt.target)
					continue;
				ratioMax = 0.85 + randFloat(0., 0.1);
				break;
			}
			if (captureRatio > ratioMax)
				continue;
			ent.destroy();
		}
		this.decayingStructures.delete(entId);
	}
};

/** Ensure that all requirements are met when phasing up*/
m.HQ.prototype.checkPhaseRequirements = function(gameState, queues)
{
	if (gameState.getNumberOfPhases() == this.currentPhase)
		return;

	let requirements = gameState.getPhaseEntityRequirements(this.currentPhase + 1);
	let plan;
	let queue;
	for (let entityReq of requirements)
	{
		// Village requirements are met elsewhere by constructing more houses
		if (entityReq.class == "Village" || entityReq.class == "NotField")
			continue;
		if (gameState.getOwnEntitiesByClass(entityReq.class, true).length >= entityReq.count)
			continue;
		switch (entityReq.class)
		{
		case "Town":
			if (!queues.economicBuilding.hasQueuedUnits() &&
			    !queues.militaryBuilding.hasQueuedUnits() &&
			    !queues.defenseBuilding.hasQueuedUnits())
			{
				if (!gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities() &&
				    this.canBuild(gameState, "structures/{civ}_market"))
				{
					plan = new m.ConstructionPlan(gameState, "structures/{civ}_market", { "phaseUp": true });
					queue = "economicBuilding";
					break;
				}
				if (!gameState.getOwnEntitiesByClass("Temple", true).hasEntities() &&
				    this.canBuild(gameState, "structures/{civ}_temple"))
				{
					plan = new m.ConstructionPlan(gameState, "structures/{civ}_temple", { "phaseUp": true });
					queue = "economicBuilding";
					break;
				}
				if (!gameState.getOwnEntitiesByClass("Blacksmith", true).hasEntities() &&
				    this.canBuild(gameState, "structures/{civ}_blacksmith"))
				{
					plan = new m.ConstructionPlan(gameState, "structures/{civ}_blacksmith", { "phaseUp": true });
					queue = "militaryBuilding";
					break;
				}
				if (this.canBuild(gameState, "structures/{civ}_defense_tower"))
				{
					plan = new m.ConstructionPlan(gameState, "structures/{civ}_defense_tower", { "phaseUp": true });
					queue = "defenseBuilding";
					break;
				}
			}
			break;
		default:
			// All classes not dealt with inside vanilla game.
			// We put them for the time being on the economic queue, except if wonder
			queue = entityReq.class == "Wonder" ? "wonder" : "economicBuilding";
			if (!queues[queue].hasQueuedUnits())
			{
				let structure = this.buildManager.findStructureWithClass(gameState, [entityReq.class]);
				if (structure && this.canBuild(gameState, structure))
					plan = new m.ConstructionPlan(gameState, structure, { "phaseUp": true });
			}
		}

		if (plan)
		{
			if (queue == "wonder")
			{
				gameState.ai.queueManager.changePriority("majorTech", 400, { "phaseUp": true });
				plan.queueToReset = "majorTech";
			}
			else
			{
				gameState.ai.queueManager.changePriority(queue, 1000, { "phaseUp": true });
				plan.queueToReset = queue;
			}
			queues[queue].addPlan(plan);
			return;
		}
	}
};

/** Called by any "phase" research plan once it's started */
m.HQ.prototype.OnPhaseUp = function(gameState, phase)
{
};

/** This code trains citizen workers, trying to keep close to a ratio of worker/soldiers */
m.HQ.prototype.trainMoreWorkers = function(gameState, queues)
{
	// default template
	let requirementsDef = [ ["costsResource", 1, "food"] ];
	let classesDef = ["Support", "Worker"];
	let templateDef = this.findBestTrainableUnit(gameState, classesDef, requirementsDef);

	// counting the workers that aren't part of a plan
	let numberOfWorkers = 0;   // all workers
	let numberOfSupports = 0;  // only support workers (i.e. non fighting)
	gameState.getOwnUnits().forEach(ent => {
		if (ent.getMetadata(PlayerID, "role") == "worker" && ent.getMetadata(PlayerID, "plan") === undefined)
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
			if (item.metadata && item.metadata.role && item.metadata.role == "worker" &&
			    item.metadata.plan === undefined)
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

	if (this.saveResources && numberTotal > this.Config.Economy.popPhase2 + 10)
		return;
	if (numberTotal > this.targetNumWorkers || (numberTotal >= this.Config.Economy.popPhase2 &&
		this.currentPhase == 1 && !gameState.isResearching(gameState.getPhaseName(2))))
		return;
	if (numberQueued > 50 || (numberOfQueuedSupports > 20 && numberOfQueuedSoldiers > 20) || numberInTraining > 15)
		return;

	// Choose whether we want soldiers or support units: when full pop, we aim at targetNumWorkers workers
	// with supportRatio fraction of support units. But we want to have more support (less cost) at startup.
	// So we take: supportRatio*targetNumWorkers*(1 - exp(-alfa*currentWorkers/supportRatio/targetNumWorkers))
	// This gives back supportRatio*targetNumWorkers when currentWorkers >> supportRatio*targetNumWorkers
	// and gives a ratio alfa at startup.

	let supportRatio = this.supportRatio;
	let alpha = 0.85;
	if (!gameState.isTemplateAvailable(gameState.applyCiv("structures/{civ}_field")))
		supportRatio = Math.min(this.supportRatio, 0.1);
	if (this.attackManager.rushNumber < this.attackManager.maxRushes || this.attackManager.upcomingAttacks.Rush.length)
		alpha = 0.7;
	if (gameState.isCeasefireActive())
		alpha += (1 - alpha) * Math.min(Math.max(gameState.ceasefireTimeRemaining - 120, 0), 180) / 180;
	let supportMax = supportRatio * this.targetNumWorkers;
	let supportNum = supportMax * (1 - Math.exp(-alpha*numberTotal/supportMax));

	let template;
	if (!templateDef || numberOfSupports + numberOfQueuedSupports > supportNum)
	{
		let requirements;
		if (numberTotal < 45)
			requirements = [ ["speed", 0.5], ["costsResource", 0.5, "stone"], ["costsResource", 0.5, "metal"] ];
		else
			requirements = [ ["strength", 1] ];

		let classes = ["CitizenSoldier", "Infantry"];
		//  We want at least 33% ranged and 33% melee
		classes.push(pickRandom(["Ranged", "Melee", "Infantry"]));

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
	if (classes.indexOf("Hero") != -1)
		units = gameState.findTrainableUnits(classes, []);
	else if (classes.indexOf("Siege") != -1)	// We do not want siege tower as AI does not know how to use it
		units = gameState.findTrainableUnits(classes, ["SiegeTower"]);
	else						// We do not want hero when not explicitely specified
		units = gameState.findTrainableUnits(classes, ["Hero"]);

	if (!units.length)
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
			if (param[0] != "costsResource" || param[2] != type)
				continue;
			param[1] = Math.min(param[1], costsResource);
			toAdd = false;
			break;
		}
		if (toAdd)
			parameters.push(["costsResource", costsResource, type]);
	}

	units.sort((a, b) => {
		let aCost = 1 + a[1].costSum();
		let bCost = 1 + b[1].costSum();
		let aValue = 0.1;
		let bValue = 0.1;
		for (let param of parameters)
		{
			if (param[0] == "strength")
			{
				aValue += m.getMaxStrength(a[1]) * param[1];
				bValue += m.getMaxStrength(b[1]) * param[1];
			}
			else if (param[0] == "siegeStrength")
			{
				aValue += m.getMaxStrength(a[1], "Structure") * param[1];
				bValue += m.getMaxStrength(b[1], "Structure") * param[1];
			}
			else if (param[0] == "speed")
			{
				aValue += a[1].walkSpeed() * param[1];
				bValue += b[1].walkSpeed() * param[1];
			}
			else if (param[0] == "costsResource")
			{
				// requires a third parameter which is the resource
				if (a[1].cost()[param[2]])
					aValue *= param[1];
				if (b[1].cost()[param[2]])
					bValue *= param[1];
			}
			else if (param[0] == "canGather")
			{
				// checking against wood, could be anything else really.
				if (a[1].resourceGatherRates() && a[1].resourceGatherRates()["wood.tree"])
					aValue *= param[1];
				if (b[1].resourceGatherRates() && b[1].resourceGatherRates()["wood.tree"])
					bValue *= param[1];
			}
			else
				API3.warn(" trainMoreUnits avec non prevu " + uneval(param));
		}
		return -aValue/aCost + bValue/bCost;
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
	let baseBest = this.baseManagers.slice().sort((a, b) => {
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
		if (base.ID == baseRef.ID)
			continue;
		base.pickBuilders(gameState, workers, needed);
		if (workers.length >= number)
			break;
		needed = number - workers.length;
	}
	if (!workers.length)
		return false;
	return workers;
};

m.HQ.prototype.getTotalResourceLevel = function(gameState)
{
	let total = {};
	for (let res of Resources.GetCodes())
		total[res] = 0;
	for (let base of this.baseManagers)
		for (let res in total)
			total[res] += base.getResourceLevel(gameState, res);

	return total;
};

/**
 * Returns the current gather rate
 * This is not per-se exact, it performs a few adjustments ad-hoc to account for travel distance, stuffs like that.
 */
m.HQ.prototype.GetCurrentGatherRates = function(gameState)
{
	if (!this.turnCache.currentRates)
	{
		let currentRates = {};
		for (let res of Resources.GetCodes())
			currentRates[res] = 0.5 * this.GetTCResGatherer(res);

		for (let base of this.baseManagers)
			base.addGatherRates(gameState, currentRates);

		for (let res of Resources.GetCodes())
			currentRates[res] = Math.max(currentRates[res], 0);

		this.turnCache.currentRates = currentRates;
	}

	return this.turnCache.currentRates;
};

/**
 * Returns the wanted gather rate.
 */
m.HQ.prototype.GetWantedGatherRates = function(gameState)
{
	if (!this.turnCache.wantedRates)
		this.turnCache.wantedRates = gameState.ai.queueManager.wantedGatherRates(gameState);

	return this.turnCache.wantedRates;
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
	let wantedRates = this.GetWantedGatherRates(gameState);
	let currentRates = this.GetCurrentGatherRates(gameState);

	let needed = [];
	for (let res in wantedRates)
		needed.push({ "type": res, "wanted": wantedRates[res], "current": currentRates[res] });

	needed.sort((a, b) => {
		if (a.current < a.wanted && b.current < b.wanted)
		{
			if (a.current && b.current)
				return b.wanted / b.current - a.wanted / a.current;
			if (a.current)
				return 1;
			if (b.current)
				return -1;
			return b.wanted - a.wanted;
		}
		if (a.current < a.wanted || a.wanted && !b.wanted)
			return -1;
		if (b.current < b.wanted || b.wanted && !a.wanted)
			return 1;
		return a.current - a.wanted - b.current + b.wanted;
	});
	return needed;
};

/**
 * Returns the best position to build a new Civil Center
 * Whose primary function would be to reach new resources of type "resource".
 */
m.HQ.prototype.findEconomicCCLocation = function(gameState, template, resource, proximity, fromStrategic)
{
	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then look for a good spot.

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
		ccList.push({ "ent": cc, "pos": cc.position(), "ally": gameState.isPlayerAlly(cc.owner()) });
	let dpList = [];
	for (let dp of dpEnts.values())
		dpList.push({ "ent": dp, "pos": dp.position(), "territory": this.territoryMap.getOwner(dp.position()) });

	let bestIdx;
	let bestVal;
	let radius = Math.ceil(template.obstructionRadius().max / obstructions.cellSize);
	let scale = 250 * 250;
	let proxyAccess;
	let nbShips = this.navalManager.transportShips.length;
	if (proximity)	// this is our first base
	{
		// if our first base, ensure room around
		radius = Math.ceil((template.obstructionRadius().max + 8) / obstructions.cellSize);
		// scale is the typical scale at which we want to find a location for our first base
		// look for bigger scale if we start from a ship (access < 2) or from a small island
		let cellArea = gameState.getPassabilityMap().cellSize * gameState.getPassabilityMap().cellSize;
		proxyAccess = gameState.ai.accessibility.getAccessValue(proximity);
		if (proxyAccess < 2 || cellArea*gameState.ai.accessibility.regionSize[proxyAccess] < 24000)
			scale = 400 * 400;
	}

	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;

	// DistanceSquare cuts to other ccs (bigger or no cuts on inaccessible ccs to allow colonizing other islands).
	let reduce = (template.hasClass("Colony") ? 30 : 0) + 30 * this.Config.personality.defensive;
	let nearbyRejected = Math.square(120);			// Reject if too near from any cc
	let nearbyAllyRejected = Math.square(200);		// Reject if too near from an allied cc
	let nearbyAllyDisfavored = Math.square(250);		// Disfavor if quite near an allied cc
	let maxAccessRejected = Math.square(410);		// Reject if too far from an accessible ally cc
	let maxAccessDisfavored = Math.square(360 - reduce);	// Disfavor if quite far from an accessible ally cc
	let maxNoAccessDisfavored = Math.square(500);		// Disfavor if quite far from an inaccessible ally cc

	let cut = 60;
	if (fromStrategic || proximity)  // be less restrictive
		cut = 30;

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.territoryMap.getOwnerIndex(j) != 0)
			continue;
		// With enough room around to build the cc
		let i = this.territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;
		// We require that it is accessible
		let index = gameState.ai.accessibility.landPassMap[i];
		if (!this.landRegions[index])
			continue;
		if (proxyAccess && nbShips == 0 && proxyAccess != index)
			continue;

		let norm = 0.5;   // TODO adjust it, knowing that we will sum 5 maps
		// Checking distance to other cc
		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		// We will be more tolerant for cc around our oversea docks
		let oversea = false;

		if (proximity)	// This is our first cc, let's do it near our units
			norm /= 1 + API3.SquareVectorDistance(proximity, pos) / scale;
		else
		{
			let minDist = Math.min();
			let accessible = false;

			for (let cc of ccList)
			{
				let dist = API3.SquareVectorDistance(cc.pos, pos);
				if (dist < nearbyRejected)
				{
					norm = 0;
					break;
				}
				if (!cc.ally)
					continue;
				if (dist < nearbyAllyRejected)
				{
					norm = 0;
					break;
				}
				if (dist < nearbyAllyDisfavored)
					norm *= 0.5;

				if (dist < minDist)
					minDist = dist;
				accessible = accessible || index == m.getLandAccess(gameState, cc.ent);
			}
			if (norm == 0)
				continue;

			if (accessible && minDist > maxAccessRejected)
				continue;

			if (minDist > maxAccessDisfavored)     // Disfavor if quite far from any allied cc
			{
				if (!accessible)
				{
					if (minDist > maxNoAccessDisfavored)
						norm *= 0.5;
					else
						norm *= 0.8;
				}
				else
					norm *= 0.5;
			}

			// Not near any of our dropsite, except for oversea docks
			oversea = !accessible && dpList.some(dp => m.getLandAccess(gameState, dp.ent) == index);
			if (!oversea)
			{
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
			}
			if (norm == 0)
				continue;
		}

		if (this.borderMap.map[j] & m.fullBorder_Mask)	// disfavor the borders of the map
			norm *= 0.5;

		let val = 2*gameState.sharedScript.ccResourceMaps[resource].map[j];
		for (let res in gameState.sharedScript.resourceMaps)
			if (res != "food")
				val += gameState.sharedScript.ccResourceMaps[res].map[j];
		val *= norm;

		// If oversea, be just above threshold to be accepted if nothing else 
		if (oversea)
			val = Math.max(val, cut + 0.1);

		if (bestVal !== undefined && val < bestVal)
			continue;
		if (this.isDangerousLocation(gameState, pos, halfSize))
			continue;
		bestVal = val;
		bestIdx = i;
	}

	Engine.ProfileStop();

	if (bestVal === undefined)
		return false;
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
		if (!base.anchor || base.accessIndex == indexIdx)
			continue;
		let sea = this.getSeaBetweenIndices(gameState, base.accessIndex, indexIdx);
		if (sea !== undefined)
			this.navalManager.setMinimalTransportShips(gameState, sea, 1);
	}

	return [x, z];
};

/**
 * Returns the best position to build a new Civil Center
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
		ccList.push({ "pos": cc.position(), "ally": ally });
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
	let radius = Math.ceil(template.obstructionRadius().max / obstructions.cellSize);

	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;
	let currentVal, delta;
	let distcc0, distcc1, distcc2;
	let favoredDistance = (template.hasClass("Colony") ? 220 : 280) - 40 * this.Config.personality.defensive;

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.territoryMap.getOwnerIndex(j) != 0)
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
		if (minDist < 1 || minDist > 170000 && !this.navalMap)
			continue;

		delta = Math.sqrt(distcc0) - favoredDistance;
		currentVal = delta*delta;
		delta = Math.sqrt(distcc1) - favoredDistance;
		currentVal += delta*delta;
		if (distcc2)
		{
			delta = Math.sqrt(distcc2) - favoredDistance;
			currentVal += delta*delta;
		}
		// disfavor border of the map
		if (this.borderMap.map[j] & m.fullBorder_Mask)
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
		if (!base.anchor || base.accessIndex == indexIdx)
			continue;
		let sea = this.getSeaBetweenIndices(gameState, base.accessIndex, indexIdx);
		if (sea !== undefined)
			this.navalManager.setMinimalTransportShips(gameState, sea, 1);
	}

	return [x, z];
};

/**
 * Returns the best position to build a new market: if the allies already have a market, build it as far as possible
 * from it, although not in our border to be able to defend it easily. If no allied market, our second market will
 * follow the same logic.
 * To do so, we suppose that the gain/distance is an increasing function of distance and look for the max distance
 * for performance reasons.
 */
m.HQ.prototype.findMarketLocation = function(gameState, template)
{
	let markets = gameState.updatingCollection("diplo-ExclusiveAllyMarkets", API3.Filters.byClass("Market"), gameState.getExclusiveAllyEntities()).toEntityArray();
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
	let bestDistSq;
	let bestGainMult;
	let radius = Math.ceil(template.obstructionRadius().max / obstructions.cellSize);
	let isNavalMarket = template.hasClass("NavalMarket");

	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;

	let traderTemplatesGains = gameState.getTraderTemplatesGains();

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		// do not try on the narrow border of our territory
		if (this.borderMap.map[j] & m.narrowFrontier_Mask)
			continue;
		if (this.basesMap.map[j] == 0)   // only in our territory
			continue;
		// with enough room around to build the market
		let i = this.territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)
			continue;
		let index = gameState.ai.accessibility.landPassMap[i];
		if (!this.landRegions[index])
			continue;
		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		// checking distances to other markets
		let maxVal = 0;
		let maxDistSq;
		let maxGainMult;
		let gainMultiplier;
		for (let market of markets)
		{
			if (isNavalMarket && market.hasClass("NavalMarket"))
			{
				if (m.getSeaAccess(gameState, market) != gameState.ai.accessibility.getAccessValue(pos, true))
					continue;
				gainMultiplier = traderTemplatesGains.navalGainMultiplier;
			}
			else if (m.getLandAccess(gameState, market) == index &&
				!m.isLineInsideEnemyTerritory(gameState, market.position(), pos))
				gainMultiplier = traderTemplatesGains.landGainMultiplier;
			else
				continue;
			if (!gainMultiplier)
				continue;
			let distSq = API3.SquareVectorDistance(market.position(), pos);
			if (gainMultiplier * distSq > maxVal)
			{
				maxVal = gainMultiplier * distSq;
				maxDistSq = distSq;
				maxGainMult = gainMultiplier;
			}
		}
		if (maxVal == 0)
			continue;
		if (bestVal !== undefined && maxVal < bestVal)
			continue;
		if (this.isDangerousLocation(gameState, pos, halfSize))
			continue;
		bestVal = maxVal;
		bestDistSq = maxDistSq;
		bestGainMult = maxGainMult;
		bestIdx = i;
		bestJdx = j;
	}

	if (this.Config.debug > 1)
		API3.warn("We found a market position with bestVal = " + bestVal);

	if (bestVal === undefined)  // no constraints. For the time being, place it arbitrarily by the ConstructionPlan
		return [-1, -1, -1, 0];
	let expectedGain = Math.round(bestGainMult * TradeGain(bestDistSq, gameState.sharedScript.mapSize));
	if (this.Config.debug > 1)
		API3.warn("this would give a trading gain of " + expectedGain);
	// do not keep it if gain is too small, except if this is our first BarterMarket
	let idx;
	if (expectedGain < this.tradeManager.minimalGain)
	{
		if (template.hasClass("BarterMarket") &&
		    !gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities())
			idx = -1;	// needed by queueplanBuilding manager to keep that market
		else
			return false;
	}
	else
		idx = this.basesMap.map[bestJdx];

	let x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	let z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;
	return [x, z, idx, expectedGain];
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
		if (!enemyStructures.hasEntities() && !gameState.getAlliedVictory())
			enemyStructures = gameState.getAllyStructures().filter(API3.Filters.not(API3.Filters.byOwner(PlayerID))).
				filter(API3.Filters.byClassesOr(["CivCentre", "Fortress", "Tower"]));
		if (!enemyStructures.hasEntities())
			return undefined;
	}
	enemyStructures = enemyStructures.toEntityArray();

	let wonderMode = gameState.getVictoryConditions().has("wonder");
	let wonderDistmin;
	let wonders;
	if (wonderMode)
	{
		wonders = gameState.getOwnStructures().filter(API3.Filters.byClass("Wonder")).toEntityArray();
		wonderMode = wonders.length != 0;
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
		radius = Math.floor((template.obstructionRadius().max + 8) / obstructions.cellSize);
	else
		radius = Math.ceil(template.obstructionRadius().max / obstructions.cellSize);

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (!wonderMode)
		{
			// do not try if well inside or outside territory
			if (!(this.borderMap.map[j] & m.fullFrontier_Mask))
				continue;
			if (this.borderMap.map[j] & m.largeFrontier_Mask && isTower)
				continue;
		}
		if (this.basesMap.map[j] == 0)   // inaccessible cell
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
		if (minDist < 0 || minDist == Math.min())
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
	if (queues.economicBuilding.hasQueuedUnits() ||
		gameState.getOwnEntitiesByClass("Temple", true).hasEntities() ||
		!gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities())
		return;
	// Try to build a temple earlier if in regicide to recruit healer guards
	if (this.currentPhase < 3 && !gameState.getVictoryConditions().has("regicide"))
		return;

	let templateName = "structures/{civ}_temple";
	if (this.canBuild(gameState, "structures/{civ}_temple_vesta"))
		templateName = "structures/{civ}_temple_vesta";
	else if (!this.canBuild(gameState, templateName))
		return;
	queues.economicBuilding.addPlan(new m.ConstructionPlan(gameState, templateName));
};

m.HQ.prototype.buildMarket = function(gameState, queues)
{
	if (gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities() ||
		!this.canBuild(gameState, "structures/{civ}_market"))
		return;

	if (queues.economicBuilding.hasQueuedUnitsWithClass("BarterMarket"))
	{
		if (!queues.economicBuilding.paused)
		{
			// Put available resources in this market
			let queueManager = gameState.ai.queueManager;
			let cost = queues.economicBuilding.plans[0].getCost();
			queueManager.setAccounts(gameState, cost, "economicBuilding");
			if (!queueManager.canAfford("economicBuilding", cost))
			{
				for (let q in queueManager.queues)
				{
					if (q == "economicBuilding")
						continue;
					queueManager.transferAccounts(cost, q, "economicBuilding");
					if (queueManager.canAfford("economicBuilding", cost))
						break;
				}
			}
		}
		return;
	}

	gameState.ai.queueManager.changePriority("economicBuilding", 3*this.Config.priorities.economicBuilding);
	let plan = new m.ConstructionPlan(gameState, "structures/{civ}_market");
	plan.queueToReset = "economicBuilding";
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

/**
 * Try to build a wonder when required
 * force = true when called from the victoryManager in case of Wonder victory condition.
 */
m.HQ.prototype.buildWonder = function(gameState, queues, force = false)
{
	if (queues.wonder && queues.wonder.hasQueuedUnits() ||
	    gameState.getOwnEntitiesByClass("Wonder", true).hasEntities() ||
	    !this.canBuild(gameState, "structures/{civ}_wonder"))
		return;

	if (!force)
	{
		let template = gameState.getTemplate(gameState.applyCiv("structures/{civ}_wonder"));
		// Check that we have enough resources to start thinking to build a wonder
		let cost = template.cost();
		let resources = gameState.getResources();
		let highLevel = 0;
		let lowLevel = 0;
		for (let res in cost)
		{
			if (resources[res] && resources[res] > 0.7 * cost[res])
				++highLevel;
			else if (!resources[res] || resources[res] < 0.3 * cost[res])
				++lowLevel;
		}
		if (highLevel == 0 || lowLevel > 1)
			return;
	}

	queues.wonder.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_wonder"));
};

/** Build a corral, and train animals there */
m.HQ.prototype.manageCorral = function(gameState, queues)
{
	if (queues.corral.hasQueuedUnits())
		return;

	let nCorral = gameState.getOwnEntitiesByClass("Corral", true).length;
	if (!nCorral || !gameState.isTemplateAvailable(gameState.applyCiv("structures/{civ}_field")) &&
	                nCorral < this.currentPhase && gameState.getPopulation() > 30*nCorral)
	{
		if (this.canBuild(gameState, "structures/{civ}_corral"))
		{
			queues.corral.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_corral"));
			return;
		}
		if (!nCorral)
			return;
	}

	// And train some animals
	let civ = gameState.getPlayerCiv();
	for (let corral of gameState.getOwnEntitiesByClass("Corral", true).values())
	{
		if (corral.foundationProgress() !== undefined)
			continue;
		let trainables = corral.trainableEntities(civ);
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
m.HQ.prototype.buildMoreHouses = function(gameState, queues)
{
	if (!gameState.isTemplateAvailable(gameState.applyCiv("structures/{civ}_house")) ||
	    gameState.getPopulationMax() <= gameState.getPopulationLimit())
		return;

	let numPlanned = queues.house.length();
	if (numPlanned < 3 || numPlanned < 5 && gameState.getPopulation() > 80)
	{
		let plan = new m.ConstructionPlan(gameState, "structures/{civ}_house");
		// change the starting condition according to the situation.
		plan.goRequirement = "houseNeeded";
		queues.house.addPlan(plan);
	}

	if (numPlanned > 0 && this.phasing && gameState.getPhaseEntityRequirements(this.phasing).length)
	{
		let houseTemplateName = gameState.applyCiv("structures/{civ}_house");
		let houseTemplate = gameState.getTemplate(houseTemplateName);

		let needed = 0;
		for (let entityReq of gameState.getPhaseEntityRequirements(this.phasing))
		{
			if (!houseTemplate.hasClass(entityReq.class))
				continue;

			let count = gameState.getOwnStructures().filter(API3.Filters.byClass(entityReq.class)).length;
			if (count < entityReq.count && this.buildManager.isUnbuildable(gameState, houseTemplateName))
			{
				if (this.Config.debug > 1)
					API3.warn("no room to place a house ... try to be less restrictive");
				this.buildManager.setBuildable(houseTemplateName);
				this.requireHouses = true;
			}
			needed = Math.max(needed, entityReq.count - count);
		}

		let houseQueue = queues.house.plans;
		for (let i = 0; i < numPlanned; ++i)
			if (houseQueue[i].isGo(gameState))
				--needed;
			else if (needed > 0)
			{
				houseQueue[i].goRequirement = undefined;
				--needed;
			}
	}

	if (this.requireHouses)
	{
		let houseTemplate = gameState.getTemplate(gameState.applyCiv("structures/{civ}_house"));
		if (!this.phasing || gameState.getPhaseEntityRequirements(this.phasing).every(req =>
			!houseTemplate.hasClass(req.class) || gameState.getOwnStructures().filter(API3.Filters.byClass(req.class)).length >= req.count))
			this.requireHouses = undefined;
	}

	// When population limit too tight
	//    - if no room to build, try to improve with technology
	//    - otherwise increase temporarily the priority of houses
	let house = gameState.applyCiv("structures/{civ}_house");
	let HouseNb = gameState.getOwnFoundations().filter(API3.Filters.byClass("House")).length;
	let popBonus = gameState.getTemplate(house).getPopulationBonus();
	let freeSlots = gameState.getPopulationLimit() + HouseNb*popBonus - this.getAccountedPopulation(gameState);
	let priority;
	if (freeSlots < 5)
	{
		if (this.buildManager.isUnbuildable(gameState, house))
		{
			if (this.Config.debug > 1)
				API3.warn("no room to place a house ... try to improve with technology");
			this.researchManager.researchPopulationBonus(gameState, queues);
		}
		else
			priority = 2*this.Config.priorities.house;
	}
	else
		priority = this.Config.priorities.house;

	if (priority && priority != gameState.ai.queueManager.getPriority("house"))
		gameState.ai.queueManager.changePriority("house", priority);
};

/** Checks the status of the territory expansion. If no new economic bases created, build some strategic ones. */
m.HQ.prototype.checkBaseExpansion = function(gameState, queues)
{
	if (queues.civilCentre.hasQueuedUnits())
		return;
	// First build one cc if all have been destroyed
	if (this.numPotentialBases() == 0)
	{
		this.buildFirstBase(gameState);
		return;
	}
	// Then expand if we have not enough room available for buildings
	if (this.buildManager.numberMissingRoom(gameState) > 1)
	{
		if (this.Config.debug > 2)
			API3.warn("try to build a new base because not enough room to build ");
		this.buildNewBase(gameState, queues);
		return;
	}
	// If we've already planned to phase up, wait a bit before trying to expand
	if (this.phasing)
		return;
	// Finally expand if we have lots of units (threshold depending on the aggressivity value)
	let activeBases = this.numActiveBases();
	let numUnits = gameState.getOwnUnits().length;
	let numvar = 10 * (1 - this.Config.personality.aggressive);
	if (numUnits > activeBases * (65 + numvar + (10 + numvar)*(activeBases-1)) || this.saveResources && numUnits > 50)
	{
		if (this.Config.debug > 2)
			API3.warn("try to build a new base because of population " + numUnits + " for " + activeBases + " CCs");
		this.buildNewBase(gameState, queues);
	}
};

m.HQ.prototype.buildNewBase = function(gameState, queues, resource)
{
	if (this.numPotentialBases() > 0 && this.currentPhase == 1 && !gameState.isResearching(gameState.getPhaseName(2)))
		return false;
	if (gameState.getOwnFoundations().filter(API3.Filters.byClass("CivCentre")).hasEntities() || queues.civilCentre.hasQueuedUnits())
		return false;

	let template;
	// We require at least one of this civ civCentre as they may allow specific units or techs
	let hasOwnCC = false;
	for (let ent of gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre")).values())
	{
		if (ent.owner() != PlayerID || ent.templateName() != gameState.applyCiv("structures/{civ}_civil_centre"))
			continue;
		hasOwnCC = true;
		break;
	}
	if (hasOwnCC && this.canBuild(gameState, "structures/{civ}_military_colony"))
		template = "structures/{civ}_military_colony";
	else if (this.canBuild(gameState, "structures/{civ}_civil_centre"))
		template = "structures/{civ}_civil_centre";
	else if (!hasOwnCC && this.canBuild(gameState, "structures/{civ}_military_colony"))
		template = "structures/{civ}_military_colony";
	else
		return false;

	// base "-1" means new base.
	if (this.Config.debug > 1)
		API3.warn("new base " + gameState.applyCiv(template) + " planned with resource " + resource);
	queues.civilCentre.addPlan(new m.ConstructionPlan(gameState, template, { "base": -1, "resource": resource }));
	return true;
};

/** Deals with building fortresses and towers along our border with enemies. */
m.HQ.prototype.buildDefenses = function(gameState, queues)
{
	if (this.saveResources && !this.canBarter || queues.defenseBuilding.hasQueuedUnits())
		return;

	if (!this.saveResources && (this.currentPhase > 2 || gameState.isResearching(gameState.getPhaseName(3))))
	{
		// try to build fortresses
		if (this.canBuild(gameState, "structures/{civ}_fortress"))
		{
			let numFortresses = gameState.getOwnEntitiesByClass("Fortress", true).length;
			if ((!numFortresses || gameState.ai.elapsedTime > (1 + 0.10*numFortresses)*this.fortressLapseTime + this.fortressStartTime) &&
				numFortresses < this.numActiveBases() + 1 + this.extraFortresses &&
				numFortresses < Math.floor(gameState.getPopulation() / 25) &&
				gameState.getOwnFoundationsByClass("Fortress").length < 2)
			{
				this.fortressStartTime = gameState.ai.elapsedTime;
				if (!numFortresses)
					gameState.ai.queueManager.changePriority("defenseBuilding", 2*this.Config.priorities.defenseBuilding);
				let plan = new m.ConstructionPlan(gameState, "structures/{civ}_fortress");
				plan.queueToReset = "defenseBuilding";
				queues.defenseBuilding.addPlan(plan);
				return;
			}
		}
	}

	if (this.Config.Military.numSentryTowers && this.currentPhase < 2 && this.canBuild(gameState, "structures/{civ}_sentry_tower"))
	{
		let numTowers = gameState.getOwnEntitiesByClass("Tower", true).length;	// we count all towers, including wall towers
		let towerLapseTime = this.saveResource ? (1 + 0.5*numTowers) * this.towerLapseTime : this.towerLapseTime;
		if (numTowers < this.Config.Military.numSentryTowers && gameState.ai.elapsedTime > towerLapseTime + this.fortStartTime)
		{
			this.fortStartTime = gameState.ai.elapsedTime;
			queues.defenseBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_sentry_tower"));
		}
		return;
	}

	if (this.currentPhase < 2 || !this.canBuild(gameState, "structures/{civ}_defense_tower"))
		return;

	let numTowers = gameState.getOwnEntitiesByClass("StoneTower", true).length;
	let towerLapseTime = this.saveResource ? (1 + numTowers) * this.towerLapseTime : this.towerLapseTime;
	if ((!numTowers || gameState.ai.elapsedTime > (1 + 0.1*numTowers)*towerLapseTime + this.towerStartTime) &&
		numTowers < 2 * this.numActiveBases() + 3 + this.extraTowers &&
		numTowers < Math.floor(gameState.getPopulation() / 8) &&
		gameState.getOwnFoundationsByClass("DefenseTower").length < 3)
	{
		this.towerStartTime = gameState.ai.elapsedTime;
		if (numTowers > 2 * this.numActiveBases() + 3)
			gameState.ai.queueManager.changePriority("defenseBuilding", Math.round(0.7*this.Config.priorities.defenseBuilding));
		let plan = new m.ConstructionPlan(gameState, "structures/{civ}_defense_tower");
		plan.queueToReset = "defenseBuilding";
		queues.defenseBuilding.addPlan(plan);
	}
};

m.HQ.prototype.buildBlacksmith = function(gameState, queues)
{
	if (this.getAccountedPopulation(gameState) < this.Config.Military.popForBlacksmith ||
		queues.militaryBuilding.hasQueuedUnits() || gameState.getOwnEntitiesByClass("Blacksmith", true).length)
		return;
	// build a market before the blacksmith
	if (!gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities())
		return;

	if (this.canBuild(gameState, "structures/{civ}_blacksmith"))
		queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_blacksmith"));
};

/**
 * Deals with constructing military buildings (barracks, stables…)
 * They are mostly defined by Config.js. This is unreliable since changes could be done easily.
 */
m.HQ.prototype.constructTrainingBuildings = function(gameState, queues)
{
	if (this.saveResources && !this.canBarter || queues.militaryBuilding.hasQueuedUnits())
		return;

	let numBarracks = gameState.getOwnEntitiesByClass("Barracks", true).length;
	if (this.saveResources && numBarracks != 0)
		return;

	let barracksTemplate = this.canBuild(gameState, "structures/{civ}_barracks") ? "structures/{civ}_barracks" : undefined;

	let rangeTemplate = this.canBuild(gameState, "structures/{civ}_range") ? "structures/{civ}_range" : undefined;
	let numRanges = gameState.getOwnEntitiesByClass("Archery", true).length;
	numBarracks -= numRanges;

	let stableTemplate = this.canBuild(gameState, "structures/{civ}_stables") ? "structures/{civ}_stables" :
	                     this.canBuild(gameState, "structures/{civ}_stable") ? "structures/{civ}_stable" : undefined;
	let numStables = gameState.getOwnEntitiesByClass("Stables", true).length;

	if (this.getAccountedPopulation(gameState) > this.Config.Military.popForBarracks1 ||
	    this.phasing == 2 && gameState.getOwnStructures().filter(API3.Filters.byClass("Village")).length < 5)
	{
		// first barracks/range and stables.
		if (numBarracks + numRanges == 0)
		{
			let template = barracksTemplate || rangeTemplate;
			if (template)
			{
				gameState.ai.queueManager.changePriority("militaryBuilding", 2 * this.Config.priorities.militaryBuilding);
				let plan = new m.ConstructionPlan(gameState, template, { "militaryBase": true });
				plan.queueToReset = "militaryBuilding";
				queues.militaryBuilding.addPlan(plan);
				return;
			}
		}
		if (numStables == 0 && stableTemplate)
		{
			queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, stableTemplate, { "militaryBase": true }));
			return;
		}

		// Second range/barracks and stables
		if (numBarracks + numRanges == 1 && this.getAccountedPopulation(gameState) > this.Config.Military.popForBarracks2)
		{
			let template = numBarracks == 0 ? (barracksTemplate || rangeTemplate) : (rangeTemplate || barracksTemplate);
			if (template)
			{
				queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, template, { "militaryBase": true }));
				return;
			}
		}
		if (numStables == 1 && stableTemplate && this.getAccountedPopulation(gameState) > this.Config.Military.popForBarracks2)
		{
			queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, stableTemplate, { "militaryBase": true }));
			return;
		}

		// Then 3rd barracks/range/stables if needed
		if (numBarracks + numRanges + numStables == 2 && this.getAccountedPopulation(gameState) > this.Config.Military.popForBarracks2 + 30)
		{
			let template = barracksTemplate || stableTemplate || rangeTemplate;
			if (template)
			{
				queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, template, { "militaryBase": true }));
				return;
			}
		}
	}

	if (this.saveResources)
		return;

	if (this.currentPhase < 3)
		return;

	if (this.canBuild(gameState, "structures/{civ}_elephant_stables") && !gameState.getOwnEntitiesByClass("ElephantStables", true).hasEntities())
	{
		queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_elephant_stables", { "militaryBase": true }));
		return;
	}

	if (this.canBuild(gameState, "structures/{civ}_workshop") && !gameState.getOwnEntitiesByClass("Workshop", true).hasEntities())
	{
		queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_workshop", { "militaryBase": true }));
		return;
	}

	if (this.getAccountedPopulation(gameState) < 80 || !this.bAdvanced.length)
		return;

	// Build advanced military buildings
	let nAdvanced = 0;
	for (let advanced of this.bAdvanced)
		nAdvanced += gameState.countEntitiesAndQueuedByType(advanced, true);

	if (!nAdvanced || nAdvanced < this.bAdvanced.length && this.getAccountedPopulation(gameState) > 110)
	{
		for (let advanced of this.bAdvanced)
		{
			if (gameState.countEntitiesAndQueuedByType(advanced, true) > 0 || !this.canBuild(gameState, advanced))
				continue;
			let template = gameState.getTemplate(advanced);
			if (!template)
				continue;
			let civ = gameState.getPlayerCiv();
			if (template.hasDefensiveFire() || template.trainableEntities(civ))
				queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, advanced, { "militaryBase": true }));
			else	// not a military building, but still use this queue
				queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, advanced));
			return;
		}
	}
};

/**
 *  Find base nearest to ennemies for military buildings.
 */
m.HQ.prototype.findBestBaseForMilitary = function(gameState)
{
	let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre")).toEntityArray();
	let bestBase;
	let enemyFound = false;
	let distMin = Math.min();
	for (let cce of ccEnts)
	{
		if (gameState.isPlayerAlly(cce.owner()))
			continue;
		if (enemyFound && !gameState.isPlayerEnemy(cce.owner()))
			continue;
		let access = m.getLandAccess(gameState, cce);
		let isEnemy = gameState.isPlayerEnemy(cce.owner());
		for (let cc of ccEnts)
		{
			if (cc.owner() != PlayerID)
				continue;
			if (m.getLandAccess(gameState, cc) != access)
				continue;
			let dist = API3.SquareVectorDistance(cc.position(), cce.position());
			if (!enemyFound && isEnemy)
				enemyFound = true;
			else if (dist > distMin)
				continue;
			bestBase = cc.getMetadata(PlayerID, "base");
			distMin = dist;
		}
	}
	return bestBase;
};

/**
 * train with highest priority ranged infantry in the nearest civil center from a given set of positions
 * and garrison them there for defense
 */
m.HQ.prototype.trainEmergencyUnits = function(gameState, positions)
{
	if (gameState.ai.queues.emergency.hasQueuedUnits())
		return false;

	let civ = gameState.getPlayerCiv();
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
			if (m.getLandAccess(gameState, base.anchor) != access)
				continue;
			if (!base.anchor.trainableEntities(civ))	// base still in construction
				continue;
			let queue = base.anchor._entity.trainingQueue;
			if (queue)
			{
				let time = 0;
				for (let item of queue)
					if (item.progress > 0 || item.metadata && item.metadata.garrisonType)
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
	let rangedWanted = randBool() && autogarrison;

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
		if (autogarrison && !MatchesClassList(template.classes(), garrisonArrowClasses))
			continue;
		if (!total.canAfford(new API3.Resources(template.cost())))
			continue;
		templateFound = [trainable, template];
		if (template.hasClass("Ranged") == rangedWanted)
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
			if (q == "emergency")
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
	if (this.buildManager.isUnbuildable(gameState, type))
		return false;

	if (gameState.isTemplateDisabled(type))
	{
		this.buildManager.setUnbuildable(gameState, type, Infinity, "disabled");
		return false;
	}

	let template = gameState.getTemplate(type);
	if (!template)
	{
		this.buildManager.setUnbuildable(gameState, type, Infinity, "notemplate");
		return false;
	}

	if (!template.available(gameState))
	{
		this.buildManager.setUnbuildable(gameState, type, 30, "tech");
		return false;
	}

	if (!this.buildManager.hasBuilder(type))
	{
		this.buildManager.setUnbuildable(gameState, type, 120, "nobuilder");
		return false;
	}

	if (this.numActiveBases() < 1)
	{
		// if no base, check that we can build outside our territory
		let buildTerritories = template.buildTerritories();
		if (buildTerritories && (!buildTerritories.length || buildTerritories.length == 1 && buildTerritories[0] == "own"))
		{
			this.buildManager.setUnbuildable(gameState, type, 180, "room");
			return false;
		}
	}

	// build limits
	let limits = gameState.getEntityLimits();
	let category = template.buildCategory();
	if (category && limits[category] !== undefined && gameState.getEntityCounts()[category] >= limits[category])
	{
		this.buildManager.setUnbuildable(gameState, type, 90, "limit");
		return false;
	}

	return true;
};

m.HQ.prototype.updateTerritories = function(gameState)
{
	const around = [ [-0.7, 0.7], [0, 1], [0.7, 0.7], [1, 0], [0.7, -0.7], [0, -1], [-0.7, -0.7], [-1, 0] ];
	let alliedVictory = gameState.getAlliedVictory();
	let passabilityMap = gameState.getPassabilityMap();
	let width = this.territoryMap.width;
	let cellSize = this.territoryMap.cellSize;
	let insideSmall = Math.round(45 / cellSize);
	let insideLarge = Math.round(80 / cellSize);	// should be about the range of towers
	let expansion = 0;

	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.borderMap.map[j] & m.outside_Mask)
			continue;
		if (this.borderMap.map[j] & m.fullFrontier_Mask)
			this.borderMap.map[j] &= ~m.fullFrontier_Mask;	// reset the frontier

		if (this.territoryMap.getOwnerIndex(j) != PlayerID)
		{
			// If this tile was already accounted, remove it
			if (this.basesMap.map[j] == 0)
				continue;
			let base = this.getBaseByID(this.basesMap.map[j]);
			if (base)
			{
				let index = base.territoryIndices.indexOf(j);
				if (index != -1)
					base.territoryIndices.splice(index, 1);
				else
					API3.warn(" problem in headquarters::updateTerritories for base " + this.basesMap.map[j]);
			}
			else
				API3.warn(" problem in headquarters::updateTerritories without base " + this.basesMap.map[j]);
			this.basesMap.map[j] = 0;
		}
		else
		{
			// Update the frontier
			let ix = j%width;
			let iz = Math.floor(j/width);
			let onFrontier = false;
			for (let a of around)
			{
				let jx = ix + Math.round(insideSmall*a[0]);
				if (jx < 0 || jx >= width)
					continue;
				let jz = iz + Math.round(insideSmall*a[1]);
				if (jz < 0 || jz >= width)
					continue;
				if (this.borderMap.map[jx+width*jz] & m.outside_Mask)
					continue;
				let territoryOwner = this.territoryMap.getOwnerIndex(jx+width*jz);
				if (territoryOwner != PlayerID && !(alliedVictory && gameState.isPlayerAlly(territoryOwner)))
				{
					this.borderMap.map[j] |= m.narrowFrontier_Mask;
					break;
				}
				jx = ix + Math.round(insideLarge*a[0]);
				if (jx < 0 || jx >= width)
					continue;
				jz = iz + Math.round(insideLarge*a[1]);
				if (jz < 0 || jz >= width)
					continue;
				if (this.borderMap.map[jx+width*jz] & m.outside_Mask)
					continue;
				territoryOwner = this.territoryMap.getOwnerIndex(jx+width*jz);
				if (territoryOwner != PlayerID && !(alliedVictory && gameState.isPlayerAlly(territoryOwner)))
					onFrontier = true;
			}
			if (onFrontier && !(this.borderMap.map[j] & m.narrowFrontier_Mask))
				this.borderMap.map[j] |= m.largeFrontier_Mask;

			// If this tile was not already accounted, add it.
			if (this.basesMap.map[j] != 0)
				continue;
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

	if (!expansion)
		return;
	// We've increased our territory, so we may have some new room to build
	this.buildManager.resetMissingRoom(gameState);
	// And if sufficient expansion, check if building a new market would improve our present trade routes
	let cellArea = this.territoryMap.cellSize * this.territoryMap.cellSize;
	if (expansion * cellArea > 960)
		this.tradeManager.routeProspection = true;
};

/** Reassign territories when a base is going to be deleted */
m.HQ.prototype.reassignTerritories = function(deletedBase)
{
	let cellSize = this.territoryMap.cellSize;
	let width = this.territoryMap.width;
	for (let j = 0; j < this.territoryMap.length; ++j)
	{
		if (this.basesMap.map[j] != deletedBase.ID)
			continue;
		if (this.territoryMap.getOwnerIndex(j) != PlayerID)
		{
			API3.warn("Petra reassignTerritories: should never happen");
			this.basesMap.map[j] = 0;
			continue;
		}

		let distmin = Math.min();
		let baseID;
		let pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		for (let base of this.baseManagers)
		{
			if (!base.anchor || !base.anchor.position())
				continue;
			if (base.accessIndex != deletedBase.accessIndex)
				continue;
			let dist = API3.SquareVectorDistance(base.anchor.position(), pos);
			if (dist >= distmin)
				continue;
			distmin = dist;
			baseID = base.ID;
		}
		if (baseID)
		{
			this.getBaseByID(baseID).territoryIndices.push(j);
			this.basesMap.map[j] = baseID;
		}
		else
			this.basesMap.map[j] = 0;
	}
};

/**
 * returns the base corresponding to baseID
 */
m.HQ.prototype.getBaseByID = function(baseID)
{
	for (let base of this.baseManagers)
		if (base.ID == baseID)
			return base;

	return undefined;
};

/**
 * returns the number of bases with a cc
 * ActiveBases includes only those with a built cc
 * PotentialBases includes also those with a cc in construction
 */
m.HQ.prototype.numActiveBases = function()
{
	if (!this.turnCache.base)
		this.updateBaseCache();
	return this.turnCache.base.active;
};

m.HQ.prototype.numPotentialBases = function()
{
	if (!this.turnCache.base)
		this.updateBaseCache();
	return this.turnCache.base.potential;
};

m.HQ.prototype.updateBaseCache = function()
{
	this.turnCache.base = { "active": 0, "potential": 0 };
	for (let base of this.baseManagers)
	{
		if (!base.anchor)
			continue;
		++this.turnCache.base.potential;
		if (base.anchor.foundationProgress() === undefined)
			++this.turnCache.base.active;
	}
};

m.HQ.prototype.resetBaseCache = function()
{
	this.turnCache.base = undefined;
};

/**
 * Count gatherers returning resources in the number of gatherers of resourceSupplies
 * to prevent the AI always reassigning idle workers to these resourceSupplies (specially in naval maps).
 */
m.HQ.prototype.assignGatherers = function()
{
	for (let base of this.baseManagers)
	{
		for (let worker of base.workers.values())
		{
			if (worker.unitAIState().split(".")[1] != "RETURNRESOURCE")
				continue;
			let orders = worker.unitAIOrderData();
			if (orders.length < 2 || !orders[1].target || orders[1].target != worker.getMetadata(PlayerID, "supply"))
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
		this.turnCache.firingStructures = gameState.updatingCollection("diplo-FiringStructures", API3.Filters.hasDefensiveFire(), gameState.getEnemyStructures());
	for (let ent of this.turnCache.firingStructures.values())
	{
		let range = radius + ent.attackRange("Ranged").max;
		if (API3.SquareVectorDistance(ent.position(), pos) < range*range)
			return true;
	}
	return false;
};

/** Compute the capture strength of all units attacking a capturable target */
m.HQ.prototype.updateCaptureStrength = function(gameState)
{
	this.capturableTargets.clear();
	for (let ent of gameState.getOwnUnits().values())
	{
		if (!ent.canCapture())
			continue;
		let state = ent.unitAIState();
		if (!state || !state.split(".")[1] || state.split(".")[1] != "COMBAT")
			continue;
		let orderData = ent.unitAIOrderData();
		if (!orderData || !orderData.length || !orderData[0].target)
			continue;
		let targetId = orderData[0].target;
		let target = gameState.getEntityById(targetId);
		if (!target || !target.isCapturable() || !ent.canCapture(target))
			continue;
		if (!this.capturableTargets.has(targetId))
			this.capturableTargets.set(targetId, {
				"strength": ent.captureStrength() * m.getAttackBonus(ent, target, "Capture"),
				"ents": new Set([ent.id()])
			});
		else
		{
			let capturableTarget = this.capturableTargets.get(target.id());
			capturableTarget.strength += ent.captureStrength() * m.getAttackBonus(ent, target, "Capture");
			capturableTarget.ents.add(ent.id());
		}
	}

	for (let [targetId, capturableTarget] of this.capturableTargets)
	{
		let target = gameState.getEntityById(targetId);
		let allowCapture;
		for (let entId of capturableTarget.ents)
		{
			let ent = gameState.getEntityById(entId);
			if (allowCapture === undefined)
				allowCapture = m.allowCapture(gameState, ent, target);
			let orderData = ent.unitAIOrderData();
			if (!orderData || !orderData.length || !orderData[0].attackType)
				continue;
			if ((orderData[0].attackType == "Capture") !== allowCapture)
				ent.attack(targetId, allowCapture);
		}
	}

	this.capturableTargetsTime = gameState.ai.elapsedTime;
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

	if (this.turnCache.currentRates)
		this.turnCache.currentRates[resource] += 0.5;
};

m.HQ.prototype.GetTCResGatherer = function(resource)
{
	if (this.turnCache["resourceGatherer-" + resource])
		return this.turnCache["resourceGatherer-" + resource];

	return 0;
};

/**
 * flag a resource as exhausted
 */
m.HQ.prototype.isResourceExhausted = function(resource)
{
	if (this.turnCache["exhausted-" + resource] == undefined)
		this.turnCache["exhausted-" + resource] = this.baseManagers.every(base =>
			!base.dropsiteSupplies[resource].nearby.length &&
			!base.dropsiteSupplies[resource].medium.length &&
			!base.dropsiteSupplies[resource].faraway.length);

	return this.turnCache["exhausted-" + resource];
};

/**
 * Check if a structure in blinking territory should/can be defended (currently if it has some attacking armies around)
 */
m.HQ.prototype.isDefendable = function(ent)
{
	if (!this.turnCache.numAround)
		this.turnCache.numAround = {};
	if (this.turnCache.numAround[ent.id()] === undefined)
		this.turnCache.numAround[ent.id()] = this.attackManager.numAttackingUnitsAround(ent.position(), 130);
	return +this.turnCache.numAround[ent.id()] > 8;
};

/**
 * Get the number of population already accounted for
 */
m.HQ.prototype.getAccountedPopulation = function(gameState)
{
	if (this.turnCache.accountedPopulation == undefined)
	{
		let pop = gameState.getPopulation();
		for (let ent of gameState.getOwnTrainingFacilities().values())
		{
			for (let item of ent.trainingQueue())
			{
				if (!item.unitTemplate)
					continue;
				let unitPop = gameState.getTemplate(item.unitTemplate).get("Cost/Population");
				if (unitPop)
					pop += item.count * unitPop;
			}
		}
		this.turnCache.accountedPopulation = pop;
	}
	return this.turnCache.accountedPopulation;
};

/**
 * Get the number of workers already accounted for
 */
m.HQ.prototype.getAccountedWorkers = function(gameState)
{
	if (this.turnCache.accountedWorkers == undefined)
	{
		let workers = gameState.getOwnEntitiesByRole("worker", true).length;
		for (let ent of gameState.getOwnTrainingFacilities().values())
		{
			for (let item of ent.trainingQueue())
			{
				if (!item.metadata || !item.metadata.role || item.metadata.role != "worker")
					continue;
				workers += item.count;
			}
		}
		this.turnCache.accountedWorkers = workers;
	}
	return this.turnCache.accountedWorkers;
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
	this.canBarter = gameState.getOwnEntitiesByClass("BarterMarket", true).filter(API3.Filters.isBuilt()).hasEntities();
	// TODO find a better way to update
	if (this.currentPhase != gameState.currentPhase())
	{
		if (this.Config.debug > 0)
			API3.warn(" civ " + gameState.getPlayerCiv() + " has phasedUp from " + this.currentPhase +
			          " to " + gameState.currentPhase() + " at time " + gameState.ai.elapsedTime +
				  " phasing " + this.phasing);
		this.currentPhase = gameState.currentPhase();

		// In principle, this.phasing should be already reset to 0 when starting the research
		// but this does not work in case of an autoResearch tech
		if (this.phasing)
			this.phasing = 0;
	}

/*	if (this.Config.debug > 1)
	{
		gameState.getOwnUnits().forEach (function (ent) {
			if (!ent.position())
				return;
			m.dumpEntity(ent);
		});
	} */

	this.checkEvents(gameState, events);
	this.navalManager.checkEvents(gameState, queues, events);

	if (this.phasing)
		this.checkPhaseRequirements(gameState, queues);
	else
		this.researchManager.checkPhase(gameState, queues);

	if (this.numActiveBases() > 0)
	{
		if (gameState.ai.playedTurn % 4 == 0)
			this.trainMoreWorkers(gameState, queues);

		if (gameState.ai.playedTurn % 4 == 1)
			this.buildMoreHouses(gameState, queues);

		if ((!this.saveResources || this.canBarter) && gameState.ai.playedTurn % 4 == 2)
			this.buildFarmstead(gameState, queues);

		if (this.needCorral && gameState.ai.playedTurn % 4 == 3)
			this.manageCorral(gameState, queues);

		if (!queues.minorTech.hasQueuedUnits() && gameState.ai.playedTurn % 5 == 1)
			this.researchManager.update(gameState, queues);
	}

	if (this.numPotentialBases() < 1 ||
	    this.canExpand && gameState.ai.playedTurn % 10 == 7 && this.currentPhase > 1)
		this.checkBaseExpansion(gameState, queues);

	if (this.currentPhase > 1 && gameState.ai.playedTurn % 3 == 0)
	{
		if (!this.canBarter)
			this.buildMarket(gameState, queues);

		if (!this.saveResources)
		{
			this.buildBlacksmith(gameState, queues);
			this.buildTemple(gameState, queues);
		}

		if (gameState.ai.playedTurn % 30 == 0 &&
		    gameState.getPopulation() > 0.9 * gameState.getPopulationMax())
			this.buildWonder(gameState, queues, false);
	}

	this.tradeManager.update(gameState, events, queues);

	this.garrisonManager.update(gameState, events);
	this.defenseManager.update(gameState, events);

	if (gameState.ai.playedTurn % 3 == 0)
	{
		this.constructTrainingBuildings(gameState, queues);
		if (this.Config.difficulty > 0)
			this.buildDefenses(gameState, queues);
	}

	this.assignGatherers();
	let nbBases = this.baseManagers.length;
	let activeBase;	// We will loop only on 1 active base per turn
	do
	{
		this.currentBase %= this.baseManagers.length;
		activeBase = this.baseManagers[this.currentBase++].update(gameState, queues, events);
		--nbBases;
// TODO what to do with this.reassignTerritories(this.baseManagers[this.currentBase]);
	}
	while (!activeBase && nbBases != 0);

	this.navalManager.update(gameState, queues, events);

	if (this.Config.difficulty > 0 && (this.numActiveBases() > 0 || !this.canBuildUnits))
		this.attackManager.update(gameState, queues, events);

	this.diplomacyManager.update(gameState, events);

	this.victoryManager.update(gameState, events, queues);

	// We update the capture strength at the end as it can change attack orders
	if (gameState.ai.elapsedTime - this.capturableTargetsTime > 3)
		this.updateCaptureStrength(gameState);

	Engine.ProfileStop();
};

m.HQ.prototype.Serialize = function()
{
	let properties = {
		"phasing": this.phasing,
		"currentBase": this.currentBase,
		"lastFailedGather": this.lastFailedGather,
		"firstBaseConfig": this.firstBaseConfig,
		"supportRatio": this.supportRatio,
		"targetNumWorkers": this.targetNumWorkers,
		"fortStartTime": this.fortStartTime,
		"towerStartTime": this.towerStartTime,
		"fortressStartTime": this.fortressStartTime,
		"bAdvanced": this.bAdvanced,
		"saveResources": this.saveResources,
		"saveSpace": this.saveSpace,
		"needCorral": this.needCorral,
		"needFarm": this.needFarm,
		"needFish": this.needFish,
		"maxFields": this.maxFields,
		"canExpand": this.canExpand,
		"canBuildUnits": this.canBuildUnits,
		"navalMap": this.navalMap,
		"landRegions": this.landRegions,
		"navalRegions": this.navalRegions,
		"decayingStructures": this.decayingStructures,
		"capturableTargets": this.capturableTargets,
		"capturableTargetsTime": this.capturableTargetsTime
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
		API3.warn(" buildManager " + uneval(this.buildManager.Serialize()));
		API3.warn(" defenseManager " + uneval(this.defenseManager.Serialize()));
		API3.warn(" tradeManager " + uneval(this.tradeManager.Serialize()));
		API3.warn(" navalManager " + uneval(this.navalManager.Serialize()));
		API3.warn(" researchManager " + uneval(this.researchManager.Serialize()));
		API3.warn(" diplomacyManager " + uneval(this.diplomacyManager.Serialize()));
		API3.warn(" garrisonManager " + uneval(this.garrisonManager.Serialize()));
		API3.warn(" victoryManager " + uneval(this.victoryManager.Serialize()));
	}

	return {
		"properties": properties,

		"baseManagers": baseManagers,
		"attackManager": this.attackManager.Serialize(),
		"buildManager": this.buildManager.Serialize(),
		"defenseManager": this.defenseManager.Serialize(),
		"tradeManager": this.tradeManager.Serialize(),
		"navalManager": this.navalManager.Serialize(),
		"researchManager": this.researchManager.Serialize(),
		"diplomacyManager": this.diplomacyManager.Serialize(),
		"garrisonManager": this.garrisonManager.Serialize(),
		"victoryManager": this.victoryManager.Serialize(),
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

	this.buildManager = new m.BuildManager();
	this.buildManager.Deserialize(data.buildManager);

	this.defenseManager = new m.DefenseManager(this.Config);
	this.defenseManager.Deserialize(gameState, data.defenseManager);

	this.tradeManager = new m.TradeManager(this.Config);
	this.tradeManager.init(gameState);
	this.tradeManager.Deserialize(gameState, data.tradeManager);

	this.researchManager = new m.ResearchManager(this.Config);
	this.researchManager.Deserialize(data.researchManager);

	this.diplomacyManager = new m.DiplomacyManager(this.Config);
	this.diplomacyManager.Deserialize(data.diplomacyManager);

	this.garrisonManager = new m.GarrisonManager(this.Config);
	this.garrisonManager.Deserialize(data.garrisonManager);

	this.victoryManager = new m.VictoryManager(this.Config);
	this.victoryManager.Deserialize(data.victoryManager);
};

return m;

}(PETRA);
