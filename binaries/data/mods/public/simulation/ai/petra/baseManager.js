var PETRA = function(m)
{
/* Base Manager
 * Handles lower level economic stuffs.
 * Some tasks:
	-tasking workers: gathering/hunting/building/repairing?/scouting/plans.
	-giving feedback/estimates on GR
	-achieving building stuff plans (scouting/getting ressource/building) or other long-staying plans, if I ever get any.
	-getting good spots for dropsites
	-managing dropsite use in the base
		> warning HQ if we'll need more space
	-updating whatever needs updating, keeping track of stuffs (rebuilding needs…)
 */

m.BaseManager = function(gameState, Config)
{
	this.Config = Config;
	this.ID = gameState.ai.uniqueIDs.bases++;

	// anchor building: seen as the main building of the base. Needs to have territorial influence
	this.anchor = undefined;
	this.anchorId = undefined;
	this.accessIndex = undefined;

	// Maximum distance (from any dropsite) to look for resources
	// 3 areas are used: from 0 to max/4, from max/4 to max/2 and from max/2 to max 
	this.maxDistResourceSquare = 360*360;

	this.constructing = false;
	// Defenders to train in this cc when its construction is finished 
	this.neededDefenders = ((this.Config.difficulty > 2) ? 3 + 2*(this.Config.difficulty - 3) : 0);

	// vector for iterating, to check one use the HQ map.
	this.territoryIndices = [];
};

m.BaseManager.prototype.init = function(gameState, state)
{
	if (state == "unconstructed")
		this.constructing = true;
	else if (state != "captured")
		this.neededDefenders = 0;
	this.workerObject = new m.Worker(this);
	// entitycollections
	this.units = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "base", this.ID));
	this.workers = this.units.filter(API3.Filters.byMetadata(PlayerID,"role","worker"));
	this.buildings = gameState.getOwnStructures().filter(API3.Filters.byMetadata(PlayerID, "base", this.ID));	

	this.units.registerUpdates();
	this.workers.registerUpdates();
	this.buildings.registerUpdates();
	
	// array of entity IDs, with each being
	this.dropsites = {};
	this.dropsiteSupplies = {};
	this.gatherers = {};
	for (let type of this.Config.resources)
	{
		this.dropsiteSupplies[type] = {"nearby": [], "medium": [], "faraway": []};
		this.gatherers[type] = {"nextCheck": 0, "used": 0, "lost": 0};
	}
};

m.BaseManager.prototype.assignEntity = function(gameState, ent)
{
	ent.setMetadata(PlayerID, "base", this.ID);
	this.units.updateEnt(ent);
	this.workers.updateEnt(ent);
	this.buildings.updateEnt(ent);
	if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
		this.assignResourceToDropsite(gameState, ent);
};

m.BaseManager.prototype.setAnchor = function(gameState, anchorEntity)
{
	if (!anchorEntity.hasClass("Structure") || !anchorEntity.hasTerritoryInfluence())
	{
		warn("Error: Petra base " + this.ID + " has been assigned an anchor building that has no territorial influence. Please report this on the forum.");
		return false;
	}
	this.anchor = anchorEntity;
	this.anchorId = anchorEntity.id();
	this.anchor.setMetadata(PlayerID, "base", this.ID);
	this.anchor.setMetadata(PlayerID, "baseAnchor", true);
	this.buildings.updateEnt(this.anchor);
	this.accessIndex = gameState.ai.accessibility.getAccessValue(this.anchor.position());
	// in case some of our other bases were destroyed, reaffect these destroyed bases to this base
	for (let base of gameState.ai.HQ.baseManagers)
	{
		if (base.anchor || base.newbaseID)
			continue;
		base.newbaseID = this.ID;
	}
	return true;
};

m.BaseManager.prototype.checkEvents = function (gameState, events, queues)
{
	for (let evt of events.Destroy)
	{
		// let's check we haven't lost an important building here.
		if (evt && !evt.SuccessfulFoundation && evt.entityObj && evt.metadata && evt.metadata[PlayerID] &&
			evt.metadata[PlayerID].base && evt.metadata[PlayerID].base == this.ID)
		{
			let ent = evt.entityObj;
			if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
				this.removeDropsite(gameState, ent);
			if (evt.metadata[PlayerID].baseAnchor && evt.metadata[PlayerID].baseAnchor === true)
				this.anchorLost(gameState, ent);
		}
	}

	for (let evt of events.OwnershipChanged)	// capture event
	{
		if (evt.from !== PlayerID)
			continue;
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || ent.getMetadata(PlayerID, "base") !== this.ID)
			continue;
		if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
			this.removeDropsite(gameState, ent);
		if (ent.getMetadata(PlayerID, "baseAnchor") === true)
			this.anchorLost(gameState, ent);
	}

	for (let evt of events.ConstructionFinished)
	{
		if (!evt || !evt.newentity)
			continue;
		let ent = gameState.getEntityById(evt.newentity);
		if (!ent)
			continue;
		if (evt.newentity == evt.entity)  // repaired building
			continue;
			
		if (ent.getMetadata(PlayerID, "base") == this.ID)
			if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
				this.assignResourceToDropsite(gameState, ent);
	}

	for (let evt of events.Create)
	{
		if (!evt || !evt.entity)
			continue;
		let ent = gameState.getEntityById(evt.entity);
		if (!ent)
			continue;
		// do necessary stuff here
	}

	for (let evt of events.EntityRenamed)
	{
		if (!this.anchorId || this.anchorId !== evt.entity)
			continue;
		this.anchorId = evt.newentity;
		this.anchor = gameState.getEntityById(evt.newentity);
	}
};

