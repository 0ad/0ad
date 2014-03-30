var AEGIS = function(m)
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

m.BaseManager = function(Config) {
	this.Config = Config;
	this.farmingFields = false;
	this.ID = m.playerGlobals[PlayerID].uniqueIDBases++;
	
	// anchor building: seen as the main building of the base. Needs to have territorial influence
	this.anchor = undefined;
	// list of IDs of buildings in our base that have a "territory pusher" function.
	this.territoryBuildings = [];
	
	// will tell if we should be considered as a source of X.
	this.willGather = { "food": 0, "wood": 0, "stone":0, "metal": 0 };
	
	this.isFarming = false;
	this.isHunting = true;
	
	this.constructing = false;
	
	// vector for iterating, to check one use the HQ map.
	this.territoryIndices = [];
};

m.BaseManager.prototype.init = function(gameState, unconstructed){
	this.constructing = unconstructed;
	// entitycollections
	this.units = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "base", this.ID));
	this.buildings = gameState.getOwnStructures().filter(API3.Filters.byMetadata(PlayerID, "base", this.ID));
	
	this.workers = this.units.filter(API3.Filters.byMetadata(PlayerID,"role","worker"));

	this.workers.allowQuickIter();
	this.buildings.allowQuickIter();
	this.units.allowQuickIter();
	
	this.units.registerUpdates();
	this.buildings.registerUpdates();
	this.workers.registerUpdates();
	
	// array of entity IDs, with each being
	// { "food" : [close entities, semi-close entities, faraway entities, closeAmount, medianAmount, assignedWorkers collection, Rebuilt ] … } (one per resource)
	// note that "median amount" also counts the closeAmount.
	this.dropsites = { };
	
	// TODO: difficulty levels for this?
	
	// smallRadius is the distance necessary to mark a resource as linked to a dropsite.
	this.smallRadius = { 'food':50*50,'wood':50*50,'stone':40*40,'metal':40*40 };
	// medRadius is the maximal distance for a link, albeit one that would still make us want to build a new dropsite.
	this.medRadius = { 'food':90*90,'wood':55*55,'stone':80*80,'metal':80*80 };
	// bigRadius is the distance for a weak link, mainly for optimizing search for resources when a DP is depleted.
	this.bigRadius = { 'food':200*200,'wood':200*200,'stone':200*200,'metal':200*200 };
};

m.BaseManager.prototype.assignEntity = function(unit){
	unit.setMetadata(PlayerID, "base", this.ID);
	this.units.updateEnt(unit);
	this.workers.updateEnt(unit);
	this.buildings.updateEnt(unit);
	// TODO: immediately assign it some task?

	if (unit.hasClass("Structure") && unit.hasTerritoryInfluence() && this.territoryBuildings.indexOf(unit.id()) === -1)
		this.territoryBuildings.push(unit.id());
};

m.BaseManager.prototype.setAnchor = function(anchorEntity) {
	if (!anchorEntity.hasClass("Structure") || !anchorEntity.hasTerritoryInfluence())
	{
		warn("Error: Aegis' base " + this.ID + " has been assigned an anchor building that has no territorial influence. Please report this on the forum.")
		return false;
	}
	this.anchor = anchorEntity;
	this.anchor.setMetadata(PlayerID, "base", this.ID);
	this.anchor.setMetadata(PlayerID, "baseAnchor", true);
	this.buildings.updateEnt(this.anchor);

	if (this.territoryBuildings.indexOf(this.anchor.id()) === -1)
		this.territoryBuildings.push(this.anchor.id());
	return true;
}

// affects the HQ map.
m.BaseManager.prototype.initTerritory = function(HQ, gameState) {
	if (!this.anchor)
		warn ("Error: Aegis tried to initialize the territory of base " + this.ID + " without assigning it an anchor building first");
	var radius = Math.round((this.anchor.territoryInfluenceRadius() / 4.0) * 1.25);

	var LandSize = gameState.sharedScript.accessibility.getRegionSize(this.anchor.position());
	this.accessIndex = gameState.sharedScript.accessibility.getAccessValue(this.anchor.position());

	if (LandSize < 6500)
	{
		// We're on a small land, we'll assign all territories in the vicinity.
		// there's a slight chance we're on an elongated weird stuff, we'll just pump up a little the radius
		radius = Math.round(radius*1.2);
	}
	var x = Math.round(this.anchor.position()[0]/gameState.cellSize);
	var y = Math.round(this.anchor.position()[1]/gameState.cellSize);
	
	this.territoryIndices = [];
	
	var width = gameState.getMap().width;
	for (var xi = -radius; xi <= radius; ++xi)
		for (var yi = -radius; yi <= radius; ++yi)
		{
			if (x+xi >= width || y+yi >= width)
				continue;
			if (xi*xi+yi*yi < radius*radius && HQ.basesMap.map[(x+xi) + (y+yi)*width] === 0)
			{
				if (this.accessIndex == gameState.sharedScript.accessibility.landPassMap[x+xi + width*(y+yi)])
				{
					this.territoryIndices.push((x+xi) + (y+yi)*width);
					HQ.basesMap.map[(x+xi) + (y+yi)*width] = this.ID;
				}
			}
		}
}

