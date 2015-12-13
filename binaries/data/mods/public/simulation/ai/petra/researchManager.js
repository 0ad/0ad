var PETRA = function(m)
{

/**
 * Manage the research
 */

m.ResearchManager = function(Config)
{
	this.Config = Config;
};

/**
 * Check if we can go to the next phase
 */
m.ResearchManager.prototype.checkPhase = function(gameState, queues)
{
	if (queues.majorTech.length() !== 0)
		return;

	var townPhase = gameState.townPhase();
	var cityPhase = gameState.cityPhase();
		
	if (gameState.canResearch(townPhase,true) && gameState.getPopulation() >= this.Config.Economy.popForTown - 10
		&& gameState.findResearchers(townPhase,true).length != 0)
	{
		var plan = new m.ResearchPlan(gameState, townPhase, true);
		plan.lastIsGo = false;
		plan.onStart = function (gameState) { gameState.ai.HQ.econState = "growth"; gameState.ai.HQ.OnTownPhase(gameState); };
		plan.isGo = function (gameState) {
			var ret = gameState.getPopulation() >= gameState.ai.Config.Economy.popForTown;
			if (ret && !this.lastIsGo)
				this.onGo(gameState);
			else if (!ret && this.lastIsGo)
				this.onNotGo(gameState);
			this.lastIsGo = ret;
			return ret;
		};
		plan.onGo = function (gameState) { gameState.ai.HQ.econState = "townPhasing"; };
		plan.onNotGo = function (gameState) { gameState.ai.HQ.econState = "growth"; };

		queues.majorTech.addPlan(plan);
	}
	else if (gameState.canResearch(cityPhase,true) && gameState.ai.elapsedTime > this.Config.Economy.cityPhase
			&& gameState.getOwnEntitiesByRole("worker", true).length > this.Config.Economy.workForCity
			&& gameState.findResearchers(cityPhase, true).length != 0
			&& queues.civilCentre.length() === 0)
	{
		var plan = new m.ResearchPlan(gameState, cityPhase, true);
		plan.onStart = function (gameState) { gameState.ai.HQ.OnCityPhase(gameState); };
		queues.majorTech.addPlan(plan);
	}
};

m.ResearchManager.prototype.researchPopulationBonus = function(gameState, queues)
{
	if (queues.minorTech.length() !== 0)
		return;

	var techs = gameState.findAvailableTech();
	for (var tech of techs)
	{
		if (!tech[1]._template.modifications)
			continue;
		// TODO may-be loop on all modifs and check if the effect if positive ?
		if (tech[1]._template.modifications[0].value !== "Cost/PopulationBonus")
			continue;
		queues.minorTech.addPlan(new m.ResearchPlan(gameState, tech[0]));
		break;
	}
};

m.ResearchManager.prototype.researchTradeBonus = function(gameState, queues)
{
	if (queues.minorTech.length() !== 0)
		return;

	var techs = gameState.findAvailableTech();
	for (var tech of techs)
	{
		if (!tech[1]._template.modifications || !tech[1]._template.affects)
			continue;
		if (tech[1]._template.affects.indexOf("Trader") === -1)
			continue;
		// TODO may-be loop on all modifs and check if the effect if positive ?
		if (tech[1]._template.modifications[0].value !== "UnitMotion/WalkSpeed" &&
                    tech[1]._template.modifications[0].value !== "Trader/GainMultiplier")
			continue;
		queues.minorTech.addPlan(new m.ResearchPlan(gameState, tech[0]));
		break;
	}
};

// Techs to be searched for as soon as they are available
m.ResearchManager.prototype.researchWantedTechs = function(gameState, techs)
{
	var available = (gameState.currentPhase() == 1 ? gameState.ai.queueManager.getAvailableResources(gameState) : null);
	var numWorkers = (gameState.currentPhase() == 1 ? gameState.getOwnEntitiesByRole("worker", true).length : 0);
	for (var tech of techs)
	{
		if (!tech[1]._template.modifications)
			continue;
		let template = tech[1]._template;
		if (gameState.currentPhase() == 1)
		{
			let cost = template.cost;
			let costMax = 0;
			for (let res in cost)
				costMax = Math.max(costMax, Math.max(cost[res]-available[res], 0));
			if (10*numWorkers < costMax)
				continue;
		}
		for (let i in template.modifications)
		{
			if (template.modifications[i].value === "ResourceGatherer/Rates/food.fruit")
				return tech[0];		
			else if (template.modifications[i].value === "ResourceGatherer/Rates/food.grain")
				return tech[0];
			else if (template.modifications[i].value === "ResourceGatherer/Rates/wood.tree")
			    return tech[0];
			else if (template.modifications[i].value.startsWith("ResourceGatherer/Capacities"))
				return tech[0];
			else if (template.modifications[i].value === "Attack/Ranged/MaxRange")
				return tech[0];
		}
	}
	return false;
};

// Techs to be searched for as soon as they are available, but only after phase 2
m.ResearchManager.prototype.researchPreferredTechs = function(gameState, techs)
{
	var available = (gameState.currentPhase() == 2 ? gameState.ai.queueManager.getAvailableResources(gameState) : null);
	var numWorkers = (gameState.currentPhase() == 2 ? gameState.getOwnEntitiesByRole("worker", true).length : 0);
	for (let tech of techs)
	{
		if (!tech[1]._template.modifications)
			continue;
		let template = tech[1]._template;
	    	if (gameState.currentPhase() == 2)
		{
			let cost = template.cost;
			let costMax = 0;
			for (let res in cost)
				costMax = Math.max(costMax, Math.max(cost[res]-available[res], 0));
			if (10*numWorkers < costMax)
				continue;
		}
		for (let i in template.modifications)
		{		
			if (template.modifications[i].value === "ResourceGatherer/Rates/stone.rock")
				return tech[0];
			else if (template.modifications[i].value === "ResourceGatherer/Rates/metal.ore")
				return tech[0];
			else if (template.modifications[i].value === "BuildingAI/DefaultArrowCount")
				return tech[0];
			else if (template.modifications[i].value === "Health/RegenRate")
				return tech[0];
		}
	}
	return false;
};

m.ResearchManager.prototype.update = function(gameState, queues)
{
	if (queues.minorTech.length() > 0 || queues.majorTech.length() > 0)
		return;

	var techs = gameState.findAvailableTech();

	var techName = this.researchWantedTechs(gameState, techs);
	if (techName)
	{
		queues.minorTech.addPlan(new m.ResearchPlan(gameState, techName));
		return;
	}

	if (gameState.currentPhase() < 2)
		return;

	var techName = this.researchPreferredTechs(gameState, techs);
	if (techName)
	{
		queues.minorTech.addPlan(new m.ResearchPlan(gameState, techName));
		return;
	}

	if (gameState.currentPhase() < 3)
		return;

	// remove some techs not yet used by this AI
	// remove alse sharedLos if we have no ally
	for (let i = 0; i < techs.length; ++i)
	{
		let template = techs[i][1]._template;
		if (template.affects && template.affects.length === 1
			&& (template.affects[0] === "Healer" || template.affects[0] === "Outpost" || template.affects[0] === "StoneWall"))
		{
			techs.splice(i--, 1);
			continue;
		}
		if (template.modifications && template.modifications.length === 1
			&& template.modifications[0].value === "Player/sharedLos"
			&& !gameState.hasAllies())
		{
			techs.splice(i--, 1);
			continue;
		}
	}
	if (techs.length === 0)
		return;
	// randomly pick one. No worries about pairs in that case.
	var p = Math.floor((Math.random()*techs.length));
	queues.minorTech.addPlan(new m.ResearchPlan(gameState, techs[p][0]));
};

m.ResearchManager.prototype.Serialize = function()
{
	return {};
};

m.ResearchManager.prototype.Deserialize = function(data)
{
};

return m;
}(PETRA);
