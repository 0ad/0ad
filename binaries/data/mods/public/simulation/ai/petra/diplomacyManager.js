/**
 * Manage the diplomacy:
 *     update our cooperative trait
 *     sent tribute to allies
 *     decide which player to turn against in "Last Man Standing" mode
 *     respond to diplomacy requests
 *     send diplomacy requests to other players (rarely)
 */

/**
 * If a player sends us an ally or neutral request, an Object in this.receivedDiplomacyRequests will be created
 * that includes the request status, and the amount and type of the resource tribute (if any)
 * that they must send in order for us to accept their request.
 * In addition, a message will be sent if the player has not sent us a tribute within a minute.
 * If two minutes pass without a tribute, we will decline their request.
 *
 * If we send a diplomacy request to another player, an Object in this.sentDiplomacyRequests will be created,
 * which consists of the requestType (i.e. "ally" or "neutral") and the timeSent. A chat message will be sent
 * to the other player, and AI players will actually be informed of the request by a DiplomacyRequest event
 * sent through AIInterface. It is expected that the other player will change their diplomacy stance to the stance
 * that we suggested within a period of time, or else the request will be deleted from this.sentDiplomacyRequests.
 */
PETRA.DiplomacyManager = function(Config)
{
	this.Config = Config;
	this.nextTributeUpdate = 90;
	this.nextTributeRequest = new Map();
	this.nextTributeRequest.set("all", 240);
	this.betrayLapseTime = -1;
	this.waitingToBetray = false;
	this.betrayWeighting = 150;
	this.receivedDiplomacyRequests = new Map();
	this.sentDiplomacyRequests = new Map();
	this.sentDiplomacyRequestLapseTime = 120 + randFloat(10, 100);
};

/**
 * If there are any players that are allied/neutral with us but we are not allied/neutral with them,
 * treat this situation like an ally/neutral request.
 */
PETRA.DiplomacyManager.prototype.init = function(gameState)
{
	this.lastManStandingCheck(gameState);

	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (i === PlayerID)
			continue;

		if (gameState.isPlayerMutualAlly(i))
			this.receivedDiplomacyRequests.set(i, { "requestType": "ally", "status": "accepted" });
		else if (gameState.sharedScript.playersData[i].isAlly[PlayerID])
			this.handleDiplomacyRequest(gameState, i, "ally");
		else if (gameState.sharedScript.playersData[i].isNeutral[PlayerID] && gameState.isPlayerEnemy(i))
			this.handleDiplomacyRequest(gameState, i, "neutral");
	}
};

/**
 * Check if any allied needs help (tribute) and sent it if we have enough resource
 * or ask for a tribute if we are in need and one ally can help
 */
PETRA.DiplomacyManager.prototype.tributes = function(gameState)
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
						PETRA.chatRequestTribute(gameState, res);
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
			PETRA.chatSentTribute(gameState, i);
		Engine.PostCommand(PlayerID, { "type": "tribute", "player": i, "amounts": tribute });
	}
};

