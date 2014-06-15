var PETRA = function(m)
{

m.TrainingPlan = function(gameState, type, metadata, number, maxMerge)
{
	if (!m.QueuePlan.call(this, gameState, type, metadata))
	{
		warn(" Plan training " + type + " canceled");
		return false;
	}

	this.category = "unit";
	this.cost = new API3.Resources(this.template.cost(), this.template._template.Cost.Population);
	
	this.number = number !== undefined ? number : 1;
	this.maxMerge = maxMerge !== undefined ? maxMerge : 5;

	return true;
};

m.TrainingPlan.prototype = Object.create(m.QueuePlan.prototype);

m.TrainingPlan.prototype.canStart = function(gameState)
{
	if (this.invalidTemplate)
		return false;

	// TODO: we should probably check pop caps

	if (this.metadata && this.metadata.sea)
		var trainers = gameState.findTrainers(this.type).filter(API3.Filters.byMetadata(PlayerID, "sea", this.metadata.sea));
	else
		var trainers = gameState.findTrainers(this.type);

	return (trainers.length != 0);
};

m.TrainingPlan.prototype.start = function(gameState)
{
	if (this.metadata && this.metadata.trainer)
	{
		var metadata = {};
		for (var key in this.metadata)
			if (key !== "trainer")
				metadata[key] = this.metadata[key];
		var trainer = gameState.getEntityById(this.metadata.trainer);
		if (trainer)
			trainer.train(this.type, this.number, metadata);
		this.onStart(gameState);
		return;
	}

	if (this.metadata && this.metadata.sea)
		var trainers = gameState.findTrainers(this.type).filter(API3.Filters.byMetadata(PlayerID, "sea", this.metadata.sea)).toEntityArray();
	else if (this.metadata && this.metadata.base)
		var trainers = gameState.findTrainers(this.type).filter(API3.Filters.byMetadata(PlayerID, "base", this.metadata.base)).toEntityArray();
	else
		var trainers = gameState.findTrainers(this.type).toEntityArray();

	// Prefer training buildings with short queues
	// (TODO: this should also account for units added to the queue by
	// plans that have already been executed this turn)
	if (trainers.length > 0)
	{
		var wantedIndex = undefined;
		if (this.metadata && this.metadata.index)
			wantedIndex = this.metadata.index;
		var supportUnit = this.template.hasClass("Support");
		trainers.sort(function(a, b) {
			var aa = a.trainingQueueTime();
			var bb = b.trainingQueueTime();
			if (a.hasClass("Civic") && !supportUnit)
				aa += 10;
			if (b.hasClass("Civic") && !supportUnit)
				bb += 10;
			if (wantedIndex)
			{
				var aBase = a.getMetadata(PlayerID, "base");
				if (!aBase || gameState.ai.HQ.baseManagers[aBase].accessIndex !== wantedIndex)
					aa += 30;
				var bBase = b.getMetadata(PlayerID, "base");
				if (!bBase || gameState.ai.HQ.baseManagers[bBase].accessIndex !== wantedIndex)
					bb += 30;
			}
			return (aa - bb);
		});
		if (this.metadata && this.metadata.base !== undefined && this.metadata.base === 0)
			this.metadata.base = trainers[0].getMetadata(PlayerID, "base");
		trainers[0].train(this.type, this.number, this.metadata);
	}
	else
		warn(" no trainers for this queue " + this.type);
	this.onStart(gameState);
};

m.TrainingPlan.prototype.addItem = function(amount)
{
	if (amount === undefined)
		amount = 1;
	this.number += amount;
};

return m;
}(PETRA);
