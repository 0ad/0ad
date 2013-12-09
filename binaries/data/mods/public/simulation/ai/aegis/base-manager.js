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

var BaseManager = function() {
	this.farmingFields = false;
	this.ID = uniqueIDBases++;
	
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

BaseManager.prototype.init = function(gameState, events, unconstructed){
	this.constructing = unconstructed;
	// entitycollections
	this.units = gameState.getOwnEntities().filter(Filters.and(Filters.byClass("Unit"),Filters.byMetadata(PlayerID, "base", this.ID)));
	this.buildings = gameState.getOwnEntities().filter(Filters.and(Filters.byClass("Structure"),Filters.byMetadata(PlayerID, "base", this.ID)));
	
	this.workers = this.units.filter(Filters.byMetadata(PlayerID,"role","worker"));

	this.workers.allowQuickIter();
	this.buildings.allowQuickIter();
	this.units.allowQuickIter();
	
	this.units.registerUpdates();
	this.buildings.registerUpdates();
	this.workers.registerUpdates();
	
	// array of entity IDs, with each being
	// { "food" : [close entities, semi-close entities, faraway entities, closeAmount, medianAmount, assignedWorkers collection ] … } (one per resource)
	// note that "median amount" also counts the closeAmount.
	this.dropsites = { };
	
	// TODO: difficulty levels for this?
	
	// smallRadius is the distance necessary to mark a resource as linked to a dropsite.
	this.smallRadius = { 'food':40*40,'wood':45*45,'stone':40*40,'metal':40*40 };
	// medRadius is the maximal distance for a link, albeit one that would still make us want to build a new dropsite.
	this.medRadius = { 'food':70*70,'wood':70*70,'stone':80*80,'metal':80*80 };
	// bigRadius is the distance for a weak link, mainly for optimizing search for resources when a DP is depleted.
	this.bigRadius = { 'food':70*70,'wood':200*200,'stone':200*200,'metal':200*200 };
};

BaseManager.prototype.assignEntity = function(unit){
	unit.setMetadata(PlayerID, "base", this.ID);
	this.units.updateEnt(unit);
	this.workers.updateEnt(unit);
	this.buildings.updateEnt(unit);
	// TODO: immediately assign it some task?

	if (unit.hasClass("Structure") && unit.hasTerritoryInfluence() && this.territoryBuildings.indexOf(unit.id()) === -1)
		this.territoryBuildings.push(unit.id());
};

BaseManager.prototype.setAnchor = function(anchorEntity) {
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
BaseManager.prototype.initTerritory = function(HQ, gameState) {
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
			if (xi*xi+yi*yi < radius*radius && HQ.basesMap.map[(x+xi) + (y+yi)*width] === 0)
			{
				if (this.accessIndex == gameState.sharedScript.accessibility.landPassMap[x+xi + width*(y+yi)])
				{
					this.territoryIndices.push((x+xi) + (y+yi)*width);
					HQ.basesMap.map[(x+xi) + (y+yi)*width] = this.ID;
				}
			}
}

BaseManager.prototype.initGatheringFunctions = function(HQ, gameState, specTypes) {
	// init our gathering functions.
	var types = ["food","wood","stone","metal"];
	if (specTypes !== undefined)
		type = specTypes;
	
	var self = this;
	var count = 0;
	
	for (i in types)
	{
		var type = types[i];
		// TODO: set us as "X" gatherer
		
		this.buildings.filter(Filters.isDropsite(type)).forEach(function(ent) { self.initializeDropsite(gameState, ent,type) });

		if (this.getResourceLevel(gameState, type, "all") > 1000)
			this.willGather[type] = 1;
	}
	if (this.willGather["food"] === 0)
	{
		var needFarm = true;
		// Let's check again for food
		for (base in HQ.baseManagers)
			if (HQ.baseManagers[base].willGather["food"] === 1)
				needFarm = false;
		if (needFarm)
			this.willGather["food"] = 1;
	}
	debug ("food" + this.willGather["food"]);
	debug (this.willGather["wood"]);
	debug (this.willGather["stone"]);
	debug (this.willGather["metal"]);
}

BaseManager.prototype.checkEvents = function (gameState, events, queues) {
	for (i in events)
	{
		if (events[i].type == "Destroy")
		{
			// let's check we haven't lost an important building here.
			var evt = events[i];
			if (evt.msg != undefined && !evt.msg.SuccessfulFoundation && evt.msg.entityObj != undefined && evt.msg.metadata !== undefined && evt.msg.metadata[PlayerID] &&
				evt.msg.metadata[PlayerID]["base"] !== undefined && evt.msg.metadata[PlayerID]["base"] == this.ID)
			{
				var ent = evt.msg.entityObj;
				if (ent.hasTerritoryInfluence())
					this.territoryBuildings.splice(this.territoryBuildings.indexOf(ent.id()),1);
				if (ent.resourceDropsiteTypes())
					this.scrapDropsite(gameState, ent);
				if (evt.msg.metadata[PlayerID]["baseAnchor"] && evt.msg.metadata[PlayerID]["baseAnchor"] == true)
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
						queues.civilCentre.addItem(new ConstructionPlan(gameState, "structures/{civ}_civil_centre", { "base" : this.ID, "baseAnchor" : true }, 0 , -1,ent.position()));
					} else {
						// TODO
						queues.civilCentre.addItem(new ConstructionPlan(gameState, "structures/{civ}_civil_centre", { "base" : this.ID, "baseAnchor" : true },0,-1,ent.position()));
					}
				}
			}
		}
	}
	for (i in events)
	{
		if (events[i].type == "ConstructionFinished")
		{
			// let's check we haven't lost an important building here.
			var evt = events[i];
			if (evt.msg && evt.msg.newentity)
			{
				// TODO: we ought to add new resources or do something.
				var ent = gameState.getEntityById(evt.msg.newentity);
				
				if (ent === undefined)
					continue;
				
				if (ent.getMetadata(PlayerID,"base") == this.ID)
				{
					if(ent.hasTerritoryInfluence())
						this.territoryBuildings.push(ent.id());
					if (ent.resourceDropsiteTypes())
						for (ress in ent.resourceDropsiteTypes())
							this.initializeDropsite(gameState, ent, ent.resourceDropsiteTypes()[ress]);
					if (ent.resourceSupplyAmount() && ent.resourceSupplyType()["specific"] == "grain")
						this.assignResourceToDP(gameState,ent);
				}
			}
		} else if (events[i].type == "Create")
		{
			// Checking for resources.
			var evt = events[i];
			if (evt.msg && evt.msg.entity)
			{
				var ent = gameState.getEntityById(evt.msg.entity);

				if (ent === undefined)
					continue;

				if (ent.resourceSupplyAmount() && ent.owner() == 0)
					this.assignResourceToDP(gameState,ent);
			}
		}
	}
};

