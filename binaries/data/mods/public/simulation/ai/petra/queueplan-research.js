var PETRA = function(m)
{

m.ResearchPlan = function(gameState, type, rush)
{
	if (!m.QueuePlan.call(this, gameState, type, {}))
		return false;

	if (this.template.researchTime === undefined)
		return false;

	this.category = "technology";

	this.rush = rush ? true : false;
	
	return true;
};

m.ResearchPlan.prototype = Object.create(m.QueuePlan.prototype);

m.ResearchPlan.prototype.canStart = function(gameState)
{
	// also checks canResearch
	return (gameState.findResearchers(this.type).length !== 0);
};

m.ResearchPlan.prototype.isInvalid = function(gameState)
{
	return (gameState.isResearched(this.type) || gameState.isResearching(this.type));
};

m.ResearchPlan.prototype.start = function(gameState)
{
	var self = this;
	
	//m.debug ("Starting the research plan for " + this.type);
	var trainers = gameState.findResearchers(this.type).toEntityArray();

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
	this.onStart(gameState);
};

return m;
}(PETRA);