m.BaseManager.prototype.initGatheringFunctions = function(HQ, gameState, specTypes) {
	// init our gathering functions.
	var types = ["food","wood","stone","metal"];
	if (specTypes !== undefined)
		type = specTypes;
	
	var self = this;
	var count = 0;
	
	for (var i in types)
	{
		var type = types[i];
		// TODO: set us as "X" gatherer
		
		this.buildings.filter(API3.Filters.isDropsite(type)).forEach(function(ent) { self.initializeDropsite(gameState, ent,type) });

		if (this.getResourceLevel(gameState, type, "all") > 1000)
			this.willGather[type] = 1;
	}
	if (this.willGather["food"] === 0)
	{
		var needFarm = true;
		// Let's check again for food
		for (var base in HQ.baseManagers)
			if (HQ.baseManagers[base].willGather["food"] === 1)
				needFarm = false;
		if (needFarm)
			this.willGather["food"] = 1;
	}
	m.debug ("food" + this.willGather["food"]);
	m.debug (this.willGather["wood"]);
	m.debug (this.willGather["stone"]);
	m.debug (this.willGather["metal"]);
}

m.BaseManager.prototype.checkEvents = function (gameState, events, queues) {
	var renameEvents = events["EntityRenamed"];
	var destEvents = events["Destroy"];
	var createEvents = events["Create"];
	var cFinishedEvents = events["ConstructionFinished"];

	for (var i in renameEvents)
	{
		var ent = gameState.getEntityById(renameEvents[i].newentity);
		if (!ent)
			continue;
		var workerObject = ent.getMetadata(PlayerID, "worker-object");
		if (workerObject)
			workerObject.ent = ent;
	}

	for (var i in destEvents)
	{
		var evt = destEvents[i];
		// let's check we haven't lost an important building here.
		if (evt != undefined && !evt.SuccessfulFoundation && evt.entityObj != undefined && evt.metadata !== undefined && evt.metadata[PlayerID] &&
			evt.metadata[PlayerID]["base"] !== undefined && evt.metadata[PlayerID]["base"] == this.ID)
		{
			var ent = evt.entityObj;
			if (ent.hasTerritoryInfluence())
				this.territoryBuildings.splice(this.territoryBuildings.indexOf(ent.id()),1);
			if (ent.resourceDropsiteTypes())
				this.scrapDropsite(gameState, ent);
			if (evt.metadata[PlayerID]["baseAnchor"] && evt.metadata[PlayerID]["baseAnchor"] == true)
			{
				// sounds like we lost our anchor. Let's try rebuilding it.
				// TODO: currently the HQ manager sets us as initgathering, we probably ouht to do it
				this.anchor = undefined;
				
				this.constructing = true;	// let's switch mode.
				this.workers.forEach( function (worker) {
									 worker.stopMoving();
									 });
				if (ent.hasClass("CivCentre"))
				{
					// TODO: might want to tell the queue manager to pause other stuffs if we are the only base.
					queues.civilCentre.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_civil_centre", { "base" : this.ID, "baseAnchor" : true }, ent.position()));
				} else {
					// TODO
					queues.civilCentre.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_civil_centre", { "base" : this.ID, "baseAnchor" : true }, ent.position()));
				}
			}
			
		}
	}
	for (var i in cFinishedEvents)
	{
		var evt = cFinishedEvents[i];
		if (evt && evt.newentity)
		{
			// TODO: we ought to add new resources or do something.
			var ent = gameState.getEntityById(evt.newentity);
			
			if (ent === undefined)
				continue;
			
			if (ent.getMetadata(PlayerID,"base") == this.ID)
			{
				if(ent.hasTerritoryInfluence())
					this.territoryBuildings.push(ent.id());
				if (ent.resourceDropsiteTypes())
					for (var ress in ent.resourceDropsiteTypes())
						this.initializeDropsite(gameState, ent, ent.resourceDropsiteTypes()[ress]);
				if (ent.resourceSupplyAmount() && ent.resourceSupplyType()["specific"] == "grain")
					this.assignResourceToDP(gameState,ent);
			}
		}
	}
	for (var i in createEvents)
	{
		var evt = createEvents[i];
		// Checking for resources.
		var evt = events[i];
		if (evt && evt.entity)
		{
			var ent = gameState.getEntityById(evt.entity);
			
			if (ent === undefined)
				continue;
			
			if (ent.resourceSupplyAmount() && ent.owner() == 0)
				this.assignResourceToDP(gameState,ent);
			
		}
	}
};

