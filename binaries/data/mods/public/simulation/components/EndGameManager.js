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
		var diplomacy = new Array(numPlayers);
		
		// If a player is currently active but has no suitable units left,
		// mark that player as defeated (else get diplomacy for victory check)
		for (var i = 0; i < numPlayers; i++)
		{
			var playerEntityId = cmpPlayerManager.GetPlayerByID(i+1);
			var cmpPlayer = Engine.QueryInterface(playerEntityId, IID_Player);
			if (cmpPlayer.GetState() == "active")
			{
				if (cmpPlayer.GetConquestCriticalEntitiesCount() == 0)
				{
					Engine.PostMessage(playerEntityId, MT_PlayerDefeated, null);
				}
				else
				{	// Get active diplomacy array
					diplomacy[i] = cmpPlayer.GetDiplomacy();
				}
			}
		}

		// Check diplomacy to see if all active players are allied - if so, they all won
		var onlyAlliesLeft = true;
		var allyIDs = [];
		
		for (var i = 0; i < numPlayers && onlyAlliesLeft; i++)
		{
			if (diplomacy[i])
			{	//Active player
				for (var j = 0; j < numPlayers && j != i && onlyAlliesLeft; j++)
				{
					if (diplomacy[j] && (diplomacy[i][j] <= 0 || diplomacy[j][i] <= 0))
					{	// Only need to find an active non-allied player
						onlyAlliesLeft = false;
					}
				}
				
				if (onlyAlliesLeft)
					allyIDs.push(i+1);
			}
		}

		// If only allies left and allied victory set (or only one player left)
		if (onlyAlliesLeft && (this.alliedVictory || allyIDs.length == 1))
		{
			for (var p in allyIDs)
			{
				var playerEntityId = cmpPlayerManager.GetPlayerByID(allyIDs[p]);
				var cmpPlayer = Engine.QueryInterface(playerEntityId, IID_Player);
				cmpPlayer.SetState("won");
			}
		}

		break;

	default:
		error("Invalid game type "+this.gameType);
		break;
	}
};

Engine.RegisterComponentType(IID_EndGameManager, "EndGameManager", EndGameManager);
