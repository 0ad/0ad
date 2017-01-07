var PETRA = function(m)
{

/**
 * Manage the diplomacy:
 *     update our cooperative trait
 *     sent tribute to allies
 *     decide which player to turn against in "Last Man Standing" mode
 *     respond to diplomacy requests
 */

/**
 * If a player sends us an ally request, an Object in this.allyRequests will be created
 * that includes the request status, and the amount and type of the resource tribute (if any)
 * that they must send in order for us to accept their request.
 * In addition, a message will be sent if the player has not sent us a tribute within a minute.
 * If two minutes pass without a tribute, we will decline their request.
 */
m.DiplomacyManager = function(Config)
{
	this.Config = Config;
	this.nextTributeUpdate = -1;
	this.nextTributeRequest = new Map();
	this.nextTributeRequest.set("all", 240);
	this.betrayLapseTime = -1;
	this.waitingToBetray = false;
	this.allyRequests = new Map();
};

/**
 * If there are any players that are allied with us but we are not allied with them,
 * treat this situation like an ally request.
 */
m.DiplomacyManager.prototype.init = function(gameState)
{
	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (i === PlayerID)
			continue;

		if (gameState.isPlayerMutualAlly(i))
			this.allyRequests.set(i, { "status": "accepted" });
		else if (gameState.sharedScript.playersData[i].isAlly[PlayerID])
			this.handleAllyRequest(gameState, i);
	}
};

/**
 * Check if any allied needs help (tribute) and sent it if we have enough resource
 * or ask for a tribute if we are in need and one ally can help
 */
m.DiplomacyManager.prototype.tributes = function(gameState)
{
	this.nextTributeUpdate = gameState.ai.elapsedTime + 30;
	let totalResources = gameState.getResources();
	let availableResources = gameState.ai.queueManager.getAvailableResources(gameState);
	let mostNeeded;
	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (i === PlayerID || !gameState.isPlayerAlly(i) || gameState.ai.HQ.attackManager.defeated[i])
			continue;
		let donor = gameState.getAlliedVictory() || gameState.getEntities(i).length < gameState.getOwnEntities().length;
		let allyResources = gameState.sharedScript.playersData[i].resourceCounts;
		let allyPop = gameState.sharedScript.playersData[i].popCount;
		let tribute = {};
		let toSend = false;
		for (let res in allyResources)
		{
			if (donor && availableResources[res] > 200 && allyResources[res] < 0.2 * availableResources[res])
			{
				tribute[res] = Math.floor(0.3*availableResources[res] - allyResources[res]);
				toSend = true;
			}
			else if (donor && allyPop < Math.min(30, 0.5*gameState.getPopulation()) && totalResources[res] > 500 && allyResources[res] < 100)
			{
				tribute[res] = 100;
				toSend = true;
			}
			else if (this.Config.chat && availableResources[res] === 0 && allyResources[res] > totalResources[res] + 600)
			{
				if (gameState.ai.elapsedTime < this.nextTributeRequest.get("all"))
					continue;
				if (this.nextTributeRequest.has(res) && gameState.ai.elapsedTime < this.nextTributeRequest.get(res))
					continue;
				if (!mostNeeded)
					mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
				for (let k = 0; k < 2; ++k)
				{
					if (mostNeeded[k].type == res && mostNeeded[k].wanted > 0)
					{
						this.nextTributeRequest.set("all", gameState.ai.elapsedTime + 90);
						this.nextTributeRequest.set(res, gameState.ai.elapsedTime + 240);
						m.chatRequestTribute(gameState, res);
						if (this.Config.debug > 1)
							API3.warn("Tribute on " + res + " requested to player " + i);
						break;
					}
				}
			}
		}
		if (!toSend)
			continue;
		if (this.Config.debug > 1)
			API3.warn("Tribute " + uneval(tribute) + " sent to player " + i);
		if (this.Config.chat)
			m.chatSentTribute(gameState, i);
		Engine.PostCommand(PlayerID, { "type": "tribute", "player": i, "amounts": tribute });
	}
};

