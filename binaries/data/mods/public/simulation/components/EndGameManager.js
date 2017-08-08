/**
 * System component to store the gametype, gametype settings and
 * check for allied victory / last-man-standing.
 */
function EndGameManager() {}

EndGameManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

EndGameManager.prototype.Init = function()
{
	this.gameType = "conquest";

	// Contains settings specific to the victory condition,
	// for example wonder victory duration.
	this.gameTypeSettings = {};

	// Allied victory means allied players can win if victory conditions are met for each of them
	// False for a "last man standing" game
	this.alliedVictory = true;

	// Don't do any checks before the diplomacies were set for each player
	// or when marking a player as won.
	this.skipAlliedVictoryCheck = true;

	this.lastManStandingMessage = undefined;

	this.endlessGame = false;
};

EndGameManager.prototype.GetGameType = function()
{
	return this.gameType;
};

EndGameManager.prototype.GetGameTypeSettings = function()
{
	return this.gameTypeSettings;
};

EndGameManager.prototype.SetGameType = function(newGameType, newSettings = {})
{
	this.gameType = newGameType;
	this.gameTypeSettings = newSettings;
	this.skipAlliedVictoryCheck = false;
	this.endlessGame = newGameType == "endless";

	Engine.BroadcastMessage(MT_GameTypeChanged, {});
};

/**
 * Sets the given player (and the allies if allied victory is enabled) as a winner.
 *
 * @param {number} playerID - The player that should win.
 * @param {function} victoryReason - Function that maps from number to plural string, for example
 *   n => markForPluralTranslation(
 *       "%(lastPlayer)s has won (game mode).",
 *       "%(players)s and %(lastPlayer)s have won (game mode).",
 *       n));
 */
EndGameManager.prototype.MarkPlayerAsWon = function(playerID, victoryString, defeatString)
{
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let numPlayers = cmpPlayerManager.GetNumPlayers();

	this.skipAlliedVictoryCheck = true;

	let winningPlayers = [];
	let defeatedPlayers = [];

	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	// Group win/defeat messages
	for (let won of [false, true])
		for (let i = 1; i < numPlayers; ++i)
		{
			let cmpPlayer = QueryPlayerIDInterface(i);
			let hasWon = playerID == i || this.alliedVictory && cmpPlayer.IsMutualAlly(playerID);

			if (hasWon == won)
			{
				if (won)
				{
					cmpPlayer.SetState("won", undefined);
					winningPlayers.push(i);
				}
				else
				{
					cmpPlayer.SetState("defeated", undefined);
					defeatedPlayers.push(i);
				}
			}
		}

		if (winningPlayers.length)
			cmpGUIInterface.PushNotification({
				"type": "won",
				"players": [winningPlayers[0]],
				"allies" : winningPlayers,
				"message": victoryString(winningPlayers.length)
			});

		if (defeatedPlayers.length)
			cmpGUIInterface.PushNotification({
				"type": "defeat",
				"players": [defeatedPlayers[0]],
				"allies" : defeatedPlayers,
				"message": defeatString(defeatedPlayers.length)
			});

	this.skipAlliedVictoryCheck = false;
};

EndGameManager.prototype.SetAlliedVictory = function(flag)
{
	this.alliedVictory = flag;
};

EndGameManager.prototype.GetAlliedVictory = function()
{
	return this.alliedVictory;
};

EndGameManager.prototype.AlliedVictoryCheck = function()
{
	if (this.skipAlliedVictoryCheck || this.endlessGame)
		return;

	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (!cmpGuiInterface || !cmpPlayerManager)
		return;

	cmpGuiInterface.DeleteTimeNotification(this.lastManStandingMessage);

	// Proceed if only allies are remaining
	let allies = [];
	for (let playerID = 1; playerID < cmpPlayerManager.GetNumPlayers(); ++playerID)
	{
		let cmpPlayer = QueryPlayerIDInterface(playerID);
		if (cmpPlayer.GetState() != "active")
			continue;

		if (allies.length && !cmpPlayer.IsMutualAlly(allies[0]))
			return;

		allies.push(playerID);
	}

	if (this.alliedVictory || allies.length == 1)
	{
		for (let playerID of allies)
		{
			let cmpPlayer = QueryPlayerIDInterface(playerID);
			if (cmpPlayer)
				cmpPlayer.SetState("won", undefined);
		}

		cmpGuiInterface.PushNotification({
			"type": "won",
			"players": [allies[0]],
			"allies" : allies,
			"message": markForPluralTranslation(
				"%(lastPlayer)s has won (last player alive).",
				"%(players)s and %(lastPlayer)s have won (last players alive).",
				allies.length)
		});
	}
	else
		this.lastManStandingMessage = cmpGuiInterface.AddTimeNotification({
			"message": markForTranslation("Last remaining player wins."),
			"translateMessage": true,
		}, 12 * 60 * 60 * 1000); // 12 hours
};

EndGameManager.prototype.OnInitGame = function(msg)
{
	this.AlliedVictoryCheck();
};

EndGameManager.prototype.OnGlobalDiplomacyChanged = function(msg)
{
	this.AlliedVictoryCheck();
};

EndGameManager.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.AlliedVictoryCheck();
};

Engine.RegisterSystemComponentType(IID_EndGameManager, "EndGameManager", EndGameManager);