// If no specific dropsite, it'll assign to the closest
BaseManager.prototype.assignResourceToDP = function (gameState, supply, specificDP) {
	var type = supply.resourceSupplyType()["generic"];
	if (type == "treasure")
		type = supply.resourceSupplyType()["specific"];
	if (!specificDP)
	{
		var closest = -1;
		var dist = Math.min();
		for (i in this.dropsites)
		{
			var dp = gameState.getEntityById(i);
			var distance = SquareVectorDistance(supply.position(), dp.position());
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
		}
	}
	// TODO: ought to recount immediatly.
}

BaseManager.prototype.initializeDropsite = function (gameState, ent, type) {
	var count = 0, farCount = 0;
	var self = this;

	var resources = gameState.getResourceSupplies(type);

	// TODO: if we're initing, we should probably remove them anyway.
	if (self.dropsites[ent.id()] === undefined || self.dropsites[ent.id()][type] === undefined) {
		resources.filter( function (supply) { //}){
			if (!supply.position() || !ent.position())
				return;
			var distance = SquareVectorDistance(supply.position(), ent.position());
			
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
		var filter = Filters.byMetadata(PlayerID, "linked-dropsite", ent.id());
		var collection = resources.filter(filter);
		collection.registerUpdates();
		
		filter = Filters.byMetadata(PlayerID, "linked-dropsite-close",true);
		var collection2 = collection.filter(filter);
		collection2.registerUpdates();

		filter = Filters.byMetadata(PlayerID, "linked-dropsite-nearby",true);
		var collection3 = collection.filter(filter);
		collection3.registerUpdates();

		filter = Filters.byMetadata(PlayerID, "linked-to-dropsite", ent.id());
		var WkCollection = this.workers.filter(filter);
		WkCollection.registerUpdates();
		
		if (!self.dropsites[ent.id()])
			self.dropsites[ent.id()] = {};
		self.dropsites[ent.id()][type] = [collection2,collection3, collection, count, farCount, WkCollection];
		
		// TODO: flag us on the SharedScript "type" map.
		// TODO: get workers on those resources and do something with them.
	}
	
	if (Config.debug)
	{
		// Make resources glow wildly
		if (type == "food") {
			self.dropsites[ent.id()][type][2].forEach(function(ent){ 
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0.5,0,0]});
			});
			self.dropsites[ent.id()][type][1].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [2,0,0]});
			});
			self.dropsites[ent.id()][type][0].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [10,0,0]});
			});
		}
		if (type == "wood") {
			self.dropsites[ent.id()][type][2].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0.5,0]});
			});
			self.dropsites[ent.id()][type][1].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,2,0]});
			});
			self.dropsites[ent.id()][type][0].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,10,0]});
			});
		}
		if (type == "stone") {
			self.dropsites[ent.id()][type][2].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0.5,0.5,0]});
			});
			self.dropsites[ent.id()][type][1].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [2,2,0]});
			});
			self.dropsites[ent.id()][type][0].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [10,10,0]});
			});
		}
		if (type == "metal") {
			self.dropsites[ent.id()][type][2].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0.5,0.5]});
			});
			self.dropsites[ent.id()][type][1].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,2,2]});
			});
			self.dropsites[ent.id()][type][0].forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,10,10]});
			});
		}
	}
};