// If no specific dropsite, it'll assign to the closest
m.BaseManager.prototype.assignResourceToDP = function (gameState, supply, specificDP) {
	var type = supply.resourceSupplyType()["generic"];
	if (type == "treasure")
		type = supply.resourceSupplyType()["specific"];
	if (!specificDP)
	{
		var closest = -1;
		var dist = Math.min();
		for (var i in this.dropsites)
		{
			var dp = gameState.getEntityById(i);
			var distance = API3.SquareVectorDistance(supply.position(), dp.position());
			if (supply.resourceSupplyType()["specific"] === "grain")
				distance /= 100;
			if (distance < dist && distance < this.bigRadius[type])
			{
				closest = dp.id();
				dist = distance;
			}
		}
		if (closest !== -1)
		{
			supply.setMetadata(PlayerID, "linked-dropsite-close", (dist < this.smallRadius[type]) );
			supply.setMetadata(PlayerID, "linked-dropsite-nearby", (dist < this.medRadius[type]) );
			supply.setMetadata(PlayerID, "linked-dropsite", closest );
			supply.setMetadata(PlayerID, "linked-dropsite-dist", +dist);

			if (m.DebugEnabled())
			{
				if (type == "food" && dist < this.smallRadius[type])
					Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [supply.id()], "rgb": [10,0,0]});
				else if (type == "food" && dist < this.medRadius[type])
					Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [supply.id()], "rgb": [2,0,0]});
				else
					Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [supply.id()], "rgb": [0.5,0,0]});
			}
		}
	}
	// TODO: ought to recount immediatly.
}

m.BaseManager.prototype.initializeDropsite = function (gameState, ent, type) {
	var count = 0, farCount = 0;
	var self = this;

	var resources = gameState.getResourceSupplies(type);

	// TODO: if we're initing, we should probably remove them anyway.
	if (self.dropsites[ent.id()] === undefined || self.dropsites[ent.id()][type] === undefined) {
		resources.filter( function (supply) { //}){
			if (!supply.position() || !ent.position())
				return;
			var distance = API3.SquareVectorDistance(supply.position(), ent.position());
			
			if (supply.getMetadata(PlayerID, "linked-dropsite") == undefined || supply.getMetadata(PlayerID, "linked-dropsite-dist") > distance) {
				if (distance < self.bigRadius[type]) {
					supply.setMetadata(PlayerID, "linked-dropsite-close", (distance < self.smallRadius[type]) );
					supply.setMetadata(PlayerID, "linked-dropsite-nearby", (distance < self.medRadius[type]) );
					supply.setMetadata(PlayerID, "linked-dropsite", ent.id() );
					supply.setMetadata(PlayerID, "linked-dropsite-dist", +distance);
					if(distance < self.smallRadius[type])
						count += supply.resourceSupplyAmount();
					if (distance < self.medRadius[type])
						 farCount += supply.resourceSupplyAmount();
				}
			}
		});
		// This one is both for the nearby and the linked
		var filter = API3.Filters.byMetadata(PlayerID, "linked-dropsite", ent.id());
		var collection = resources.filter(filter);
		collection.registerUpdates();
		
		filter = API3.Filters.byMetadata(PlayerID, "linked-dropsite-close",true);
		var collection2 = collection.filter(filter);
		collection2.registerUpdates();

		filter = API3.Filters.byMetadata(PlayerID, "linked-dropsite-nearby",true);
		var collection3 = collection.filter(filter);
		collection3.registerUpdates();

		filter = API3.Filters.byMetadata(PlayerID, "linked-to-dropsite", ent.id());
		var WkCollection = this.workers.filter(filter);
		WkCollection.registerUpdates();
		
		if (!self.dropsites[ent.id()])
			self.dropsites[ent.id()] = {};
		self.dropsites[ent.id()][type] = [collection2,collection3, collection, count, farCount, WkCollection, false];
		
		// TODO: flag us on the SharedScript "type" map.
		// TODO: get workers on those resources and do something with them.
	}
	
	if (m.DebugEnabled())
	{
		// Make resources glow wildly
		if (type == "food") {
			self.dropsites[ent.id()][type][2].forEach(function(ent){ 
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0.5,0,0]});
			});
			self.dropsites[ent.id()][type][1].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [2,0,0]});
			});
			self.dropsites[ent.id()][type][0].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [10,0,0]});
			});
		}
		if (type == "wood") {
			self.dropsites[ent.id()][type][2].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0.5,0]});
			});
			self.dropsites[ent.id()][type][1].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,2,0]});
			});
			self.dropsites[ent.id()][type][0].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,10,0]});
			});
		}
		if (type == "stone") {
			self.dropsites[ent.id()][type][2].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0.5,0.5,0]});
			});
			self.dropsites[ent.id()][type][1].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [2,2,0]});
			});
			self.dropsites[ent.id()][type][0].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [10,10,0]});
			});
		}
		if (type == "metal") {
			self.dropsites[ent.id()][type][2].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0.5,0.5]});
			});
			self.dropsites[ent.id()][type][1].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,2,2]});
			});
			self.dropsites[ent.id()][type][0].forEach(function(ent){
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,10,10]});
			});
		}
	}
};

