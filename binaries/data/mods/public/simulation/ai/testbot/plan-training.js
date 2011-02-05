var UnitTrainingPlan = Class({

	_init: function(gameState, type, amount, metadata)
	{
		this.type = type;
		this.amount = amount;
		this.metadata = metadata;

		this.cost = new Resources(gameState.getTemplate(type).cost());
		this.cost.multiply(amount); // (assume no batch discount)
	},

	canExecute: function(gameState)
	{
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
