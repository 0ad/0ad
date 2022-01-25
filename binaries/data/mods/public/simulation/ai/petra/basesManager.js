/**
 * Bases Manager
 * Manages the list of available bases and queries information from those (e.g. resource levels).
 * Only one base is run every turn.
 */

PETRA.BasesManager = function(Config)
{
	this.Config = Config;

	this.currentBase = 0;

	// Cache some quantities for performance.
	this.turnCache = {};

	// Deals with unit/structure without base.
	this.noBase = undefined;

	this.baseManagers = [];
};

PETRA.BasesManager.prototype.init = function(gameState)
{
	// Initialize base map. Each pixel is a base ID, or 0 if not or not accessible.
	this.basesMap = new API3.Map(gameState.sharedScript, "territory");

	this.noBase = new PETRA.BaseManager(gameState, this);
	this.noBase.init(gameState, PETRA.BaseManager.STATE_WITH_ANCHOR);
	this.noBase.accessIndex = 0;

	for (const cc of gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre")).values())
		if (cc.foundationProgress() === undefined)
			this.createBase(gameState, cc, PETRA.BaseManager.STATE_WITH_ANCHOR);
		else
			this.createBase(gameState, cc, PETRA.BaseManager.STATE_UNCONSTRUCTED);
};

/**
 * Initialization needed after deserialization (only called when deserialising).
 */
PETRA.BasesManager.prototype.postinit = function(gameState)
{
	// Rebuild the base maps from the territory indices of each base.
	this.basesMap = new API3.Map(gameState.sharedScript, "territory");
	for (const base of this.baseManagers)
		for (const j of base.territoryIndices)
			this.basesMap.map[j] = base.ID;

	for (const ent of gameState.getOwnEntities().values())
	{
		if (!ent.resourceDropsiteTypes() || !ent.hasClass("Structure"))
			continue;
		// Entities which have been built or have changed ownership after the last AI turn have no base.
		// they will be dealt with in the next checkEvents
		const baseID = ent.getMetadata(PlayerID, "base");
		if (baseID === undefined)
			continue;
		const base = this.getBaseByID(baseID);
		base.assignResourceToDropsite(gameState, ent);
	}
};

/**
 * Create a new base in the baseManager:
 * If an existing one without anchor already exist, use it.
 * Otherwise create a new one.
 * TODO when buildings, criteria should depend on distance
 */
PETRA.BasesManager.prototype.createBase = function(gameState, ent, type = PETRA.BaseManager.STATE_WITH_ANCHOR)
{
	const access = PETRA.getLandAccess(gameState, ent);
	let newbase;
	for (const base of this.baseManagers)
	{
		if (base.accessIndex != access)
			continue;
		if (type !== PETRA.BaseManager.STATE_ANCHORLESS && base.anchor)
			continue;
		if (type !== PETRA.BaseManager.STATE_ANCHORLESS)
		{
			// TODO we keep the first one, we should rather use the nearest if buildings
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
		API3.warn(" BasesManager createBase entrance avec access " + access + " and type " + type);
		API3.warn(" with access " + uneval(this.baseManagers.map(base => base.accessIndex)) +
			  " and base nbr " + uneval(this.baseManagers.map(base => base.ID)) +
			  " and anchor " + uneval(this.baseManagers.map(base => !!base.anchor)));
	}

	if (!newbase)
	{
		newbase = new PETRA.BaseManager(gameState, this);
		newbase.init(gameState, type);
		this.baseManagers.push(newbase);
	}
	else
		newbase.reset(type);

	if (type !== PETRA.BaseManager.STATE_ANCHORLESS)
		newbase.setAnchor(gameState, ent);
	else
		newbase.setAnchorlessEntity(gameState, ent);

	return newbase;
};

/** TODO check if the new anchorless bases should be added to addBase */
PETRA.BasesManager.prototype.checkEvents = function(gameState, events)
{
	let addBase = false;

	for (const evt of events.Destroy)
	{
		// Let's check we haven't lost an important building here.
		if (evt && !evt.SuccessfulFoundation && evt.entityObj && evt.metadata && evt.metadata[PlayerID] &&
			evt.metadata[PlayerID].base)
		{
			const ent = evt.entityObj;
			if (evt?.metadata?.[PlayerID]?.assignedResource)
				this.getBaseByID(evt.metadata[PlayerID].base).removeFromAssignedDropsite(ent);
			if (ent.owner() != PlayerID)
				continue;
			// A new base foundation was created and destroyed on the same (AI) turn
			if (evt.metadata[PlayerID].base == -1 || evt.metadata[PlayerID].base == -2)
				continue;
			const base = this.getBaseByID(evt.metadata[PlayerID].base);
			if (ent.resourceDropsiteTypes() && ent.hasClass("Structure"))
				base.removeDropsite(gameState, ent);
			if (evt.metadata[PlayerID].baseAnchor && evt.metadata[PlayerID].baseAnchor === true)
				base.anchorLost(gameState, ent);
		}
	}

	for (const evt of events.EntityRenamed)
	{
		const ent = gameState.getEntityById(evt.newentity);
		if (!ent || ent.owner() != PlayerID || ent.getMetadata(PlayerID, "base") === undefined)
			continue;
		const base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
		if (!base.anchorId || base.anchorId != evt.entity)
			continue;
		base.anchorId = evt.newentity;
		base.anchor = ent;
	}

	for (const evt of events.Create)
	{
		// Let's check if we have a valuable foundation needing builders quickly
		// (normal foundations are taken care in baseManager.assignToFoundations)
		const ent = gameState.getEntityById(evt.entity);
		if (!ent || ent.owner() != PlayerID || ent.foundationProgress() === undefined)
			continue;

		if (ent.getMetadata(PlayerID, "base") == -1)	// Standard base around a cc
		{
			// Okay so let's try to create a new base around this.
			const newbase = this.createBase(gameState, ent, PETRA.BaseManager.STATE_UNCONSTRUCTED);
			// Let's get a few units from other bases there to build this.
			const builders = this.bulkPickWorkers(gameState, newbase, 10);
			if (builders !== false)
			{
				builders.forEach(worker => {
					worker.setMetadata(PlayerID, "base", newbase.ID);
					worker.setMetadata(PlayerID, "subrole", PETRA.Worker.SUBROLE_BUILDER);
					worker.setMetadata(PlayerID, "target-foundation", ent.id());
				});
			}
		}
		else if (ent.getMetadata(PlayerID, "base") == -2)	// anchorless base around a dock
		{
			const newbase = this.createBase(gameState, ent, PETRA.BaseManager.STATE_ANCHORLESS);
			// Let's get a few units from other bases there to build this.
			const builders = this.bulkPickWorkers(gameState, newbase, 4);
			if (builders != false)
			{
				builders.forEach(worker => {
					worker.setMetadata(PlayerID, "base", newbase.ID);
					worker.setMetadata(PlayerID, "subrole", PETRA.Worker.SUBROLE_BUILDER);
					worker.setMetadata(PlayerID, "target-foundation", ent.id());
				});
			}
		}
	}

	for (const evt of events.ConstructionFinished)
	{
		if (evt.newentity == evt.entity)  // repaired building
			continue;
		const ent = gameState.getEntityById(evt.newentity);
		if (!ent || ent.owner() != PlayerID)
			continue;
		if (ent.getMetadata(PlayerID, "base") === undefined)
			continue;
		const base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
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

	for (const evt of events.OwnershipChanged)
	{
		if (evt.from == PlayerID)
		{
			const ent = gameState.getEntityById(evt.entity);
			if (!ent || ent.getMetadata(PlayerID, "base") === undefined)
				continue;
			const base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
			if (ent.resourceDropsiteTypes() && ent.hasClass("Structure"))
				base.removeDropsite(gameState, ent);
			if (ent.getMetadata(PlayerID, "baseAnchor") === true)
				base.anchorLost(gameState, ent);
		}

		if (evt.to != PlayerID)
			continue;
		const ent = gameState.getEntityById(evt.entity);
		if (!ent)
			continue;
		if (ent.hasClass("Unit"))
		{
			PETRA.getBestBase(gameState, ent).assignEntity(gameState, ent);
			continue;
		}
		if (ent.hasClass("CivCentre"))   // build a new base around it
		{
			let newbase;
			if (ent.foundationProgress() !== undefined)
				newbase = this.createBase(gameState, ent, PETRA.BaseManager.STATE_UNCONSTRUCTED);
			else
			{
				newbase = this.createBase(gameState, ent, PETRA.BaseManager.STATE_CAPTURED);
				addBase = true;
			}
			newbase.assignEntity(gameState, ent);
		}
		else
		{
			let base;
			// If dropsite on new island, create a base around it
			if (!ent.decaying() && ent.resourceDropsiteTypes())
				base = this.createBase(gameState, ent, PETRA.BaseManager.STATE_ANCHORLESS);
			else
				base = PETRA.getBestBase(gameState, ent) || this.noBase;
			base.assignEntity(gameState, ent);
		}
	}

	for (const evt of events.TrainingFinished)
	{
		for (const entId of evt.entities)
		{
			const ent = gameState.getEntityById(entId);
			if (!ent || !ent.isOwn(PlayerID))
				continue;

			// Assign it immediately to something useful to do.
			if (ent.getMetadata(PlayerID, "role") === PETRA.Worker.ROLE_WORKER)
			{
				let base;
				if (ent.getMetadata(PlayerID, "base") === undefined)
				{
					base = PETRA.getBestBase(gameState, ent);
					base.assignEntity(gameState, ent);
				}
				else
					base = this.getBaseByID(ent.getMetadata(PlayerID, "base"));
				base.reassignIdleWorkers(gameState, [ent]);
				base.workerObject.update(gameState, ent);
			}
			else if (ent.resourceSupplyType() && ent.position())
			{
				const type = ent.resourceSupplyType();
				if (!type.generic)
					continue;
				const dropsites = gameState.getOwnDropsites(type.generic);
				const pos = ent.position();
				const access = PETRA.getLandAccess(gameState, ent);
				let distmin = Math.min();
				let goal;
				for (const dropsite of dropsites.values())
				{
					if (!dropsite.position() || PETRA.getLandAccess(gameState, dropsite) != access)
						continue;
					const dist = API3.SquareVectorDistance(pos, dropsite.position());
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

	if (addBase)
		gameState.ai.HQ.handleNewBase(gameState);
};

/**
 * returns an entity collection of workers through BaseManager.pickBuilders
 * TODO: when same accessIndex, sort by distance
 */
PETRA.BasesManager.prototype.bulkPickWorkers = function(gameState, baseRef, number)
{
	const accessIndex = baseRef.accessIndex;
	if (!accessIndex)
		return false;
	const baseBest = this.baseManagers.slice();
	// We can also use workers without a base.
	baseBest.push(this.noBase);
	baseBest.sort((a, b) => {
		if (a.accessIndex == accessIndex && b.accessIndex != accessIndex)
			return -1;
		else if (b.accessIndex == accessIndex && a.accessIndex != accessIndex)
			return 1;
		return 0;
	});

	let needed = number;
	const workers = new API3.EntityCollection(gameState.sharedScript);
	for (const base of baseBest)
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

/**
 * @return {Object} - Resources (estimation) still gatherable in our territory.
 */
PETRA.BasesManager.prototype.getTotalResourceLevel = function(gameState, resources = Resources.GetCodes(), proximity = ["nearby", "medium"])
{
	const total = {};
	for (const res of resources)
		total[res] = 0;
	for (const base of this.baseManagers)
		for (const res in total)
			total[res] += base.getResourceLevel(gameState, res, proximity);

	return total;
};

/**
 * Returns the current gather rate
 * This is not per-se exact, it performs a few adjustments ad-hoc to account for travel distance, stuffs like that.
 */
PETRA.BasesManager.prototype.GetCurrentGatherRates = function(gameState)
{
	if (!this.turnCache.currentRates)
	{
		const currentRates = {};
		for (const res of Resources.GetCodes())
			currentRates[res] = 0.5 * this.GetTCResGatherer(res);

		this.addGatherRates(gameState, currentRates);

		for (const res of Resources.GetCodes())
			currentRates[res] = Math.max(currentRates[res], 0);

		this.turnCache.currentRates = currentRates;
	}

	return this.turnCache.currentRates;
};

/** Some functions that register that we assigned a gatherer to a resource this turn */

/** Add a gatherer to the turn cache for this supply. */
PETRA.BasesManager.prototype.AddTCGatherer = function(supplyID)
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

/** Remove a gatherer from the turn cache for this supply. */
PETRA.BasesManager.prototype.RemoveTCGatherer = function(supplyID)
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

PETRA.BasesManager.prototype.GetTCGatherer = function(supplyID)
{
	if (this.turnCache.resourceGatherer && this.turnCache.resourceGatherer[supplyID])
		return this.turnCache.resourceGatherer[supplyID];

	return 0;
};

/** The next two are to register that we assigned a gatherer to a resource this turn. */
PETRA.BasesManager.prototype.AddTCResGatherer = function(resource)
{
	const check = "resourceGatherer-" + resource;
	if (this.turnCache[check])
		++this.turnCache[check];
	else
		this.turnCache[check] = 1;

	if (this.turnCache.currentRates)
		this.turnCache.currentRates[resource] += 0.5;
};

PETRA.BasesManager.prototype.GetTCResGatherer = function(resource)
{
	const check = "resourceGatherer-" + resource;
	if (this.turnCache[check])
		return this.turnCache[check];

	return 0;
};

/**
 * flag a resource as exhausted
 */
PETRA.BasesManager.prototype.isResourceExhausted = function(resource)
{
	const check = "exhausted-" + resource;
	if (this.turnCache[check] == undefined)
		this.turnCache[check] = this.basesManager.isResourceExhausted(resource);

	return this.turnCache[check];
};

/**
 * returns the number of bases with a cc
 * ActiveBases includes only those with a built cc
 * PotentialBases includes also those with a cc in construction
 */
PETRA.BasesManager.prototype.numActiveBases = function()
{
	if (!this.turnCache.base)
		this.updateBaseCache();
	return this.turnCache.base.active;
};

PETRA.BasesManager.prototype.hasActiveBase = function()
{
	return !!this.numActiveBases();
};

PETRA.BasesManager.prototype.numPotentialBases = function()
{
	if (!this.turnCache.base)
		this.updateBaseCache();
	return this.turnCache.base.potential;
};

PETRA.BasesManager.prototype.hasPotentialBase = function()
{
	return !!this.numPotentialBases();
};

/**
 * Updates the number of active and potential bases.
 *		.potential {number} - Bases that may or may not still be a foundation.
 *		.active {number} - Usable bases.
 */
PETRA.BasesManager.prototype.updateBaseCache = function()
{
	this.turnCache.base = { "active": 0, "potential": 0 };
	for (const base of this.baseManagers)
	{
		if (!base.anchor)
			continue;
		++this.turnCache.base.potential;
		if (base.anchor.foundationProgress() === undefined)
			++this.turnCache.base.active;
	}
};

PETRA.BasesManager.prototype.resetBaseCache = function()
{
	this.turnCache.base = undefined;
};

PETRA.BasesManager.prototype.baselessBase = function()
{
	return this.noBase;
};

/**
 * @param {number} baseID
 * @return {Object} - The base corresponding to baseID.
 */
PETRA.BasesManager.prototype.getBaseByID = function(baseID)
{
	if (this.noBase.ID === baseID)
		return this.noBase;
	return this.baseManagers.find(base => base.ID === baseID);
};

/**
 * flag a resource as exhausted
 */
PETRA.BasesManager.prototype.isResourceExhausted = function(resource)
{
	return this.baseManagers.every(base =>
		!base.dropsiteSupplies[resource].nearby.length &&
		!base.dropsiteSupplies[resource].medium.length &&
		!base.dropsiteSupplies[resource].faraway.length);
};

/**
 * Count gatherers returning resources in the number of gatherers of resourceSupplies
 * to prevent the AI always reassigning idle workers to these resourceSupplies (specially in naval maps).
 */
PETRA.BasesManager.prototype.assignGatherers = function()
{
	for (const base of this.baseManagers)
		for (const worker of base.workers.values())
		{
			if (worker.unitAIState().split(".").indexOf("RETURNRESOURCE") === -1)
				continue;
			const orders = worker.unitAIOrderData();
			if (orders.length < 2 || !orders[1].target || orders[1].target != worker.getMetadata(PlayerID, "supply"))
				continue;
			this.AddTCGatherer(orders[1].target);
		}
};

/**
 * Assign an entity to the closest base.
 * Used by the starting strategy.
 */
PETRA.BasesManager.prototype.assignEntity = function(gameState, ent, territoryIndex)
{
	let bestbase;
	for (const base of this.baseManagers)
	{
		if ((!ent.getMetadata(PlayerID, "base") || ent.getMetadata(PlayerID, "base") != base.ID) &&
		    base.territoryIndices.indexOf(territoryIndex) == -1)
			continue;
		base.assignEntity(gameState, ent);
		bestbase = base;
		break;
	}
	if (!bestbase)	// entity outside our territory
	{
		if (ent.hasClass("Structure") && !ent.decaying() && ent.resourceDropsiteTypes())
			bestbase = this.createBase(gameState, ent, PETRA.BaseManager.STATE_ANCHORLESS);
		else
			bestbase = PETRA.getBestBase(gameState, ent) || this.noBase;
		bestbase.assignEntity(gameState, ent);
	}
	// now assign entities garrisoned inside this entity
	if (ent.isGarrisonHolder() && ent.garrisoned().length)
		for (const id of ent.garrisoned())
			bestbase.assignEntity(gameState, gameState.getEntityById(id));
	// and find something useful to do if we already have a base
	if (ent.position() && bestbase.ID !== this.noBase.ID)
	{
		bestbase.assignRolelessUnits(gameState, [ent]);
		if (ent.getMetadata(PlayerID, "role") === PETRA.Worker.ROLE_WORKER)
		{
			bestbase.reassignIdleWorkers(gameState, [ent]);
			bestbase.workerObject.update(gameState, ent);
		}
	}
};

/**
 * Adds the gather rates of individual bases to a shared object.
 * @param {Object} gameState
 * @param {Object} rates - The rates to add the gather rates to.
 */
PETRA.BasesManager.prototype.addGatherRates = function(gameState, rates)
{
	for (const base of this.baseManagers)
		base.addGatherRates(gameState, rates);
};

/**
 * @param {number} territoryIndex
 * @return {number} - The ID of the base at the given territory index.
 */
PETRA.BasesManager.prototype.baseAtIndex = function(territoryIndex)
{
	return this.basesMap.map[territoryIndex];
};

/**
 * @param {number} territoryIndex
 */
PETRA.BasesManager.prototype.removeBaseFromTerritoryIndex = function(territoryIndex)
{
	const baseID = this.basesMap.map[territoryIndex];
	if (baseID == 0)
		return;
	const base = this.getBaseByID(baseID);
	if (base)
	{
		const index = base.territoryIndices.indexOf(territoryIndex);
		if (index != -1)
			base.territoryIndices.splice(index, 1);
		else
			API3.warn(" problem in headquarters::updateTerritories for base " + baseID);
	}
	else
		API3.warn(" problem in headquarters::updateTerritories without base " + baseID);
	this.basesMap.map[territoryIndex] = 0;
};

/**
 * @return {boolean} - Whether the index was added to a base.
 */
PETRA.BasesManager.prototype.addTerritoryIndexToBase = function(gameState, territoryIndex, passabilityMap)
{
	if (this.baseAtIndex(territoryIndex) != 0)
		return false;
	let landPassable = false;
	const ind = API3.getMapIndices(territoryIndex, gameState.ai.HQ.territoryMap, passabilityMap);
	let access;
	for (const k of ind)
	{
		if (!gameState.ai.HQ.landRegions[gameState.ai.accessibility.landPassMap[k]])
			continue;
		landPassable = true;
		access = gameState.ai.accessibility.landPassMap[k];
		break;
	}
	if (!landPassable)
		return false;
	let distmin = Math.min();
	let baseID;
	const pos = [gameState.ai.HQ.territoryMap.cellSize * (territoryIndex % gameState.ai.HQ.territoryMap.width + 0.5), gameState.ai.HQ.territoryMap.cellSize * (Math.floor(territoryIndex / gameState.ai.HQ.territoryMap.width) + 0.5)];
	for (const base of this.baseManagers)
	{
		if (!base.anchor || !base.anchor.position())
			continue;
		if (base.accessIndex != access)
			continue;
		const dist = API3.SquareVectorDistance(base.anchor.position(), pos);
		if (dist >= distmin)
			continue;
		distmin = dist;
		baseID = base.ID;
	}
	if (!baseID)
		return false;
	this.getBaseByID(baseID).territoryIndices.push(territoryIndex);
	this.basesMap.map[territoryIndex] = baseID;
	return true;
};

/** Reassign territories when a base is going to be deleted */
PETRA.BasesManager.prototype.reassignTerritories = function(deletedBase, territoryMap)
{
	const cellSize = territoryMap.cellSize;
	const width = territoryMap.width;
	for (let j = 0; j < territoryMap.length; ++j)
	{
		if (this.basesMap.map[j] != deletedBase.ID)
			continue;
		if (territoryMap.getOwnerIndex(j) != PlayerID)
		{
			API3.warn("Petra reassignTerritories: should never happen");
			this.basesMap.map[j] = 0;
			continue;
		}

		let distmin = Math.min();
		let baseID;
		const pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];
		for (const base of this.baseManagers)
		{
			if (!base.anchor || !base.anchor.position())
				continue;
			if (base.accessIndex != deletedBase.accessIndex)
				continue;
			const dist = API3.SquareVectorDistance(base.anchor.position(), pos);
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
 * We will loop only on one active base per turn.
 */
PETRA.BasesManager.prototype.update = function(gameState, queues, events)
{
	Engine.ProfileStart("BasesManager update");

	this.turnCache = {};
	this.assignGatherers();
	let nbBases = this.baseManagers.length;
	let activeBase = false;
	this.noBase.update(gameState, queues, events);
	while (!activeBase && nbBases != 0)
	{
		this.currentBase %= this.baseManagers.length;
		activeBase = this.baseManagers[this.currentBase++].update(gameState, queues, events);
		--nbBases;
		// TODO what to do with this.reassignTerritories(this.baseManagers[this.currentBase]);
	}

	Engine.ProfileStop();
};

PETRA.BasesManager.prototype.Serialize = function()
{
	const properties = {
		"currentBase": this.currentBase
	};

	const baseManagers = [];
	for (const base of this.baseManagers)
		baseManagers.push(base.Serialize());

	return {
		"properties": properties,
		"noBase": this.noBase.Serialize(),
		"baseManagers": baseManagers
	};
};

PETRA.BasesManager.prototype.Deserialize = function(gameState, data)
{
	for (const key in data.properties)
		this[key] = data.properties[key];

	this.noBase = new PETRA.BaseManager(gameState, this);
	this.noBase.Deserialize(gameState, data.noBase);
	this.noBase.init(gameState, PETRA.BaseManager.STATE_WITH_ANCHOR);
	this.noBase.Deserialize(gameState, data.noBase);

	this.baseManagers = [];
	for (const basedata of data.baseManagers)
	{
		// The first call to deserialize set the ID base needed by entitycollections.
		const newbase = new PETRA.BaseManager(gameState, this);
		newbase.Deserialize(gameState, basedata);
		newbase.init(gameState, PETRA.BaseManager.STATE_WITH_ANCHOR);
		newbase.Deserialize(gameState, basedata);
		this.baseManagers.push(newbase);
	}
};
