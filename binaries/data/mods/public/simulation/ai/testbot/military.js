/*
 * Military strategy:
 *   * Try training an attack squad of a specified size
 *   * When it's the appropriate size, send it to attack the enemy
 *   * Repeat forever
 *
 */

var MilitaryAttackManager = Class({

	_init: function()
	{
		this.targetSquadSize = 10;
		this.squadTypes = [
			"units/{civ}_infantry_spearman_b",
			"units/{civ}_infantry_javelinist_b",
//			"units/{civ}_infantry_archer_b", // TODO: should only include this if hele
		];
	},

	/**
	 * Returns the unit type we should begin training.
	 * (Currently this is whatever we have least of.)
	 */
	findBestNewUnit: function(gameState)
	{
		// Count each type
		var types = [];
		for each (var t in this.squadTypes)
			types.push([t, gameState.countEntitiesAndQueuedWithType(t)]);

		// Sort by increasing count
		types.sort(function (a, b) { return a[1] - b[1]; });

		// TODO: we shouldn't return units that we don't have any
		// buildings capable of training

		return types[0][0];
	},

	update: function(gameState, planGroups)
	{
		// Pause for a minute before starting any work, to give the economy a chance
		// to start up
		if (gameState.getTimeElapsed() < 60*1000)
			return;

		Engine.ProfileStart("military update");

		// Continually try training new units, in batches of 5
		planGroups.militaryPersonnel.addPlan(100,
			new UnitTrainingPlan(gameState,
				this.findBestNewUnit(gameState), 5, { "role": "attack-pending" })
		);

		// Find the units ready to join the attack
		var pending = gameState.getOwnEntitiesWithRole("attack-pending");

		// If we have enough units yet, start the attack
		if (pending.length >= this.targetSquadSize)
		{
			// Find the enemy CCs we could attack
			var targets = gameState.entities.filter(function(ent) {
				return (ent.isEnemy() && ent.hasClass("CivCentre"));
			});

			// If there's no CCs, attack anything else that's critical
			if (targets.length == 0)
			{
				targets = gameState.entities.filter(function(ent) {
					return (ent.isEnemy() && ent.hasClass("ConquestCritical"));
				});
			}

			// If we have a target, move to it
			if (targets.length)
			{
				// Remove the pending role
				pending.forEach(function(ent) {
					ent.setMetadata("role", "attack");
				});

				var target = targets.toEntityArray()[0];
				var targetPos = target.position();

				// TODO: this should be an attack-move command
				pending.move(targetPos[0], targetPos[1]);
			}
		}

		Engine.ProfileStop();
	},

});
