var EconomyManager = Class({

	_init: function()
	{
		this.targetNumWorkers = 30; // minimum number of workers we want
		this.targetNumBuilders = 5; // number of workers we want working on construction

		// (This is a stupid design where we just construct certain numbers
		// of certain buildings in sequence)
		this.targetBuildings = [
			{
				"template": "structures/{civ}_civil_centre",
				"priority": 500,
				"count": 1,
			},
			{
				"template": "structures/{civ}_house",
				"priority": 100,
				"count": 5,
			},
			{
				"template": "structures/{civ}_barracks",
				"priority": 75,
				"count": 1,
			},
			{
				"template": "structures/{civ}_field",
				"priority": 50,
				"count": 5,
			},
		];

		// Relative proportions of workers to assign to each resource type
		this.gatherWeights = {
			"food": 150,
			"wood": 100,
			"stone": 50,
			"metal": 100,
		};
	},

	buildMoreBuildings: function(gameState, planGroups)
	{
		// Limit ourselves to constructing one building at a time
		if (gameState.findFoundations().length)
			return;

		for each (var building in this.targetBuildings)
		{
			var numBuildings = gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(building.template));

			// If we have too few, build another
			if (numBuildings < building.count)
			{
				planGroups.economyConstruction.addPlan(building.priority,
					new BuildingConstructionPlan(gameState, building.template)
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
		if (numWorkers < this.targetNumWorkers)
		{
			planGroups.economyPersonnel.addPlan(100,
				new UnitTrainingPlan(gameState,
					"units/{civ}_support_female_citizen", 1, { "role": "worker" })
			);
		}
	},

	pickMostNeededResource: function(gameState)
	{
		// Find what resource type we're most in need of
		var numGatherers = {};
		for (var type in this.gatherWeights)
			numGatherers[type] = 0;

		gameState.getOwnEntitiesWithRole("worker").forEach(function(ent) {
			if (ent.getMetadata("subrole") === "gatherer")
				numGatherers[ent.getMetadata("gather-type")] += 1;
		});

		var bestType = "food";
		var bestTypeVal = Infinity; // num gatherers divided by weight
		for (var type in this.gatherWeights)
		{
			var v = numGatherers[type] / this.gatherWeights[type];
			if (v < bestTypeVal)
			{
				bestTypeVal = v;
				bestType = type;
			}
		}

		return bestType;
	},

	reassignRolelessUnits: function(gameState)
	{
		var roleless = gameState.getOwnEntitiesWithRole(undefined);

		roleless.forEach(function(ent) {
			if (ent.hasClass("Worker"))
				ent.setMetadata("role", "worker");
			else
				ent.setMetadata("role", "unknown");
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

				var type = self.pickMostNeededResource(gameState);

				// Make sure there's actually some of that type
				// (We probably shouldn't pick impossible ones in the first place)
				if (!resourceSupplies[type])
					return;

				// Pick the closest one.
				// TODO: we should care about distance to dropsites, not (just) to the worker,
				// and gather rates of workers

				var workerPosition = ent.position();
				var supplies = [];
				resourceSupplies[type].forEach(function(supply) {
					// Skip targets that are too hard to hunt
					if (supply.entity.isUnhuntable())
						return;

					var dist = VectorDistance(supply.position, workerPosition);
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
					ent.gather(supplies[0].entity);
					ent.setMetadata("subrole", "gatherer");
					ent.setMetadata("gather-type", type);
				}
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

	update: function(gameState, planGroups)
	{
		Engine.ProfileStart("economy update");

		this.reassignRolelessUnits(gameState);

		this.buildMoreBuildings(gameState, planGroups);

		this.trainMoreWorkers(gameState, planGroups);

		this.reassignIdleWorkers(gameState, planGroups);

		this.assignToFoundations(gameState, planGroups);

		Engine.ProfileStop();
	},

});