/* we lost our anchor. Let's reaffect our units and buildings */
m.BaseManager.prototype.anchorLost = function (gameState, ent)
{
	this.anchor = undefined;
	this.anchorId = undefined;
	this.neededDefenders = 0;
	let bestbase = m.getBestBase(gameState, ent);
	this.newbaseID = bestbase.ID;
	for (let entity of this.units.values())
		bestbase.assignEntity(gameState, entity);
	for (let entity of this.buildings.values())
		bestbase.assignEntity(gameState, entity);
	gameState.ai.HQ.updateTerritories(gameState);
};

/**
 * Assign the resources around the dropsites of this basis in three areas according to distance, and sort them in each area.
 * Moving resources (animals) and buildable resources (fields) are treated elsewhere.
 */
m.BaseManager.prototype.assignResourceToDropsite = function (gameState, dropsite)
{
	if (this.dropsites[dropsite.id()])
	{
		if (this.Config.debug > 0)
			warn("assignResourceToDropsite: dropsite already in the list. Should never happen");
		return;
	}
	this.dropsites[dropsite.id()] = true;

	var self = this;
	let dropsitePos = dropsite.position();
	let accessIndex = this.accessIndex;
	if (this.ID === gameState.ai.HQ.baseManagers[0].ID)
	{
		accessIndex = dropsite.getMetadata(PlayerID, "access");
		if (!accessIndex)
		{
			accessIndex = gameState.ai.accessibility.getAccessValue(dropsitePos);
			dropsite.setMetadata(PlayerID, "access", accessIndex);
		}
	}

	for (var type of dropsite.resourceDropsiteTypes())
	{
		var resources = gameState.getResourceSupplies(type);
		if (!resources.length)
			continue;

		var nearby = this.dropsiteSupplies[type].nearby;
		var medium = this.dropsiteSupplies[type].medium;
		var faraway = this.dropsiteSupplies[type].faraway;

		resources.forEach(function(supply)
		{
			if (!supply.position())
				return;
			if (supply.hasClass("Animal"))    // moving resources are treated differently
				return;
			if (supply.hasClass("Field"))     // fields are treated separately
				return;
			if (supply.resourceSupplyType().generic === "treasure")  // treasures are treated separately
				return;
			// quick accessibility check
			let access = supply.getMetadata(PlayerID, "access");
			if (!access)
			{
				access = gameState.ai.accessibility.getAccessValue(supply.position());
				supply.setMetadata(PlayerID, "access", access);
			}
			if (access !== accessIndex)
				return;

			let dist = API3.SquareVectorDistance(supply.position(), dropsitePos);
			if (dist < self.maxDistResourceSquare)
			{
				if (dist < self.maxDistResourceSquare/16)        // distmax/4
				    nearby.push({ "dropsite": dropsite.id(), "id": supply.id(), "ent": supply, "dist": dist }); 
				else if (dist < self.maxDistResourceSquare/4)    // distmax/2
					medium.push({ "dropsite": dropsite.id(), "id": supply.id(), "ent": supply, "dist": dist });
				else
					faraway.push({ "dropsite": dropsite.id(), "id": supply.id(), "ent": supply, "dist": dist });
			}
		});

		nearby.sort(function(r1, r2) { return (r1.dist - r2.dist);});
		medium.sort(function(r1, r2) { return (r1.dist - r2.dist);});
		faraway.sort(function(r1, r2) { return (r1.dist - r2.dist);});

/*		var debug = false;
		if (debug)
		{
			faraway.forEach(function(res){ 
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [res.ent.id()], "rgb": [2,0,0]});
			});
			medium.forEach(function(res){ 
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [res.ent.id()], "rgb": [0,2,0]});
			});
			nearby.forEach(function(res){ 
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [res.ent.id()], "rgb": [0,0,2]});
			});
		} */
	}
};

