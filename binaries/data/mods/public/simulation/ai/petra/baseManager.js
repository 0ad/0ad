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

m.BaseManager = function(Config)
{
	this.Config = Config;
	this.ID = m.playerGlobals[PlayerID].uniqueIDBases++;
	
	// anchor building: seen as the main building of the base. Needs to have territorial influence
	this.anchor = undefined;
	this.accessIndex = undefined;
	
	// Maximum distance (from any dropsite) to look for resources
	// 3 areas are used: from 0 to max/4, from max/4 to max/2 and from max/2 to max 
	this.maxDistResourceSquare = 360*360;
	
	this.constructing = false;
	
	// vector for iterating, to check one use the HQ map.
	this.territoryIndices = [];
};

m.BaseManager.prototype.init = function(gameState, unconstructed)
{
	this.constructing = unconstructed;
	// entitycollections
	this.units = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "base", this.ID));
	this.workers = this.units.filter(API3.Filters.byMetadata(PlayerID,"role","worker"));
	this.buildings = gameState.getOwnStructures().filter(API3.Filters.byMetadata(PlayerID, "base", this.ID));	

	this.units.allowQuickIter();
	this.workers.allowQuickIter();
	this.buildings.allowQuickIter();
	
	this.units.registerUpdates();
	this.workers.registerUpdates();
	this.buildings.registerUpdates();
	
	// array of entity IDs, with each being
	this.dropsites = {};
	this.dropsiteSupplies = {};
	this.gatherers = {};
	for (var type of this.Config.resources)
	{
		this.dropsiteSupplies[type] = {"nearby": [], "medium": [], "faraway": []};
		this.gatherers[type] = {"nextCheck": 0, "used": 0, "lost": 0};
	}
};

m.BaseManager.prototype.assignEntity = function(unit)
{
	unit.setMetadata(PlayerID, "base", this.ID);
	this.units.updateEnt(unit);
	this.workers.updateEnt(unit);
	this.buildings.updateEnt(unit);
};

m.BaseManager.prototype.setAnchor = function(gameState, anchorEntity)
{
	if (!anchorEntity.hasClass("Structure") || !anchorEntity.hasTerritoryInfluence())
	{
		warn("Error: Petra base " + this.ID + " has been assigned an anchor building that has no territorial influence. Please report this on the forum.")
		return false;
	}
	this.anchor = anchorEntity;
	this.anchor.setMetadata(PlayerID, "base", this.ID);
	this.anchor.setMetadata(PlayerID, "baseAnchor", true);
	this.buildings.updateEnt(this.anchor);
	this.accessIndex = gameState.ai.accessibility.getAccessValue(this.anchor.position());
	return true;
};

m.BaseManager.prototype.checkEvents = function (gameState, events, queues)
{
	var renameEvents = events["EntityRenamed"];
	var destEvents = events["Destroy"];
	var createEvents = events["Create"];
	var cFinishedEvents = events["ConstructionFinished"];

	for (var evt of renameEvents)
	{
		var ent = gameState.getEntityById(evt.newentity);
		if (!ent)
			continue;
		var workerObject = ent.getMetadata(PlayerID, "worker-object");
		if (workerObject)
			workerObject.ent = ent;
	}

	for (var evt of destEvents)
	{
		// let's check we haven't lost an important building here.
		if (evt && !evt.SuccessfulFoundation && evt.entityObj != undefined && evt.metadata !== undefined && evt.metadata[PlayerID] &&
			evt.metadata[PlayerID]["base"] !== undefined && evt.metadata[PlayerID]["base"] == this.ID)
		{
			var ent = evt.entityObj;
			if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
				this.removeDropsite(gameState, ent);
			if (evt.metadata[PlayerID]["baseAnchor"] && evt.metadata[PlayerID]["baseAnchor"] == true)
			{
				// sounds like we lost our anchor. Let's try rebuilding it.
				// TODO: currently the HQ manager sets us as initgathering, we probably ouht to do it
				this.anchor = undefined;

				this.constructing = true;	// let's switch mode.
				this.workers.forEach( function (worker) { worker.stopMoving(); });
				queues.civilCentre.addItem(new m.ConstructionPlan(gameState, gameState.ai.HQ.bBase[0], { "base": this.ID, "baseAnchor": true }, ent.position()));
			}
			
		}
	}

	for (var evt of cFinishedEvents)
	{
		if (!evt || !evt.newentity)
			continue;
		var ent = gameState.getEntityById(evt.newentity);
		if (ent === undefined)
			continue;
		if (evt.newentity === evt.entity)  // repaired building
			continue;
			
		if (ent.getMetadata(PlayerID,"base") == this.ID)
		{
			if (ent.resourceDropsiteTypes() && !ent.hasClass("Elephant"))
				this.assignResourceToDropsite(gameState, ent);
		}
	}

	for (var evt of createEvents)
	{
		if (!evt || !evt.entity)
			continue;
		var ent = gameState.getEntityById(evt.entity);
		if (ent === undefined)
			continue;

		// do necessary stuff here
	}
};

