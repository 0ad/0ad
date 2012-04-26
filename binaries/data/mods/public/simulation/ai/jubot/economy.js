var EconomyManager = Class({

	_init: function()
	{
		this.baseNumWorkers = 30; // minimum number of workers we want
		this.targetNumWorkers = 75; // minimum number of workers we want
		this.targetNumBuilders = 6; // number of workers we want working on construction
		this.changetimeRegBui = 180*1000;
		this.changetimeWorkers = 60*1000;
		this.changetimeBoost = 30*1000;
		this.worknumbers = 1.5;
		// (This is a stupid design where we just construct certain numbers
		// of certain buildings in sequence)
		// Greek building list
		// Relative proportions of workers to assign to each resource type
		this.gatherWeights = {
			"food": 180,
			"wood": 180,
			"stone": 45,
			"metal": 120,
		};
	},

	villageBuildingList: function (gameState)
	{
		// Hele building list
		if (gameState.displayCiv() == "hele")
		{
			this.villageBuildings = [
				{
					"template": "structures/{civ}_defense_tower",
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
					"template": "structures/{civ}_field",
					"priority": 70,
					"count": 2,
				},
			];
		}
		// Celt building list
		else if (gameState.displayCiv() == "celt")
		{
			this.villageBuildings = [
				{
					"template": "structures/{civ}_defense_tower",
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
					"template": "structures/{civ}_field",
					"priority": 70,
					"count": 2,
				},
			];
		}
		// Carthage building list
		else if (gameState.displayCiv() == "cart")
		{
			this.villageBuildings = [
				{
					"template": "structures/{civ}_defense_tower",
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
					"template": "structures/{civ}_field",
					"priority": 70,
					"count": 2,
				},
			];
		}
		// Iberian building list
		else if (gameState.displayCiv() == "iber")
		{
			this.villageBuildings = [
				{
					"template": "structures/{civ}_defense_tower",
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
					"template": "structures/{civ}_field",
					"priority": 70,
					"count": 2,
				},
			];
		}
		// Persian building list
		else if (gameState.displayCiv() == "pers")
		{
			this.villageBuildings = [
				{
					"template": "structures/{civ}_defense_tower",
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
					"template": "structures/{civ}_field",
					"priority": 70,
					"count": 2,
				},
			];
		}
		// Roman building list
		else if (gameState.displayCiv() == "rome")
		{
			this.villageBuildings = [
				{
					"template": "structures/{civ}_defense_tower",
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
					"template": "structures/{civ}_field",
					"priority": 70,
					"count": 2,
				},
			];
		}
		// Fallback option just in case
		else
		{
			this.villageBuildings = [
				{
					"template": "structures/{civ}_defense_tower",
					"priority": 105,
					"count": 1,
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
					"template": "structures/{civ}_defense_tower",
					"priority": 60,
					"count": 4,
				},
				{
					"template": "structures/{civ}_field",
					"priority": 40,
					"count": 5,
				},
			];
		}
	},
	
	checkBuildingList: function (gameState)
	{
		// Hele building list
		if (gameState.displayCiv() == "hele")
		{
			this.targetBuildings = [
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
					"template": "structures/{civ}_field",
					"priority": 40,
					"count": 3,
				},
			];
		}
		// Celt building list
		else if (gameState.displayCiv() == "celt")
		{
			this.targetBuildings = [
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
					"template": "structures/celt_kennel",
					"priority": 75,
					"count": 1,
				},
			];
		}
		// Carthage building list
		else if (gameState.displayCiv() == "cart")
		{
			this.targetBuildings = [
				{
					"template": "structures/cart_fortress",
					"priority": 80,
					"count": 1,
				},
				{
					"template": "structures/cart_temple",
					"priority": 75,
					"count": 1,
				},
				{
					"template": "structures/cart_embassy_celtic",
					"priority": 50,
					"count": 1,
				},
				{
					"template": "structures/cart_embassy_iberian",
					"priority": 50,
					"count": 1,
				},
				{
					"template": "structures/cart_embassy_italiote",
					"priority": 50,
					"count": 1,
				},
			];
		}
		// Iberian building list
		else if (gameState.displayCiv() == "iber")
		{
			this.targetBuildings = [
				{
					"template": "structures/iber_fortress",
					"priority": 80,
					"count": 1,
				},
			];
		}
		// Perian building list
		else if (gameState.displayCiv() == "pers")
		{
			this.targetBuildings = [
				{
					"template": "structures/pers_fortress",
					"priority": 80,
					"count": 1,
				},
				{
					"template": "structures/pers_stables",
					"priority": 75,
					"count": 1,
				},
				{
					"template": "structures/pers_apadana",
					"priority": 50,
					"count": 1,
				},
			];
		}
		// Roman building list
		else if (gameState.displayCiv() == "rome")
		{
			this.targetBuildings = [
				{
					"template": "structures/rome_fortress",
					"priority": 80,
					"count": 1,
				},
				{
					"template": "structures/rome_army_camp",
					"priority": 75,
					"count": 1,
				},
			];
		}
		// Fallback option just in case
		else
		{
			this.targetBuildings = [
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
					"template": "structures/{civ}_defense_tower",
					"priority": 60,
					"count": 4,
				},
				{
					"template": "structures/{civ}_field",
					"priority": 40,
					"count": 5,
				},
			];
		}
	},
	
	
	buildMoreBuildings: function(gameState, planGroups)
	{
		var numCCs = gameState.countEntitiesAndQueuedWithType(gameState.applyCiv("structures/{civ}_civil_centre"));
		var numMills = gameState.countEntitiesAndQueuedWithType(gameState.applyCiv("structures/{civ}_mill"));
		var numFarmsteads = gameState.countEntitiesAndQueuedWithType(gameState.applyCiv("structures/{civ}_farmstead"));
		var defensequot = numMills + numFarmsteads
		if (numCCs < 1)
		{
			planGroups.economyConstruction.addPlan(1000,
				new BuildingConstructionPlan(gameState, "structures/{civ}_civil_centre", 1)
			);
			return;
		}
		// Limit ourselves to constructing two buildings at a time
		if (gameState.findFoundations().length > 0)
			return;
			
		var pop = gameState.getPopulation();
		var poplim = gameState.getPopulationLimit();
		var space = poplim - pop;
		
		if (space < 9)
		{
			planGroups.economyConstruction.addPlan(160,
				new BuildingConstructionPlan(gameState, "structures/{civ}_house", 1)
			);
			return;
		}
		
		if (gameState.findFoundations().length > 0)
			return;	
			
		var numTowers = gameState.countEntitiesAndQueuedWithType(gameState.applyCiv("structures/{civ}_defense_tower"));
		if (numTowers < defensequot)
		{
			planGroups.economyConstruction.addPlan(150,
				new BuildingConstructionPlanDefensePoints(gameState, "structures/{civ}_defense_tower", 1)
			);
			return;
		}
			
		// START BY GETTING ALL CCs UP TO SMALL VILLAGE LEVEL
		for each (var building in this.villageBuildings)
		{
			var numBuildings = gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(building.template));
			
			var wantedtotal = building.count * numCCs;
			// If we have too few, build another
			if (numBuildings < wantedtotal && building.template != gameState.applyCiv("structures/{civ}_field"))
			{
				planGroups.economyConstruction.addPlan(building.priority,
					new BuildingConstructionPlan(gameState, building.template, 1)
				);
				return;
			}
			else if (numBuildings < wantedtotal && building.template == gameState.applyCiv("structures/{civ}_field"))
			{
				planGroups.economyConstruction.addPlan(building.priority,
					new BuildingConstructionPlanResources(gameState, building.template, 1)
				);
				return;
			}
		}
		// THEN BUILD THE MAIN BASE INTO A TOWN
		for each (var building in this.targetBuildings)
		{
			var numBuildings = gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(building.template));
			// If we have too few, build another
			if (numBuildings < building.count && building.template != gameState.applyCiv("structures/{civ}_field"))
			{
				planGroups.economyConstruction.addPlan(building.priority,
					new BuildingConstructionPlan(gameState, building.template, 1)
				);
				return;
			}
			else if (numBuildings < building.count && building.template == gameState.applyCiv("structures/{civ}_field"))
			{
				planGroups.economyConstruction.addPlan(building.priority,
					new BuildingConstructionPlanResources(gameState, building.template, 1)
				);
				return;
			}
		}
	},

	trainMoreWorkers: function(gameState, planGroups)
	{
		if (gameState.getTimeElapsed() > this.changetime){
		this.worknumbers = Math.random()*2;
		this.changetimeWorkers = this.changetime + (30*1000);
		}
		// Count the workers in the world and in progress
		var numWorkers = gameState.countEntitiesAndQueuedWithRoles("worker", "militia");
		var workNumMod = this.worknumbers;
		var miliNo = gameState.countEntitiesAndQueuedWithRole("militia");
		var workNo = gameState.countEntitiesAndQueuedWithRole("worker");
		
		if (miliNo > workNo)
		{
			workNumMod = workNumMod - 0.6;
		}
		if (gameState.getTimeElapsed() < 150 * 100)
		{
			workNumMod = workNumMod - 0.6;
		}
		// If we have too few, train another
//		print("Want "+this.targetNumWorkers+" workers; got "+numWorkers+"\n");
		if (numWorkers < this.baseNumWorkers)
		{
			var priority = 180;
		}
		else if (numWorkers < this.targetNumWorkers)
		{
			var priority = 140;
		}

		// Hele and Celt training list
		if (gameState.displayCiv() == "hele" || gameState.displayCiv() == "celt")
		{
			if (workNumMod < 0.95)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_support_female_citizen", 2, { "role": "worker" })
				);
			}
			else if (workNumMod > 1.6)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_spearman_b", 2, { "role": "militia" })
				);
			}
			else
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_javelinist_b", 2, { "role": "militia" })
				);
			}
		}
		// Carthage training list
		else if (gameState.displayCiv() == "cart")
		{
			if (workNumMod < 0.95)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_support_female_citizen", 2, { "role": "worker" })
				);
			}
			else if (workNumMod > 1.6)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_spearman_b", 2, { "role": "militia" })
				);
			}
			else
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_archer_b", 2, { "role": "militia" })
				);
			}
		}
		// Iberian training list
		else if (gameState.displayCiv() == "iber")
		{
			if (workNumMod < 0.95)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_support_female_citizen", 2, { "role": "worker" })
				);
			}
			else if (workNumMod > 1.6)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_swordsman_b", 2, { "role": "militia" })
				);
			}
			else
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_javelinist_b", 2, { "role": "militia" })
				);
			}
		}
		// Persian training list
		else if (gameState.displayCiv() == "pers")
		{
			if (workNumMod < 0.95)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_support_female_citizen", 2, { "role": "worker" })
				);
			}
			else if (workNumMod > 1.6)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_spearman_b", 2, { "role": "militia" })
				);
			}
			else
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_archer_b", 2, { "role": "militia" })
				);
			}
		}
		// Roman training list
		else if (gameState.displayCiv() == "rome")
		{
			if (workNumMod < 0.99)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_support_female_citizen", 2, { "role": "worker" })
				);
			}
			else if (workNumMod > 1.3)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_swordsman_b", 2, { "role": "militia" })
				);
			}
			else if (workNumMod > 1.6)
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_spearman_b", 2, { "role": "militia" })
				);
			}
			else
			{
				planGroups.economyPersonnel.addPlan(priority,
					new UnitTrainingPlan(gameState,
						"units/{civ}_infantry_javelinist_b", 2, { "role": "militia" })
				);
			}
		}
		else
		{
			planGroups.economyPersonnel.addPlan(priority,
				new UnitTrainingPlan(gameState,
					"units/{civ}_support_female_citizen", 2, { "role": "worker" })
			);
		}
		
		
	},

	pickMostNeededResources: function(gameState)
	{
		var self = this;

		// Find what resource type we're most in need of
		var numGatherers = {};
		for (var type in this.gatherWeights)
			numGatherers[type] = 0;

		gameState.getOwnRoleGroup("worker").forEach(function(ent) {
			if (ent.getMetadata("subrole") === "gatherer")
				numGatherers[ent.getMetadata("gather-type")] += 1;
		});
		gameState.getOwnRoleGroup("militia").forEach(function(ent) {
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
		var roleless = gameState.getOwnRoleGroup(undefined);


		roleless.forEach(function(ent)
		{
			if (ent.hasClass("Worker"))
			{
				ent.setMetadata("role", "worker");
			}
			else if (ent.hasClass("CitizenSoldier") && ent.hasClass("Infantry"))
			{
				var currentPosition = ent.position();
				var targets = gameState.getJustEnemies().filter(function(enten) {
					var foeposition = enten.position();
					if (foeposition)
					{
						var dist = SquareVectorDistance(foeposition, currentPosition);
						return (dist < 2500);
					}
					else
					{
						return false;
					}
				});
				if (targets.length == 0)
				{
					// If we're clear go back to work
					ent.setMetadata("role", "militia");
				}
				else
				{
					// If not, go home!
					var targets = gameState.getOwnEntities().filter(function(squeak) {
						return (!squeak.isEnemy() && squeak.hasClass("Village"));
					});
					// If we have a target, move to it
					if (targets.length)
					{
						var targetrandomiser = Math.floor(Math.random()*targets.length);
						var target = targets.toEntityArray()[targetrandomiser];
						var targetPos = target.position();
						// TODO: this should be an attack-move command
						ent.move(targetPos[0], targetPos[1]);
					}
				}
			}
			else
			{
				ent.setMetadata("role", "randomcannonfodder");
			}
		});
	},

	reassignIdleWorkers: function(gameState, planGroups)
	{
		var self = this;

		var allWorkers = gameState.getOwnEntitiesWithTwoRoles("worker", "militia")
		allWorkers.forEach(function(worker){
			var shallIstop = Math.random();
			if (shallIstop > 0.9975)
			{
				var targetPos = worker.position();	
				worker.move(targetPos[0], targetPos[1]);
			}
		});
		// Search for idle workers, and tell them to gather resources
		// Maybe just pick a random nearby resource type at first;
		// later we could add some timer that redistributes workers based on
		// resource demand.

		var idleWorkers = gameState.getOwnEntitiesWithTwoRoles("worker", "militia").filter(function(ent) {
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
					// The types are food wood stone metal
					// Pick the closest one.
					// TODO: we should care about distance to dropsites, not (just) to the worker,
					// and gather rates of workers

					var workerPosition = ent.position();
					var supplies = [];
					resourceSupplies[type].forEach(function(supply) {
						// Skip targets that are too hard to hunt
						if (supply.entity.isUnhuntable())
							return;
						// And don't go for the bloody fish!
						if (supply.entity.hasClass("SeaCreature"))
							return;
		
						var distcheck = 10000000000;
						var supplydistcheck = 100000000000;
						gameState.getOwnEntities().forEach(function(centre) {
							if (centre.hasClass("CivCentre"))
							{
								var centrePosition = centre.position();
								var currentsupplydistcheck = SquareVectorDistance(supply.position, centrePosition);
								if (currentsupplydistcheck < currentsupplydistcheck)
								{
									supplydistcheck = currentsupplydistcheck;
								}
								var currentdistcheck = SquareVectorDistance(supply.position, centrePosition);
								if (currentdistcheck < distcheck)
								{
									distcheck = currentdistcheck;
								}
								// Skip targets that are far too far away (e.g. in the enemy base)
							}
							else if (centre.hasClass("DropsiteFood") && type == "food")
							{
								var centrePosition = centre.position();
								var currentsupplydistcheck = SquareVectorDistance(supply.position, centrePosition);
								if (currentsupplydistcheck < currentsupplydistcheck)
								{
									supplydistcheck = currentsupplydistcheck;
								}
								// Skip targets that are far too far away (e.g. in the enemy base)
							}
							else if (centre.hasClass("DropsiteWood") && type == "wood")
							{
								var centrePosition = centre.position();
								var currentsupplydistcheck = SquareVectorDistance(supply.position, centrePosition);
								if (currentsupplydistcheck < currentsupplydistcheck)
								{
									supplydistcheck = currentsupplydistcheck;
								}
								// Skip targets that are far too far away (e.g. in the enemy base)
							}
							else if (centre.hasClass("DropsiteStone") && type == "stone")
							{
								var centrePosition = centre.position();
								var currentsupplydistcheck = SquareVectorDistance(supply.position, centrePosition);
								if (currentsupplydistcheck < currentsupplydistcheck)
								{
									supplydistcheck = currentsupplydistcheck;
								}
								// Skip targets that are far too far away (e.g. in the enemy base)
							}
							else if (centre.hasClass("DropsiteMetal") && type == "metal")
							{
								var centrePosition = centre.position();
								var currentsupplydistcheck = SquareVectorDistance(supply.position, centrePosition);
								if (currentsupplydistcheck < currentsupplydistcheck)
								{
									supplydistcheck = currentsupplydistcheck;
								}
								// Skip targets that are far too far away (e.g. in the enemy base)
							}
						});
						if (distcheck > 500000)
							return;
							
						var dist = SquareVectorDistance(supply.position, workerPosition);

						// Skip targets that are far too far away (e.g. in the enemy base)
						if (dist > 500000)
							return;							
							
						supplies.push({ dist: dist, entity: supply.entity });
					});

					supplies.sort(function (a, b) {
						// Prefer smaller distances
						if (a.dist != b.dist)
							return a.dist - b.dist;

						return false;
					});
					
					if (type == "food")
					{
						var whatshallwebuild = "structures/{civ}_farmstead"
					}
					else
					{
						var whatshallwebuild = "structures/{civ}_mill"
					}
					
					// Start gathering
					if (supplies.length)
					{
						// THIS SHOULD BE A GLOBAL VARIABLE
						var currentposformill = supplies[0].entity.position();
						var distcheckoldII = 1000000000;
						// CHECK DISTANCE
						gameState.getOwnEntities().forEach(function(centre) {
							if (centre.hasClass("CivCentre"))
							{
								var centrePosition = centre.position();
								var distcheckII = SquareVectorDistance(currentposformill, centrePosition);
								if (distcheckII < distcheckoldII)
								{
									distcheckoldII = distcheckII;
								}
							}
							else if (centre.hasClass("DropsiteFood") && type == "food")
							{
								var centrePosition = centre.position();
								var distcheckII = SquareVectorDistance(currentposformill, centrePosition);
								if (distcheckII < distcheckoldII)
								{
									distcheckoldII = distcheckII;
								}
							}
							else if (centre.hasClass("DropsiteWood") && type == "wood")
							{
								var centrePosition = centre.position();
								var distcheckII = SquareVectorDistance(currentposformill, centrePosition);
								if (distcheckII < distcheckoldII)
								{
									distcheckoldII = distcheckII;
								}
							}
							else if (centre.hasClass("DropsiteMetal") && type == "metal")
							{
								var centrePosition = centre.position();
								var distcheckII = SquareVectorDistance(currentposformill, centrePosition);
								if (distcheckII < distcheckoldII)
								{
									distcheckoldII = distcheckII;
								}
							}
							else if (centre.hasClass("DropsiteStone") && type == "stone")
							{
								var centrePosition = centre.position();
								var distcheckII = SquareVectorDistance(currentposformill, centrePosition);
								if (distcheckII < distcheckoldII)
								{
									distcheckoldII = distcheckII;
								}
							}
						});
						var foundationsyes = false;
						if (gameState.findFoundations().length > 1)
						{
							foundationsyes = false;
						}
						else
						{
							foundationsyes = true;
						}
						//warn(type + " is the resource and " + distcheckoldII + " is the distance.");
						
						if (distcheckoldII > 5000 && foundationsyes == true)
						{
							//JuBotAI.prototype.chat("Building Mill");
							planGroups.economyConstruction.addPlan(100,
								new BuildingConstructionPlanEcon(gameState, whatshallwebuild, 1, currentposformill)
							);
							//JuBotAI.prototype.chat("Gathering");
							ent.gather(supplies[0].entity);
							ent.setMetadata("subrole", "gatherer");
							ent.setMetadata("gather-type", type);
							return;
						}
						else
						{
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

		var workers = gameState.getOwnEntitiesWithTwoRoles("worker", "militia");

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
		if (gameState.getTimeElapsed() > this.changetimeRegBui)
		{
			var buildregroupers = gameState.getOwnEntitiesWithTwoRoles("worker", "militia");
			buildregroupers.forEach(function(shirk) {
				if (shirk.getMetadata("subrole") == "builder")
				{
					var targets = gameState.getOwnEntities().filter(function(ent) {
						return (!ent.isEnemy() && ent.hasClass("CivCentre"));
					});
					if (targets.length)
					{
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

		//this.buildRegroup(gameState, planGroups)
		
		this.checkBuildingList(gameState);
		this.villageBuildingList(gameState);
		
		this.reassignRolelessUnits(gameState);

		this.buildMoreBuildings(gameState, planGroups);

		this.trainMoreWorkers(gameState, planGroups);

		this.reassignIdleWorkers(gameState, planGroups);

		this.assignToFoundations(gameState, planGroups);

		Engine.ProfileStop();
	},

});
