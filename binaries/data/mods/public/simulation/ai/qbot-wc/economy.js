var EconomyManager = function() {
	this.targetNumBuilders = 3; // number of workers we want building stuff
	this.targetNumFields = 3;
	
	this.resourceMaps = {}; // Contains maps showing the density of wood, stone and metal
	
	this.setCount = 0;  //stops villagers being reassigned to other resources too frequently, count a set number of 
	                    //turns before trying to reassign them.
	
	// this means we'll have about a big third of women, and thus we can maximize resource gathering rates.
	this.femaleRatio = 0.4;

	this.farmingFields = false;
	
	this.dropsiteNumbers = {"wood": 1, "stone": 0.5, "metal": 0.5};
};
// More initialisation for stuff that needs the gameState
EconomyManager.prototype.init = function(gameState){
	this.targetNumWorkers = Math.max(Math.floor(gameState.getPopulationMax()*0.55), 1);
	
	// initialize once all the resource maps.
	this.updateResourceMaps(gameState, ["food","wood","stone","metal"]);
	this.updateResourceConcentrations(gameState,"food");
	this.updateResourceConcentrations(gameState,"wood");
	this.updateResourceConcentrations(gameState,"stone");
	this.updateResourceConcentrations(gameState,"metal");
	this.updateNearbyResources(gameState, "food");
	this.updateNearbyResources(gameState, "wood");
	this.updateNearbyResources(gameState, "stone");
	this.updateNearbyResources(gameState, "metal");
};

