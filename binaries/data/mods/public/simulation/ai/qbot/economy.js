var EconomyManager = function() {
	this.targetNumBuilders = 5; // number of workers we want building stuff
	this.targetNumFields = 3;
	
	this.resourceMaps = {}; // Contains maps showing the density of wood, stone and metal
	
	this.setCount = 0;  //stops villagers being reassigned to other resources too frequently, count a set number of 
	                    //turns before trying to reassign them.
	
	this.dropsiteNumbers = {wood: 2, stone: 1, metal: 1};
};
// More initialisation for stuff that needs the gameState
EconomyManager.prototype.init = function(gameState){
	this.targetNumWorkers = Math.max(Math.floor(gameState.getPopulationMax()/3), 1);
};

EconomyManager.prototype.trainMoreWorkers = function(gameState, queues) {
	// Count the workers in the world and in progress
	var numWorkers = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("units/{civ}_support_female_citizen"));
	numWorkers += queues.villager.countTotalQueuedUnits();

	// If we have too few, train more
	if (numWorkers < this.targetNumWorkers) {
		for ( var i = 0; i < this.targetNumWorkers - numWorkers; i++) {
			queues.villager.addItem(new UnitTrainingPlan(gameState, "units/{civ}_support_female_citizen", {
				"role" : "worker"
			}));
		}
	}
};

// Pick the resource which most needs another worker
EconomyManager.prototype.pickMostNeededResources = function(gameState) {

	var self = this;

	// Find what resource type we're most in need of
	if (!gameState.turnCache["gather-weights-calculated"]){
		this.gatherWeights = gameState.ai.queueManager.futureNeeds(gameState);
		gameState.turnCache["gather-weights-calculated"] = true;
	}

	var numGatherers = {};
	for ( var type in this.gatherWeights){
		numGatherers[type] = gameState.updatingCollection("workers-gathering-" + type, 
				Filters.byMetadata("gather-type", type), gameState.getOwnEntitiesByRole("worker")).length; 
	}

	var types = Object.keys(this.gatherWeights);
	types.sort(function(a, b) {
		// Prefer fewer gatherers (divided by weight)
		var va = numGatherers[a] / (self.gatherWeights[a]+1);
		var vb = numGatherers[b] / (self.gatherWeights[b]+1);
		return va-vb;
	});

	return types;
};

EconomyManager.prototype.reassignRolelessUnits = function(gameState) {
	//TODO: Move this out of the economic section
	var roleless = gameState.getOwnEntitiesByRole(undefined);

	roleless.forEach(function(ent) {
		if (ent.hasClass("Worker")){
			ent.setMetadata("role", "worker");
		}else if(ent.hasClass("CitizenSoldier") || ent.hasClass("Champion")){
			ent.setMetadata("role", "soldier");
		}else{
			ent.setMetadata("role", "unknown");
		}
	});
};

// If the numbers of workers on the resources is unbalanced then set some of workers to idle so 
// they can be reassigned by reassignIdleWorkers.
EconomyManager.prototype.setWorkersIdleByPriority = function(gameState){
	this.gatherWeights = gameState.ai.queueManager.futureNeeds(gameState);
	
	var numGatherers = {};
	var totalGatherers = 0;
	var totalWeight = 0;
	for ( var type in this.gatherWeights){
		numGatherers[type] = 0;
		totalWeight += this.gatherWeights[type];
	}

	gameState.getOwnEntitiesByRole("worker").forEach(function(ent) {
		if (ent.getMetadata("subrole") === "gatherer"){
			numGatherers[ent.getMetadata("gather-type")] += 1;
			totalGatherers += 1;
		}
	});

	for ( var type in this.gatherWeights){
		var allocation = Math.floor(totalGatherers * (this.gatherWeights[type]/totalWeight));
		if (allocation < numGatherers[type]){
			var numToTake = numGatherers[type] - allocation;
			gameState.getOwnEntitiesByRole("worker").forEach(function(ent) {
				if (ent.getMetadata("subrole") === "gatherer" && ent.getMetadata("gather-type") === type && numToTake > 0){
					ent.setMetadata("subrole", "idle");
					numToTake -= 1;
				}
			});
		}
	}
};

