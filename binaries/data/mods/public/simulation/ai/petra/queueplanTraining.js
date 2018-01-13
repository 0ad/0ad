var PETRA = function(m)
{

m.TrainingPlan = function(gameState, type, metadata, number = 1, maxMerge = 5)
{
	if (!m.QueuePlan.call(this, gameState, type, metadata))
	{
		API3.warn(" Plan training " + type + " canceled");
		return false;
	}

	// Refine the estimated cost and add pop cost
	let trainers = this.getBestTrainers(gameState);
	let trainer = trainers ? trainers[0] : undefined;
	this.cost = new API3.Resources(this.template.cost(trainer), +this.template._template.Cost.Population);

	this.category = "unit";
	this.number = number;
	this.maxMerge = maxMerge;

	return true;
};

m.TrainingPlan.prototype = Object.create(m.QueuePlan.prototype);

m.TrainingPlan.prototype.canStart = function(gameState)
{
	this.trainers = this.getBestTrainers(gameState);
	if (!this.trainers)
		return false;
	this.cost = new API3.Resources(this.template.cost(this.trainers[0]), +this.template._template.Cost.Population);
	return true;
};

m.TrainingPlan.prototype.getBestTrainers = function(gameState)
{
	if (this.metadata && this.metadata.trainer)
	{
		let trainer = gameState.getEntityById(this.metadata.trainer);
		if (trainer)
			return [trainer];
	}

	let allTrainers = gameState.findTrainers(this.type);
	if (this.metadata && this.metadata.sea)
		allTrainers = allTrainers.filter(API3.Filters.byMetadata(PlayerID, "sea", this.metadata.sea));
	if (this.metadata && this.metadata.base)
		allTrainers = allTrainers.filter(API3.Filters.byMetadata(PlayerID, "base", this.metadata.base));
	if (!allTrainers || !allTrainers.hasEntities())
		return undefined;

	// Keep only trainers with smallest cost
	let costMin = Math.min();
	let trainers;
	for (let ent of allTrainers.values())
	{
		let cost = this.template.costSum(ent);
		if (cost === costMin)
			trainers.push(ent);
		else if (cost < costMin)
		{
			costMin = cost;
			trainers = [ent];
		}
	}
	return trainers;
};

m.TrainingPlan.prototype.start = function(gameState)
{
	if (this.metadata && this.metadata.trainer)
	{
		let metadata = {};
		for (let key in this.metadata)
			if (key !== "trainer")
				metadata[key] = this.metadata[key];
		this.metadata = metadata;
	}

	if (this.trainers.length > 1)
	{
		let wantedIndex;
		if (this.metadata && this.metadata.index)
			wantedIndex = this.metadata.index;
		let workerUnit = this.metadata && this.metadata.role && this.metadata.role == "worker";
		let supportUnit = this.template.hasClass("Support");
		this.trainers.sort(function(a, b) {
			// Prefer training buildings with short queues
			let aa = a.trainingQueueTime();
			let bb = b.trainingQueueTime();
			// Give priority to support units in the cc
			if (a.hasClass("Civic") && !supportUnit)
				aa += 10;
			if (b.hasClass("Civic") && !supportUnit)
				bb += 10;
			// And support units should not be too near to dangerous place
			if (supportUnit)
			{
				if (gameState.ai.HQ.isNearInvadingArmy(a.position()))
					aa += 50;
				if (gameState.ai.HQ.isNearInvadingArmy(b.position()))
					bb += 50;
			}
			// Give also priority to buildings with the right accessibility
			let aBase = a.getMetadata(PlayerID, "base");
			let bBase = b.getMetadata(PlayerID, "base");
			if (wantedIndex)
			{
				if (!aBase || gameState.ai.HQ.getBaseByID(aBase).accessIndex != wantedIndex)
					aa += 30;
				if (!bBase || gameState.ai.HQ.getBaseByID(bBase).accessIndex != wantedIndex)
					bb += 30;
			}
			// Then, if workers, small preference for bases with less workers
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
	}

	if (this.metadata && this.metadata.base !== undefined && this.metadata.base === 0)
		this.metadata.base = this.trainers[0].getMetadata(PlayerID, "base");
	this.trainers[0].train(gameState.getPlayerCiv(), this.type, this.number, this.metadata, this.promotedTypes(gameState));

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
	return {
		"category": this.category,
		"type": this.type,
		"ID": this.ID,
		"metadata": this.metadata,
		"cost": this.cost.Serialize(),
		"number": this.number,
		"maxMerge": this.maxMerge
	};
};

m.TrainingPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data)
		this[key] = data[key];

	this.cost = new API3.Resources();
	this.cost.Deserialize(data.cost);
};

return m;
}(PETRA);
