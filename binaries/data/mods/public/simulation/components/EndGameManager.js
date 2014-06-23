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
	Engine.BroadcastMessage(MT_GameTypeChanged, {});
};

EndGameManager.prototype.CheckGameType = function(type)
{
	return this.gameType == type;
};

EndGameManager.prototype.MarkPlayerAsWon = function(playerID)
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var numPlayers = cmpPlayerManager.GetNumPlayers();
	for (var i = 1; i < numPlayers; i++)
	{
		var playerEntityId = cmpPlayerManager.GetPlayerByID(i);
		var cmpPlayer = Engine.QueryInterface(playerEntityId, IID_Player);
		if (cmpPlayer.GetState() != "active")
			continue;
		if (playerID == cmpPlayer.GetPlayerID() || this.alliedVictory && cmpPlayer.IsMutualAlly(playerID))
			cmpPlayer.SetState("won")
		else
			Engine.PostMessage(playerEntityId, MT_PlayerDefeated, { "playerId": i } );
	}

	// Reveal the map to all players
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.SetLosRevealAll(-1, true);
};

EndGameManager.prototype.SetAlliedVictory = function(flag)
{
	this.alliedVictory = flag;
};

Engine.RegisterSystemComponentType(IID_EndGameManager, "EndGameManager", EndGameManager);
