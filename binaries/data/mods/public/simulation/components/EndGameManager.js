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
};

EndGameManager.prototype.SetGameType = function(newGameType)
{
	this.gameType = newGameType;
};

/**
 * Begin checking the end-game conditions.
 * Must be called once, after calling SetGameType.
 */
EndGameManager.prototype.Start = function()
{
	if (this.gameType != "endless")
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_EndGameManager, "ProgressTimeout", g_ProgressInterval, {});
	}
};

EndGameManager.prototype.OnDestroy = function()
{
	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
	}
};

EndGameManager.prototype.ProgressTimeout = function(data)
{
	this.UpdatePlayerStates();
	
	// Repeat the timer
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetTimeout(this.entity, IID_EndGameManager, "ProgressTimeout", g_ProgressInterval, data);
};

EndGameManager.prototype.UpdatePlayerStates = function()
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	switch (this.gameType)
	{
	case "conquest":

		// If a player is currently active but has no suitable units left,
		// mark that player as defeated
		// (Start from player 1 since we ignore Gaia)
		for (var i = 1; i < cmpPlayerManager.GetNumPlayers(); i++)
		{
			var playerEntityId = cmpPlayerManager.GetPlayerByID(i);
			var cmpPlayer = Engine.QueryInterface(playerEntityId, IID_Player);
			if (cmpPlayer.GetState() == "active")
			{
				if (cmpPlayer.GetConquestCriticalEntitiesCount() == 0)
				{
					Engine.PostMessage(playerEntityId, MT_PlayerDefeated, null);
				}
			}
		}

		// If there's only player remaining active, mark them as the winner
		// TODO: update this code for allies

		var alivePlayersCount = 0;
		var lastAlivePlayerId;
		// (Start from 1 to ignore Gaia)
		for (var i = 1; i < cmpPlayerManager.GetNumPlayers(); i++)
		{
			var playerEntityId = cmpPlayerManager.GetPlayerByID(i);
			var cmpPlayer = Engine.QueryInterface(playerEntityId, IID_Player);
			if (cmpPlayer.GetState() == "active")
			{
				alivePlayersCount++;
				lastAlivePlayerId = i;
			}
		}
		if (alivePlayersCount == 1)
		{
			var playerEntityId = cmpPlayerManager.GetPlayerByID(lastAlivePlayerId);
			var cmpPlayer = Engine.QueryInterface(playerEntityId, IID_Player);
			cmpPlayer.SetState("won");
		}

		break;

	default:
		error("Invalid game type "+this.gameType);
		break;
	}
};

Engine.RegisterComponentType(IID_EndGameManager, "EndGameManager", EndGameManager);