EconomyManager.prototype.reassignIdleWorkers = function(gameState) {
	
	var self = this;

	// Search for idle workers, and tell them to gather resources based on demand
	var filter = Filters.or(Filters.isIdle(), Filters.byMetadata("subrole", "idle"));
	var idleWorkers = gameState.updatingCollection("idle-workers", filter, gameState.getOwnEntitiesByRole("worker"));
	
	if (idleWorkers.length) {
		var resourceSupplies;
		
		idleWorkers.forEach(function(ent) {
			// Check that the worker isn't garrisoned
			if (ent.position() === undefined){
				return;
			}
			
			var types = self.pickMostNeededResources(gameState);
			
			ent.setMetadata("subrole", "gatherer");
			ent.setMetadata("gather-type", types[0]);
		});
	}
};

EconomyManager.prototype.workersBySubrole = function(gameState, subrole) {
	var workers = gameState.getOwnEntitiesByRole("worker");
	return gameState.updatingCollection("subrole-" + subrole, Filters.byMetadata("subrole", subrole), workers);
};

EconomyManager.prototype.assignToFoundations = function(gameState) {
	// If we have some foundations, and we don't have enough
	// builder-workers,
	// try reassigning some other workers who are nearby

	var foundations = gameState.getOwnFoundations();

	// Check if nothing to build
	if (!foundations.length){
		return;
	}

	var workers = gameState.getOwnEntitiesByRole("worker");

	var builderWorkers = this.workersBySubrole(gameState, "builder");

	// Check if enough builders
	var extraNeeded = this.targetNumBuilders - builderWorkers.length;
	if (extraNeeded <= 0){
		return;
	}

	// Pick non-builders who are closest to the first foundation,
	// and tell them to start building it

	var target = foundations.toEntityArray()[0];

	var nonBuilderWorkers = workers.filter(function(ent) {
		// check position so garrisoned units aren't tasked
		return (ent.getMetadata("subrole") !== "builder" && ent.position() !== undefined);
	});

	var nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), extraNeeded);

	// Order each builder individually, not as a formation
	nearestNonBuilders.forEach(function(ent) {
		ent.setMetadata("subrole", "builder");
		ent.setMetadata("target-foundation", target);
	});
};

EconomyManager.prototype.buildMoreFields = function(gameState, queues) {
	// give time for treasures to be gathered
	if (gameState.getTimeElapsed() < 30 * 1000)
		return;
	
	var numFood = 0;
	
	gameState.updatingCollection("active-dropsite-food", Filters.byMetadata("active-dropsite-food", true), 
			gameState.getOwnDropsites("food")).forEach(function (dropsite){
		numFood += dropsite.getMetadata("nearby-resources-food").length;
	});
	
	numFood += gameState.countFoundationsWithType(gameState.applyCiv("structures/{civ}_field"));
	numFood += queues.field.totalLength();
	
	for ( var i = numFood; i < this.targetNumFields; i++) {
		queues.field.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_field"));
	}
};

// If all the CC's are destroyed then build a new one
EconomyManager.prototype.buildNewCC= function(gameState, queues) {
	var numCCs = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_civil_centre"));
	numCCs += queues.civilCentre.totalLength();

	for ( var i = numCCs; i < 1; i++) {
		queues.civilCentre.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_civil_centre"));
	}
};

