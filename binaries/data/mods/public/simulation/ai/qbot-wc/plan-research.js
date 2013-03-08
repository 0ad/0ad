var ResearchPlan = function(gameState, type, rush) {
	this.type = type;

	this.template = gameState.getTemplate(this.type);
	if (!this.template || this.template.researchTime === undefined) {
		this.invalidTemplate = true;
		this.template = undefined;
		debug ("Invalid research");
		return false;
	}
	this.category = "technology";
	this.cost = new Resources(this.template.cost(),0);
	this.number = 1; // Obligatory for compatibility
	if (rush)
		this.rush = true;
	else
		this.rush = false;
	return true;
};

ResearchPlan.prototype.canExecute = function(gameState) {
	if (this.invalidTemplate)
		return false;

	// also checks canResearch
	return (gameState.findResearchers(this.type).length !== 0);
};

ResearchPlan.prototype.execute = function(gameState) {
	var self = this;
	//debug ("Starting the research plan for " + this.type);
	var trainers = gameState.findResearchers(this.type).toEntityArray();

	//for (i in trainers)
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
	return this.cost;
};

