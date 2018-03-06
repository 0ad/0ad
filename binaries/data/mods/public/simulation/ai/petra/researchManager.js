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
	if (queues.majorTech.hasQueuedUnits())
		return;
	// Don't try to phase up if already trying to gather resources for a civil-centre or wonder
	if (queues.civilCentre.hasQueuedUnits() || queues.wonder.hasQueuedUnits())
		return;

	let currentPhaseIndex = gameState.currentPhase();
	let nextPhaseName = gameState.getPhaseName(currentPhaseIndex+1);
	if (!nextPhaseName)
		return;

	let petraRequirements =
		currentPhaseIndex == 1 && gameState.ai.HQ.getAccountedPopulation(gameState) >= this.Config.Economy.popPhase2 ||
		currentPhaseIndex == 2 && gameState.ai.HQ.getAccountedWorkers(gameState) > this.Config.Economy.workPhase3 ||
		currentPhaseIndex >= 3 && gameState.ai.HQ.getAccountedWorkers(gameState) > this.Config.Economy.workPhase4;
	if (petraRequirements && gameState.hasResearchers(nextPhaseName, true))
	{
		gameState.ai.HQ.phasing = currentPhaseIndex + 1;
		// Reset the queue priority in case it was changed during a previous phase update
		gameState.ai.queueManager.changePriority("majorTech", gameState.ai.Config.priorities.majorTech);
		queues.majorTech.addPlan(new m.ResearchPlan(gameState, nextPhaseName, true));
	}
};

m.ResearchManager.prototype.researchPopulationBonus = function(gameState, queues)
{
	if (queues.minorTech.hasQueuedUnits())
		return;

	let techs = gameState.findAvailableTech();
	for (let tech of techs)
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
	if (queues.minorTech.hasQueuedUnits())
		return;

	let techs = gameState.findAvailableTech();
	for (let tech of techs)
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

/** Techs to be searched for as soon as they are available */
m.ResearchManager.prototype.researchWantedTechs = function(gameState, techs)
{
	let phase1 = gameState.currentPhase() === 1;
	let available = phase1 ? gameState.ai.queueManager.getAvailableResources(gameState) : null;
	let numWorkers = phase1 ? gameState.getOwnEntitiesByRole("worker", true).length : 0;
	for (let tech of techs)
	{
		if (!tech[1]._template.modifications)
			continue;
		let template = tech[1]._template;
		if (phase1)
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
			if (gameState.ai.HQ.navalMap && template.modifications[i].value === "ResourceGatherer/Rates/food.fish")
				return { "name": tech[0], "increasePriority": this.CostSum(template.cost) < 400 };
			else if (template.modifications[i].value === "ResourceGatherer/Rates/food.fruit")
				return { "name": tech[0], "increasePriority": this.CostSum(template.cost) < 400 };
			else if (template.modifications[i].value === "ResourceGatherer/Rates/food.grain")
				return { "name": tech[0], "increasePriority": false };
			else if (template.modifications[i].value === "ResourceGatherer/Rates/wood.tree")
				return { "name": tech[0], "increasePriority": this.CostSum(template.cost) < 400 };
			else if (template.modifications[i].value.startsWith("ResourceGatherer/Capacities"))
				return { "name": tech[0], "increasePriority": false };
			else if (template.modifications[i].value === "Attack/Ranged/MaxRange")
				return { "name": tech[0], "increasePriority": false };
		}
	}
	return null;
};

/** Techs to be searched for as soon as they are available, but only after phase 2 */
m.ResearchManager.prototype.researchPreferredTechs = function(gameState, techs)
{
	let phase2 = gameState.currentPhase() === 2;
	let available = phase2 ? gameState.ai.queueManager.getAvailableResources(gameState) : null;
	let numWorkers = phase2 ? gameState.getOwnEntitiesByRole("worker", true).length : 0;
	for (let tech of techs)
	{
		if (!tech[1]._template.modifications)
			continue;
		let template = tech[1]._template;
		if (phase2)
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
				return { "name": tech[0], "increasePriority": this.CostSum(template.cost) < 400 };
			else if (template.modifications[i].value === "ResourceGatherer/Rates/metal.ore")
				return { "name": tech[0], "increasePriority": this.CostSum(template.cost) < 400 };
			else if (template.modifications[i].value === "BuildingAI/DefaultArrowCount")
				return { "name": tech[0], "increasePriority": this.CostSum(template.cost) < 400 };
			else if (template.modifications[i].value === "Health/RegenRate")
				return { "name": tech[0], "increasePriority": false };
			else if (template.modifications[i].value === "Health/IdleRegenRate")
				return { "name": tech[0], "increasePriority": false };
		}
	}
	return null;
};

m.ResearchManager.prototype.update = function(gameState, queues)
{
	if (queues.minorTech.hasQueuedUnits() || queues.majorTech.hasQueuedUnits())
		return;

	let techs = gameState.findAvailableTech();

	let techName = this.researchWantedTechs(gameState, techs);
	if (techName)
	{
		if (techName.increasePriority)
		{
			gameState.ai.queueManager.changePriority("minorTech", 2*this.Config.priorities.minorTech);
			let plan = new m.ResearchPlan(gameState, techName.name);
			plan.queueToReset = "minorTech";
			queues.minorTech.addPlan(plan);
		}
		else
			queues.minorTech.addPlan(new m.ResearchPlan(gameState, techName.name));
		return;
	}

	if (gameState.currentPhase() < 2)
		return;

	techName = this.researchPreferredTechs(gameState, techs);
	if (techName)
	{
		if (techName.increasePriority)
		{
			gameState.ai.queueManager.changePriority("minorTech", 2*this.Config.priorities.minorTech);
			let plan = new m.ResearchPlan(gameState, techName.name);
			plan.queueToReset = "minorTech";
			queues.minorTech.addPlan(plan);
		}
		else
			queues.minorTech.addPlan(new m.ResearchPlan(gameState, techName.name));
		return;
	}

	if (gameState.currentPhase() < 3)
		return;

	// remove some techs not yet used by this AI
	// remove also sharedLos if we have no ally
	for (let i = 0; i < techs.length; ++i)
	{
		let template = techs[i][1]._template;
		if (template.affects && template.affects.length === 1 &&
			(template.affects[0] === "Healer" || template.affects[0] === "Outpost" || template.affects[0] === "StoneWall"))
		{
			techs.splice(i--, 1);
			continue;
		}
		if (template.modifications && template.modifications.length === 1 &&
			template.modifications[0].value === "Player/sharedLos" &&
			!gameState.hasAllies())
		{
			techs.splice(i--, 1);
			continue;
		}
	}
	if (!techs.length)
		return;

	// randomly pick one. No worries about pairs in that case.
	queues.minorTech.addPlan(new m.ResearchPlan(gameState, pickRandom(techs)[0]));
};

m.ResearchManager.prototype.CostSum = function(cost)
{
	let costSum = 0;
	for (let res in cost)
		costSum += cost[res];
	return costSum;
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
