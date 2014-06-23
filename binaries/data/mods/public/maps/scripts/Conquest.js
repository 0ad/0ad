/*
 * Check players the next turn. Avoids problems in Atlas, with promoting entities etc
 */
Trigger.prototype.CheckConquestCriticalEntities = function()
{
	if (this.checkingConquestCriticalEntities)
		return;
	// wait a turn for actually checking the players
	this.DoAfterDelay(0, "CheckConquestCriticalEntitiesNow", null);
	this.checkingConquestCriticalEntities = true;
};

/*
 * Check players immediately. Might cause problems with converting/promoting entities.
 */
Trigger.prototype.CheckConquestCriticalEntitiesNow = function()
{
	this.checkingConquestCriticalEntities = false;
	// for all other game types, defeat that player
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	
	// Ignore gaia
	var numPlayers = cmpPlayerManager.GetNumPlayers();
	var cmpPlayers = [];
	
	var allies = [];
	var onlyAlliesLeft = true;
	// If the player is currently active but needs to be defeated,
	// mark that player as defeated
	// cache the cmpPlayer instances of the other players and search the allies
	for (var i = 1; i < numPlayers; i++)
	{
		// cmpPlayer should always exist for the player ids from 1 to numplayers
		// so no tests on the existance of cmpPlayer are needed
		var playerEntityId = cmpPlayerManager.GetPlayerByID(i);
		cmpPlayers[i] = Engine.QueryInterface(playerEntityId, IID_Player);
		if (cmpPlayers[i].GetState() != "active") 
			continue;
		if (cmpPlayers[i].GetConquestCriticalEntitiesCount() == 0)
			Engine.PostMessage(playerEntityId, MT_PlayerDefeated, { "playerId": i } );
		else
		{
			if (!allies.length || cmpPlayers[allies[0]].IsMutualAlly(i))
				allies.push(i);
			else
				onlyAlliesLeft = false;
		}
	}

	// check if there are winners, or the game needs to continue
	if (!allies.length || !onlyAlliesLeft || !(cmpEndGameManager.alliedVictory || allies.length == 1))
		return; 

	for each (var p in allies)
		cmpPlayers[p].SetState("won");

	// Reveal the map to all players
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.SetLosRevealAll(-1, true);
};

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

var data = {"enabled": true};
cmpTrigger.RegisterTrigger("OnOwnershipChanged", "CheckConquestCriticalEntities", data);
// also check at the start of the game
cmpTrigger.DoAfterDelay(0, "CheckConquestCriticalEntitiesNow", null);
cmpTrigger.checkingConquestCriticalEntities = false;

