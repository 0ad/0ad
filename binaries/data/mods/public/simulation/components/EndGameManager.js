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

		// Ignore gaia
		var numPlayers = cmpPlayerManager.GetNumPlayers() - 1;
		var cmpPlayers = new Array(numPlayers);
		
		// If a player is currently active but has no suitable units left,
		// mark that player as defeated
		for (var i = 0; i < numPlayers; i++)
		{
			var playerEntityId = cmpPlayerManager.GetPlayerByID(i+1);
			cmpPlayers[i] = Engine.QueryInterface(playerEntityId, IID_Player);
			if (cmpPlayers[i].GetState() == "active")
			{
				if (cmpPlayers[i].GetConquestCriticalEntitiesCount() == 0)
				{	// Defeated
					Engine.PostMessage(playerEntityId, MT_PlayerDefeated, null);
				}
			}
		}

		var onlyAlliesLeft = true;
		var allies = [];
		for (var i = 0; i < numPlayers && onlyAlliesLeft; i++)
		{
			if (cmpPlayers[i].GetState() == "active")
			{	//Active player
				for (var j = 0; j < numPlayers && onlyAlliesLeft; j++)
				{
					if (cmpPlayers[j].GetState() == "active" && (cmpPlayers[i].IsEnemy(j+1) || cmpPlayers[j].IsEnemy(i+1)))
					{	// Only need to find an active non-allied player
						onlyAlliesLeft = false;
					}
				}
				
				if (onlyAlliesLeft)
					allies.push(i);
			}
		}

		// If only allies left and allied victory set (or only one player left)
		if (onlyAlliesLeft && (this.alliedVictory || allies.length == 1))
		{
			for each (var p in allies)
			{
				cmpPlayers[p].SetState("won");
			}

			// Reveal the map to all players
			var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
			cmpRangeManager.SetLosRevealAll(-1, true);
		}

		break;

	default:
		error("Invalid game type "+this.gameType);
		break;
	}
};

Engine.RegisterComponentType(IID_EndGameManager, "EndGameManager", EndGameManager);
