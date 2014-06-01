var PETRA = function(m)
{

/**
 * Manage the research
 */

m.ResearchManager = function(Config)
{
	this.Config = Config;
};

m.ResearchManager.prototype.searchPopulationBonus = function(gameState, queues)
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
		if (this.Config.debug > 0)
			warn(" ... ok we've found the " + tech[0] + " tech");
		queues.minorTech.addItem(new m.ResearchPlan(gameState, tech[0]));
		break;
	}
};

m.ResearchManager.prototype.update = function(gameState, queues)
{
	if (gameState.currentPhase() < 2 || queues.minorTech.length() !== 0)
		return;

	var techs = gameState.findAvailableTech();
	for (var tech of techs)
	{
		var techName = tech[0];
		if (techName.indexOf("attack_tower_watch") !== -1 || techName.indexOf("gather_mining_servants") !== -1 ||
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
