var TrainingPlan = function(gameState, type, metadata, number, startTime, expectedTime, maxMerge) {
	this.type = gameState.applyCiv(type);
	this.metadata = metadata;

	this.ID = uniqueIDBOPlans++;
	
	this.template = gameState.getTemplate(this.type);
	if (!this.template)
		return false;

	this.category = "unit";
	this.cost = new Resources(this.template.cost(), this.template._template.Cost.Population);
	if (!number)
		this.number = 1;
	else
		this.number = number;
	
	if (!maxMerge)
		this.maxMerge = 5;
	else
		this.maxMerge = maxMerge;

	if (!startTime)
		this.startTime = 0;
	else
		this.startTime = startTime;

	if (!expectedTime)
		this.expectedTime = -1;
	else
		this.expectedTime = expectedTime;
	return true;
};

// return true if we willstart amassing resource for this plan
TrainingPlan.prototype.isGo = function(gameState) {
	return (gameState.getTimeElapsed() > this.startTime);
};

TrainingPlan.prototype.canStart = function(gameState) {
	if (this.invalidTemplate)
		return false;

	// TODO: we should probably check pop caps

	var trainers = gameState.findTrainers(this.type);

	return (trainers.length != 0);
};

TrainingPlan.prototype.start = function(gameState) {
	//warn("Executing TrainingPlan " + uneval(this));
	var self = this;
	var trainers = gameState.findTrainers(this.type).toEntityArray();

	// Prefer training buildings with short queues
	// (TODO: this should also account for units added to the queue by
	// plans that have already been executed this turn)
	if (trainers.length > 0){
		trainers.sort(function(a, b) {
			var aa = a.trainingQueueTime();
			var bb = b.trainingQueueTime();
			if (a.hasClass("Civic") && !self.template.hasClass("Support"))
				aa += 0.9;
			if (b.hasClass("Civic") && !self.template.hasClass("Support"))
				bb += 0.9;
			return (aa - bb);
		});
		if (this.metadata && this.metadata.base !== undefined && this.metadata.base === 0)
			this.metadata.base = trainers[0].getMetadata(PlayerID,"base");
		trainers[0].train(this.type, this.number, this.metadata);
	}
};

TrainingPlan.prototype.getCost = function(){
	var multCost = new Resources();
	multCost.add(this.cost);
	multCost.multiply(this.number);
	return multCost;
};

TrainingPlan.prototype.addItem = function(amount){
	if (amount === undefined)
		amount = 1;
	this.number += amount;
};