// completely remove the dropsite resources from our list.
m.BaseManager.prototype.removeDropsite = function (gameState, ent)
{
	if (!ent.id())
		return;

	var removeSupply = function(entId, supply){
		for (let i = 0; i < supply.length; ++i)
		{
			// exhausted resource, remove it from this list
			if (!supply[i].ent || !gameState.getEntityById(supply[i].id))
				supply.splice(i--, 1);
			// resource assigned to the removed dropsite, remove it
			else if (supply[i].dropsite == entId)
				supply.splice(i--, 1);
		}
	};

	for (let type in this.dropsiteSupplies)
	{
		removeSupply(ent.id(), this.dropsiteSupplies[type].nearby);
		removeSupply(ent.id(), this.dropsiteSupplies[type].medium);
		removeSupply(ent.id(), this.dropsiteSupplies[type].faraway);
	}

	this.dropsites[ent.id()] = undefined;
	return;
};

// Returns the position of the best place to build a new dropsite for the specified resource 
// TODO check dropsites of other bases ... may-be better to do a simultaneous liste of dp and foundations
m.BaseManager.prototype.findBestDropsiteLocation = function(gameState, resource)
{
	
	var template = gameState.getTemplate(gameState.applyCiv("structures/{civ}_storehouse"));
	var halfSize = 0;
	if (template.get("Footprint/Square"))
		halfSize = Math.max(+template.get("Footprint/Square/@depth"), +template.get("Footprint/Square/@width")) / 2;
	else if (template.get("Footprint/Circle"))
		halfSize = +template.get("Footprint/Circle/@radius");

	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.

	var obstructions = m.createObstructionMap(gameState, this.accessIndex, template);

	var dpEnts = gameState.getOwnEntitiesByClass("Storehouse", true).toEntityArray();
	var ccEnts = gameState.getOwnEntitiesByClass("CivCentre", true).toEntityArray();

	var bestIdx;
	var bestVal = 0;
	var radius = Math.ceil(template.obstructionRadius() / obstructions.cellSize);

	var territoryMap = gameState.ai.HQ.territoryMap;
	var width = territoryMap.width;
	var cellSize = territoryMap.cellSize;
    
	for (let j of this.territoryIndices)
	{
		var i = territoryMap.getNonObstructedTile(j, radius, obstructions);
		if (i < 0)  // no room around
			continue;

		// we add 3 times the needed resource and once the other two (not food)
		var total = 0;
		for (let res in gameState.sharedScript.resourceMaps)
		{
			if (res === "food")
				continue;
			total += gameState.sharedScript.resourceMaps[res].map[j];
			if (res === resource)
				total += 2*gameState.sharedScript.resourceMaps[res].map[j];
		}

		total = 0.7*total;   // Just a normalisation factor as the locateMap is limited to 255
		if (total <= bestVal)
			continue;

		var pos = [cellSize * (j%width+0.5), cellSize * (Math.floor(j/width)+0.5)];

		for (let dp of dpEnts)
		{
			let dpPos = dp.position();
			if (!dpPos)
				continue;
			let dist = API3.SquareVectorDistance(dpPos, pos);
			if (dist < 3600)
			{
				total = 0;
				break;
			}
			else if (dist < 6400)
				total *= (Math.sqrt(dist)-60)/20;
		}
		if (total <= bestVal)
			continue;

		for (let cc of ccEnts)
		{
			let ccPos = cc.position();
			if (!ccPos)
				continue;
			let dist = API3.SquareVectorDistance(ccPos, pos);
			if (dist < 3600)
			{
				total = 0;
				break;
			}
			else if (dist < 6400)
				total *= (Math.sqrt(dist)-60)/20;
		}
		if (total <= bestVal)
			continue;
		if (gameState.ai.HQ.isDangerousLocation(gameState, pos, halfSize))
			continue;
		bestVal = total;
		bestIdx = i;
	}

	if (this.Config.debug > 2)
		warn(" for dropsite best is " + bestVal);

	if (bestVal <= 0)
		return {"quality": bestVal, "pos": [0, 0]};

	var x = (bestIdx % obstructions.width + 0.5) * obstructions.cellSize;
	var z = (Math.floor(bestIdx / obstructions.width) + 0.5) * obstructions.cellSize;
	return {"quality": bestVal, "pos": [x, z]};
};

m.BaseManager.prototype.getResourceLevel = function (gameState, type, nearbyOnly = false)
{
	var count = 0;
	var check = {};
	var nearby = this.dropsiteSupplies[type]["nearby"];
	for (let supply of nearby)
	{
		if (check[supply.id])    // avoid double counting as same resource can appear several time
			continue;
		check[supply.id] = true;
		count += supply.ent.resourceSupplyAmount();
	}
	if (nearbyOnly)
		return count;
	var medium = this.dropsiteSupplies[type]["medium"];
	for (let supply of medium)
	{
		if (check[supply.id])
			continue;
		check[supply.id] = true;
		count += 0.6*supply.ent.resourceSupplyAmount();
	}
	return count;
};