m.DiplomacyManager.prototype.checkEvents = function (gameState, events)
{
	// Increase slowly the cooperative personality trait either when we receive tribute from our allies
	// or if our allies attack enemies inside our territory
	for (let evt of events.TributeExchanged)
	{
		if (evt.to === PlayerID && !gameState.isPlayerAlly(evt.from) && this.allyRequests.has(evt.from))
		{
			let request = this.allyRequests.get(evt.from);
			if (request.status === "waitingForTribute")
			{
				request.wanted -= evt.amounts[request.type];

				if (request.wanted <= 0)
				{
					if (this.Config.debug > 1)
						API3.warn("Player " + uneval(evt.from) + " has sent the required tribute amount");

					this.changePlayerDiplomacy(gameState, evt.from, "ally");
					request.status = "accepted";
				}
				else
				{
					// Reset the warning sent to the player that reminds them to speed up the tributes
					request.warnTime = gameState.ai.elapsedTime + 60;
					request.sentWarning = false;
				}
			}
		}

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
		this.Config.personality.cooperative = Math.min(1, this.Config.personality.cooperative + 0.0001 * tributes);
	}

	for (let evt of events.Attacked)
	{
		let target = gameState.getEntityById(evt.target);
		if (!target || !target.position() ||
			gameState.ai.HQ.territoryMap.getOwner(target.position()) !== PlayerID ||
			!gameState.isPlayerEnemy(target.owner()))
			continue;
		let attacker = gameState.getEntityById(evt.attacker);
		if (!attacker || attacker.owner() === PlayerID || !gameState.isPlayerAlly(attacker.owner()))
			continue;
		this.Config.personality.cooperative = Math.min(1, this.Config.personality.cooperative + 0.003);
	}

	if (events.DiplomacyChanged.length || events.PlayerDefeated.length || events.CeasefireEnded.length)
		this.lastManStandingCheck(gameState);

	for (let evt of events.DiplomacyChanged)
	{
		if (evt.otherPlayer !== PlayerID)
			continue;

		if (this.allyRequests.has(evt.player) && !gameState.sharedScript.playersData[evt.player].isAlly[PlayerID])
		{
			// a player that had requested to be allies changed their stance with us
			let request = this.allyRequests.get(evt.player);
			if (request.status === "accepted")
				request.status = "allianceBroken";
			else if (request.status !== "allianceBroken")
				request.status = "declinedRequest";
		}
		else if (gameState.sharedScript.playersData[evt.player].isAlly[PlayerID] && gameState.isPlayerEnemy(evt.player))
			m.chatAnswerRequestAlly(gameState, evt.player, "declineSuggestNeutral");
		else if (gameState.sharedScript.playersData[evt.player].isAlly[PlayerID] && gameState.isPlayerNeutral(evt.player))
			this.handleAllyRequest(gameState, evt.player);
	}
};

/**
 * Check the diplomacy at the start of the game
 */
m.DiplomacyManager.prototype.diplomacyCheck = function(gameState)
{
	if (!gameState.getAlliedVictory() && !gameState.isCeasefireActive())
		this.lastManStandingCheck(gameState);
};

/**
 * If the "Last Man Standing" option is enabled, check if the only remaining players are allies or neutral.
 * If so, turn against the strongest first, but be more likely to first turn against neutral players, if there are any.
 */
m.DiplomacyManager.prototype.lastManStandingCheck = function(gameState)
{
	if (gameState.getAlliedVictory() || gameState.isCeasefireActive())
		return;

	if (gameState.hasEnemies())
	{
		this.waitingToBetray = false;
		return;
	}

	if (!gameState.hasAllies() && !gameState.hasNeutrals())
		return;

	// wait a bit before turning
	if (!this.waitingToBetray)
	{
		this.betrayLapseTime = gameState.ai.elapsedTime + Math.random() * 100 + 10;
		this.waitingToBetray = true;
		return;
	}

	// do not turn against a player yet if we are not strong enough
	if (gameState.getOwnUnits().length < 50)
	{
		this.betrayLapseTime += 60;
		return;
	}

	let playerToTurnAgainst;
	let turnFactor = 0;
	let max = 0;

	// count the amount of entities remaining players have
	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (i === PlayerID || gameState.ai.HQ.attackManager.defeated[i])
			continue;

		turnFactor = gameState.getEntities(i).length;

		if (gameState.isPlayerNeutral(i)) // be more inclined to turn against neutral players
			turnFactor += 150;

		if (turnFactor < max)
			continue;

		max = turnFactor;
		playerToTurnAgainst = i;
	}

	if (playerToTurnAgainst)
		this.changePlayerDiplomacy(gameState, playerToTurnAgainst, "enemy");
	this.betrayLapseTime = -1;
	this.waitingToBetray = false;
};