/**
 * Assign the resources around the dropsites of this basis in three areas according to distance, and sort them in each area.
 * Moving resources (animals) and buildable resources (fields) are treated elsewhere.
 */
m.BaseManager.prototype.assignResourceToDropsite = function (gameState, dropsite)
{
	if (this.dropsites[dropsite.id()])
	{
		if (this.Config.debug)
			warn("assignResourceToDropsite: dropsite already in the list. Should never happen");
		return;
	}
	this.dropsites[dropsite.id()] = true;

	var self = this;
	for (var type of dropsite.resourceDropsiteTypes())
	{
		var resources = gameState.getResourceSupplies(type);
		if (resources.length === 0)
			continue;

		var nearby = this.dropsiteSupplies[type]["nearby"];
		var medium = this.dropsiteSupplies[type]["medium"];
		var faraway = this.dropsiteSupplies[type]["faraway"];

		resources.forEach(function(supply)
		{
			if (!supply.position())
				return;
			if (supply.getMetadata(PlayerID, "inaccessible") === true)
				return;
			if (supply.hasClass("Animal"))    // moving resources are treated differently TODO
				return;
			if (supply.hasClass("Field"))     // fields are treated separately
				return;
			if (supply.resourceSupplyType()["generic"] === "treasure")  // treasures are treated separately
				return;
			// quick accessibility check
			var access = supply.getMetadata(PlayerID, "access");
			if (!access)
			{
				access = gameState.ai.accessibility.getAccessValue(supply.position());
				supply.setMetadata(PlayerID, "access", access);
			}
			if (access !== self.accessIndex)
				return;

			var dist = API3.SquareVectorDistance(supply.position(), dropsite.position());
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
		for (var i = 0; i < supply.length; ++i)
		{
			// exhausted resource, remove it from this list
			if (!supply[i].ent || !gameState.getEntityById(supply[i].id))
				supply.splice(i--, 1);
			// resource assigned to the removed dropsite, remove it
			else if (supply[i].dropsite === entId)
				supply.splice(i--, 1);
		}
	};

	for (var type in this.dropsiteSupplies)
	{
		removeSupply(ent.id(), this.dropsiteSupplies[type]["nearby"]);
		removeSupply(ent.id(), this.dropsiteSupplies[type]["medium"]);
		removeSupply(ent.id(), this.dropsiteSupplies[type]["faraway"]);
	}

	this.dropsites[ent.id()] = undefined;
	return;
};

// Returns the position of the best place to build a new dropsite for the specified resource 
// TODO check dropsites of other bases ... may-be better to do a simultaneous liste of dp and foundations
m.BaseManager.prototype.findBestDropsiteLocation = function(gameState, resource)
{
	
	var template = gameState.getTemplate(gameState.applyCiv("structures/{civ}_storehouse"));

	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.

	var obstructions = m.createObstructionMap(gameState, this.accessIndex, template);
	obstructions.expandInfluences();
	
	var DPFoundations = gameState.getOwnFoundations().filter(API3.Filters.byType(gameState.applyCiv("foundation|structures/{civ}_storehouse")));

	var ccEnts = gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre")).toEntityArray();

	var width = obstructions.width;
	var bestIdx = undefined;
	var bestVal = undefined;
	var radius = Math.ceil(template.obstructionRadius() / gameState.cellSize);

	for (var p = 0; p < this.territoryIndices.length; ++p)
	{
		var j = this.territoryIndices[p];
		if (obstructions.map[j] <= radius)  // check room around
			continue;
		
		// we add 3 times the needed resource and once the other two (not food)
		var total = 0;
		for (var i in gameState.sharedScript.resourceMaps)
		{
			if (i === "food")
				continue;
			total += gameState.sharedScript.resourceMaps[i].map[j];
			if (i === resource)
				total += 2*gameState.sharedScript.resourceMaps[i].map[j];
		}

		total = 0.7*total;   // Just a normalisation factor as the locateMap is limited to 255

		var pos = [j%width+0.5, Math.floor(j/width)+0.5];
		pos = [gameState.cellSize*pos[0], gameState.cellSize*pos[1]];
		for (var i in this.dropsites)
		{
			if (!gameState.getEntityById(i))
				continue;
			var dpPos = gameState.getEntityById(i).position();
			if (!dpPos)
				continue;
			var dist = API3.SquareVectorDistance(dpPos, pos);
			if (dist < 3600)
			{
				total = 0;
				break;
			}
			else if (dist < 6400)
				total /= 2;
		}
		if (total == 0)
			continue;

		for (var i in DPFoundations._entities)
		{
			var dpPos = gameState.getEntityById(i).position();
			if (!dpPos)
				continue;
			var dist = API3.SquareVectorDistance(dpPos, pos);
			if (dist < 3600)
			{
				total = 0;
				break;
			}
			else if (dist < 6400)
				total /= 2;
		}
		if (total == 0)
			continue;

		for (var cc of ccEnts)
		{
			var ccPos = cc.position();
			if (!ccPos)
				continue;
			var dist = API3.SquareVectorDistance(ccPos, pos);
			if (dist < 3600)
			{
				total = 0;
				break;
			}
			else if (dist < 6400)
				total /= 2;
		}
		if (total == 0)
			continue;

		if (bestVal !== undefined && total < bestVal)
			continue;
		bestVal = total;
		bestIdx = j;
	}

	if (this.Config.debug == 2)
		warn(" for dropsite best is " + bestVal);

	if (bestVal <= 0)
		return {"quality": bestVal, "pos": [0, 0]};
	var x = ((bestIdx % width) + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx / width) + 0.5) * gameState.cellSize;
	return {"quality": bestVal, "pos": [x, z]};
};

m.BaseManager.prototype.getResourceLevel = function (gameState, type)
{
	var count = 0;
	var check = {};
	var nearby = this.dropsiteSupplies[type]["nearby"];
	for (var supply of nearby)
	{
		if (check[supply.id])    // avoid double counting as same resource can appear several time
			continue;
		check[supply.id] = true;
		count += supply.ent.resourceSupplyAmount();
	}
	var medium = this.dropsiteSupplies[type]["medium"];
	for (var supply of medium)
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
			var count = this.getResourceLevel(gameState, type);  // TODO animals are not accounted, may-be we should
			var numFarms = gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_field"), true);
			var numFound = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_field"), true);
			var numQueue = queues.field.countQueuedUnits();

			// TODO  if not yet farms, add a check on time used/lost and build farmstead if needed
			if (count < 1200 && numFarms + numFound + numQueue === 0)     // tell the queue manager we'll be trying to build fields shortly.
			{
				for (var  i = 0; i < this.Config.Economy.initialFields; ++i)
				{
					var plan = new m.ConstructionPlan(gameState, "structures/{civ}_field", { "base" : this.ID });
					plan.isGo = function() { return false; };	// don't start right away.
					queues.field.addItem(plan);
				}
			}
			else if (count < 400 && numFarms + numFound === 0)
			{
				for (var i in queues.field.queue)
					queues.field.queue[i].isGo = function() { return true; };	// start them
			}
			else if (gameState.ai.HQ.canBuild(gameState, "structures/{civ}_field"))	// let's see if we need to add new farms.
			{
				if ((!gameState.ai.HQ.saveResources && numFound < 2 && numFound + numQueue < 3) ||
					(gameState.ai.HQ.saveResources && numFound < 1 && numFound + numQueue < 2))
					queues.field.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_field", { "base" : this.ID }));
			}
		}
		else if (queues.dropsites.length() === 0 && gameState.countFoundationsByType(gameState.applyCiv("structures/{civ}_storehouse"), true) === 0)
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
							queues.dropsites.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse", { "base": this.ID }, newDP.pos));
						else if (gameState.countFoundationsByType(gameState.ai.HQ.bBase[0], true) === 0 && queues.civilCentre.length() === 0)
						{
							// No good dropsite, try to build a new base if no base already planned,
							// and if not possible, be less strict on dropsite quality
							if (!gameState.ai.HQ.buildNewBase(gameState, queues, type) && newDP.quality > Math.min(25, 50*0.15/ratio)
								&& gameState.ai.HQ.canBuild(gameState, "structures/{civ}_storehouse"))
								queues.dropsites.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse", { "base": this.ID }, newDP.pos));
						}
					}
					this.gatherers[type].nextCheck = gameState.ai.playedTurn + 20;
					this.gatherers[type].used = 0;
					this.gatherers[type].lost = 0;
				}
				else if (total === 0)
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

