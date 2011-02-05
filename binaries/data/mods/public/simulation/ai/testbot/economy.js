var EconomyManager = Class({

	_init: function()
	{
		this.targetNumWorkers = 10; // minimum number of workers we want
	},

	update: function(gameState, planGroups)
	{
		// Count the workers in the world and in progress
		var numWorkers = gameState.countEntitiesAndQueuedWithRole("worker");

		// If we have too few, train another
//		print("Want "+this.targetNumWorkers+" workers; got "+numWorkers+"\n");
		if (numWorkers < this.targetNumWorkers)
		{
			planGroups.economyPersonnel.addPlan(100,
				new UnitTrainingPlan(gameState,
					"units/hele_support_female_citizen", 1, { "role": "worker" })
			);
		}

		// Search for idle workers, and tell them to gather resources
		// Maybe just pick a random nearby resource type at first;
		// later we could add some timer that redistributes workers based on
		// resource demand.

		var idleWorkers = gameState.entities.filter(function(ent) {
			return (ent.getMetadata("role") === "worker" && ent.isIdle());
		});

		if (idleWorkers.length)
		{
			var resourceSupplies = gameState.findResourceSupplies(gameState);

			idleWorkers.forEach(function(ent) {
				// Pick a resource type at random
				// TODO: should limit to what this worker can gather
				var type = Resources.prototype.types[Math.floor(Math.random()*Resources.prototype.types.length)];

				// Make sure there's actually some of that type
				// (We probably shouldn't pick impossible ones in the first place)
				if (!resourceSupplies[type])
					return;

				// Pick the closest one.
				// TODO: we should care about distance to dropsites,
				// and gather rates of workers

				var workerPosition = ent.position();
				var closestEntity = null;
				var closestDist = Infinity;
				resourceSupplies[type].forEach(function(supply) {
					var dist = VectorDistance(supply.position, workerPosition);
					if (dist < closestDist)
					{
						closestDist = dist;
						closestEntity = supply.entity;
					}
				});

				// Start gathering
				if (closestEntity)
					ent.gather(closestEntity);
			});
		}
	},

});
