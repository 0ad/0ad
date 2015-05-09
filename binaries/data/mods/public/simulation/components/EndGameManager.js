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

EndGameManager.prototype.GetGameType = function()
{
	return this.gameType;
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
			Engine.PostMessage(playerEntityId, MT_PlayerDefeated, { "playerId": i, "skip": true } );
	}

	// Reveal the map to all players
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.SetLosRevealAll(-1, true);
};

EndGameManager.prototype.SetAlliedVictory = function(flag)
{
	this.alliedVictory = flag;
};

EndGameManager.prototype.OnGlobalPlayerDefeated = function(msg)
{
	if (msg.skip)
		return;

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayers = [];

	var allies = [];
	var onlyAlliesLeft = true;

	var numPlayers = cmpPlayerManager.GetNumPlayers(); 

	for (var i = 1; i < numPlayers; ++i)
	{
		cmpPlayers[i] = QueryPlayerIDInterface(i);
		if (cmpPlayers[i].GetState() != "active" || i == msg.playerId) 
			return;

		if (!allies.length || cmpPlayers[allies[0]].IsMutualAlly(i))
			allies.push(i);
		else
			onlyAlliesLeft = false;
	}

	// check if there are winners, or the game needs to continue
	if (!allies.length || !onlyAlliesLeft || !(this.alliedVictory || allies.length == 1))
		return; 

	for each (var p in allies)
		cmpPlayers[p].SetState("won");

	// Reveal the map to all players
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.SetLosRevealAll(-1, true);
}

Engine.RegisterSystemComponentType(IID_EndGameManager, "EndGameManager", EndGameManager);