// check our resource levels and react accordingly
m.BaseManager.prototype.checkResourceLevels = function (gameState, queues)
{
	for (var type of this.Config.resources)
	{
		if (type == "food")
		{
			if (gameState.ai.HQ.canBuild(gameState, "structures/{civ}_field"))	// let's see if we need to add new farms.
			{
				var count = this.getResourceLevel(gameState, type, (gameState.currentPhase() > 1));  // animals are not accounted
				var numFarms = gameState.getOwnStructures().filter(API3.Filters.byClass("Field")).length;  // including foundations
				var numQueue = queues.field.countQueuedUnits();

				// TODO  if not yet farms, add a check on time used/lost and build farmstead if needed
				if (numFarms + numQueue === 0)	// starting game, rely on fruits as long as we have enough of them
				{
					if (count < 600)
						queues.field.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_field", { "base": this.ID }));
				}
				else
				{
					let numFound = gameState.getOwnFoundations().filter(API3.Filters.byClass("Field")).length;
					let goal = this.Config.Economy.provisionFields;
					if (gameState.ai.HQ.saveResources || gameState.ai.HQ.saveSpace || count > 300 || numFarms > 5)
						goal = Math.max(goal-1, 1);
					if (numFound + numQueue < goal)
						queues.field.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_field", { "base": this.ID }));
				}
			}
		}
		else if (queues.dropsites.length() == 0 && gameState.getOwnFoundations().filter(API3.Filters.byClass("Storehouse")).length == 0)
		{
			if (gameState.ai.playedTurn > this.gatherers[type].nextCheck)
			{
				var self = this;
				this.gatherersByType(gameState, type).forEach(function (ent) {
					if (ent.unitAIState() === "INDIVIDUAL.GATHER.GATHERING")
						++self.gatherers[type].used;
					else if (ent.unitAIState() === "INDIVIDUAL.RETURNRESOURCE.APPROACHING")
						++self.gatherers[type].lost;
				});
				// TODO  add also a test on remaining resources
				var total = this.gatherers[type].used + this.gatherers[type].lost;
				if (total > 150 || (total > 60 && type !== "wood"))
				{
					var ratio = this.gatherers[type].lost / total;
					if (ratio > 0.15)
					{
						var newDP = this.findBestDropsiteLocation(gameState, type);
						if (newDP.quality > 50 && gameState.ai.HQ.canBuild(gameState, "structures/{civ}_storehouse"))
							queues.dropsites.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse", { "base": this.ID, "type": type }, newDP.pos));
						else if (gameState.getOwnFoundations().filter(API3.Filters.byClass("CivCentre")).length == 0 && queues.civilCentre.length() == 0)
						{
							// No good dropsite, try to build a new base if no base already planned,
							// and if not possible, be less strict on dropsite quality
							if (!gameState.ai.HQ.buildNewBase(gameState, queues, type) && newDP.quality > Math.min(25, 50*0.15/ratio)
								&& gameState.ai.HQ.canBuild(gameState, "structures/{civ}_storehouse"))
								queues.dropsites.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse", { "base": this.ID, "type": type }, newDP.pos));
						}
					}
					this.gatherers[type].nextCheck = gameState.ai.playedTurn + 20;
					this.gatherers[type].used = 0;
					this.gatherers[type].lost = 0;
				}
				else if (total == 0)
					this.gatherers[type].nextCheck = gameState.ai.playedTurn + 10;
			}
		}
		else
		{
			this.gatherers[type].nextCheck = gameState.ai.playedTurn;
			this.gatherers[type].used = 0;
			this.gatherers[type].lost = 0;
		}
	}
	
};

// Adds the estimated gather rates from this base to the currentRates
m.BaseManager.prototype.getGatherRates = function(gameState, currentRates)
{
	for (let res in currentRates)
	{
		// I calculate the exact gathering rate for each unit.
		// I must then lower that to account for travel time.
		// Given that the faster you gather, the more travel time matters,
		// I use some logarithms.
		// TODO: this should take into account for unit speed and/or distance to target

		this.gatherersByType(gameState, res).forEach(function (ent) {
			if (ent.isIdle() || !ent.position())
					return;		
			let gRate = ent.currentGatherRate();
			if (gRate)
				currentRates[res] += Math.log(1+gRate)/1.1;
		});
		if (res === "food")
		{
			this.workersBySubrole(gameState, "hunter").forEach(function (ent) {
				if (ent.isIdle() || !ent.position())
					return;
				let gRate = ent.currentGatherRate();
				if (gRate)
					currentRates[res] += Math.log(1+gRate)/1.1;
			});
			this.workersBySubrole(gameState, "fisher").forEach(function (ent) {
				if (ent.isIdle() || !ent.position())
					return;
				let gRate = ent.currentGatherRate();
				if (gRate)
					currentRates[res] += Math.log(1+gRate)/1.1;
			});
		}
	}
};