// okay, so here we'll create both females and male workers.
// We'll try to keep close to the "ratio" defined atop.
// qBot picks the best citizen soldier available: the cheapest and the fastest walker
// some civs such as Macedonia have 2 kinds of citizen soldiers: phalanx that are slow
// (speed:6) and peltasts that are very fast (speed: 11). Here, qBot will choose the peltast
// resulting in faster resource gathering.
// I'll also avoid creating citizen soldiers in the beginning because it's slower.
EconomyManager.prototype.trainMoreWorkers = function(gameState, queues) {
	// Count the workers in the world and in progress
	var numFemales = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("units/{civ}_support_female_citizen"));
	numFemales += queues.villager.countTotalQueuedUnits();

	// counting the workers that aren't part of a plan
	var numWorkers = 0;
	gameState.getOwnEntities().forEach (function (ent) {
		if (ent.getMetadata("role") == "worker" && ent.getMetadata("plan") == undefined)
			numWorkers++;
	});
	gameState.getOwnTrainingFacilities().forEach(function(ent) {
	ent.trainingQueue().forEach(function(item) {
		if (item.metadata && item.metadata.role == "worker" && item.metadata.plan == undefined)
			numWorkers += item.count;
		});
	});
	var numQueued = queues.villager.countTotalQueuedUnits() + queues.citizenSoldier.countTotalQueuedUnits();
	var numTotal = numWorkers + numQueued;
	
	this.targetNumFields = numFemales/15;
	if ((gameState.ai.playedTurn+2) % 3 === 0) {
		this.dropsiteNumbers = {"wood": Math.ceil((numWorkers)/25)/2, "stone": Math.ceil((numWorkers)/40)/2, "metal": Math.ceil((numWorkers)/30)/2};
	}

	//debug (numTotal + "/" +this.targetNumWorkers + ", " +numFemales +"/" +numTotal);
	
	// If we have too few, train more
	// should plan enough to always have females…
	if (numTotal < this.targetNumWorkers && numQueued < 20) {
		var template = gameState.applyCiv("units/{civ}_support_female_citizen");
		var size = Math.min(Math.ceil(gameState.getTimeElapsed() / 240000),5);
		if (numFemales/numTotal > this.femaleRatio && gameState.getTimeElapsed() > 60*1000) {
			var size = Math.min(Math.ceil(gameState.getTimeElapsed() / 120000),5);
			template = this.findBestTrainableUnit(gameState, ["CitizenSoldier", "Infantry"], [ ["cost",1], ["speed",0.5]]);
			if (!template) {
				template = gameState.applyCiv("units/{civ}_support_female_citizen");
			}
		}
		if (template === gameState.applyCiv("units/{civ}_support_female_citizen"))
			queues.villager.addItem(new UnitTrainingPlan(gameState, template, { "role" : "worker" },size ));
		else
			queues.citizenSoldier.addItem(new UnitTrainingPlan(gameState, template, { "role" : "worker" },size ));
	}
};
// picks the best template based on parameters and classes
EconomyManager.prototype.findBestTrainableUnit = function(gameState, classes, parameters) {
	var units = gameState.findTrainableUnits(classes);

	if (units.length === 0)
		return undefined;

	units.sort(function(a, b) { //}) {
		var aDivParam = 0, bDivParam = 0;
		var aTopParam = 0, bTopParam = 0;
		for (i in parameters) {
			var param = parameters[i];
			
			if (param[0] == "base") {
				aTopParam = param[1];
				bTopParam = param[1];
			}
			if (param[0] == "strength") {
				aTopParam += a[1].getMaxStrength() * param[1];
				bTopParam += b[1].getMaxStrength() * param[1];
			}
			if (param[0] == "speed") {
				aTopParam += a[1].walkSpeed() * param[1];
				bTopParam += b[1].walkSpeed() * param[1];
			}
			
			if (param[0] == "cost") {
				aDivParam += a[1].costSum() * param[1];
				bDivParam += b[1].costSum() * param[1];
			}
		}
		return -(aTopParam/(aDivParam+1)) + (bTopParam/(bDivParam+1));
	});
	return units[0][0];
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
	for (type in this.gatherWeights){
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
	
	// up to 2.5 buildings at once (that is 3, but one won't be complete).

	var foundations = gameState.getOwnFoundations().toEntityArray();
	var damagedBuildings = gameState.getOwnEntities().filter(function (ent) { if (ent.needsRepair() && ent.getMetadata("plan") == undefined) { return true; } return false; }).toEntityArray();

	// Check if nothing to build
	if (!foundations.length && !damagedBuildings.length){
		return;
	}
	var workers = gameState.getOwnEntitiesByRole("worker");
	var builderWorkers = this.workersBySubrole(gameState, "builder");
	
	var addedWorkers = 0;
	
	for (i in foundations) {
		var target = foundations[i];
		if (target._template.BuildRestrictions.Category === "Field")
			continue; // we do not build fields
		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target).length;
		if (assigned < this.targetNumBuilders) {
			if (builderWorkers.length + addedWorkers < this.targetNumBuilders*Math.min(2.5,gameState.getTimeElapsed()/60000)) {
				var nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata("subrole") !== "builder" && ent.getMetadata("gather-type") !== "food" && ent.position() !== undefined); });
				var nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), this.targetNumBuilders - assigned);

				nearestNonBuilders.forEach(function(ent) {
					addedWorkers++;
					ent.setMetadata("subrole", "builder");
					ent.setMetadata("target-foundation", target);
				});
				if (this.targetNumBuilders - assigned - nearestNonBuilders.length > 0) {
					var nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata("subrole") !== "builder" && ent.position() !== undefined); });
					var nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), this.targetNumBuilders - assigned);
					nearestNonBuilders.forEach(function(ent) {
						addedWorkers++;
						ent.setMetadata("subrole", "builder");
						ent.setMetadata("target-foundation", target);
					});
				}
			}
		}
	}
	// don't repair if we're still under attack, unless it's like a vital (civcentre or wall) building that's getting destroyed.
	for (i in damagedBuildings) {
		var target = damagedBuildings[i];
		if (gameState.defcon() < 5) {
			if (target.healthLevel() > 0.5 || !target.hasClass("CivCentre") || !target.hasClass("StoneWall")) {
				continue;
			}
		}
		var assigned = gameState.getOwnEntitiesByMetadata("target-foundation", target).length;
		if (assigned < this.targetNumBuilders) {
			if (builderWorkers.length + addedWorkers < this.targetNumBuilders*2.5) {
				
				var nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata("subrole") !== "builder" && ent.position() !== undefined); });
				if (gameState.defcon() < 5)
					nonBuilderWorkers = workers.filter(function(ent) { return (ent.getMetadata("subrole") !== "builder" && ent.hasClass("Female") && ent.position() !== undefined); });
				var nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), this.targetNumBuilders - assigned);
				
				nearestNonBuilders.forEach(function(ent) {
					addedWorkers++;
					ent.setMetadata("subrole", "builder");
					ent.setMetadata("target-foundation", target);
				});
			}
		}
	}
};