// completely and "safely" remove a dropsite from our list.
// this also removes any linked resource and so on.
// TODO: should re-add the resources to another dropsite.
BaseManager.prototype.scrapDropsite = function (gameState, ent) {
	if (this.dropsites[ent.id()] === undefined)
		return true;
	
	for (i in this.dropsites[ent.id()])
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
BaseManager.prototype.findBestDropsiteLocation = function(gameState, resource){
	
	var storeHousePlate = gameState.getTemplate(gameState.applyCiv("structures/{civ}_storehouse"));

	// This builds a map. The procedure is fairly simple. It adds the resource maps
	//	(which are dynamically updated and are made so that they will facilitate DP placement)
	// Then checks for a good spot in the territory. If none, and town/city phase, checks outside
	// The AI will currently not build a CC if it wouldn't connect with an existing CC.
	
	var territory = Map.createTerritoryMap(gameState);
	
	var obstructions = Map.createObstructionMap(gameState,this.accessIndex,storeHousePlate);
	obstructions.expandInfluences();
	
	// copy the resource map as initialization.
	var friendlyTiles = new Map(gameState.sharedScript, gameState.sharedScript.resourceMaps[resource].map, true);

	var DPFoundations = gameState.getOwnFoundations().filter(Filters.byType(gameState.applyCiv("foundation|structures/{civ}_storehouse")));
	
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
			if (dpPos && SquareVectorDistance(friendlyTiles.gamePosToMapPos(dpPos), pos) < 250)
			{
				friendlyTiles.map[j] = 0;
				continue;
			} else if (dpPos && SquareVectorDistance(friendlyTiles.gamePosToMapPos(dpPos), pos) < 450)
				friendlyTiles.map[j] /= 2;
		}
		for (var i in DPFoundations._entities)
		{
			var pos = [j%friendlyTiles.width, Math.floor(j/friendlyTiles.width)];
			var dpPos = gameState.getEntityById(i).position();
			if (dpPos && SquareVectorDistance(friendlyTiles.gamePosToMapPos(dpPos), pos) < 250)
				friendlyTiles.map[j] = 0;
			else if (dpPos && SquareVectorDistance(friendlyTiles.gamePosToMapPos(dpPos), pos) < 450)
				friendlyTiles.map[j] /= 2;
		}
	}
	
	if (Config.debug)
		friendlyTiles.dumpIm("DP_" + resource + "_" + gameState.getTimeElapsed() + ".png");
	
	var best = friendlyTiles.findBestTile(2, obstructions);	// try to find a spot to place a DP.
	var bestIdx = best[0];

	// tell the dropsite builder we haven't found anything satisfactory.
	if (best[1] < 60)
		return false;
	
	var x = ((bestIdx % friendlyTiles.width) + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * gameState.cellSize;
	
	return [x,z];
};