m.BaseManager.prototype.assignRolelessUnits = function(gameState, roleless)
{
	if (!roleless)
		roleless = this.units.filter(API3.Filters.not(API3.Filters.byHasMetadata(PlayerID, "role"))).values();

	for (let ent of roleless)
	{
		if (ent.hasClass("Worker") || ent.hasClass("CitizenSoldier") || ent.hasClass("FishingBoat"))
			ent.setMetadata(PlayerID, "role", "worker");
		else if (ent.hasClass("Support") && ent.hasClass("Elephant"))
			ent.setMetadata(PlayerID, "role", "worker");
	}
};

// If the numbers of workers on the resources is unbalanced then set some of workers to idle so
// they can be reassigned by reassignIdleWorkers.
// TODO: actually this probably should be in the HQ.
m.BaseManager.prototype.setWorkersIdleByPriority = function(gameState)
{
	// change resource only towards one which is more needed, and if changing will not change this order
	var nb = 1;    // no more than 1 change per turn (otherwise we should update the rates)
	var mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
	var sumWanted = 0;
	var sumCurrent = 0;
	for (let need of mostNeeded)
	{
		sumWanted += need.wanted;
		sumCurrent += need.current;
	}
	var scale = 1;
	if (sumWanted > 0)
		scale = sumCurrent / sumWanted;

	for (var i = mostNeeded.length-1; i > 0; --i)
	{
		var lessNeed = mostNeeded[i];
		for (var j = 0; j < i; ++j) 
		{
			var moreNeed = mostNeeded[j];
			var lastFailed = gameState.ai.HQ.lastFailedGather[moreNeed.type];
			if (lastFailed && gameState.ai.elapsedTime - lastFailed < 20)
				continue;
			// If we assume a mean rate of 0.5 per gatherer, this diff should be > 1
			// but we require a bit more to avoid too frequent changes
			if ((scale*moreNeed.wanted - moreNeed.current) - (scale*lessNeed.wanted - lessNeed.current) > 1.5)
			{
				let only;
				// in average, females are less efficient for stone and metal, and citizenSoldiers for food
				let gatherers = this.gatherersByType(gameState, lessNeed.type);
				if (lessNeed.type === "food" && gatherers.filter(API3.Filters.byClass("CitizenSoldier")).length)
					only = "CitizenSoldier";
				else if ((lessNeed.type === "stone" || lessNeed.type === "metal") && moreNeed.type !== "stone" && moreNeed.type !== "metal"
					&& gatherers.filter(API3.Filters.byClass("Female")).length) 
					only = "Female";

				gatherers.forEach( function (ent) {
					if (nb == 0)
						return;
					if (only && !ent.hasClass(only))
						return;
					--nb;
					ent.stopMoving();
					ent.setMetadata(PlayerID, "gather-type", moreNeed.type);
					gameState.ai.HQ.AddTCResGatherer(moreNeed.type);
				});
				if (nb == 0)
					return;
			}
		}
	}
};

m.BaseManager.prototype.reassignIdleWorkers = function(gameState, idleWorkers)
{
	// Search for idle workers, and tell them to gather resources based on demand
	if (!idleWorkers)
	{
		let filter = API3.Filters.byMetadata(PlayerID, "subrole", "idle");
		idleWorkers = gameState.updatingCollection("idle-workers-base-" + this.ID, filter, this.workers).values();
	}
	
	for (let ent of idleWorkers)
	{
		// Check that the worker isn't garrisoned
		if (!ent.position())
			continue;
		// Support elephant can only be builders
		if (ent.hasClass("Support") && ent.hasClass("Elephant"))
		{
			ent.setMetadata(PlayerID, "subrole", "idle");
			continue;
		}

		if (ent.hasClass("Worker"))
		{
			// Just emergency repairing here. It is better managed in assignToFoundations
			if (this.anchor && this.anchor.needsRepair() === true
				&& gameState.getOwnEntitiesByMetadata("target-foundation", this.anchor.id()).length < 2)
				ent.repair(this.anchor);
			else
			{
				var mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
				for (var needed of mostNeeded)
				{
					var lastFailed = gameState.ai.HQ.lastFailedGather[needed.type];
					if (lastFailed && gameState.ai.elapsedTime - lastFailed < 20)
						continue;
					ent.setMetadata(PlayerID, "subrole", "gatherer");
					ent.setMetadata(PlayerID, "gather-type", needed.type);
					gameState.ai.HQ.AddTCResGatherer(needed.type);
					break;
				}
			}
		}
		else if (ent.hasClass("Cavalry"))
			ent.setMetadata(PlayerID, "subrole", "hunter");
		else if (ent.hasClass("FishingBoat"))
			ent.setMetadata(PlayerID, "subrole", "fisher");
	}
};

m.BaseManager.prototype.workersBySubrole = function(gameState, subrole)
{
	return gameState.updatingCollection("subrole-" + subrole +"-base-" + this.ID, API3.Filters.byMetadata(PlayerID, "subrole", subrole), this.workers);
};

