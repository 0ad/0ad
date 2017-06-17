var PETRA = function(m)
{

m.ResearchPlan = function(gameState, type, rush = false)
{
	if (!m.QueuePlan.call(this, gameState, type, {}))
		return false;

	if (this.template.researchTime === undefined)
		return false;

	// Refine the estimated cost
	let researchers = this.getBestResearchers(gameState, true);
	if (researchers)
		this.cost = new API3.Resources(this.template.cost(researchers[0]));

	this.category = "technology";
	this.rush = rush;

	return true;
};

m.ResearchPlan.prototype = Object.create(m.QueuePlan.prototype);

m.ResearchPlan.prototype.canStart = function(gameState)
{
	this.researchers = this.getBestResearchers(gameState);
	if (!this.researchers)
		return false;
	this.cost = new API3.Resources(this.template.cost(this.researchers[0]));
	return true;
};

m.ResearchPlan.prototype.getBestResearchers = function(gameState, noRequirementCheck = false)
{
	let allResearchers = gameState.findResearchers(this.type, noRequirementCheck);
	if (!allResearchers || !allResearchers.hasEntities())
		return undefined;

	// Keep only researchers with smallest cost
	let costMin = Math.min();
	let researchers;
	for (let ent of allResearchers.values())
	{
		let cost = this.template.costSum(ent);
		if (cost === costMin)
			researchers.push(ent);
		else if (cost < costMin)
		{
			costMin = cost;
			researchers = [ent];
		}
	}
	return researchers;
};

m.ResearchPlan.prototype.isInvalid = function(gameState)
{
	return gameState.isResearched(this.type) || gameState.isResearching(this.type);
};

m.ResearchPlan.prototype.start = function(gameState)
{
	// Prefer researcher with shortest queues (no need to serialize this.researchers
	// as the functions canStart and start are always called on the same turn)
	this.researchers.sort((a, b) => a.trainingQueueTime() - b.trainingQueueTime());
	// Drop anything in the queue if we rush it.
	if (this.rush)
		this.researchers[0].stopAllProduction(0.45);
	this.researchers[0].research(this.type);
	this.onStart(gameState);
};

m.ResearchPlan.prototype.isGo = function(gameState)
{
	if (this.type === gameState.townPhase())
	{
		let ret = gameState.getPopulation() >= gameState.ai.Config.Economy.popForTown;
		if (ret && gameState.ai.HQ.econState !== "growth")
			gameState.ai.HQ.econState = "growth";
		else if (!ret && gameState.ai.HQ.econState !== "townPhasing")
			gameState.ai.HQ.econState = "townPhasing";
		return ret;
	}
	else if (this.type === gameState.cityPhase())
		gameState.ai.HQ.econState = "cityPhasing";
	return true;
};

m.ResearchPlan.prototype.onStart = function(gameState)
{
	if (this.queueToReset)
		gameState.ai.queueManager.changePriority(this.queueToReset, gameState.ai.Config.priorities[this.queueToReset]);

	if (this.type == gameState.townPhase())
	{
		gameState.ai.HQ.econState = "growth";
		gameState.ai.HQ.OnTownPhase(gameState);
	}
	else if (this.type == gameState.cityPhase())
	{
		gameState.ai.HQ.econState = "growth";
		gameState.ai.HQ.OnCityPhase(gameState);
	}
};

m.ResearchPlan.prototype.Serialize = function()
{
	return {
		"category": this.category,
		"type": this.type,
		"ID": this.ID,
		"metadata": this.metadata,
		"cost": this.cost.Serialize(),
		"number": this.number,
		"rush": this.rush,
		"queueToReset": this.queueToReset || undefined
	};
};

m.ResearchPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data)
		this[key] = data[key];

	let cost = new API3.Resources();
	cost.Deserialize(data.cost);
	this.cost = cost;
};

return m;
}(PETRA);