// completely and "safely" remove a dropsite from our list.
// this also removes any linked resource and so on.
// TODO: should re-add the resources to another dropsite.
m.BaseManager.prototype.scrapDropsite = function (gameState, ent) {
	if (this.dropsites[ent.id()] === undefined)
		return true;
	
	for (var i in this.dropsites[ent.id()])
	{
		var type = i;
		var dp = this.dropsites[ent.id()][i];
		dp[2].forEach(function (supply) { //}){
			supply.deleteMetadata(PlayerID,"linked-dropsite-nearby");
			supply.deleteMetadata(PlayerID,"linked-dropsite-close");
			supply.deleteMetadata(PlayerID,"linked-dropsite");
			supply.deleteMetadata(PlayerID,"linked-dropsite-dist");
		});
		dp[5].forEach(function (worker) {
			worker.deleteMetadata(PlayerID,"linked-to-dropsite");
			// TODO: should probably stop the worker or something.
		});
		dp = [undefined, undefined, undefined, 0, 0, undefined];
		delete this.dropsites[ent.id()][i];
	}
	this.dropsites[ent.id()] = undefined;
	delete this.dropsites[ent.id()];
	return true;
};

// Returns the position of the best place to build a new dropsite for the specified resource
m.BaseManager.prototype.findBestDropsiteLocation = function(gameState, resource){
	
	var storeHousePlate = gameState.getTemplate(gameState.applyCiv("structures/{civ}_storehouse"));

	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.
	
	var territory = m.createTerritoryMap(gameState);
	
	var obstructions = m.createObstructionMap(gameState,this.accessIndex,storeHousePlate);
	obstructions.expandInfluences();
	
	// copy the resource map as initialization.
	var friendlyTiles = new API3.Map(gameState.sharedScript, gameState.sharedScript.resourceMaps[resource].map, true);

	var DPFoundations = gameState.getOwnFoundations().filter(API3.Filters.byType(gameState.applyCiv("foundation|structures/{civ}_storehouse")));
	
	// TODO: might be better to check dropsites someplace else.
	// loop over this in this.terrytoryindices. It's usually a little too much, but it's always enough.
	for (var p = 0; p < this.territoryIndices.length; ++p)
	{
		var j = this.territoryIndices[p];
		friendlyTiles.map[j] *= 1.5;
		
		// only add where the map is currently not null, ie in our territory and some "Resource" would be close.
		// This makes the placement go from "OK" to "human-like".
		for (var i in gameState.sharedScript.resourceMaps)
			if (friendlyTiles.map[j] !== 0 && i !== "food")
				friendlyTiles.map[j] += gameState.sharedScript.resourceMaps[i].map[j];
		
		for (var i in this.dropsites)
		{
			var pos = [j%friendlyTiles.width, Math.floor(j/friendlyTiles.width)];
			var dpPos = gameState.getEntityById(i).position();
			if (dpPos && API3.SquareVectorDistance(friendlyTiles.gamePosToMapPos(dpPos), pos) < 250)
			{
				friendlyTiles.map[j] = 0;
				continue;
			} else if (dpPos && API3.SquareVectorDistance(friendlyTiles.gamePosToMapPos(dpPos), pos) < 450)
				friendlyTiles.map[j] /= 2;
		}
		for (var i in DPFoundations._entities)
		{
			var pos = [j%friendlyTiles.width, Math.floor(j/friendlyTiles.width)];
			var dpPos = gameState.getEntityById(i).position();
			if (dpPos && API3.SquareVectorDistance(friendlyTiles.gamePosToMapPos(dpPos), pos) < 250)
				friendlyTiles.map[j] = 0;
			else if (dpPos && API3.SquareVectorDistance(friendlyTiles.gamePosToMapPos(dpPos), pos) < 450)
				friendlyTiles.map[j] /= 2;
		}
	}
	
	if (m.DebugEnabled())
		friendlyTiles.dumpIm("DP_" + resource + "_" + gameState.getTimeElapsed() + ".png");
	
	var best = friendlyTiles.findBestTile(2, obstructions);	// try to find a spot to place a DP.
	var bestIdx = best[0];

	m.debug ("for dropsite best is " +best[1] + " at " + gameState.getTimeElapsed());
	
	// tell the dropsite builder we haven't found anything satisfactory.
	if (best[1] < 60)
		return false;
	
	var x = ((bestIdx % friendlyTiles.width) + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * gameState.cellSize;
	
	return [x,z];
};

// update the resource level of a dropsite.
m.BaseManager.prototype.updateDropsite = function (gameState, ent, type) {
	if (this.dropsites[ent.id()] === undefined || this.dropsites[ent.id()][type] === undefined)
		return undefined;	// should initialize it first.

	var count = 0, farCount = 0;
	var resources = gameState.getResourceSupplies(type);
	
	var dropsite = this.dropsites[ent.id()][type];

	var medianPositionX = 0;
	var medianPositionY = 0;
	var divider = 0;
	dropsite[1].forEach( function (supply) { //}){
		farCount += supply.resourceSupplyAmount();
		medianPositionX += supply.position()[0];
		medianPositionY += supply.position()[1];
		++divider;
	});
	dropsite[0].forEach( function (supply) { //}){
		count += supply.resourceSupplyAmount();
		medianPositionX += supply.position()[0];
		medianPositionY += supply.position()[1];
		++divider;
	});

	// once per dropsite, if the average wood resource is too far away, try to build a closer one.
	if (type === "wood" && divider !== 0 && !dropsite[6] && count < 300 && farCount > 700
		&& gameState.ai.queues.dropsites.length() === 0 && gameState.countFoundationsByType(gameState.applyCiv("structures/{civ}_storehouse"), true) === 0)
	{
		medianPositionX /= divider;
		medianPositionY /= divider;
		if (API3.SquareVectorDistance([medianPositionX,medianPositionY], ent.position()) > 600)
		{
			dropsite[6] = true;
			gameState.ai.queues.dropsites.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse", { "base" : this.ID }, [medianPositionX,medianPositionY]));
		}
	}

	dropsite[3] = count;
	dropsite[4] = farCount;
	return true;
};

// Updates dropsites.
m.BaseManager.prototype.updateDropsites = function (gameState) {
	// for each dropsite, recalculate
	for (var i in this.dropsites)
	{
		for (var type in this.dropsites[i])
		{
			this.updateDropsite(gameState,gameState.getEntityById(i),type);
		}
	}
	
};

// TODO: ought to be cached or something probably
// Returns the number of slots available for workers on this dropsite.
m.BaseManager.prototype.getDpWorkerCapacity = function (gameState, entID, type, faraway) {
	if (this.dropsites[entID] === undefined || this.dropsites[entID][type] === undefined)
		return undefined;	// should initialize it first, or it just doesn't exist.
	var dropsite = this.dropsites[entID];

	var count = 0;
	if (type == "food")
	{
		var slot = dropsite[type][0];
		if (faraway === true)
			slot = dropsite[type][1];
		slot.forEach(function (ent) {
			if (ent.resourceSupplyAmount() > 50)
				count += ent.maxGatherers();
		});
	}
	else if ((type === "stone" && dropsite["stone"])|| (type === "metal" && dropsite["metal"]))
	{
		var slot = dropsite[type][0];
		if (faraway === true)
			slot = dropsite[type][1];
		slot.forEach(function (ent) {
			if (ent.resourceSupplyAmount() > 500)
				count += ent.maxGatherers();
		});
	} else if (type === "wood")
	{
		count = (faraway === true) ? dropsite[type][4] / 250 : dropsite[type][3] / 200;
	}
	return count;
};

// TODO: ought to be cached or something probably
// Returns the number of slots available for workers here.
m.BaseManager.prototype.getWorkerCapacity = function (gameState, type, faraway) {
	var count = 0;
	for (var i in this.dropsites)
		count += this.getDpWorkerCapacity(gameState,i,type, faraway);
	return count;
};

// TODO: ought to be cached or something probably
// Returns the amount of resource left
m.BaseManager.prototype.getResourceLevel = function (gameState, type, searchType, threshold) {
	var count = 0;
	if (searchType == "all")
	{
		// return all resources in the base area.
		gameState.getResourceSupplies(type).filter(API3.Filters.byTerritory(gameState.ai.HQ.basesMap, this.ID)).forEach( function (ent) { //}){
			count += ent.resourceSupplyAmount();
		});
		return count;
	}
	if (searchType == "dropsites")
	{
		// for each dropsite, recalculate
		for (var i in this.dropsites)
			if (this.dropsites[i][type] !== undefined)
				count += this.dropsites[i][type][4];
		return count;
	}
	if (searchType == "dropsitesClose")
	{
		// for each dropsite, recalculate
		for (var i in this.dropsites)
			if (this.dropsites[i][type] !== undefined)
				count += this.dropsites[i][type][3];
		return count;
	}
	if (searchType == "dropsites-dpcount")
	{
		var seuil = 800;
		if (threshold)
			seuil = threshold;
		// for each dropsite, recalculate
		for (var i in this.dropsites)
			if (this.dropsites[i][type] !== undefined)
			{
				if (this.dropsites[i][type][4] > seuil)
					count++;
			}
		return count;
	}
	return 0;
};

// check our resource levels and react accordingly
m.BaseManager.prototype.checkResourceLevels = function (gameState,queues) {
	for (var type in this.willGather)
	{
		if (this.willGather[type] === 0)
			continue;
		// not enough resources on the map, tell us we've stopped.
		if (type !== "food" && gameState.ai.playedTurn % 10 === 4 && this.getResourceLevel(gameState,type, "all") < 200)
			this.willGather[type] = 0;	// won't gather at all
		// we're waiting for a new dropsite.
		if (this.willGather[type] === 2)
			continue;

		var count = this.getResourceLevel(gameState,type, "dropsites");
		if (type == "food")
		{
			if (!this.isFarming && count < 1600 && queues.field.length === 0)
			{
				// tell the queue manager we'll be trying to build fields shortly.
				for (var  i = 0; i < this.Config.Economy.initialFields;++i)
				{
					var plan = new m.ConstructionPlan(gameState, "structures/{civ}_field", { "base" : this.ID });
					plan.isGo = function() { return false; };	// don't start right away.
					queues.field.addItem(plan);
				}
			} else if (!this.isFarming && count < 400)
			{
				for (var i in queues.field.queue)
					queues.field.queue[i].isGo = function() { return true; };	// start them
				this.isFarming = true;
			}
			if (this.isFarming)
			{
				var numFarms = 0;
				this.buildings.filter(API3.Filters.byClass("Field")).forEach(function (field) {
					if (field.resourceSupplyAmount() > 400)
						numFarms++;
				});
				var numFd = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_field"), true);
				if (numFarms+numFd > 15)
				{
					warn("treu");
					this.willGather["food"] = 2;
				}
				var numQueued = queues.field.countQueuedUnits();
				numFarms += numFd + numQueued;
				

				// let's see if we need to push new farms.
				var maxGatherers = gameState.getTemplate(gameState.applyCiv("structures/{civ}_field")).maxGatherers();
				if (numQueued < 3)
					if (numFarms < Math.round(this.gatherersByType(gameState, "food").length / (maxGatherers*0.9)))
						queues.field.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_field", { "base" : this.ID }));
				// TODO: refine count to only count my base.
			}
		} else if (queues.dropsites.length() === 0 && gameState.countFoundationsByType(gameState.applyCiv("structures/{civ}_storehouse"), true) === 0) {
			var workerCapacity = this.getWorkerCapacity(gameState, type);	// only close;
			// check how often we'll want new dropsites.
			// if we're booming we'll aggressively grab terrain
			if (gameState.currentPhase() >= 1 && gameState.ai.aggressiveness < 0.15)
				var wantDropsite = (this.gatherersByType(gameState, type).length / workerCapacity) > 0.45;
			else if (gameState.currentPhase() >= 1)
				var wantDropsite = (this.gatherersByType(gameState, type).length / workerCapacity) > 0.6;
			else
				var wantDropsite = (this.gatherersByType(gameState, type).length / workerCapacity) > 0.9;
			if (wantDropsite)
			{
				var pos = this.findBestDropsiteLocation(gameState, type);
				if (!pos)
				{
					m.debug ("Found no right position for a " + type + " dropsite, going into \"noSpot\" mode");
					this.willGather[type] = 2;	// won't build
					// TODO: tell the HQ we'll be needing a new base for this resource, or tell it we've ran out of resource Z.
				} else {
					m.debug ("planning new dropsite for " + type);
					queues.dropsites.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_storehouse",{ "base" : this.ID }, pos));
				}
			}
		}
	}
	
};

// let's return the estimated gather rates.
m.BaseManager.prototype.getGatherRates = function(gameState, currentRates) {
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
				var gRate = ent.currentGatherRate()
				if (gRate !== undefined)
					currentRates[i] += Math.log(1+gRate)/1.1;
			});
		}
		currentRates[i] += 0.5*m.GetTCRessGatherer(gameState,i);
	}
};

