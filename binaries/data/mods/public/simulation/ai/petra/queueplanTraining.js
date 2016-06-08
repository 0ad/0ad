var PETRA = function(m)
{

m.TrainingPlan = function(gameState, type, metadata, number = 1, maxMerge = 5)
{
	if (!m.QueuePlan.call(this, gameState, type, metadata))
	{
		API3.warn(" Plan training " + type + " canceled");
		return false;
	}

	this.category = "unit";
	this.cost = new API3.Resources(this.template.cost(), +this.template._template.Cost.Population);

	this.number = number;
	this.maxMerge = maxMerge;

	return true;
};

m.TrainingPlan.prototype = Object.create(m.QueuePlan.prototype);

m.TrainingPlan.prototype.canStart = function(gameState)
{
	let trainers = gameState.findTrainers(this.type);
	if (this.metadata && this.metadata.sea)
		trainers = trainers.filter(API3.Filters.byMetadata(PlayerID, "sea", this.metadata.sea));

	return trainers.length !== 0;
};

m.TrainingPlan.prototype.start = function(gameState)
{
	if (this.metadata && this.metadata.trainer)
	{
		let metadata = {};
		for (let key in this.metadata)
			if (key !== "trainer")
				metadata[key] = this.metadata[key];
		let trainer = gameState.getEntityById(this.metadata.trainer);
		if (trainer)
			trainer.train(gameState.civ(), this.type, this.number, metadata, this.promotedTypes(gameState));
		this.onStart(gameState);
		return;
	}

	let trainersColl = gameState.findTrainers(this.type);
	if (this.metadata && this.metadata.sea)
		trainersColl = trainersColl.filter(API3.Filters.byMetadata(PlayerID, "sea", this.metadata.sea));
	else if (this.metadata && this.metadata.base)
		trainersColl = trainersColl.filter(API3.Filters.byMetadata(PlayerID, "base", this.metadata.base));

	// Prefer training buildings with short queues
	// (TODO: this should also account for units added to the queue by
	// plans that have already been executed this turn)
	if (trainersColl.length)
	{
		let trainers = trainersColl.toEntityArray();
		let wantedIndex;
		if (this.metadata && this.metadata.index)
			wantedIndex = this.metadata.index;
		let workerUnit = this.metadata && this.metadata.role && this.metadata.role == "worker";
		let supportUnit = this.template.hasClass("Support");
		trainers.sort(function(a, b) {
			let aa = a.trainingQueueTime();
			let bb = b.trainingQueueTime();
			if (a.hasClass("Civic") && !supportUnit)
				aa += 10;
			if (b.hasClass("Civic") && !supportUnit)
				bb += 10;
			if (supportUnit)
			{
				if (gameState.ai.HQ.isNearInvadingArmy(a.position()))
					aa += 50;
				if (gameState.ai.HQ.isNearInvadingArmy(b.position()))
					bb += 50;
			}
			let aBase = a.getMetadata(PlayerID, "base");
			let bBase = b.getMetadata(PlayerID, "base");
			if (wantedIndex)
			{
				if (!aBase || gameState.ai.HQ.getBaseByID(aBase).accessIndex != wantedIndex)
					aa += 30;
				if (!bBase || gameState.ai.HQ.getBaseByID(bBase).accessIndex != wantedIndex)
					bb += 30;
			}
			// then, if worker, small preference for bases with less workers
			if (workerUnit && aBase && bBase && aBase != bBase)
			{
				let apop = gameState.ai.HQ.getBaseByID(aBase).workers.length;
				let bpop = gameState.ai.HQ.getBaseByID(bBase).workers.length;
				if (apop > bpop)
					aa++;
				else if (bpop > apop)
					bb++;
			}
			return aa - bb;
		});
		if (this.metadata && this.metadata.base !== undefined && this.metadata.base === 0)
			this.metadata.base = trainers[0].getMetadata(PlayerID, "base");
		trainers[0].train(gameState.civ(), this.type, this.number, this.metadata, this.promotedTypes(gameState));
	}
	else if (gameState.ai.Config.debug > 1)
		warn(" no trainers for this queue " + this.type);
	this.onStart(gameState);
};

m.TrainingPlan.prototype.addItem = function(amount = 1)
{
	this.number += amount;
};

/** Find the promoted types corresponding to this.type */
m.TrainingPlan.prototype.promotedTypes = function(gameState)
{
	let types = [];
	let promotion = this.template.promotion();
	let previous;
	let template;
	while (promotion)
	{
		types.push(promotion);
		previous = promotion;
		template = gameState.getTemplate(promotion);
		if (!template)
		{
			if (gameState.ai.Config.debug > 0)
				API3.warn(" promotion template " + promotion + " is not found");
			promotion = undefined;
			break;
		}
		promotion = template.promotion();
		if (previous === promotion)
		{
			if (gameState.ai.Config.debug > 0)
				API3.warn(" unit " + promotion + " is its own promoted unit");
			promotion = undefined;
		}
	}
	return types;
};

m.TrainingPlan.prototype.Serialize = function()
{
	let prop = {
		"category": this.category,
		"type": this.type,
		"ID": this.ID,
		"metadata": this.metadata,
		"cost": this.cost.Serialize(),
		"number": this.number,
		"maxMerge": this.maxMerge
	};

	return { "prop": prop };
};

m.TrainingPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.prop)
		this[key] = data.prop[key];

	let cost = new API3.Resources();
	cost.Deserialize(data.prop.cost);
	this.cost = cost;
};

return m;
}(PETRA);