//creates and maintains a map of tree density
EconomyManager.prototype.updateResourceMaps = function(gameState, events){
	// The weight of the influence function is amountOfResource/decreaseFactor 
	var decreaseFactor = {'wood': 15, 'stone': 100, 'metal': 100, 'food': 20};
	// This is the maximum radius of the influence
	var radius = {'wood':13, 'stone': 10, 'metal': 10, 'food': 10};
	
	var self = this;
	
	for (var resource in radius){
		// if there is no resourceMap create one with an influence for everything with that resource
		if (! this.resourceMaps[resource]){
			this.resourceMaps[resource] = new Map(gameState);

			var supplies = gameState.getResourceSupplies(resource);
			supplies.forEach(function(ent){
				if (!ent.position()){
					return;
				}
				var x = Math.round(ent.position()[0] / gameState.cellSize);
				var z = Math.round(ent.position()[1] / gameState.cellSize);
				var strength = Math.round(ent.resourceSupplyMax()/decreaseFactor[resource]);
				self.resourceMaps[resource].addInfluence(x, z, radius[resource], strength);
			});
		}
		// TODO: fix for treasure and move out of loop
		// Look for destroy events and subtract the entities original influence from the resourceMap
		for (var i in events) {
			var e = events[i];

			if (e.type === "Destroy") {
				if (e.msg.entityObj){
					var ent = e.msg.entityObj;
					if (ent && ent.position() && ent.resourceSupplyType() && ent.resourceSupplyType().generic === resource){
						var x = Math.round(ent.position()[0] / gameState.cellSize);
						var z = Math.round(ent.position()[1] / gameState.cellSize);
						var strength = Math.round(ent.resourceSupplyMax()/decreaseFactor[resource]);
						this.resourceMaps[resource].addInfluence(x, z, radius[resource], -strength);
					}
				}
			}else if (e.type === "Create") {
				if (e.msg.entityObj){
					var ent = e.msg.entityObj;
					if (ent && ent.position() && ent.resourceSupplyType() && ent.resourceSupplyType().generic === resource){
						var x = Math.round(ent.position()[0] / gameState.cellSize);
						var z = Math.round(ent.position()[1] / gameState.cellSize);
						var strength = Math.round(ent.resourceSupplyMax()/decreaseFactor[resource]);
						this.resourceMaps[resource].addInfluence(x, z, radius[resource], strength);
					}
				}
			}
		}
	} 
	
	//this.resourceMaps['wood'].dumpIm("tree_density.png");
};