m.BaseManager.prototype.assignRolelessUnits = function(gameState) {
	// TODO: make this cleverer.
	var roleless = this.units.filter(API3.Filters.not(API3.Filters.byHasMetadata(PlayerID, "role")));
	var self = this;
	roleless.forEach(function(ent) {
		if (ent.hasClass("Worker") || ent.hasClass("CitizenSoldier")) {
			if (ent.hasClass("Cavalry") && !self.isHunting)
				return;
			ent.setMetadata(PlayerID, "role", "worker");
		}
	});
};

// If the numbers of workers on the resources is unbalanced then set some of workers to idle so
// they can be reassigned by reassignIdleWorkers.
// TODO: actually this probably should be in the HQ.
m.BaseManager.prototype.setWorkersIdleByPriority = function(gameState){
	var self = this;
	if (gameState.currentPhase() < 2 && gameState.getTimeElapsed() < 360000)
		return;	// not in the first phase or the first 6 minutes.
	
	var types = gameState.ai.queueManager.getAvailableResources(gameState);
	
	var bestType = "";
	
	var avgOverdraft = 0;
	
	for (var i in types.types)
		avgOverdraft += types[types.types[i]];

	avgOverdraft /= 4;
	
	for (var i in types.types)
		if (types[types.types[i]] > avgOverdraft + 200 || (types[types.types[i]] > avgOverdraft && avgOverdraft > 200))
			if (this.gatherersByType(gameState,types.types[i]).length > 0)
			{
				// TODO: perhaps change this?
				var nb = 2;
				this.gatherersByType(gameState,types.types[i]).forEach( function (ent) { //}){
					if (nb > 0)
					{
						//m.debug ("Moving " +ent.id() + " from " + types.types[i]);
						nb--;
						// TODO: might want to direct assign.
						ent.stopMoving();
						ent.setMetadata(PlayerID, "subrole","idle");
					}
				});
			}
	//m.debug (currentRates);
};