// let's return the estimated gather rates.
m.BaseManager.prototype.getGatherRates = function(gameState, currentRates)
{
	for (var i in currentRates)
	{
		// I calculate the exact gathering rate for each unit.
		// I must then lower that to account for travel time.
		// Given that the faster you gather, the more travel time matters,
		// I use some logarithms.
		// TODO: this should take into account for unit speed and/or distance to target
		
		var units = this.gatherersByType(gameState, i);
		units.forEach(function (ent) {
			var gRate = ent.currentGatherRate();
			if (gRate !== undefined)
				currentRates[i] += Math.log(1+gRate)/1.1;
		});
		if (i === "food")
		{
			units = this.workers.filter(API3.Filters.byMetadata(PlayerID, "subrole", "hunter"));
			units.forEach(function (ent) {
				if (ent.isIdle())
					return;
				var gRate = ent.currentGatherRate()
				if (gRate !== undefined)
					currentRates[i] += Math.log(1+gRate)/1.1;
			});
			units = this.workers.filter(API3.Filters.byMetadata(PlayerID, "subrole", "fisher"));
			units.forEach(function (ent) {
				if (ent.isIdle())
					return;
				var gRate = ent.currentGatherRate()
				if (gRate !== undefined)
					currentRates[i] += Math.log(1+gRate)/1.1;
			});
		}
		currentRates[i] += 0.5*m.GetTCRessGatherer(gameState, i);
	}
};

