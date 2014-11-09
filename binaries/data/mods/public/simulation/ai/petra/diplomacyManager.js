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
	this.lastTributeUpdate = -1;
};

// Check if any allied needs help (tribute) and sent it if we have enough resource
m.DiplomacyManager.prototype.tributes = function(gameState)
{
	this.lastTributeUpdate = gameState.ai.elapsedTime;
	var availableResources = gameState.ai.queueManager.getAvailableResources(gameState);
	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (i === PlayerID || !gameState.isPlayerAlly(i))
			continue;
		let allyResources = gameState.sharedScript.playersData[i].resourceCounts;
		let tribute = {};
		let toSend = false;
		for (let res in allyResources)
		{
			if (allyResources[res] > 500 || availableResources[res] < 1000*Math.ceil((allyResources[res]+1)/100))
				continue;
			tribute[res] = 100*Math.floor(availableResources[res]/1000);
			toSend = true;
		}
		if (toSend)
		{
			if (this.Config.debug > 1)
				API3.warn("Tribute " + uneval(tribute) + " sent to player " + i);
			if (this.Config.chat)
				m.chatSentTribute(gameState, i);
			Engine.PostCommand(PlayerID, {"type": "tribute", "player": i, "amounts": tribute});
		}
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

	if (gameState.ai.elapsedTime - this.lastTributeUpdate > 60)
		this.tributes(gameState);
};

m.DiplomacyManager.prototype.Serialize = function()
{
	return { "lastTributeUpdate": this.lastTributeUpdate };
};

m.DiplomacyManager.prototype.Deserialize = function(data)
{
	this.lastTributeUpdate = data.lastTributeUpdate;
};

return m;
}(PETRA);