// TODO: work on this.
m.BaseManager.prototype.reassignIdleWorkers = function(gameState) {
	
	var self = this;

	// Search for idle workers, and tell them to gather resources based on demand
	var filter = API3.Filters.or(API3.Filters.byMetadata(PlayerID,"subrole","idle"), API3.Filters.not(API3.Filters.byHasMetadata(PlayerID,"subrole")));
	var idleWorkers = gameState.updatingCollection("idle-workers-base-" + this.ID, filter, this.workers);

	if (idleWorkers.length) {
		idleWorkers.forEach(function(ent) {
			// Check that the worker isn't garrisoned
			if (ent.position() === undefined){
				return;
			}
			if (ent.hasClass("Worker")) {
				var types = gameState.ai.HQ.pickMostNeededResources(gameState);

				for (var i = 0; i < 4; ++i)
				{
					// Okay let's now check we can actually remain here for that
					if (self.willGather[types[i]] !== 1)
					{
						if (!gameState.ai.HQ.switchWorkerBase(gameState, ent, types[i]))
							continue;
						else
							break;
					}
					//m.debug ("assigning " +ent.id() + " to " + types[0]);
					ent.setMetadata(PlayerID, "subrole", "gatherer");
					ent.setMetadata(PlayerID, "gather-type", types[i]);
					m.AddTCRessGatherer(gameState,types[i]);
					break;
				}
			} else {
				ent.setMetadata(PlayerID, "subrole", "hunter");
			}
		});
	}
};