m.BaseManager.prototype.assignRolelessUnits = function(gameState)
{
	// TODO: make this cleverer.
	var roleless = this.units.filter(API3.Filters.not(API3.Filters.byHasMetadata(PlayerID, "role")));
	var self = this;
	roleless.forEach(function(ent) {
		if (ent.hasClass("Worker") || ent.hasClass("CitizenSoldier") || ent.hasClass("FishingBoat"))
			ent.setMetadata(PlayerID, "role", "worker");
		else if (ent.hasClass("Support") && ent.hasClass("Elephant"))
			ent.setMetadata(PlayerID, "role", "worker");
	});
};

// If the numbers of workers on the resources is unbalanced then set some of workers to idle so
// they can be reassigned by reassignIdleWorkers.
// TODO: actually this probably should be in the HQ.
m.BaseManager.prototype.setWorkersIdleByPriority = function(gameState)
{
	if (gameState.currentPhase() < 2)
		return;

	// change resource only towards one which is more needed, and if changing will not change this order
	var nb = 1;    // no more than 1 change per turn (otherwise we should update the rates)
	var mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
	var sumWanted = 0;
	var sumCurrent = 0;
	for (var need of mostNeeded)
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
				this.gatherersByType(gameState, lessNeed.type).forEach( function (ent) {
					if (nb === 0)
						return;
					--nb;
					ent.stopMoving();
					ent.setMetadata(PlayerID, "gather-type", moreNeed.type);
					m.AddTCRessGatherer(gameState, moreNeed.type);
				});
				if (nb === 0)
					return;
			}
		}
	}
};

