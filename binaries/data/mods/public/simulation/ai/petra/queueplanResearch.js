var PETRA = function(m)
{

m.ResearchPlan = function(gameState, type, rush = false)
{
	if (!m.QueuePlan.call(this, gameState, type, {}))
		return false;

	if (this.template.researchTime === undefined)
		return false;

	this.category = "technology";

	this.rush = rush;
	
	return true;
};

m.ResearchPlan.prototype = Object.create(m.QueuePlan.prototype);

m.ResearchPlan.prototype.canStart = function(gameState)
{
	// also checks canResearch
	return gameState.hasResearchers(this.type);
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
	let prop = {
		"category": this.category,
		"type": this.type,
		"ID": this.ID,
		"metadata": this.metadata,
		"cost": this.cost.Serialize(),
		"number": this.number,
		"rush": this.rush,
		"lastIsGo": this.lastIsGo,
	};

	let func = {
		"isGo": uneval(this.isGo),
		"onGo": uneval(this.onGo),
		"onNotGo": uneval(this.onNotGo),
		"onStart": uneval(this.onStart)
	};

	return { "prop": prop, "func": func };
};

m.ResearchPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.prop)
		this[key] = data.prop[key];

	let cost = new API3.Resources();
	cost.Deserialize(data.prop.cost);
	this.cost = cost;

	for (let fun in data.func)
		this[fun] = eval(data.func[fun]);
};

return m;
}(PETRA);
