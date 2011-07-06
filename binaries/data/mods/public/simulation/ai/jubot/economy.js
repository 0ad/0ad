var EconomyManager = Class({

	_init: function()
	{
		this.baseNumWorkers = 30; // minimum number of workers we want
		this.targetNumWorkers = 55; // minimum number of workers we want
		this.targetNumBuilders = 6; // number of workers we want working on construction
		this.changetimeRegBui = 180*1000;
		// (This is a stupid design where we just construct certain numbers
		// of certain buildings in sequence)
		// Greek building list
		// Relative proportions of workers to assign to each resource type
		this.gatherWeights = {
			"food": 140,
			"wood": 140,
			"stone": 50,
			"metal": 120,
		};
	},

	
	checkBuildingList: function (gameState) {
			if (gameState.displayCiv() == "hele"){
		this.targetBuildings = [
			{
				"template": "structures/{civ}_civil_centre",
				"priority": 500,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 110,
				"count": 2,
			},
			{
				"template": "structures/{civ}_scout_tower",
				"priority": 105,
				"count": 1,
			},
				{
				"template": "structures/{civ}_field",
				"priority": 103,
				"count": 1,
			},
			{
				"template": "structures/{civ}_barracks",
				"priority": 101,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 100,
				"count": 5,
			},
			{
				"template": "structures/{civ}_scout_tower",
				"priority": 90,
				"count": 3,
			},
			{
				"template": "structures/hele_gymnasion",
				"priority": 80,
				"count": 1,
			},
				{
				"template": "structures/{civ}_field",
				"priority": 70,
				"count": 2,
			},
			{
				"template": "structures/hele_fortress",
				"priority": 60,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 55,
				"count": 10,
			},
			{
				"template": "structures/{civ}_scout_tower",
				"priority": 50,
				"count": 5,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 45,
				"count": 15,
			},
			{
				"template": "structures/{civ}_field",
				"priority": 40,
				"count": 3,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 30,
				"count": 20,
			},
		];
			}
			// Celt building list
			else if (gameState.displayCiv() == "celt"){
		this.targetBuildings = [
			{
				"template": "structures/{civ}_civil_centre",
				"priority": 500,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 110,
				"count": 6,
			},
				{
				"template": "structures/{civ}_field",
				"priority": 100,
				"count": 2,
			},
			{
				"template": "structures/{civ}_barracks",
				"priority": 90,
				"count": 1,
			},
			{
				"template": "structures/celt_fortress_b",
				"priority": 80,
				"count": 1,
			},
			{
				"template": "structures/{civ}_scout_tower",
				"priority": 60,
				"count": 3,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 55,
				"count": 15,
			},
			{
				"template": "structures/{civ}_field",
				"priority": 40,
				"count": 3,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 30,
				"count": 25,
			},
		];
			}
			// Celt building list
			else if (gameState.displayCiv() == "iber"){
		this.targetBuildings = [
			{
				"template": "structures/{civ}_civil_centre",
				"priority": 500,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 110,
				"count": 6,
			},
				{
				"template": "structures/{civ}_field",
				"priority": 100,
				"count": 2,
			},
			{
				"template": "structures/{civ}_barracks",
				"priority": 100,
				"count": 1,
			},
			{
				"template": "structures/iber_fortress",
				"priority": 80,
				"count": 1,
			},
			{
				"template": "structures/{civ}_scout_tower",
				"priority": 70,
				"count": 3,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 55,
				"count": 15,
			},
			{
				"template": "structures/{civ}_field",
				"priority": 40,
				"count": 3,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 30,
				"count": 25,
			},
		];
			}

			// Fallback option just in case
		else {
		this.targetBuildings = [
			{
				"template": "structures/{civ}_civil_centre",
				"priority": 500,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 110,
				"count": 2,
			},
			{
				"template": "structures/{civ}_scout_tower",
				"priority": 105,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 100,
				"count": 5,
			},
				{
				"template": "structures/{civ}_field",
				"priority": 100,
				"count": 2,
			},
			{
				"template": "structures/{civ}_barracks",
				"priority": 99,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 98,
				"count": 7,
			},
			{
				"template": "structures/{civ}_scout_tower",
				"priority": 60,
				"count": 4,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 55,
				"count": 15,
			},
			{
				"template": "structures/{civ}_field",
				"priority": 40,
				"count": 5,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 30,
				"count": 20,
			},
		];
			}
	},
	
	
	buildMoreBuildings: function(gameState, planGroups)
	{
		// Limit ourselves to constructing two buildings at a time
		if (gameState.findFoundations().length > 1)
			return;

		for each (var building in this.targetBuildings)
		{
			var numBuildings = gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(building.template));

			// If we have too few, build another
			if (numBuildings < building.count)
			{
				planGroups.economyConstruction.addPlan(building.priority,
					new BuildingConstructionPlan(gameState, building.template, 1)
				);
			}
		}
	},

	trainMoreWorkers: function(gameState, planGroups)
	{
		// Count the workers in the world and in progress
		var numWorkers = gameState.countEntitiesAndQueuedWithRole("worker");

		// If we have too few, train another
//		print("Want "+this.targetNumWorkers+" workers; got "+numWorkers+"\n");
		if (numWorkers < this.baseNumWorkers)
		{
			var menorwomen = Math.random()*2;
			if (gameState.displayCiv() == "hele" || gameState.displayCiv() == "celt"){
			if (menorwomen < 1.5){
			planGroups.economyPersonnel.addPlan(120,
				new UnitTrainingPlan(gameState,
					"units/{civ}_support_female_citizen", 2, { "role": "worker" })
			);
			}
			else if (menorwomen > 1.82) {
			planGroups.economyPersonnel.addPlan(120,
				new UnitTrainingPlan(gameState,
					"units/{civ}_infantry_spearman_b", 2, { "role": "worker" })
			);
			}
			else {
			planGroups.economyPersonnel.addPlan(120,
				new UnitTrainingPlan(gameState,
					"units/{civ}_infantry_javelinist_b", 2, { "role": "worker" })
			);
			}
			}
			else {
			if (menorwomen < 1.5){
			planGroups.economyPersonnel.addPlan(120,
				new UnitTrainingPlan(gameState,
					"units/{civ}_support_female_citizen", 2, { "role": "worker" })
			);
			}
			else {
			planGroups.economyPersonnel.addPlan(120,
				new UnitTrainingPlan(gameState,
					"units/{civ}_infantry_swordsman_b", 2, { "role": "worker" })
			);
			}
			}
		}
		else if (numWorkers < this.targetNumWorkers)
		{
			var menorwomen = Math.random()*2;
			if (gameState.displayCiv() == "hele" || gameState.displayCiv() == "celt"){
			if (menorwomen < 1.5){
			planGroups.economyPersonnel.addPlan(90,
				new UnitTrainingPlan(gameState,
					"units/{civ}_support_female_citizen", 2, { "role": "worker" })
			);
			}
			else if (menorwomen > 1.82) {
			planGroups.economyPersonnel.addPlan(90,
				new UnitTrainingPlan(gameState,
					"units/{civ}_infantry_spearman_b", 2, { "role": "worker" })
			);
			}
			else {
			planGroups.economyPersonnel.addPlan(90,
				new UnitTrainingPlan(gameState,
					"units/{civ}_infantry_javelinist_b", 2, { "role": "worker" })
			);
			}
			}
			else {
			if (menorwomen < 1.5){
			planGroups.economyPersonnel.addPlan(90,
				new UnitTrainingPlan(gameState,
					"units/{civ}_support_female_citizen", 2, { "role": "worker" })
			);
			}
			else {
			planGroups.economyPersonnel.addPlan(90,
				new UnitTrainingPlan(gameState,
					"units/{civ}_infantry_swordsman_b", 2, { "role": "worker" })
			);
			}
			}
		}
	},

	pickMostNeededResources: function(gameState)
	{
		var self = this;

		// Find what resource type we're most in need of
		var numGatherers = {};
		for (var type in this.gatherWeights)
			numGatherers[type] = 0;

		gameState.getOwnEntitiesWithRole("worker").forEach(function(ent) {
			if (ent.getMetadata("subrole") === "gatherer")
				numGatherers[ent.getMetadata("gather-type")] += 1;
		});

		var types = Object.keys(this.gatherWeights);
		types.sort(function(a, b) {
			// Prefer fewer gatherers (divided by weight)
			var va = numGatherers[a] / self.gatherWeights[a];
			var vb = numGatherers[b] / self.gatherWeights[b];
			return va - vb;
		});

		return types;
	},

	reassignRolelessUnits: function(gameState)
	{
		var roleless = gameState.getOwnEntitiesWithRole(undefined);

		roleless.forEach(function(ent) {
			if (ent.hasClass("Worker"))
				ent.setMetadata("role", "worker");
			else
				ent.setMetadata("role", "randomcannonfodder");
		});
	},

	reassignIdleWorkers: function(gameState, planGroups)
	{
		var self = this;

		// Search for idle workers, and tell them to gather resources
		// Maybe just pick a random nearby resource type at first;
		// later we could add some timer that redistributes workers based on
		// resource demand.

		var idleWorkers = gameState.getOwnEntitiesWithRole("worker").filter(function(ent) {
			return ent.isIdle();
		});

		if (idleWorkers.length)
		{
			var resourceSupplies = gameState.findResourceSupplies();

			idleWorkers.forEach(function(ent) {

				var types = self.pickMostNeededResources(gameState);
				for each (var type in types)
				{
					// Make sure there's actually some of that type
					if (!resourceSupplies[type])
						continue;

					// Pick the closest one.
					// TODO: we should care about distance to dropsites, not (just) to the worker,
					// and gather rates of workers

					var workerPosition = ent.position();
					var supplies = [];
					resourceSupplies[type].forEach(function(supply) {
						// Skip targets that are too hard to hunt
						if (supply.entity.isUnhuntable())
							return;
		
		var distcheck = 1000000;
		gameState.getOwnEntities().forEach(function(centre) {
			if (centre.hasClass("CivCentre"))
			{
					var centrePosition = centre.position();
							var currentdistcheck = VectorDistance(supply.position, centrePosition);
							if (currentdistcheck < distcheck){
							distcheck = currentdistcheck;
							}
						// Skip targets that are far too far away (e.g. in the enemy base)
			}
		});
						if (distcheck > 500)
						return;
							
						var dist = VectorDistance(supply.position, workerPosition);

						// Skip targets that are far too far away (e.g. in the enemy base)
						if (dist > 500)
							return;							
							
						supplies.push({ dist: dist, entity: supply.entity });
					});

					supplies.sort(function (a, b) {
						// Prefer smaller distances
						if (a.dist != b.dist)
							return a.dist - b.dist;

						return false;
					});
									
					// Start gathering
					if (supplies.length)
					{
					// THIS SHOULD BE A GLOBAL VARIABLE
					var currentposformill = supplies[0].entity.position();
					var distcheckold = 10000;
					// CHECK DISTANCE
					gameState.getOwnEntities().forEach(function(centre) {
						if (centre.hasClass("CivCentre") || centre.hasClass("Economic"))
						{
								var centrePosition = centre.position();
								var distcheck = VectorDistance(currentposformill, centrePosition);
									if (distcheck < distcheckold){
									distcheckold = distcheck;
									}
						}
					});
					var foundationsyes = false;
					if (gameState.findFoundations().length > 2){
					foundationsyes = false;
					}
					else{
					foundationsyes = true;
					}
					
					if (distcheckold > 60 && foundationsyes == true){
					//JuBotAI.prototype.chat("Building Mill");
						planGroups.economyConstruction.addPlan(80,
						new BuildingConstructionPlanEcon(gameState, "structures/{civ}_mill", 1, currentposformill)
						);
						//JuBotAI.prototype.chat("Gathering");
						ent.gather(supplies[0].entity);
						ent.setMetadata("subrole", "gatherer");
						ent.setMetadata("gather-type", type);
						return;
					}
					else {
					//JuBotAI.prototype.chat("Gathering");
					ent.gather(supplies[0].entity);
					ent.setMetadata("subrole", "gatherer");
					ent.setMetadata("gather-type", type);
					return;
					}
					}
				}

				// Couldn't find any types to gather
				ent.setMetadata("subrole", "idle");
			});
		}
	},

	assignToFoundations: function(gameState, planGroups)
	{
		// If we have some foundations, and we don't have enough builder-workers,
		// try reassigning some other workers who are nearby

		var foundations = gameState.findFoundations();

		// Check if nothing to build
		if (!foundations.length)
			return;

		var workers = gameState.getOwnEntitiesWithRole("worker");

		var builderWorkers = workers.filter(function(ent) {
			return (ent.getMetadata("subrole") === "builder");
		});

		// Check if enough builders
		var extraNeeded = this.targetNumBuilders - builderWorkers.length;
		if (extraNeeded <= 0)
			return;

		// Pick non-builders who are closest to the first foundation,
		// and tell them to start building it

		var target = foundations.toEntityArray()[0];

		var nonBuilderWorkers = workers.filter(function(ent) {
			return (ent.getMetadata("subrole") !== "builder");
		});

		var nearestNonBuilders = nonBuilderWorkers.filterNearest(target.position(), extraNeeded);

		// Order each builder individually, not as a formation
		nearestNonBuilders.forEach(function(ent) {
			ent.repair(target);
			ent.setMetadata("subrole", "builder");
		});
	},
// This function recalls builders to the CC every 2 minutes; theoretically, this prevents the issue where they build a farm and people try to cross it and there's a traffic jam which wrecks the AI economy.
	buildRegroup: function(gameState, planGroups)
	{
			if (gameState.getTimeElapsed() > this.changetimeRegBui){
			var buildregroupers = gameState.getOwnEntitiesWithRole("worker");
			buildregroupers.forEach(function(shirk) {
			if (shirk.getMetadata("subrole") == "builder"){
			var targets = gameState.entities.filter(function(ent) {
				return (!ent.isEnemy() && ent.hasClass("CivCentre"));
			});
			if (targets.length){
				var target = targets.toEntityArray()[0];
				var targetPos = target.position();
				shirk.move(targetPos[0], targetPos[1]);
			}
			}
			});
			// Wait 4 mins to do this again.
			this.changetimeRegBui = this.changetimeRegBui + (120*1000);
			}
	},

	update: function(gameState, planGroups)
	{
		Engine.ProfileStart("economy update");

		this.buildRegroup(gameState, planGroups)
		
		this.checkBuildingList(gameState);
		
		this.reassignRolelessUnits(gameState);

		this.buildMoreBuildings(gameState, planGroups);

		this.trainMoreWorkers(gameState, planGroups);

		this.reassignIdleWorkers(gameState, planGroups);

		this.assignToFoundations(gameState, planGroups);

		Engine.ProfileStop();
	},

});