EconomyManager.prototype.buildMoreFields = function(gameState, queues) {
	if (this.farmingFields === true) {
		var numFarms = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_field"));
		numFarms += queues.field.countTotalQueuedUnits();
	
		if (numFarms < this.targetNumFields + Math.floor(gameState.getTimeElapsed() / 900000))
			queues.field.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_field"));
	} else {
		var foodAmount = 0;
		gameState.getOwnDropsites("food").forEach( function (ent) { //}){
			if (ent.getMetadata("resource-quantity-food") != undefined) {
				foodAmount += ent.getMetadata("resource-quantity-food");
			} else {
				foodAmount = 300; // wait till we initialize
			}
		});
		if (foodAmount < 300)
			this.farmingFields = true;
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

// creates and maintains a map of unused resource density
// this also takes dropsites into account.
// resources that are "part" of a dropsite are not counted.
EconomyManager.prototype.updateResourceMaps = function(gameState, events) {
	
	// TODO: centralize with that other function that uses the same variables
	// The weight of the influence function is amountOfResource/decreaseFactor
	var decreaseFactor = {'wood': 12.0, 'stone': 10.0, 'metal': 10.0, 'food': 20.0};
	// This is the maximum radius of the influence
	var radius = {'wood':25.0, 'stone': 24.0, 'metal': 24.0, 'food': 24.0};
	// smallRadius is the distance necessary to mark a resource as linked to a dropsite.
	var smallRadius = { 'food':70*70,'wood':120*120,'stone':60*60,'metal':60*60 };
	// bigRadius is the distance for a weak link (resources are considered when building other dropsites)
	// and their resource amount is divided by 3 when checking for dropsite resource level.
	var bigRadius = { 'food':100*100,'wood':180*180,'stone':120*120,'metal':120*120 };

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
	}
	// Look for destroy events and subtract the entities original influence from the resourceMap
	// also look for dropsite destruction and add the associated entities (along with unmarking them)
	for (var i in events) {
		var e = events[i];
		if (e.type === "Destroy") {
			
			if (e.msg.entityObj){
				var ent = e.msg.entityObj;
				if (ent && ent.position() && ent.resourceSupplyType() && ent.resourceSupplyType().generic !== "treasure") {
					if  (e.msg.metadata[gameState.getPlayerID()] && !e.msg.metadata[gameState.getPlayerID()]["linked-dropsite"]) {
						var resource = ent.resourceSupplyType().generic;
						var x = Math.round(ent.position()[0] / gameState.cellSize);
						var z = Math.round(ent.position()[1] / gameState.cellSize);
						var strength = Math.round(ent.resourceSupplyMax()/decreaseFactor[resource]);
						this.resourceMaps[resource].addInfluence(x, z, radius[resource], -strength);
					}
				}
				if (ent && ent.owner() == gameState.player && ent.resourceDropsiteTypes() !== undefined) {
					var resources = ent.resourceDropsiteTypes();
					for (i in resources) {
						var resource = resources[i];
						// loop through all dropsites to see if the resources of his entity collection could
						// be taken over by another dropsite
						var dropsites = gameState.getOwnDropsites(resource);
						var metadata = e.msg.metadata[gameState.getPlayerID()];
						metadata["linked-resources-" + resource].filter( function (supply) { //}){
							var takenOver = false;
							dropsites.forEach( function (otherDropsite) { //}) {
								var distance = SquareVectorDistance(supply.position(), otherDropsite.position());
								if (supply.getMetadata("linked-dropsite") == undefined || supply.getMetadata("linked-dropsite-dist") > distance) {
									if (distance < bigRadius[resource]) {
										supply.setMetadata("linked-dropsite", otherDropsite.id() );
										supply.setMetadata("linked-dropsite-dist", +distance);
										if (distance < smallRadius[resource]) {
											takenOver = true;
											supply.setMetadata("linked-dropsite-nearby", true );
										} else {
											supply.setMetadata("linked-dropsite-nearby", false );
										}
									}
								}
							});
							if (!takenOver) {
								var x = Math.round(supply.position()[0] / gameState.cellSize);
								var z = Math.round(supply.position()[1] / gameState.cellSize);
								var strength = Math.round(supply.resourceSupplyMax()/decreaseFactor[resource]);
								self.resourceMaps[resource].addInfluence(x, z, radius[resource], strength);
							}
						});
					}
				}
			}
		} else if (e.type === "Create") {
			if (e.msg.entityObj){
				var ent = e.msg.entityObj;
				if (ent && ent.position() && ent.resourceSupplyType() && ent.resourceSupplyType().generic !== "treasure"){
					var resource = ent.resourceSupplyType().generic;
					
					var addToMap = true;
					var dropsites = gameState.getOwnDropsites(resource);
					dropsites.forEach( function (otherDropsite) { //}) {
						var distance = SquareVectorDistance(ent.position(), otherDropsite.position());
						if (ent.getMetadata("linked-dropsite") == undefined || ent.getMetadata("linked-dropsite-dist") > distance) {
							if (distance < bigRadius[resource]) {
								if (distance < smallRadius[resource]) {
									if (ent.getMetadata("linked-dropsite") == undefined)
										addToMap = false;
									ent.setMetadata("linked-dropsite-nearby", true );
								} else {
									ent.setMetadata("linked-dropsite-nearby", false );
								}
								ent.setMetadata("linked-dropsite", otherDropsite.id() );
								ent.setMetadata("linked-dropsite-dist", +distance);
							}
						}
					});
					if (addToMap) {
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
	
	var friendlyTiles = new Map(gameState);

	friendlyTiles.add(this.resourceMaps[resource]);

	for (i in this.resourceMaps)
		if (i !== "food")
			friendlyTiles.multiply(this.resourceMaps[i],true,100,1.5);
	
	//friendlyTiles.dumpIm(gameState.getTimeElapsed() + "_" + resource + "_density_fade_base.png", 65000);

	var territory = Map.createTerritoryMap(gameState);
	friendlyTiles.multiplyTerritory(gameState,territory);
	friendlyTiles.annulateTerritory(gameState,territory);
	
	var resources = ["wood","stone","metal"];
	for (i in resources) {
		gameState.getOwnDropsites(resources[i]).forEach(function(ent) { //)){
			// We don't want multiple dropsites at one spot so set to zero if too close.
			var pos = ent.position();
			var x = Math.round(pos[0] / gameState.cellSize);
			var z = Math.round(pos[1] / gameState.cellSize);
			friendlyTiles.setInfluence(x, z, 17, 0);
		});
	}
	//friendlyTiles.dumpIm(gameState.getTimeElapsed() + "_" + resource + "_density_fade_final.png", 10000);
	friendlyTiles.multiply(gameState.ai.distanceFromMeMap,true,gameState.ai.distanceFromMeMap.width/3,2);
	//friendlyTiles.dumpIm(gameState.getTimeElapsed() + "_" + resource + "_density_fade_final2.png", 10000);

	var obstructions = Map.createObstructionMap(gameState);
	obstructions.expandInfluences();
	
	var isCivilCenter = false;
	var bestIdx = friendlyTiles.findBestTile(2, obstructions)[0];
	var x = ((bestIdx % friendlyTiles.width) + 0.5) * gameState.cellSize;
	var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * gameState.cellSize;

	if (territory.getOwner([x,z]) === 0) {
		isCivilCenter = true;
		bestIdx = friendlyTiles.findBestTile(4, obstructions)[0];
		x = ((bestIdx % friendlyTiles.width) + 0.5) * gameState.cellSize;
		z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * gameState.cellSize;
	}
	return [isCivilCenter, [x,z]];
};
EconomyManager.prototype.updateResourceConcentrations = function(gameState, resource){
	var self = this;
	gameState.getOwnDropsites(resource).forEach(function(dropsite) { //}){
		var amount = 0;
		var amountFar = 0;
		if (dropsite.getMetadata("linked-resources-" + resource) == undefined)
			return;
		dropsite.getMetadata("linked-resources-" + resource).forEach(function(supply){ //}){
			if (supply.getMetadata("full") == true)
				return;
			if (supply.getMetadata("linked-dropsite-nearby") == true)
				amount += supply.resourceSupplyAmount();
			else
				amountFar += supply.resourceSupplyAmount();
			supply.setMetadata("dp-update-value",supply.resourceSupplyAmount());
		});
		dropsite.setMetadata("resource-quantity-" + resource, amount);
		dropsite.setMetadata("resource-quantity-far-" + resource, amountFar);
	});
};

// Stores lists of nearby resources
EconomyManager.prototype.updateNearbyResources = function(gameState,resource){
	var self = this;
	var resources = ["food", "wood", "stone", "metal"];
	var resourceSupplies;

	// TODO: centralize with that other function that uses the same variables
	// The weight of the influence function is amountOfResource/decreaseFactor
	var decreaseFactor = {'wood': 12.0, 'stone': 10.0, 'metal': 10.0, 'food': 20.0};
	// This is the maximum radius of the influence
	var radius = {'wood':25.0, 'stone': 24.0, 'metal': 24.0, 'food': 24.0};
	// smallRadius is the distance necessary to mark a resource as linked to a dropsite.
	var smallRadius = { 'food':80*80,'wood':60*60,'stone':70*70,'metal':70*70 };
	// bigRadius is the distance for a weak link (resources are considered when building other dropsites)
	// and their resource amount is divided by 3 when checking for dropsite resource level.
	var bigRadius = { 'food':140*140,'wood':140*140,'stone':140*140,'metal':140*140 };
	
	gameState.getOwnDropsites(resource).forEach(function(ent) { //}){
		
		if (ent.getMetadata("nearby-resources-" + resource) === undefined){
			// let's defined the entity collections (by metadata)
			gameState.getResourceSupplies(resource).filter( function (supply) { //}){
				var distance = SquareVectorDistance(supply.position(), ent.position());
				// if we're close than the current linked-dropsite, or if it's not linked
				// TODO: change when actualy resource counting is implemented.
				
				if (supply.getMetadata("linked-dropsite") == undefined || supply.getMetadata("linked-dropsite-dist") > distance) {
					if (distance < bigRadius[resource]) {
						if (distance < smallRadius[resource]) {
							// it's new to the game, remove it from the resource maps
							if (supply.getMetadata("linked-dropsite") == undefined || supply.getMetadata("linked-dropsite-nearby") == false) {
								var x = Math.round(supply.position()[0] / gameState.cellSize);
								var z = Math.round(supply.position()[1] / gameState.cellSize);
								var strength = Math.round(supply.resourceSupplyMax()/decreaseFactor[resource]);
								self.resourceMaps[resource].addInfluence(x, z, radius[resource], -strength);
							}
							supply.setMetadata("linked-dropsite-nearby", true );
						} else {
							supply.setMetadata("linked-dropsite-nearby", false );
						}
						supply.setMetadata("linked-dropsite", ent.id() );
						supply.setMetadata("linked-dropsite-dist", +distance);
					}
				}
			});
			// This one is both for the nearby and the linked
			var filter = Filters.byMetadata("linked-dropsite", ent.id());
			var collection = gameState.getResourceSupplies(resource).filter(filter);
			collection.registerUpdates();
			ent.setMetadata("linked-resources-" + resource, collection);
			
			filter = Filters.byMetadata("linked-dropsite-nearby",true);
			var collection2 = collection.filter(filter);
			collection2.registerUpdates();
			ent.setMetadata("nearby-resources-" + resource, collection2);
			
		}
		
		/*
		// Make resources glow wildly
		if (resource == "food"){
			ent.getMetadata("linked-resources-" + resource).forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [1,0,0]});
			});
			ent.getMetadata("nearby-resources-" + resource).forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [10,0,0]});
			});
		}
		if (resource == "wood"){
			ent.getMetadata("linked-resources-" + resource).forEach(function(ent){
			Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,1,0]});
			});
			ent.getMetadata("nearby-resources-" + resource).forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,10,0]});
			});
		}
		if (resource == "metal"){
			ent.getMetadata("linked-resources-" + resource).forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0,1]});
			});
			ent.getMetadata("nearby-resources-" + resource).forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0,10]});
			});
		}
		if (resource == "stone"){
			ent.getMetadata("linked-resources-" + resource).forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,0.5,1]});
			});
			ent.getMetadata("nearby-resources-" + resource).forEach(function(ent){
				Engine.PostCommand({"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,5,10]});
			});
		}*/
	});
};

//return the number of resource dropsites with an acceptable amount of the resource nearby
EconomyManager.prototype.checkResourceConcentrations = function(gameState, resource){
	//TODO: make these values adaptive
	var requiredInfluence = {"wood": 2500, "stone": 600, "metal": 600};
	var count = 0;
	gameState.getOwnDropsites(resource).forEach(function(ent) { //}){
		if (ent.getMetadata("resource-quantity-" + resource) == undefined || typeof(ent.getMetadata("resource-quantity-" + resource)) !== "number") {
			count++;	// assume it's OK if we don't know.
			return;
		}
		var quantity = +ent.getMetadata("resource-quantity-" + resource);
		var quantityFar = +ent.getMetadata("resource-quantity-far-" + resource);

		if (quantity >= requiredInfluence[resource]) {
			count++;
		} else if (quantity + quantityFar >= requiredInfluence[resource]) {
			count += 0.5 + (quantity/requiredInfluence[resource])/2;
		} else {
			count += ((quantity + quantityFar)/requiredInfluence[resource])/2;
		}
	});
	return count;
};

EconomyManager.prototype.buildMarket = function(gameState, queues){
	if (gameState.getTimeElapsed() > 620 * 1000){
		if (queues.economicBuilding.countTotalQueuedUnitsWithClass("BarterMarket") === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market")) === 0){ 
			//only ever build one mill/CC/market at a time
			queues.economicBuilding.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_market"));
		}
	}
};
// if qBot has resources it doesn't need, it'll try to barter it for resources it needs
// once per turn because the info doesn't update between a turn and I don't want to fix it.
// pretty efficient.
EconomyManager.prototype.tryBartering = function(gameState){
	var done = false;
	if (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_market")) >= 1) {
		
		var needs = gameState.ai.queueManager.futureNeeds(gameState,true);
		var ress = gameState.ai.queueManager.getAvailableResources(gameState);
		
		for (sell in needs) {
			for (buy in needs) {
				if (!done && buy != sell && needs[sell] <= 0 && ress[sell] > 400) {	// if we don't need it and have a buffer
					if ( (ress[buy] < 400) || needs[buy] > 0) {	// if we need that other resource/ have too little of it
						var markets = gameState.getOwnEntitiesByType(gameState.applyCiv("structures/{civ}_market")).toEntityArray();
						markets[0].barter(buy,sell,100);
						//debug ("bartered " +sell +" for " + buy + ", value 100");
						done = true;
					}
				}
			}
		}
	}
};
// so this always try to build dropsites.
EconomyManager.prototype.buildDropsites = function(gameState, queues){
	if ( (queues.economicBuilding.totalLength() - queues.economicBuilding.countTotalQueuedUnitsWithClass("BarterMarket")) === 0 &&
		 gameState.countFoundationsWithType(gameState.applyCiv("structures/{civ}_mill")) === 0 &&
		 gameState.countFoundationsWithType(gameState.applyCiv("structures/{civ}_civil_centre")) === 0){
			//only ever build one mill/CC/market at a time
		if (gameState.getTimeElapsed() > 30 * 1000){
			for (var resource in this.dropsiteNumbers){
				if (this.checkResourceConcentrations(gameState, resource) < this.dropsiteNumbers[resource]){
					
					var spot = this.getBestResourceBuildSpot(gameState, resource);
					
					if (spot[0] === true){
						queues.economicBuilding.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_civil_centre", spot[1]));
					} else {
						queues.economicBuilding.addItem(new BuildingConstructionPlan(gameState, "structures/{civ}_mill", spot[1]));
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

	// this function also deals with a few things that are number-of-workers related
	Engine.ProfileStart("Train workers and build farms");
	this.trainMoreWorkers(gameState, queues);

	if ((gameState.ai.playedTurn+1) % 3 === 0)
		this.buildMoreFields(gameState,queues);
	
	Engine.ProfileStop();
	
	//Later in the game we want to build stuff faster.
	if (gameState.getTimeElapsed() > 15*60*1000) {
		this.targetNumBuilders = 6;
	}else{
		this.targetNumBuilders = 3;
	}
		
	Engine.ProfileStart("Update Resource Maps and Concentrations");
	this.updateResourceMaps(gameState, events);
	if (gameState.ai.playedTurn % 2 === 0) {
		var resources = ["food", "wood", "stone", "metal"];
		this.updateNearbyResources(gameState, resources[(gameState.ai.playedTurn % 8)/2]);
	} else if (gameState.ai.playedTurn % 2 === 1) {
		var resources = ["food", "wood", "stone", "metal"];
		this.updateResourceConcentrations(gameState, resources[((gameState.ai.playedTurn+1) % 8)/2]);
	}
	Engine.ProfileStop();
	
	if (gameState.ai.playedTurn % 8 === 0) {
		Engine.ProfileStart("Build new Dropsites");
		this.buildDropsites(gameState, queues);
		Engine.ProfileStop();
	}
	this.tryBartering(gameState);
	this.buildMarket(gameState, queues);
	
	// TODO: implement a timer based system for this
	this.setCount += 1;
	if (this.setCount >= 20){
		this.setWorkersIdleByPriority(gameState);
		this.setCount = 0;
	}
	
	
	Engine.ProfileStart("Assign builders");
	this.assignToFoundations(gameState);
	Engine.ProfileStop();

	Engine.ProfileStart("Reassign Idle Workers");
	this.reassignIdleWorkers(gameState);
	Engine.ProfileStop();
	
	// this is pretty slow, run it once in a while
	if (gameState.ai.playedTurn % 4 === 0) {
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
				if (a !== undefined && b !== undefined)
					if (a["food.grain"]/b["food.grain"] > a["wood.tree"]/b["wood.tree"] && gathererGroups[i]["wood"].length > 0
						&& gathererGroups[j]["food"].length > 0){
						for (var k = 0; k < Math.min(gathererGroups[i]["wood"].length, gathererGroups[j]["food"].length); k++){
							gathererGroups[i]["wood"][k].setMetadata("gather-type", "food");
							gathererGroups[j]["food"][k].setMetadata("gather-type", "wood");
						}
				}
			}
		}
		Engine.ProfileStop();
	}
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
				delete e.msg.metadata[gameState.getPlayerID()]["worker-object"];
			}
		}
	}
	Engine.ProfileStop();

	Engine.ProfileStop();
};