m.BaseManager.prototype.gatherersByType = function(gameState, type)
{
	return gameState.updatingCollection("workers-gathering-" + type +"-base-" + this.ID, API3.Filters.byMetadata(PlayerID, "gather-type", type), this.workersBySubrole(gameState, "gatherer"));
};


// returns an entity collection of workers.
// They are idled immediatly and their subrole set to idle.
m.BaseManager.prototype.pickBuilders = function(gameState, workers, number)
{
	var availableWorkers = this.workers.filter(function (ent) {
		if (!ent.position())
			return false;
		if (ent.getMetadata(PlayerID, "plan") == -2 || ent.getMetadata(PlayerID, "plan") == -3)
			return false;
		if (ent.getMetadata(PlayerID, "transport"))
			return false;
		if (ent.hasClass("Cavalry") || ent.hasClass("Ship"))
			return false;
		return true;
	}).toEntityArray();
	availableWorkers.sort(function (a,b) {
		var vala = 0, valb = 0;
		if (a.getMetadata(PlayerID, "subrole") == "builder")
			vala = 100;
		if (b.getMetadata(PlayerID, "subrole") == "builder")
			valb = 100;
		if (a.getMetadata(PlayerID, "subrole") == "idle")
			vala = -50;
		if (b.getMetadata(PlayerID, "subrole") == "idle")
			valb = -50;
		if (a.getMetadata(PlayerID, "plan") === undefined)
			vala = -20;
		if (b.getMetadata(PlayerID, "plan") === undefined)
			valb = -20;
		return (vala - valb);
	});
	var needed = Math.min(number, availableWorkers.length - 3);
	for (var i = 0; i < needed; ++i)
	{
		availableWorkers[i].stopMoving();
		availableWorkers[i].setMetadata(PlayerID, "subrole", "idle");
		workers.addEnt(availableWorkers[i]);
	}
	return;
};

