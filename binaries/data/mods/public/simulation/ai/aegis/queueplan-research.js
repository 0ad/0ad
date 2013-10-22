var ResearchPlan = function(gameState, type, startTime, expectedTime, rush) {
	this.type = type;

	this.ID = uniqueIDBOPlans++;

	this.template = gameState.getTemplate(this.type);
	if (!this.template || this.template.researchTime === undefined) {
		return false;
	}
	this.category = "technology";
	this.cost = new Resources(this.template.cost(),0);
	this.number = 1; // Obligatory for compatibility
	
	if (!startTime)
		this.startTime = 0;
	else
		this.startTime = startTime;
	
	if (!expectedTime)
		this.expectedTime = -1;
	else
		this.expectedTime = expectedTime;

	if (rush)
		this.rush = true;
	else
		this.rush = false;
	
	return true;
};

// return true if we willstart amassing resource for this plan
ResearchPlan.prototype.isGo = function(gameState) {
	return (gameState.getTimeElapsed() > this.startTime);
};

ResearchPlan.prototype.canStart = function(gameState) {
	// also checks canResearch
	return (gameState.findResearchers(this.type).length !== 0);
};

ResearchPlan.prototype.start = function(gameState) {
	var self = this;
	
	// TODO: this is special cased for "rush" technologies, ie the town phase
	// which currently is a 100% focus.
	gameState.ai.queueManager.unpauseAll();
	
	//debug ("Starting the research plan for " + this.type);
	var trainers = gameState.findResearchers(this.type).toEntityArray();

	//for (var i in trainers)
	//	warn (this.type + " - " +trainers[i].genericName());
	
	// Prefer training buildings with short queues
	// (TODO: this should also account for units added to the queue by
	// plans that have already been executed this turn)
	if (trainers.length > 0){
		trainers.sort(function(a, b) {
			return (a.trainingQueueTime() - b.trainingQueueTime());
		});
		// drop anything in the queue if we rush it.
		if (this.rush)
			trainers[0].stopAllProduction(0.45);
		trainers[0].research(this.type);
	}
};

ResearchPlan.prototype.getCost = function(){
	var costs = new Resources();
	costs.add(this.cost);
	return costs;
};

