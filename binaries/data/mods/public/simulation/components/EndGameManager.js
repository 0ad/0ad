// Repetition interval (msecs) for checking end game conditions
var g_ProgressInterval = 1000;

/**
 * System component which regularly checks victory/defeat conditions
 * and if they are satisfied then it marks the player as victorious/defeated.
 */
function EndGameManager() {}

EndGameManager.prototype.Schema =
	"<a:component type='system'/><empty/>";
	
EndGameManager.prototype.Init = function()
{
	// Game type, initialised from the map settings.
	// One of: "conquest" (default) and "endless"
	this.gameType = "conquest";
	
	// Allied victory means allied players can win if victory conditions are met for each of them
	// Would be false for a "last man standing" game (when diplomacy is fully implemented)
	this.alliedVictory = true;
};

EndGameManager.prototype.SetGameType = function(newGameType)
{
	this.gameType = newGameType;
};

EndGameManager.prototype.SetAlliedVictory = function(flag)
{
	this.alliedVictory = flag;
};

EndGameManager.prototype.PlayerLostAllConquestCriticalEntities = function(playerID)
{
	if (this.gameType == "endless")
		return; 

	// for all other game types, defeat that player
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	
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
		if (i == playerID)
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
	if (!allies.length || !onlyAlliesLeft || !(this.alliedVictory || allies.length == 1))
		return; 

	for each (var p in allies)
		cmpPlayers[p].SetState("won");

	// Reveal the map to all players
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.SetLosRevealAll(-1, true);
};

Engine.RegisterComponentType(IID_EndGameManager, "EndGameManager", EndGameManager);