m.BaseManager.prototype.assignToFoundations = function(gameState, noRepair)
{
	// If we have some foundations, and we don't have enough builder-workers,
	// try reassigning some other workers who are nearby	
	// AI tries to use builders sensibly, not completely stopping its econ.
	
	var self = this;
	
	// TODO: this is not perfect performance-wise.
	var foundations = this.buildings.filter(API3.Filters.and(API3.Filters.isFoundation(),API3.Filters.not(API3.Filters.byClass("Field")))).toEntityArray();
	
	var damagedBuildings = this.buildings.filter(function (ent) {
		if (ent.foundationProgress() === undefined && ent.needsRepair())
			return true;
		return false;
	});
	
	// Check if nothing to build
	if (!foundations.length && !damagedBuildings.length){
		return;
	}
	var workers = this.workers.filter(API3.Filters.not(API3.Filters.or(API3.Filters.byClass("Cavalry"), API3.Filters.byClass("Ship"))));
	var builderWorkers = this.workersBySubrole(gameState, "builder");
	var idleBuilderWorkers = builderWorkers.filter(API3.Filters.isIdle());

	// if we're constructing and we have the foundations to our base anchor, only try building that.
	if (this.constructing == true && this.buildings.filter(API3.Filters.and(API3.Filters.isFoundation(), API3.Filters.byMetadata(PlayerID, "baseAnchor", true))).length != 0)
	{
		foundations = this.buildings.filter(API3.Filters.byMetadata(PlayerID, "baseAnchor", true)).toEntityArray();
		var tID = foundations[0].id();
		workers.forEach(function (ent) {
			let target = ent.getMetadata(PlayerID, "target-foundation");
			if (target && target != tID)
			{
				ent.stopMoving();
				ent.setMetadata(PlayerID, "target-foundation", tID);
			}
		});
	}

	if (workers.length < 2)
	{
		var noobs = gameState.ai.HQ.bulkPickWorkers(gameState, this, 2);
		if(noobs)
		{
			noobs.forEach(function (worker) {
				worker.setMetadata(PlayerID, "base", self.ID);
				worker.setMetadata(PlayerID, "subrole", "builder");
				workers.updateEnt(worker);
				builderWorkers.updateEnt(worker);
				idleBuilderWorkers.updateEnt(worker);
			});
		}
	}

	var builderTot = builderWorkers.length - idleBuilderWorkers.length;	
	
	for (var target of foundations)
	{
		if (target.hasClass("Field"))
			continue; // we do not build fields

		if (gameState.ai.HQ.isNearInvadingArmy(target.position()))
			if (!target.hasClass("CivCentre") && !target.hasClass("StoneWall") && (!target.hasClass("Wonder") || gameState.getGameType() !== "wonder"))
				continue;

		// if our territory has shrinked since this foundation was positioned, do not build it
		if (m.isNotWorthBuilding(gameState, target))
			continue;

		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target.id()).length;
		var maxTotalBuilders = Math.ceil(workers.length * 0.2);
		if (target.hasClass("House") && gameState.getPopulationLimit() < (gameState.getPopulation() + 5)
			&& gameState.getPopulationLimit() < gameState.getPopulationMax())
			maxTotalBuilders = maxTotalBuilders + 2;
		var targetNB = 2;
		if (target.hasClass("House") || target.hasClass("DropsiteWood"))
			targetNB = 3;
		else if (target.hasClass("Barracks") || target.hasClass("DefenseTower") || target.hasClass("Market"))
			targetNB = 4;
		else if (target.hasClass("Fortress"))
			targetNB = 7;
		if (target.getMetadata(PlayerID, "baseAnchor") == true || (target.hasClass("Wonder") && gameState.getGameType() === "wonder"))
		{
			targetNB = 15;
			maxTotalBuilders = Math.max(maxTotalBuilders, 15);
		}
		// if no base yet, everybody should build
		if (gameState.ai.HQ.numActiveBase() === 0)
		{
			targetNB = workers.length;
			maxTotalBuilders = targetNB;
		}

		if (assigned < targetNB)
		{
			idleBuilderWorkers.forEach(function(ent) {
				if (ent.getMetadata(PlayerID, "target-foundation") !== undefined)
					return;
				if (assigned >= targetNB || !ent.position() || API3.SquareVectorDistance(ent.position(), target.position()) > 40000)
					return;
				assigned++;
				builderTot++;
				ent.setMetadata(PlayerID, "target-foundation", target.id());
			});
			if (assigned < targetNB && builderTot < maxTotalBuilders)
			{
				var nonBuilderWorkers = workers.filter(function(ent) {
					if (ent.getMetadata(PlayerID, "subrole") === "builder")
						return false;
					if (!ent.position())
						return false;
					if (ent.getMetadata(PlayerID, "plan") == -2 || ent.getMetadata(PlayerID, "plan") == -3)
						return false;
					if (ent.getMetadata(PlayerID, "transport"))
						return false;
					return true;
				}).toEntityArray();
				var time = target.buildTime();
				nonBuilderWorkers.sort(function (workerA,workerB)
				{
					let coeffA = API3.SquareVectorDistance(target.position(),workerA.position());
					// elephant moves slowly, so when far away they are only useful if build time is long
					if (workerA.hasClass("Elephant"))
						coeffA *= 0.5 * (1 + (Math.sqrt(coeffA)/150)*(30/time));
					else if (workerA.getMetadata(PlayerID, "gather-type") === "food")
						coeffA *= 3;
					let coeffB = API3.SquareVectorDistance(target.position(),workerB.position());
					if (workerB.hasClass("Elephant"))
						coeffB *= 0.5 * (1 + (Math.sqrt(coeffB)/150)*(30/time));
					else if (workerB.getMetadata(PlayerID, "gather-type") === "food")
						coeffB *= 3;
					return (coeffA - coeffB);						
				});
				let current = 0;
				let nonBuilderTot = nonBuilderWorkers.length;
				while (assigned < targetNB && builderTot < maxTotalBuilders && current < nonBuilderTot)
				{
					assigned++;
					builderTot++;
					var ent = nonBuilderWorkers[current++];
					ent.stopMoving();
					ent.setMetadata(PlayerID, "subrole", "builder");
					ent.setMetadata(PlayerID, "target-foundation", target.id());
				}
			}
		}
	}

	for (var target of damagedBuildings.values())
	{
		// don't repair if we're still under attack, unless it's a vital (civcentre or wall) building that's getting destroyed.
		if (gameState.ai.HQ.isNearInvadingArmy(target.position()))
			if (target.healthLevel() > 0.5 ||
				(!target.hasClass("CivCentre") && !target.hasClass("StoneWall") && (!target.hasClass("Wonder") || gameState.getGameType() !== "wonder")))
				continue;
		else if (noRepair && !target.hasClass("CivCentre"))
			continue;
		
		if (target.decaying())
			continue;
		
		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target.id()).length;
		var maxTotalBuilders = Math.ceil(workers.length * 0.2);
		var targetNB = 1;
		if (target.hasClass("Fortress"))
			targetNB = 3;
		if (target.getMetadata(PlayerID, "baseAnchor") == true || (target.hasClass("Wonder") && gameState.getGameType() === "wonder"))
		{
			maxTotalBuilders = Math.ceil(workers.length * 0.3);
			targetNB = 5;
			if (target.healthLevel() < 0.3)
			{
				maxTotalBuilders = Math.ceil(workers.length * 0.6);
				targetNB = 7;
			}

		}

		if (assigned < targetNB)
		{
			idleBuilderWorkers.forEach(function(ent) {
				if (ent.getMetadata(PlayerID, "target-foundation") !== undefined)
					return;
				if (assigned >= targetNB || !ent.position() || API3.SquareVectorDistance(ent.position(), target.position()) > 40000)
					return;
				assigned++;
				builderTot++;
				ent.setMetadata(PlayerID, "target-foundation", target.id());
			});
			if (assigned < targetNB && builderTot < maxTotalBuilders)
			{
				let nonBuilderWorkers = workers.filter(function(ent) {
					if (ent.getMetadata(PlayerID, "subrole") === "builder")
						return false;
					if (!ent.position())
						return false;
					if (ent.getMetadata(PlayerID, "plan") == -2 || ent.getMetadata(PlayerID, "plan") == -3)
						return false;
					if (ent.getMetadata(PlayerID, "transport"))
						return false;
					return true;
				});
				let num = Math.min(nonBuilderWorkers.length, targetNB-assigned);
				let nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), num);
				
				nearestNonBuilders.forEach(function(ent) {
					assigned++;
					builderTot++;
					ent.stopMoving();
					ent.setMetadata(PlayerID, "subrole", "builder");
					ent.setMetadata(PlayerID, "target-foundation", target.id());
				});
			}
		}
	}
};