// TODO: work on this.
m.BaseManager.prototype.reassignIdleWorkers = function(gameState)
{
	// Search for idle workers, and tell them to gather resources based on demand
	var filter = API3.Filters.or(API3.Filters.byMetadata(PlayerID,"subrole","idle"), API3.Filters.not(API3.Filters.byHasMetadata(PlayerID,"subrole")));
	var idleWorkers = gameState.updatingCollection("idle-workers-base-" + this.ID, filter, this.workers);

	var self = this;
	if (idleWorkers.length)
	{
		idleWorkers.forEach(function(ent)
		{
			// Check that the worker isn't garrisoned
			if (ent.position() === undefined)
				return;
			// Support elephant can only be builders
			if (ent.hasClass("Support") && ent.hasClass("Elephant"))
			{
				ent.setMetadata(PlayerID, "subrole", "idle");
				return;
			}

			if (ent.hasClass("Worker"))
			{
				if (self.anchor && self.anchor.needsRepair() === true)
					ent.repair(self.anchor);
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
						m.AddTCRessGatherer(gameState, needed.type);
						break;
					}
				}
			}
			else if (ent.hasClass("Cavalry"))
				ent.setMetadata(PlayerID, "subrole", "hunter");
			else if (ent.hasClass("FishingBoat"))
				ent.setMetadata(PlayerID, "subrole", "fisher");
		});
	}
};

m.BaseManager.prototype.workersBySubrole = function(gameState, subrole)
{
	return gameState.updatingCollection("subrole-" + subrole +"-base-" + this.ID, API3.Filters.byMetadata(PlayerID, "subrole", subrole), this.workers, true);
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
		if (ent.getMetadata(PlayerID, "plan") === -2 || ent.getMetadata(PlayerID, "plan") === -3)
			return false;
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			return false;
		if (ent.hasClass("Cavalry") || ent.hasClass("Ship") || ent.position() === undefined)
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
	}).toEntityArray();
	
	// Check if nothing to build
	if (!foundations.length && !damagedBuildings.length){
		return;
	}
	var workers = this.workers.filter(API3.Filters.not(API3.Filters.or(API3.Filters.byClass("Cavalry"), API3.Filters.byClass("Ship"))));
	var builderWorkers = this.workersBySubrole(gameState, "builder");
	var idleBuilderWorkers = this.workersBySubrole(gameState, "builder").filter(API3.Filters.isIdle());

	// if we're constructing and we have the foundations to our base anchor, only try building that.
	if (this.constructing == true && this.buildings.filter(API3.Filters.and(API3.Filters.isFoundation(), API3.Filters.byMetadata(PlayerID, "baseAnchor", true))).length !== 0)
	{
		foundations = this.buildings.filter(API3.Filters.byMetadata(PlayerID, "baseAnchor", true)).toEntityArray();
		var tID = foundations[0].id();
		workers.forEach(function (ent) {
			var target = ent.getMetadata(PlayerID, "target-foundation");
			if (target && target != tID)
			{
				ent.stopMoving();
				ent.setMetadata(PlayerID, "target-foundation", tID);
			}
		});
	}

	if (workers.length < 2)
	{
		var noobs = gameState.ai.HQ.bulkPickWorkers(gameState, this.ID, 2);
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
	var addedWorkers = 0;
	
	var maxTotalBuilders = Math.ceil(workers.length * 0.2);
	if (this.constructing == true && maxTotalBuilders < 15)
		maxTotalBuilders = 15;
	
	for (var target of foundations)
	{
		if (target.hasClass("Field"))
			continue; // we do not build fields

		if (gameState.ai.HQ.isDangerousLocation(target.position()))
			if (!target.hasClass("CivCentre") && !target.hasClass("StoneWall"))
				continue;

		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target.id()).length;
		var targetNB = this.Config.Economy.targetNumBuilders;	// TODO: dynamic that.
		if (target.hasClass("House") || target.hasClass("Market"))
			targetNB *= 2;
		else if (target.hasClass("Barracks") || target.hasClass("Tower"))
			targetNB = 4;
		else if (target.hasClass("Fortress"))
			targetNB = 7;
		if (target.getMetadata(PlayerID, "baseAnchor") == true || (target.hasClass("Wonder") && gameState.getGameType() === "wonder"))
			targetNB = 15;

		if (assigned < targetNB)
		{
			if (builderWorkers.length - idleBuilderWorkers.length + addedWorkers < maxTotalBuilders) {

				var addedToThis = 0;
				
				idleBuilderWorkers.forEach(function(ent) {
					if (ent.position() && API3.SquareVectorDistance(ent.position(), target.position()) < 10000 && assigned + addedToThis < targetNB)
					{
						addedWorkers++;
						addedToThis++;
						ent.setMetadata(PlayerID, "target-foundation", target.id());
					}
				});
				if (assigned + addedToThis < targetNB)
				{
					var nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata(PlayerID, "subrole") !== "builder" && ent.position() !== undefined); }).toEntityArray();
					var time = target.buildTime();
					nonBuilderWorkers.sort(function (workerA,workerB)
					{
						var coeffA = API3.SquareVectorDistance(target.position(),workerA.position());
						// elephant moves slowly, so when far away they are only useful if build time is long
						if (workerA.hasClass("Elephant"))
							coeffA *= 0.5 * (1 + (Math.sqrt(coeffA)/150)*(30/time));
						else if (workerA.getMetadata(PlayerID, "gather-type") === "food")
							coeffA *= 3;
						var coeffB = API3.SquareVectorDistance(target.position(),workerB.position());
						if (workerB.hasClass("Elephant"))
							coeffB *= 0.5 * (1 + (Math.sqrt(coeffB)/150)*(30/time));
						else if (workerB.getMetadata(PlayerID, "gather-type") === "food")
							coeffB *= 3;
						return (coeffA - coeffB);						
					});
					var current = 0;
					while (assigned + addedToThis < targetNB && current < nonBuilderWorkers.length)
					{
						addedWorkers++;
						addedToThis++;
						var ent = nonBuilderWorkers[current++];
						ent.stopMoving();
						ent.setMetadata(PlayerID, "subrole", "builder");
						ent.setMetadata(PlayerID, "target-foundation", target.id());
					};
				}
			}
		}
	}

	// don't repair if we're still under attack, unless it's like a vital (civcentre or wall) building that's getting destroyed.
	for (var target of damagedBuildings)
	{
		if (gameState.ai.HQ.isDangerousLocation(target.position()))
			if (target.healthLevel() > 0.5 || (!target.hasClass("CivCentre") && !target.hasClass("StoneWall")))
				continue;
		else if (noRepair && !target.hasClass("CivCentre"))
			continue;
		
		if (target.decaying())
			continue;
		
		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target.id()).length;
		if (assigned < targetNB/3)
		{
			if (builderWorkers.length + addedWorkers < targetNB*2)
			{	
				var nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata(PlayerID, "subrole") !== "builder" && ent.position() !== undefined && ent.getMetadata(PlayerID, "transport") === undefined); });
				var nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), targetNB/3 - assigned);
				
				nearestNonBuilders.forEach(function(ent) {
					ent.stopMoving();
					addedWorkers++;
					ent.setMetadata(PlayerID, "subrole", "builder");
					ent.setMetadata(PlayerID, "target-foundation", target.id());
				});
			}
		}
	}
};