// Returns the position of the best place to build a new dropsite for the specified resource
EconomyManager.prototype.getBestResourceBuildSpot = function(gameState, resource){
	// A map which gives a positive weight for all CCs and adds a negative weight near all dropsites
	var friendlyTiles = new Map(gameState);
	gameState.getOwnEntities().forEach(function(ent) {
		// We want to build near a CC of ours
		if (ent.hasClass("CivCentre")){
			var infl = 200;

			var pos = ent.position();
			var x = Math.round(pos[0] / gameState.cellSize);
			var z = Math.round(pos[1] / gameState.cellSize);
			friendlyTiles.addInfluence(x, z, infl, 0.1 * infl);
			friendlyTiles.addInfluence(x, z, infl/2, 0.1 * infl);
		}
		// We don't want multiple dropsites at one spot so add a negative for all dropsites
		if (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf(resource) !== -1){
			var infl = 20;
			
			var pos = ent.position();
			var x = Math.round(pos[0] / gameState.cellSize);
			var z = Math.round(pos[1] / gameState.cellSize);
			
			friendlyTiles.addInfluence(x, z, infl, -50, 'quadratic');
		}
	});
	
	// Multiply by tree density to get a combination of the two maps
	friendlyTiles.multiply(this.resourceMaps[resource]);
	
	//friendlyTiles.dumpIm(resource + "_density_fade.png", 10000);
	
	var obstructions = Map.createObstructionMap(gameState);
	obstructions.expandInfluences();
	
	var bestIdx = friendlyTiles.findBestTile(4, obstructions)[0];
	
	// Convert from 1d map pixel coordinates to game engine coordinates
	var x = ((bestIdx % friendlyTiles.width) + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * gameState.cellSize;
	return [x,z];
};

EconomyManager.prototype.updateResourceConcentrations = function(gameState){
	var self = this;
	var resources = ["food", "wood", "stone", "metal"];
	for (var key in resources){
		var resource = resources[key];
		gameState.getOwnEntities().forEach(function(ent) {
			if (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf(resource) !== -1){
				var radius = 14;
				
				var pos = ent.position();
				var x = Math.round(pos[0] / gameState.cellSize);
				var z = Math.round(pos[1] / gameState.cellSize);
				
				var quantity = self.resourceMaps[resource].sumInfluence(x, z, radius);
				
				ent.setMetadata("resourceQuantity_" + resource, quantity);
			}
		});	
	}
};

// Stores lists of nearby resources
EconomyManager.prototype.updateNearbyResources = function(gameState){
	var self = this;
	var resources = ["food", "wood", "stone", "metal"];
	var resourceSupplies;
	var radius = 100;
	for (var key in resources){
		var resource = resources[key];
		
		gameState.getOwnDropsites(resource).forEach(function(ent) {
			if (ent.getMetadata("nearby-resources-" + resource) === undefined){
				var filterPos = Filters.byStaticDistance(ent.position(), radius);
				
				var collection = gameState.getResourceSupplies(resource).filter(filterPos);
				collection.registerUpdates();
				
				ent.setMetadata("nearby-resources-" + resource, collection);
				ent.setMetadata("active-dropsite-" + resource, true);
			}
			
			if (ent.getMetadata("nearby-resources-" + resource).length === 0){
				ent.setMetadata("active-dropsite-" + resource, false);
			}else{
				ent.setMetadata("active-dropsite-" + resource, true);
			}
			/*
			// Make resources glow wildly 
			if (resource == "food"){
				ent.getMetadata("nearby-resources-" + resource).forEach(function(ent){
					Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [10,0,0]});
				});
			}
			if (resource == "wood"){
				ent.getMetadata("nearby-resources-" + resource).forEach(function(ent){
					Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,10,0]});
				});
			}
			if (resource == "metal"){
				ent.getMetadata("nearby-resources-" + resource).forEach(function(ent){
					Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0,10]});
				});
			}*/
		});
	}
};

//return the number of resource dropsites with an acceptable amount of the resource nearby
EconomyManager.prototype.checkResourceConcentrations = function(gameState, resource){
	//TODO: make these values adaptive 
	var requiredInfluence = {wood: 16000, stone: 300, metal: 300};
	var count = 0;
	gameState.getOwnEntities().forEach(function(ent) {
		if (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf(resource) !== -1){
			var quantity = ent.getMetadata("resourceQuantity_" + resource);
			
			if (quantity >= requiredInfluence[resource]){
				count ++;
			}
		}
	});
	return count;
};

EconomyManager.prototype.buildMarket = function(gameState, queues){
	if (gameState.getTimeElapsed() > 600 * 1000){
		if (queues.economicBuilding.totalLength() === 0 && 
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market")) === 0){ 
			//only ever build one storehouse/CC/market at a time
			queues.economicBuilding.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_market"));
		}
	}
};

EconomyManager.prototype.buildDropsites = function(gameState, queues){
	if (queues.economicBuilding.totalLength() === 0 && 
			gameState.countFoundationsWithType(gameState.applyCiv("structures/{civ}_storehouse")) === 0 &&
			gameState.countFoundationsWithType(gameState.applyCiv("structures/{civ}_civil_centre")) === 0){ 
			//only ever build one storehouse/CC/market at a time
		if (gameState.getTimeElapsed() > 30 * 1000){
			for (var resource in this.dropsiteNumbers){
				if (this.checkResourceConcentrations(gameState, resource) < this.dropsiteNumbers[resource]){
					var spot = this.getBestResourceBuildSpot(gameState, resource);
					
					var myCivCentres = gameState.getOwnEntities().filter(function(ent) {
						if (!ent.hasClass("CivCentre") || ent.position() === undefined){
							return false;
						} 
						var dx = (spot[0]-ent.position()[0]);
						var dy = (spot[1]-ent.position()[1]);
						var dist2 = dx*dx + dy*dy;
						return (ent.hasClass("CivCentre") && dist2 < 180*180);
					});
					
					if (myCivCentres.length === 0){
						queues.economicBuilding.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_civil_centre", spot));
					}else{
						queues.economicBuilding.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_storehouse", spot));
					}
					break;
				}
			}
		}
	}
};

EconomyManager.prototype.update = function(gameState, queues, events) {
	Engine.ProfileStart("economy update");
	
	this.reassignRolelessUnits(gameState);
	
	this.buildNewCC(gameState,queues);

	Engine.ProfileStart("Train workers and build farms");
	this.trainMoreWorkers(gameState, queues);

	this.buildMoreFields(gameState, queues);
	Engine.ProfileStop();
	
	//Later in the game we want to build stuff faster.
	if (gameState.countEntitiesByType(gameState.applyCiv("units/{civ}_support_female_citizen")) > this.targetNumWorkers * 0.5) {
		this.targetNumBuilders = 10;
	}else{
		this.targetNumBuilders = 5;
	}
	
	if (gameState.countEntitiesByType(gameState.applyCiv("units/{civ}_support_female_citizen")) > this.targetNumWorkers * 0.8) {
		this.dropsiteNumbers = {wood: 3, stone: 2, metal: 2};
	}else{
		this.dropsiteNumbers = {wood: 2, stone: 1, metal: 1};
	}
	
	Engine.ProfileStart("Update Resource Maps and Concentrations");
	this.updateResourceMaps(gameState, events);
	this.updateResourceConcentrations(gameState);
	this.updateNearbyResources(gameState);
	Engine.ProfileStop();
	
	Engine.ProfileStart("Build new Dropsites");
	this.buildDropsites(gameState, queues);
	Engine.ProfileStop();
	
	this.buildMarket(gameState, queues);
	
	// TODO: implement a timer based system for this
	this.setCount += 1;
	if (this.setCount >= 20){
		this.setWorkersIdleByPriority(gameState);
		this.setCount = 0;
	}
	
	Engine.ProfileStart("Reassign Idle Workers");
	this.reassignIdleWorkers(gameState);
	Engine.ProfileStop();
	
	Engine.ProfileStart("Swap Workers");
	var gathererGroups = {};
	gameState.getOwnEntitiesByRole("worker").forEach(function(ent){
		var key = uneval(ent.resourceGatherRates());
		if (!gathererGroups[key]){
			gathererGroups[key] = {"food": [], "wood": [], "metal": [], "stone": []};
		}
		if (ent.getMetadata("gather-type") in gathererGroups[key]){
			gathererGroups[key][ent.getMetadata("gather-type")].push(ent);
		}
	});
	
	for (var i in gathererGroups){
		for (var j in gathererGroups){
			var a = eval(i);
			var b = eval(j);
			if (a["food.grain"]/b["food.grain"] > a["wood.tree"]/b["wood.tree"] && gathererGroups[i]["wood"].length > 0 && gathererGroups[j]["food"].length > 0){
				for (var k = 0; k < Math.min(gathererGroups[i]["wood"].length, gathererGroups[j]["food"].length); k++){
					gathererGroups[i]["wood"][k].setMetadata("gather-type", "food");
					gathererGroups[j]["food"][k].setMetadata("gather-type", "wood");
				}
			}
		}
	}
	Engine.ProfileStop();
	
	Engine.ProfileStart("Assign builders");
	this.assignToFoundations(gameState);
	Engine.ProfileStop();
	
	Engine.ProfileStart("Run Workers");
	gameState.getOwnEntitiesByRole("worker").forEach(function(ent){
		if (!ent.getMetadata("worker-object")){
			ent.setMetadata("worker-object", new Worker(ent));
		}
		ent.getMetadata("worker-object").update(gameState);
	});
	
	// Gatherer count updates for non-workers
	var filter = Filters.and(Filters.not(Filters.byMetadata("worker-object", undefined)), 
	                         Filters.not(Filters.byMetadata("role", "worker")));
	gameState.updatingCollection("reassigned-workers", filter, gameState.getOwnEntities()).forEach(function(ent){
		ent.getMetadata("worker-object").updateGathererCounts(gameState);
	});
	
	// Gatherer count updates for destroyed units
	for (var i in events) {
		var e = events[i];

		if (e.type === "Destroy") {
			if (e.msg.metadata && e.msg.metadata[gameState.getPlayerID()] && e.msg.metadata[gameState.getPlayerID()]["worker-object"]){
				e.msg.metadata[gameState.getPlayerID()]["worker-object"].updateGathererCounts(gameState, true);
			}
		}
	}
	Engine.ProfileStop();

	Engine.ProfileStop();
};