m.BaseManager.prototype.update = function(gameState, queues, events)
{
	if (this.ID === gameState.ai.HQ.baseManagers[0].ID)	// base for unaffected units
	{
		// if some active base, reassigns the workers/buildings
		// otherwise look for anything useful to do, i.e. treasures to gather
		if (gameState.ai.HQ.numActiveBase() > 0)
		{
			for (var ent of this.units.values())
				m.getBestBase(gameState, ent).assignEntity(gameState, ent);
			for (var ent of this.buildings.values())
			{
				if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
					this.removeDropsite(gameState, ent);
				m.getBestBase(gameState, ent).assignEntity(gameState, ent);
			}
		}
		else if (gameState.ai.HQ.canBuildUnits)
		{
			this.assignToFoundations(gameState);
			if (gameState.ai.playedTurn % 4 === 0)
				this.setWorkersIdleByPriority(gameState);
			this.assignRolelessUnits(gameState);
			this.reassignIdleWorkers(gameState);
			for (let ent of this.workers.values())
				this.workerObject.update(gameState, ent);
		}
		return;
	}

	if (!this.anchor)   // this base has been destroyed
	{
		// transfer possible remaining units (probably they were in training during previous transfers)
		if (this.newbaseID)
		{
			var newbaseID = this.newbaseID;
			for (let ent of this.units.values())
				ent.setMetadata(PlayerID, "base", newbaseID);
			for (let ent of this.buildings.values())
				ent.setMetadata(PlayerID, "base", newbaseID);
		}
		return;
	}

	if (this.anchor.getMetadata(PlayerID, "access") != this.accessIndex)
		API3.warn("Petra baseManager " + this.ID + " problem with accessIndex " + this.accessIndex
			+ " while metadata access is " + this.anchor.getMetadata(PlayerID, "access"));

	Engine.ProfileStart("Base update - base " + this.ID);

	this.checkResourceLevels(gameState, queues);
	this.assignToFoundations(gameState);

	if (this.constructing)
	{
		var owner = gameState.ai.HQ.territoryMap.getOwner(this.anchor.position());
		if(owner !== 0 && !gameState.isPlayerAlly(owner))
		{
			// we're in enemy territory. If we're too close from the enemy, destroy us.
			let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
			for (let cc of ccEnts.values())
			{
				if (cc.owner() !== owner)
					continue;
				if (API3.SquareVectorDistance(cc.position(), this.anchor.position()) > 8000)
					continue;
				this.anchor.destroy();
				break;
			}
		}
	}
	else if (this.neededDefenders && gameState.ai.HQ.trainEmergencyUnits(gameState, [this.anchor.position()]))
		--this.neededDefenders;

	if (gameState.ai.playedTurn % 2 === 0 && gameState.currentPhase() > 1)
		this.setWorkersIdleByPriority(gameState);

	this.assignRolelessUnits(gameState);
	this.reassignIdleWorkers(gameState);
	// check if workers can find something useful to do
	for (let ent of this.workers.values())
		this.workerObject.update(gameState, ent);

	Engine.ProfileStop();
};

m.BaseManager.prototype.Serialize = function()
{
	return {
		"ID": this.ID,
		"anchorId": this.anchorId,
		"accessIndex": this.accessIndex,
		"maxDistResourceSquare": this.maxDistResourceSquare,
		"constructing": this.constructing,
		"gatherers": this.gatherers,
		"neededDefenders": this.neededDefenders,
		"territoryIndices": this.territoryIndices
	};
};

m.BaseManager.prototype.Deserialize = function(gameState, data)
{
	for (let key in data)
		this[key] = data[key];

	this.anchor = ((this.anchorId !== undefined) ? gameState.getEntityById(this.anchorId) : undefined);
};

return m;

}(PETRA);
