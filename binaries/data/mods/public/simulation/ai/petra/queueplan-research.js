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
	var trainers = gameState.findResearchers(this.type).toEntityArray();

	// Prefer training buildings with short queues
	// (TODO: this should also account for units added to the queue by
	// plans that have already been executed this turn)
	if (trainers.length > 0)
	{
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

m.ResearchPlan.prototype.Serialize = function()
{
	return {
		"type": this.type,
		"metadata": this.metadata,
		"ID": this.ID,
		"category": this.category,
		"cost": this.cost.Serialize(),
		"number": this.number,
		"lastIsGo": this.lastIsGo,
		"rush": this.rush
	};
};

m.ResearchPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data)
		this[key] = data[key];

	let cost = new API3.Resources();
	cost.Deserialize(data.cost);
	this.cost = cost;

	// TODO find a way to properly serialize functions. For the time being, they are manually added
	if (this.type == gameState.townPhase())
	{
		this.onStart = function (gameState) {
			gameState.ai.HQ.econState = "growth";
			gameState.ai.HQ.OnTownPhase(gameState);
		};
		this.isGo = function (gameState) {
			var ret = gameState.getPopulation() >= gameState.ai.Config.Economy.popForTown;
			if (ret && !this.lastIsGo)
				this.onGo(gameState);
			else if (!ret && this.lastIsGo)
				this.onNotGo(gameState);
			this.lastIsGo = ret;
			return ret;
		};
		this.onGo = function (gameState) { gameState.ai.HQ.econState = "townPhasing"; };
		this.onNotGo = function (gameState) { gameState.ai.HQ.econState = "growth"; };
	}
	else if (this.type == gameState.cityPhase())
	{
		this.onStart = function (gameState) { gameState.ai.HQ.OnCityPhase(gameState) };
	}
};

return m;
}(PETRA);
