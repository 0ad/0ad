var PETRA = function(m)
{

/**
 * Manage the diplomacy:
 *     update our cooperative trait
 *     sent tribute to allies
 */

m.DiplomacyManager = function(Config)
{
	this.Config = Config;
	this.nextTributeUpdate = -1;
};

// Check if any allied needs help (tribute) and sent it if we have enough resource
m.DiplomacyManager.prototype.tributes = function(gameState)
{
	this.nextTributeUpdate = gameState.ai.elapsedTime + 30;
	var totalResources = gameState.getResources();
	var availableResources = gameState.ai.queueManager.getAvailableResources(gameState);
	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (i === PlayerID || !gameState.isPlayerAlly(i))
			continue;
		let allyResources = gameState.sharedScript.playersData[i].resourceCounts;
		let allyPop = gameState.sharedScript.playersData[i].popCount;
		let tribute = {};
		let toSend = false;
		for (let res in allyResources)
		{
			if (availableResources[res] > 200 && allyResources[res] < 0.2 * availableResources[res])
			{
				tribute[res] = Math.floor(0.3*availableResources[res] - allyResources[res]);
				toSend = true;
			}
			else if (allyPop < Math.min(30, 0.5*gameState.getPopulation()) && totalResources[res] > 500 && allyResources[res] < 100)
			{
				tribute[res] = 100;
				toSend = true;
			}
		}
		if (!toSend)
			continue;
		if (this.Config.debug > 1)
			API3.warn("Tribute " + uneval(tribute) + " sent to player " + i);
		if (this.Config.chat)
			m.chatSentTribute(gameState, i);
		Engine.PostCommand(PlayerID, {"type": "tribute", "player": i, "amounts": tribute});
	}
};

m.DiplomacyManager.prototype.checkEvents = function (gameState, events)
{
	// Increase slowly the cooperative personality trait either when we receive tribute from our allies
	// or if our allies attack enemies inside our territory
	for (let evt of events["TributeExchanged"])
	{
		if (evt.to !== PlayerID || !gameState.isPlayerAlly(evt.from))
			continue;
		let tributes = 0;
		for (let key in evt.amounts)
		{
			if (key === "food")
				tributes += evt.amounts[key];
			else
				tributes += 2*evt.amounts[key];
		}
		this.Config.personality.cooperative = Math.min(1, this.Config.personality.cooperative + 0.0002 * tributes);
	}

	for (let evt of events["Attacked"])
	{
		let target = gameState.getEntityById(evt.target);
		if (!target || !target.position()
			|| gameState.ai.HQ.territoryMap.getOwner(target.position()) !== PlayerID
			|| !gameState.isPlayerEnemy(target.owner()))
			continue;
		let attacker = gameState.getEntityById(evt.attacker);
		if (!attacker || attacker.owner() === PlayerID || !gameState.isPlayerAlly(attacker.owner()))
			continue;
		this.Config.personality.cooperative = Math.min(1, this.Config.personality.cooperative + 0.003);
	}
};


m.DiplomacyManager.prototype.update = function(gameState, events)
{
	this.checkEvents(gameState, events);

	if (!gameState.ai.HQ.saveResources && gameState.ai.elapsedTime > this.nextTributeUpdate)
		this.tributes(gameState);
};

m.DiplomacyManager.prototype.Serialize = function()
{
	return { "nextTributeUpdate": this.nextTributeUpdate };
};

m.DiplomacyManager.prototype.Deserialize = function(data)
{
	this.nextTributeUpdate = data.nextTributeUpdate;
};

return m;
}(PETRA);