m.BaseManager.prototype.update = function(gameState, queues, events)
{
	if (this.anchor && this.anchor.getMetadata(PlayerID, "access") !== this.accessIndex)
		warn("Petra baseManager problem with accessIndex " + this.accessIndex
			+ " while metadata acess is " + this.anchor.getMetadata(PlayerID, "access"));

	Engine.ProfileStart("Base update - base " + this.ID);

	this.checkResourceLevels(gameState, queues);
	this.assignToFoundations(gameState);

	if (this.constructing && this.anchor)
	{
		var owner = gameState.ai.HQ.territoryMap.getOwner(this.anchor.position());
		if(owner !== 0 && !gameState.isPlayerAlly(owner))
		{
			// we're in enemy territory. If we're too close from the enemy, destroy us.
			var eEnts = gameState.getEnemyStructures().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
			for (var i in eEnts)
			{
				var entPos = eEnts[i].position();
				if (API3.SquareVectorDistance(entPos, this.anchor.position()) < 8000)
					this.anchor.destroy();
			}
		}
	}

	if (gameState.ai.playedTurn % 2 === 0)
		this.setWorkersIdleByPriority(gameState);

	this.assignRolelessUnits(gameState);
	
	// should probably be last to avoid reallocations of units that would have done stuffs otherwise.
	this.reassignIdleWorkers(gameState);

	// TODO: do this incrementally a la defense.js
	var self = this;
	this.workers.forEach(function(ent) {
		if (!ent.getMetadata(PlayerID, "worker-object"))
			ent.setMetadata(PlayerID, "worker-object", new m.Worker(ent));
		ent.getMetadata(PlayerID, "worker-object").update(self, gameState);
	});

	Engine.ProfileStop();
};

return m;

}(PETRA);