PETRA.DiplomacyManager.prototype.checkEvents = function(gameState, events)
{
	// Increase slowly the cooperative personality trait either when we receive tribute from our allies
	// or if our allies attack enemies inside our territory
	for (let evt of events.TributeExchanged)
	{
		if (evt.to === PlayerID && !gameState.isPlayerAlly(evt.from) && this.receivedDiplomacyRequests.has(evt.from))
		{
			let request = this.receivedDiplomacyRequests.get(evt.from);
			if (request.status === "waitingForTribute")
			{
				request.wanted -= evt.amounts[request.type];

				if (request.wanted <= 0)
				{
					if (this.Config.debug > 1)
						API3.warn("Player " + uneval(evt.from) + " has sent the required tribute amount");

					this.changePlayerDiplomacy(gameState, evt.from, request.requestType);
					request.status = "accepted";
				}
				else if (evt.amounts[request.type] > 0)
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

		if (this.sentDiplomacyRequests.has(evt.player)) // If another player has accepted a diplomacy request we sent
		{
			let sentRequest = this.sentDiplomacyRequests.get(evt.player);
			if (gameState.sharedScript.playersData[evt.player].isAlly[PlayerID] && sentRequest.requestType === "ally" ||
			    gameState.sharedScript.playersData[evt.player].isNeutral[PlayerID] && sentRequest.requestType === "neutral")
				this.changePlayerDiplomacy(gameState, evt.player, sentRequest.requestType);

			// Just remove the request if the other player switched their stance to a different and/or more negative state
			// TODO: Keep this send request and take it into account for later diplomacy changes (maybe be less inclined to offer to this player)
			this.sentDiplomacyRequests.delete(evt.player);
			continue;
		}

		let request = this.receivedDiplomacyRequests.get(evt.player);
		if (request !== undefined &&
		   (!gameState.sharedScript.playersData[evt.player].isAlly[PlayerID] && request.requestType === "ally" ||
		     gameState.sharedScript.playersData[evt.player].isEnemy[PlayerID] && request.requestType === "neutral"))
		{
			// a player that had requested to be allies changed their stance with us
			if (request.status === "accepted")
				request.status = "allianceBroken";
			else if (request.status !== "allianceBroken")
				request.status = "declinedRequest";
		}
		else if (gameState.sharedScript.playersData[evt.player].isAlly[PlayerID] && gameState.isPlayerEnemy(evt.player))
		{
			let response = request !== undefined && (request.status === "declinedRequest" || request.status === "allianceBroken") ?
				"decline" : "declineSuggestNeutral";
			PETRA.chatAnswerRequestDiplomacy(gameState, evt.player, "ally", response);
		}
		else if (gameState.sharedScript.playersData[evt.player].isAlly[PlayerID] && gameState.isPlayerNeutral(evt.player))
			this.handleDiplomacyRequest(gameState, evt.player, "ally");
		else if (gameState.sharedScript.playersData[evt.player].isNeutral[PlayerID] && gameState.isPlayerEnemy(evt.player))
			this.handleDiplomacyRequest(gameState, evt.player, "neutral");
	}

	// These events will only be sent by other AI players
	for (let evt of events.DiplomacyRequest)
	{
		if (evt.player !== PlayerID)
			continue;

		this.handleDiplomacyRequest(gameState, evt.source, evt.to);
		let request = this.receivedDiplomacyRequests.get(evt.source);
		if (this.Config.debug > 0)
			API3.warn("Responding to diplomacy request from AI player " + evt.source + " with " + uneval(request));

		// Our diplomacy will have changed already if the response was "accept"
		if (request.status === "waitingForTribute")
		{
			Engine.PostCommand(PlayerID, {
				"type": "tribute-request",
				"source": PlayerID,
				"player": evt.source,
				"resourceWanted": request.wanted,
				"resourceType": request.type
			});
		}
	}

	// An AI player we sent a diplomacy request to demanded we send them a tribute
	for (let evt of events.TributeRequest)
	{
		if (evt.player !== PlayerID)
			continue;

		let availableResources = gameState.ai.queueManager.getAvailableResources(gameState);
		// TODO: Save this event and wait until we get more resources if we don't have enough
		if (evt.resourceWanted < availableResources[evt.resourceType])
		{
			let responseTribute = {};
			responseTribute[evt.resourceType] = evt.resourceWanted;
			if (this.Config.debug > 0)
				API3.warn("Responding to tribute request from AI player " + evt.source + " with " + uneval(responseTribute));
			Engine.PostCommand(PlayerID, { "type": "tribute", "player": evt.source, "amounts": responseTribute });
			this.nextTributeUpdate = gameState.ai.elapsedTime + 15;
		}
	}
};

/**
 * If the "Last Man Standing" option is enabled, check if the only remaining players are allies or neutral.
 * If so, turn against the strongest first, but be more likely to first turn against neutral players, if there are any.
 */
PETRA.DiplomacyManager.prototype.lastManStandingCheck = function(gameState)
{
	if (gameState.sharedScript.playersData[PlayerID].teamsLocked || gameState.isCeasefireActive() ||
	    gameState.getAlliedVictory() && gameState.hasAllies())
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
		this.betrayLapseTime = gameState.ai.elapsedTime + randFloat(10, 110);
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
			turnFactor += this.betrayWeighting;

		if (gameState.getVictoryConditions().has("wonder"))
		{
			let wonder = gameState.getEnemyStructures(i).filter(API3.Filters.byClass("Wonder"))[0];
			if (wonder)
			{
				let wonderProgess = wonder.foundationProgress();
				if (wonderProgess === undefined)
				{
					playerToTurnAgainst = i;
					break;
				}
				turnFactor += wonderProgess * 2.5 + this.betrayWeighting;
			}
		}

		if (gameState.getVictoryConditions().has("capture_the_relic"))
		{
			let relicsCount = gameState.updatingGlobalCollection("allRelics", API3.Filters.byClass("Relic"))
				.filter(relic => relic.owner() === i).length;
			turnFactor += relicsCount * this.betrayWeighting;
		}

		if (turnFactor < max)
			continue;

		max = turnFactor;
		playerToTurnAgainst = i;
	}

	if (playerToTurnAgainst)
	{
		this.changePlayerDiplomacy(gameState, playerToTurnAgainst, "enemy");
		let request = this.receivedDiplomacyRequests.get(playerToTurnAgainst);
		if (request && request.status !== "allianceBroken")
		{
			if (request.status === "waitingForTribute")
				PETRA.chatAnswerRequestDiplomacy(gameState, player, request.requestType, "decline");
			request.status = request.status === "accepted" ? "allianceBroken" : "declinedRequest";
		}
		// If we had sent this player a diplomacy request, just rescind it
		this.sentDiplomacyRequests.delete(playerToTurnAgainst);
	}
	this.betrayLapseTime = -1;
	this.waitingToBetray = false;
};

/**
 * Do not become allies with a player if the game would be over.
 * Overall, be reluctant to become allies with any one player, but be more likely to accept neutral requests.
 */
PETRA.DiplomacyManager.prototype.handleDiplomacyRequest = function(gameState, player, requestType)
{
	if (gameState.sharedScript.playersData[PlayerID].teamsLocked)
		return;
	let response;
	let requiredTribute;
	let request = this.receivedDiplomacyRequests.get(player);
	let moreEnemiesThanAllies = gameState.getEnemies().length > gameState.getMutualAllies().length;

	// For any given diplomacy request be likely to permanently decline
	if (!request && gameState.getPlayerCiv() !== gameState.getPlayerCiv(player) && randBool(0.6) ||
	    !moreEnemiesThanAllies || gameState.ai.HQ.attackManager.currentEnemyPlayer === player)
	{
		this.receivedDiplomacyRequests.set(player, { "requestType": requestType, "status": "declinedRequest" });
		response = "decline";
	}
	else if (request && request.status !== "accepted" && request.requestType !== "ally")
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
	else if (requestType === "ally" && gameState.getEntities(player).length < gameState.getOwnEntities().length && randBool(0.4) ||
	         requestType === "neutral" && moreEnemiesThanAllies && randBool(0.8))
	{
		response = "accept";
		this.changePlayerDiplomacy(gameState, player, requestType);
		this.receivedDiplomacyRequests.set(player, { "requestType": requestType, "status": "accepted" });
	}
	else
	{
		response = "acceptWithTribute";
		requiredTribute = gameState.ai.HQ.pickMostNeededResources(gameState)[0];
		requiredTribute.wanted = Math.max(1000, gameState.getOwnUnits().length * (requestType === "ally" ? 10 : 5));
		this.receivedDiplomacyRequests.set(player, {
			"status": "waitingForTribute",
			"wanted": requiredTribute.wanted,
			"type": requiredTribute.type,
			"warnTime": gameState.ai.elapsedTime + 60,
			"sentWarning": false,
			"requestType": requestType
		});
	}
	PETRA.chatAnswerRequestDiplomacy(gameState, player, requestType, response, requiredTribute);
};

PETRA.DiplomacyManager.prototype.changePlayerDiplomacy = function(gameState, player, newDiplomaticStance)
{
	if (gameState.isPlayerEnemy(player) && (newDiplomaticStance === "ally" || newDiplomaticStance === "neutral"))
		gameState.ai.HQ.attackManager.cancelAttacksAgainstPlayer(gameState, player);
	Engine.PostCommand(PlayerID, { "type": "diplomacy", "player": player, "to": newDiplomaticStance });
	if (this.Config.debug > 1)
		API3.warn("diplomacy stance with player " + player + " is now " + newDiplomaticStance);
	if (this.Config.chat)
		PETRA.chatNewDiplomacy(gameState, player, newDiplomaticStance);
};

PETRA.DiplomacyManager.prototype.checkRequestedTributes = function(gameState)
{
	for (let [player, data] of this.receivedDiplomacyRequests)
		if (data.status === "waitingForTribute" && gameState.ai.elapsedTime > data.warnTime)
		{
			if (data.sentWarning)
			{
				this.receivedDiplomacyRequests.delete(player);
				PETRA.chatAnswerRequestDiplomacy(gameState, player, data.requestType, "decline");
			}
			else
			{
				data.sentWarning = true;
				data.warnTime = gameState.ai.elapsedTime + 60;
				PETRA.chatAnswerRequestDiplomacy(gameState, player, data.requestType, "waitingForTribute", {
					"wanted": data.wanted,
					"type": data.type
				});
			}
		}
};

/**
 * Try to become allies with a player who has a lot of mutual enemies in common with us.
 * TODO: Possibly let human players demand tributes from AIs who send diplomacy requests.
 */
PETRA.DiplomacyManager.prototype.sendDiplomacyRequest = function(gameState)
{
	let player;
	let max = 0;
	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		let mutualEnemies = 0;
		let request = this.receivedDiplomacyRequests.get(i); // Do not send to players we have already rejected before
		if (i === PlayerID || gameState.isPlayerMutualAlly(i) || gameState.ai.HQ.attackManager.defeated[i] ||
		    gameState.ai.HQ.attackManager.currentEnemyPlayer === i ||
		    this.sentDiplomacyRequests.get(i) !== undefined || request && request.status === "declinedRequest")
			continue;

		for (let j = 1; j < gameState.sharedScript.playersData.length; ++j)
		{
			if (gameState.sharedScript.playersData[i].isEnemy[j] && gameState.isPlayerEnemy(j) &&
			    !gameState.ai.HQ.attackManager.defeated[j])
				++mutualEnemies;

			if (mutualEnemies < max)
				continue;

			max = mutualEnemies;
			player = i;
		}
	}
	if (!player)
		return;

	let requestType = gameState.isPlayerNeutral(player) ? "ally" : "neutral";

	this.sentDiplomacyRequests.set(player, {
		"requestType": requestType,
		"timeSent": gameState.ai.elapsedTime
	});

	if (this.Config.debug > 0)
		API3.warn("Sending diplomacy request to player " + player + " with " + requestType);
	Engine.PostCommand(PlayerID, { "type": "diplomacy-request", "source": PlayerID, "player": player, "to": requestType });
	PETRA.chatNewRequestDiplomacy(gameState, player, requestType, "sendRequest");
};