m.BaseManager.prototype.workersBySubrole = function(gameState, subrole) {
	return gameState.updatingCollection("subrole-" + subrole +"-base-" + this.ID, API3.Filters.byMetadata(PlayerID, "subrole", subrole), this.workers, true);
};

m.BaseManager.prototype.gatherersByType = function(gameState, type) {
	return gameState.updatingCollection("workers-gathering-" + type +"-base-" + this.ID, API3.Filters.byMetadata(PlayerID, "gather-type", type), this.workersBySubrole(gameState, "gatherer"));
};


// returns an entity collection of workers.
// They are idled immediatly and their subrole set to idle.
m.BaseManager.prototype.pickBuilders = function(gameState, workers, number) {
	// TODO: choose better.
	var availableWorkers = this.workers.filter(API3.Filters.not(API3.Filters.byClass("Cavalry"))).toEntityArray();
	availableWorkers.sort(function (a,b) {
		var vala = 0, valb = 0;
		if (a.getMetadata(PlayerID,"subrole") == "builder")
			vala = 100;
		if (b.getMetadata(PlayerID,"subrole") == "builder")
			valb = 100;
		if (a.getMetadata(PlayerID,"plan") != undefined)
			vala = -100;
		if (b.getMetadata(PlayerID,"plan") != undefined)
			valb = -100;
		return vala < valb
	});
	var needed = Math.min(number, availableWorkers.length);
	for (var i = 0; i < needed; ++i)
	{
		availableWorkers[i].stopMoving();
		availableWorkers[i].setMetadata(PlayerID, "subrole", "idle");
		workers.addEnt(availableWorkers[i]);
	}
	return;
};

