var UnitTrainingPlan = Class({

	_init: function(gameState, type, amount, metadata)
	{
		this.type = gameState.applyCiv(type);
		this.amount = amount;
		this.metadata = metadata;

		var template = gameState.getTemplate(this.type);
		if (!template)
		{
			this.invalidTemplate = true;
			return;
		}

		this.cost = new Resources(template.cost());
		this.cost.multiply(amount); // (assume no batch discount)
	},

	canExecute: function(gameState)
	{
		if (this.invalidTemplate)
			return false;

		// TODO: we should probably check pop caps

		var trainers = gameState.findTrainers(this.type);

		return (trainers.length != 0);
	},

	execute: function(gameState)
	{
//		warn("Executing UnitTrainingPlan "+uneval(this));

		var trainers = gameState.findTrainers(this.type).toEntityArray();

		// Prefer training buildings with short queues
		// (TODO: this should also account for units added to the queue by
		// plans that have already been executed this turn)
		trainers.sort(function(a, b) {
			return a.trainingQueueTime() - b.trainingQueueTime();
		});

		trainers[0].train(this.type, this.amount, this.metadata);
	},

	getCost: function()
	{
		return this.cost;
	},
});