// update the resource level of a dropsite.
BaseManager.prototype.updateDropsite = function (gameState, ent, type) {
	if (this.dropsites[ent.id()] === undefined || this.dropsites[ent.id()][type] === undefined)
		return undefined;	// should initialize it first.

	var count = 0, farCount = 0;
	
	var resources = gameState.getResourceSupplies(type);
	
	this.dropsites[ent.id()][type][1].forEach( function (supply) { //}){
		farCount += supply.resourceSupplyAmount();
	});
	this.dropsites[ent.id()][type][0].forEach( function (supply) { //}){
		count += supply.resourceSupplyAmount();
	});

	this.dropsites[ent.id()][type][3] = count;
	this.dropsites[ent.id()][type][4] = farCount;
	return true;
};

// Updates dropsites.
BaseManager.prototype.updateDropsites = function (gameState) {
	// for each dropsite, recalculate
	for (i in this.dropsites)
	{
		for (type in this.dropsites[i])
		{
			this.updateDropsite(gameState,gameState.getEntityById(i),type);
		}
	}
	
};

// TODO: ought to be cached or something probably
// Returns the number of slots available for workers here.
// we're assuming Max - 3 for metal/stone mines, and 20 for any dropsite that has wood.
// TODO: for wood might want to count the trees too.
// TODO: this returns "future" worker capacity, might want to have a current one.
BaseManager.prototype.getWorkerCapacity = function (gameState, type) {
	var count = 0;
	if (type == "food")
		return 1000000;	// TODO: perhaps return something sensible here.
	if (type === "stone" || type === "metal")
	{
		for (id in this.dropsites)
			if (this.dropsites[id][type])
				this.dropsites[id][type][1].forEach(function (ent) {// }){
					if (ent.resourceSupplyAmount() > 500)
						count += ent.maxGatherers() - 3;
				});
	} else if (type === "wood")
	{
		for (id in this.dropsites)
			if (this.dropsites[id][type] && (this.dropsites[id][type][4]) > 1000)
				count += Math.min(15, this.dropsites[id][type][4] / 200);
	}
	return count;
};