PETRA.DiplomacyManager.prototype.checkSentDiplomacyRequests = function(gameState)
{
	for (let [player, data] of this.sentDiplomacyRequests)
		if (gameState.ai.elapsedTime > data.timeSent + 60 && !gameState.ai.HQ.saveResources &&
		    gameState.getPopulation() > 70)
		{
			PETRA.chatNewRequestDiplomacy(gameState, player, data.requestType, "requestExpired");
			this.sentDiplomacyRequests.delete(player);
		}
};

PETRA.DiplomacyManager.prototype.update = function(gameState, events)
{
	this.checkEvents(gameState, events);

	if (!gameState.ai.HQ.saveResources && gameState.ai.elapsedTime > this.nextTributeUpdate)
		this.tributes(gameState);

	if (this.waitingToBetray && gameState.ai.elapsedTime > this.betrayLapseTime)
		this.lastManStandingCheck(gameState);

	this.checkRequestedTributes(gameState);

	if (gameState.sharedScript.playersData[PlayerID].teamsLocked || gameState.isCeasefireActive())
		return;

	// Be unlikely to send diplomacy requests to other players
	if (gameState.ai.elapsedTime > this.sentDiplomacyRequestLapseTime)
	{
		this.sentDiplomacyRequestLapseTime = gameState.ai.elapsedTime + 300 + randFloat(10, 100);
		let numEnemies = gameState.getEnemies().length;
		// Don't consider gaia
		if (numEnemies > 2 && gameState.getMutualAllies().length < numEnemies - 1 && randBool(0.1))
			this.sendDiplomacyRequest(gameState);
	}

	this.checkSentDiplomacyRequests(gameState);
};

PETRA.DiplomacyManager.prototype.Serialize = function()
{
	return {
		"nextTributeUpdate": this.nextTributeUpdate,
		"nextTributeRequest": this.nextTributeRequest,
		"betrayLapseTime": this.betrayLapseTime,
		"waitingToBetray": this.waitingToBetray,
		"betrayWeighting": this.betrayWeighting,
		"receivedDiplomacyRequests": this.receivedDiplomacyRequests,
		"sentDiplomacyRequests": this.sentDiplomacyRequests,
		"sentDiplomacyRequestLapseTime": this.sentDiplomacyRequestLapseTime
	};
};

PETRA.DiplomacyManager.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};