/**
 * Do not become allies with a player if the game would be over.
 * Overall, be reluctant to become allies with any one player.
 */
m.DiplomacyManager.prototype.handleAllyRequest = function(gameState, player)
{
	let response;
	let requiredTribute;
	let request = this.allyRequests.get(player);

	// For any given ally request be likely to permanently decline
	if (!request && gameState.getPlayerCiv() !== gameState.getPlayerCiv(player) && Math.random() > 0.4 ||
	    gameState.getEnemies().length < gameState.getMutualAllies().length ||
	    gameState.ai.HQ.attackManager.currentEnemyPlayer === player)
	{
		this.allyRequests.set(player, { "status": "declinedRequest" });
		response = "decline";
	}
	else if (request)
	{
		if (request.status === "declinedRequest")
			response = "decline";
		else if (request.status === "allianceBroken") // Previous alliance was broken, so decline
			response = "declineRepeatedOffer";
		else if (request.status === "waitingForTribute")
		{
			response = "waitingForTribute";
			requiredTribute = request;
		}
	}
	else if (gameState.getEntities(player).length < gameState.getOwnEntities().length && Math.random() > 0.6)
	{
		response = "accept";
		this.changePlayerDiplomacy(gameState, player, "ally");
		this.allyRequests.set(player, { "status": "accepted" });
	}
	else
	{
		response = "acceptWithTribute";
		requiredTribute = gameState.ai.HQ.pickMostNeededResources(gameState)[0];
		requiredTribute.wanted = Math.max(1000, gameState.getOwnUnits().length * 10);
		this.allyRequests.set(player, {
			"status": "waitingForTribute",
			"wanted": requiredTribute.wanted,
			"type": requiredTribute.type,
			"warnTime": gameState.ai.elapsedTime + 60,
			"sentWarning": false
		});
	}
	m.chatAnswerRequestAlly(gameState, player, response, requiredTribute);
};

m.DiplomacyManager.prototype.changePlayerDiplomacy = function(gameState, player, newDiplomaticStance)
{
	Engine.PostCommand(PlayerID, { "type": "diplomacy", "player": player, "to": newDiplomaticStance });
	if (this.Config.debug > 1)
		API3.warn("diplomacy stance with player " + player + " is now " + newDiplomaticStance);
	if (this.Config.chat)
		m.chatNewDiplomacy(gameState, player, newDiplomaticStance);
};

m.DiplomacyManager.prototype.checkRequestedTributes = function(gameState)
{
	for (let [player, data] of this.allyRequests.entries())
		if (data.status === "waitingForTribute" && gameState.ai.elapsedTime > data.warnTime)
		{
			if (data.sentWarning)
			{
				this.allyRequests.delete(player);
				m.chatAnswerRequestAlly(gameState, player, "decline");
			}
			else
			{
				data.sentWarning = true;
				data.warnTime = gameState.ai.elapsedTime + 60;
				m.chatAnswerRequestAlly(gameState, player, "waitingForTribute", {
					"wanted": data.wanted,
					"type": data.type
				});
			}
		}
};

m.DiplomacyManager.prototype.update = function(gameState, events)
{
	this.checkEvents(gameState, events);

	if (!gameState.ai.HQ.saveResources && gameState.ai.elapsedTime > this.nextTributeUpdate)
		this.tributes(gameState);

	if (this.waitingToBetray && gameState.ai.elapsedTime > this.betrayLapseTime)
		this.lastManStandingCheck(gameState);

	this.checkRequestedTributes(gameState);
};

m.DiplomacyManager.prototype.Serialize = function()
{
	return {
		"nextTributeUpdate": this.nextTributeUpdate,
		"nextTributeRequest": this.nextTributeRequest,
		"betrayLapseTime": this.betrayLapseTime,
		"waitingToBetray": this.waitingToBetray,
		"allyRequests": this.allyRequests
	};
};

m.DiplomacyManager.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