m.BaseManager.prototype.assignToFoundations = function(gameState, noRepair) {
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
	var workers = this.workers.filter(API3.Filters.not(API3.Filters.byClass("Cavalry")));
	var builderWorkers = this.workersBySubrole(gameState, "builder");
	var idleBuilderWorkers = this.workersBySubrole(gameState, "builder").filter(API3.Filters.isIdle());

	// if we're constructing and we have the foundations to our base anchor, only try building that.
	if (this.constructing == true && this.buildings.filter(API3.Filters.and(API3.Filters.isFoundation(), API3.Filters.byMetadata(PlayerID, "baseAnchor", true))).length !== 0)
	{
		foundations = this.buildings.filter(API3.Filters.byMetadata(PlayerID, "baseAnchor", true)).toEntityArray();
		var tID = foundations[0].id();
		workers.forEach(function (ent) { //}){
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
			noobs.forEach(function (worker) { //}){
				worker.setMetadata(PlayerID,"base", self.ID);
				worker.setMetadata(PlayerID,"subrole", "builder");
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
	
	for (var i in foundations) {
		var target = foundations[i];

		if (target.hasClass("Field"))
			continue; // we do not build fields
		
		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target.id()).length;
		var targetNB = this.Config.Economy.targetNumBuilders;	// TODO: dynamic that.
		if (target.hasClass("House"))
			targetNB *= 2;
		else if (target.hasClass("Barracks"))
			targetNB = 4;
		else if (target.hasClass("Fortress"))
			targetNB = 7;
		if (target.getMetadata(PlayerID, "baseAnchor") == true)
			targetNB = 15;

		if (assigned < targetNB) {
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
					nonBuilderWorkers.sort(function (workerA,workerB)
					{
						var coeffA = API3.SquareVectorDistance(target.position(),workerA.position());
						if (workerA.getMetadata(PlayerID, "gather-type") === "food")
							coeffA *= 3;
						var coeffB = API3.SquareVectorDistance(target.position(),workerB.position());
						if (workerB.getMetadata(PlayerID, "gather-type") === "food")
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
	for (var i in damagedBuildings) {
		var target = damagedBuildings[i];
		if (gameState.defcon() < 5) {
			if (target.healthLevel() > 0.5 || !target.hasClass("CivCentre") || !target.hasClass("StoneWall")) {
				continue;
			}
		} else if (noRepair && !target.hasClass("CivCentre"))
			continue;
		
		var territory = m.createTerritoryMap(gameState);
		if (territory.getOwner(target.position()) !== PlayerID || territory.getOwner([target.position()[0] + 5, target.position()[1]]) !== PlayerID)
			continue;
		
		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target.id()).length;
		if (assigned < targetNB/3) {
			if (builderWorkers.length + addedWorkers < targetNB*2) {
				
				var nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata(PlayerID, "subrole") !== "builder" && ent.position() !== undefined); });
				if (gameState.defcon() < 5)
					nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata(PlayerID, "subrole") !== "builder" && ent.hasClass("Female") && ent.position() !== undefined); });
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

m.BaseManager.prototype.update = function(gameState, queues, events) {
	Engine.ProfileStart("Base update - base " + this.ID);
	var self = this;
	
	this.updateDropsites(gameState);
	this.checkResourceLevels(gameState, queues);
	
	Engine.ProfileStart("Assign builders");
	this.assignToFoundations(gameState);
	Engine.ProfileStop();
	
	if (this.constructing && this.anchor)
	{
		var terrMap = m.createTerritoryMap(gameState);
		if(terrMap.getOwner(this.anchor.position()) !== 0 && terrMap.getOwner(this.anchor.position()) !== PlayerID)
		{
			// we're in enemy territory. If we're too close from the enemy, destroy us.
			var eEnts = gameState.getEnemyStructures().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
			for (var i in eEnts)
			{
				var entPos = eEnts[i].position();
				entPos = [entPos[0]/4.0,entPos[1]/4.0];
				if (API3.SquareVectorDistance(entPos, this.anchor.position()) < 500)
					this.anchor.destroy();
			}
		}
	}


//	if (!this.constructing)
//	{
	if (gameState.ai.playedTurn % 2 === 0)
		this.setWorkersIdleByPriority(gameState);

	this.assignRolelessUnits(gameState);
		
		/*Engine.ProfileStart("Swap Workers");
		 var gathererGroups = {};
		 gameState.getOwnEntitiesByRole("worker", true).forEach(function(ent){ }){
		 if (ent.hasClass("Cavalry"))
		 return;
		 var key = uneval(ent.resourceGatherRates());
		 if (!gathererGroups[key]){
		 gathererGroups[key] = {"food": [], "wood": [], "metal": [], "stone": []};
		 }
		 if (ent.getMetadata(PlayerID, "gather-type") in gathererGroups[key]){
		 gathererGroups[key][ent.getMetadata(PlayerID, "gather-type")].push(ent);
		 }
		 });
		 for (var i in gathererGroups){
		 for (var j in gathererGroups){
		 var a = eval(i);
		 var b = eval(j);
		 if (a !== undefined && b !== undefined)
		 if (a["food.grain"]/b["food.grain"] > a["wood.tree"]/b["wood.tree"] && gathererGroups[i]["wood"].length > 0
		 && gathererGroups[j]["food"].length > 0){
		 for (var k = 0; k < Math.min(gathererGroups[i]["wood"].length, gathererGroups[j]["food"].length); k++){
		 gathererGroups[i]["wood"][k].setMetadata(PlayerID, "gather-type", "food");
		 gathererGroups[j]["food"][k].setMetadata(PlayerID, "gather-type", "wood");
		 }
		 }
		 }
		 }
		 Engine.ProfileStop();*/
		
		// should probably be last to avoid reallocations of units that would have done stuffs otherwise.
		Engine.ProfileStart("Assigning Workers");
		this.reassignIdleWorkers(gameState);
		Engine.ProfileStop();
//	}

	// TODO: do this incrementally a la defence.js
	Engine.ProfileStart("Run Workers");
	this.workers.forEach(function(ent) {
		if (!ent.getMetadata(PlayerID, "worker-object"))
			ent.setMetadata(PlayerID, "worker-object", new m.Worker(ent));
		ent.getMetadata(PlayerID, "worker-object").update(self, gameState);
	});
	Engine.ProfileStop();
		
	Engine.ProfileStop();
};

return m;

}(AEGIS);
