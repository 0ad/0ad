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
		plan.onStart = function (gameState) { gameState.ai.HQ.econState = "growth"; gameState.ai.HQ.OnTownPhase(gameState) };
		plan.isGo = function (gameState) {
			var ret = gameState.getPopulation() >= gameState.Config.Economy.popForTown;
			if (ret && !this.lastIsGo)
				this.onGo(gameState);
			else if (!ret && this.lastIsGo)
				this.onNotGo(gameState);
			this.lastIsGo = ret;
			return ret;
		};
		plan.onGo = function (gameState) { gameState.ai.HQ.econState = "townPhasing"; };
		plan.onNotGo = function (gameState) { gameState.ai.HQ.econState = "growth"; };

		queues.majorTech.addItem(plan);
	}
	else if (gameState.canResearch(cityPhase,true) && gameState.ai.elapsedTime > this.Config.Economy.cityPhase
			&& gameState.getOwnEntitiesByRole("worker", true).length > 80
			&& gameState.findResearchers(cityPhase, true).length != 0
			&& queues.civilCentre.length() === 0)
	{
		var plan = new m.ResearchPlan(gameState, cityPhase, true);
		plan.onStart = function (gameState) { gameState.ai.HQ.OnCityPhase(gameState) };
		queues.majorTech.addItem(plan);
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
		queues.minorTech.addItem(new m.ResearchPlan(gameState, tech[0]));
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
		if (tech[1]._template.modifications[0].value !== "UnitMotion/WalkSpeed")
			continue;
		queues.minorTech.addItem(new m.ResearchPlan(gameState, tech[0]));
		break;
	}
};

// Techs to be searched for as soon as they are available
m.ResearchManager.prototype.researchWantedTechs = function(gameState, queues)
{
	var techs = gameState.findAvailableTech();

	var techName = undefined;
	for (var tech of techs)
	{
		if (!tech[1]._template.modifications)
			continue;
		var template = tech[1]._template;
		for (var i in template.modifications)
		{
			if (template.modifications[i].value.indexOf("ResourceGatherer/Capacities") !== -1)
			{
				queues.minorTech.addItem(new m.ResearchPlan(gameState, tech[0]));
				return;
			}
			else if (template.modifications[i].value === "ResourceGatherer/Rates/food.grain")
			{
				queues.minorTech.addItem(new m.ResearchPlan(gameState, tech[0]));
				return;
			}
			else if (template.modifications[i].value === "Health/RegenRate")
			{
				queues.minorTech.addItem(new m.ResearchPlan(gameState, tech[0]));
				return;
			}
			else if (template.modifications[i].value === "Attack/Ranged/MaxRange")
			{
				queues.minorTech.addItem(new m.ResearchPlan(gameState, tech[0]));
				return;
			}
			else if (template.modifications[i].value === "BuildingAI/DefaultArrowCount"
				&& template.affects.indexOf("Tower"))
			{
				queues.minorTech.addItem(new m.ResearchPlan(gameState, tech[0]));
				return;
			}
		}
	}
};

m.ResearchManager.prototype.update = function(gameState, queues)
{
	this.checkPhase(gameState, queues);

	if (queues.minorTech.length() !== 0 || gameState.ai.playedTurn % 4 !== 0)
		return;

	this.researchWantedTechs(gameState, queues);

	if (gameState.currentPhase() < 2)
		return;

	var techs = gameState.findAvailableTech();
	for (var tech of techs)
	{
		var techName = tech[0];
		if (techName.indexOf("gather_mining_servants") !== -1 ||
			techName.indexOf("gather_mining_shaftmining") !== -1)
		{
			queues.minorTech.addItem(new m.ResearchPlan(gameState, techName));
			return;
		}
	}

	if (gameState.currentPhase() < 3)
		return;

	// remove some tech not yet used by this AI
	for (var i = 0; i < techs.length; ++i)
	{
		var techName = techs[i][0];
		if (techName.indexOf("heal_rate") !== -1 || techName.indexOf("heal_range") !== -1 ||
			techName.indexOf("heal_temple") !== -1 || techName.indexOf("unlock_females_house") !== -1)
			techs.splice(i--, 1);
	}
	if (techs.length === 0)
		return;
	// randomly pick one. No worries about pairs in that case.
	var p = Math.floor((Math.random()*techs.length));
	queues.minorTech.addItem(new m.ResearchPlan(gameState, techs[p][0]));
};

return m;
}(PETRA);