// TODO: ought to be cached or something probably
// Returns the amount of resource left
BaseManager.prototype.getResourceLevel = function (gameState, type, searchType, threshold) {
	var count = 0;
	if (searchType == "all")
	{
		// return all resources in the base area.
		gameState.getResourceSupplies(type).filter(Filters.byTerritory(gameState.ai.HQ.basesMap, this.ID)).forEach( function (ent) { //}){
			count += ent.resourceSupplyAmount();
		});
		return count;
	}
	if (searchType == "dropsites")
	{
		// for each dropsite, recalculate
		for (i in this.dropsites)
			if (this.dropsites[i][type] !== undefined)
				count += this.dropsites[i][type][4];
		return count;
	}
	if (searchType == "dropsitesClose")
	{
		// for each dropsite, recalculate
		for (i in this.dropsites)
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
		for (i in this.dropsites)
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
BaseManager.prototype.checkResourceLevels = function (gameState,queues) {
	for (type in this.willGather)
	{
		if (this.willGather[type] === 0)
			continue;
		if (type !== "food" && gameState.playedTurn % 10 === 4 && this.getResourceLevel(gameState,type, "all") < 200)
			this.willGather[type] = 0;	// won't gather at all
		if (this.willGather[type] === 2)
			continue;
		var count = this.getResourceLevel(gameState,type, "dropsites");
		if (type == "food")
		{
			if (!this.isFarming && count < 1600 && queues.field.length === 0)
			{
				// tell the queue manager we'll be trying to build fields shortly.
				for (var  i = 0; i < Config.Economy.initialFields;++i)
				{
					var plan = new ConstructionPlan(gameState, "structures/{civ}_field", { "base" : this.ID });
					plan.isGo = function() { return false; };	// don't start right away.
					queues.field.addItem(plan);
				}
			} else if (!this.isFarming && count < 650)
			{
				for (i in queues.field.queue)
					queues.field.queue[i].isGo = function() { return true; };	// start them
				this.isFarming = true;
			}
			if (this.isFarming)
			{
				var numFarms = 0;
				this.buildings.filter(Filters.byClass("Field")).forEach(function (field) {
					if (field.resourceSupplyAmount() > 400)
						numFarms++;
				});
				var numFd = gameState.countEntitiesByType(gameState.applyCiv("foundation|structures/{civ}_field"), true);
				numFarms += numFd;
				numFarms += queues.field.countQueuedUnits();
				
				// let's see if we need to push new farms.
				if (numFd < 2)
					if (numFarms < Math.round(this.gatherersByType(gameState, "food").length / 4.6) || numFarms < Math.round(this.workers.length / 15.0))
						queues.field.addItem(new ConstructionPlan(gameState, "structures/{civ}_field", { "base" : this.ID }));
				// TODO: refine count to only count my base.
			}
		} else if (queues.dropsites.length() === 0 && gameState.countFoundationsWithType(gameState.applyCiv("structures/{civ}_storehouse")) === 0) {
			var wantedDPs = Math.ceil(this.gatherersByType(gameState, type).length / 12.0);
			var need = wantedDPs - this.getResourceLevel(gameState,type, "dropsites-dpcount",2000);
			if (need > 0)
			{
				var pos = this.findBestDropsiteLocation(gameState, type);
				if (!pos)
				{
					debug ("Found no right position for a " + type + " dropsite, going into \"noSpot\" mode");
					this.willGather[type] = 2;	// won't build
					// TODO: tell the HQ we'll be needing a new base for this resource, or tell it we've ran out of resource Z.
				} else {
					debug ("planning new dropsite for " + type);
					queues.dropsites.addItem(new ConstructionPlan(gameState, "structures/{civ}_storehouse",{ "base" : this.ID }, 0, -1, pos));
				}
			}
		}
	}
	
};

// let's return the estimated gather rates.
BaseManager.prototype.getGatherRates = function(gameState, currentRates) {

};

BaseManager.prototype.assignRolelessUnits = function(gameState) {
	// TODO: make this cleverer.
	var roleless = this.units.filter(Filters.not(Filters.byHasMetadata(PlayerID, "role")));
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
BaseManager.prototype.setWorkersIdleByPriority = function(gameState){
	var self = this;
	if (gameState.currentPhase() < 2 && gameState.getTimeElapsed() < 360000)
		return;	// not in the first phase or the first 6 minutes.
	
	var types = gameState.ai.queueManager.getAvailableResources(gameState);
	
	var bestType = "";
	
	var avgOverdraft = 0;
	
	for (i in types.types)
		avgOverdraft += types[types.types[i]];

	avgOverdraft /= 4;
	
	for (i in types.types)
		if (types[types.types[i]] > avgOverdraft + 200 || (types[types.types[i]] > avgOverdraft && avgOverdraft > 200))
			if (this.gatherersByType(gameState,types.types[i]).length > 0)
			{
				// TODO: perhaps change this?
				var nb = 2;
				this.gatherersByType(gameState,types.types[i]).forEach( function (ent) { //}){
					if (nb > 0)
					{
						//debug ("Moving " +ent.id() + " from " + types.types[i]);
						nb--;
						// TODO: might want to direct assign.
						ent.stopMoving();
						ent.setMetadata(PlayerID, "subrole","idle");
					}
				});
			}
	//debug (currentRates);
};

// TODO: work on this.
BaseManager.prototype.reassignIdleWorkers = function(gameState) {
	
	var self = this;

	// Search for idle workers, and tell them to gather resources based on demand
	var filter = Filters.or(Filters.byMetadata(PlayerID,"subrole","idle"), Filters.not(Filters.byHasMetadata(PlayerID,"subrole")));
	var idleWorkers = gameState.updatingCollection("idle-workers-base-" + this.ID, filter, this.workers);

	if (idleWorkers.length) {
		idleWorkers.forEach(function(ent) {
			// Check that the worker isn't garrisoned
			if (ent.position() === undefined){
				return;
			}
			if (ent.hasClass("Worker")) {
				var types = gameState.ai.HQ.pickMostNeededResources(gameState);
				//debug ("assigning " +ent.id() + " to " + types[0]);
				ent.setMetadata(PlayerID, "subrole", "gatherer");
				ent.setMetadata(PlayerID, "gather-type", types[0]);
				
				if (gameState.turnCache["gathererAssignementCache-" + types[0]])
					gameState.turnCache["gathererAssignementCache-" + types[0]]++;
				else
					gameState.turnCache["gathererAssignementCache-" + types[0]] = 1;
				// Okay let's now check we can actually remain here for that
				if (self.willGather[types[0]] !== 1)
				{
					// TODO: if fail, we should probably pick the second most needed resource.
					gameState.ai.HQ.switchWorkerBase(gameState, ent, types[0]);
				}
			} else {
				ent.setMetadata(PlayerID, "subrole", "hunter");
			}
		});
	}
};

BaseManager.prototype.workersBySubrole = function(gameState, subrole) {
	return gameState.updatingCollection("subrole-" + subrole +"-base-" + this.ID, Filters.byMetadata(PlayerID, "subrole", subrole), this.workers, true);
};

BaseManager.prototype.gatherersByType = function(gameState, type) {
	return gameState.updatingCollection("workers-gathering-" + type +"-base-" + this.ID, Filters.byMetadata(PlayerID, "gather-type", type), this.workersBySubrole(gameState, "gatherer"));
};


// returns an entity collection of workers.
// They are idled immediatly and their subrole set to idle.
BaseManager.prototype.pickBuilders = function(gameState, number) {
	var collec = new EntityCollection(gameState.sharedScript);
	// TODO: choose better.
	var workers = this.workers.filter(Filters.not(Filters.byClass("Cavalry"))).toEntityArray();
	workers.sort(function (a,b) {
		var vala = 0, valb = 0;
		if (a.getMetadata(PlayerID,"subrole") == "builder")
			vala = 100;
		if (b.getMetadata(PlayerID,"subrole") == "builder")
			valb = 100;
		if (a.getMetadata(PlayerID,"plan") != undefined)
			vala = -100;
		if (b.getMetadata(PlayerID,"plan") != undefined)
			valb = -100;
		return a < b
	});
	for (var  i = 0; i < number; ++i)
	{
		workers[i].stopMoving();
		workers[i].setMetadata(PlayerID, "subrole","idle");
		collec.addEnt(workers[i]);
	}
	return collec;
}

BaseManager.prototype.assignToFoundations = function(gameState, noRepair) {
	// If we have some foundations, and we don't have enough builder-workers,
	// try reassigning some other workers who are nearby
	
	// AI tries to use builders sensibly, not completely stopping its econ.
	
	var self = this;
	
	var foundations = this.buildings.filter(Filters.and(Filters.isFoundation(),Filters.not(Filters.byClass("Field")))).toEntityArray();
	var damagedBuildings = this.buildings.filter(function (ent) { if (ent.needsRepair() && ent.getMetadata(PlayerID, "plan") == undefined) { return true; } return false; }).toEntityArray();
	
	// Check if nothing to build
	if (!foundations.length && !damagedBuildings.length){
		return;
	}
	var workers = this.workers.filter(Filters.not(Filters.byClass("Cavalry")));
	var builderWorkers = this.workersBySubrole(gameState, "builder");
	var idleBuilderWorkers = this.workersBySubrole(gameState, "builder").filter(Filters.isIdle());

	// if we're constructing and we have the foundations to our base anchor, only try building that.
	if (this.constructing == true && this.buildings.filter(Filters.and(Filters.isFoundation(), Filters.byMetadata(PlayerID, "baseAnchor", true))).length !== 0)
	{
		foundations = this.buildings.filter(Filters.byMetadata(PlayerID, "baseAnchor", true)).toEntityArray();
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
	
	var maxTotalBuilders = Math.ceil(workers.length * 0.15);
	if (this.constructing == true && maxTotalBuilders < 15)
		maxTotalBuilders = 15;
	
	for (var i in foundations) {
		var target = foundations[i];
		// Removed: sometimes the AI would not notice it has empty unbuilt fields
		//if (target._template.BuildRestrictions.Category === "Field")
		//	continue; // we do not build fields
		
		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target.id()).length;

		var targetNB = Config.Economy.targetNumBuilders;	// TODO: dynamic that.
		if (target.hasClass("CivCentre") || target.buildTime() > 150 || target.hasClass("House"))
			targetNB *= 2;
		if (target.getMetadata(PlayerID, "baseAnchor") == true)
			targetNB = 15;

		if (assigned < targetNB) {
			if (builderWorkers.length - idleBuilderWorkers.length + addedWorkers < maxTotalBuilders) {

				var addedToThis = 0;
				
				idleBuilderWorkers.forEach(function(ent) {
					if (ent.position() && SquareVectorDistance(ent.position(), target.position()) < 10000 && assigned + addedToThis < targetNB)
					{
						addedWorkers++;
						addedToThis++;
						ent.setMetadata(PlayerID, "target-foundation", target.id());
					}
				});
				if (assigned + addedToThis < targetNB)
				{
					var nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata(PlayerID, "subrole") !== "builder" && ent.position() !== undefined); });
					var nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), targetNB - assigned - addedToThis);
					nearestNonBuilders.forEach(function(ent) {
						addedWorkers++;
						addedToThis++;
						ent.stopMoving();
						ent.setMetadata(PlayerID, "subrole", "builder");
						ent.setMetadata(PlayerID, "target-foundation", target.id());
					});
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
		
		var territory = Map.createTerritoryMap(gameState);
		if (territory.getOwner(target.position()) !== PlayerID || territory.getOwner([target.position()[0] + 5, target.position()[1]]) !== PlayerID)
			continue;
		
		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target.id()).length;
		if (assigned < this.targetNumBuilders/3) {
			if (builderWorkers.length + addedWorkers < this.targetNumBuilders*2) {
				
				var nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata(PlayerID, "subrole") !== "builder" && ent.position() !== undefined); });
				if (gameState.defcon() < 5)
					nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata(PlayerID, "subrole") !== "builder" && ent.hasClass("Female") && ent.position() !== undefined); });
				var nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), this.targetNumBuilders/3 - assigned);
				
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

BaseManager.prototype.update = function(gameState, queues, events) {
	Engine.ProfileStart("Base update - base " + this.ID);
	var self = this;
	
	this.updateDropsites(gameState);
	this.checkResourceLevels(gameState, queues);
	
	Engine.ProfileStart("Assign builders");
	this.assignToFoundations(gameState);
	Engine.ProfileStop()

//	if (!this.constructing)
//	{
	if (gameState.ai.playedTurn % 2 === 0)
		this.setWorkersIdleByPriority(gameState);

	this.assignRolelessUnits(gameState);
		
		/*Engine.ProfileStart("Swap Workers");
		 var gathererGroups = {};
		 gameState.getOwnEntitiesByRole("worker").forEach(function(ent){ }){
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
			ent.setMetadata(PlayerID, "worker-object", new Worker(ent));
		ent.getMetadata(PlayerID, "worker-object").update(self, gameState);
	});
	Engine.ProfileStop();
		
	Engine.ProfileStop();
};
