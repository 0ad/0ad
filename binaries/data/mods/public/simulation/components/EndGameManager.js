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
};

EndGameManager.prototype.GetGameType = function()
{
	return this.gameType;
};

EndGameManager.prototype.GetGameTypeSettings = function()
{
	return this.gameTypeSettings;
};

EndGameManager.prototype.CheckGameType = function(type)
{
	return this.gameType == type;
};

EndGameManager.prototype.SetGameType = function(newGameType, newSettings = {})
{
	this.gameType = newGameType;
	this.gameTypeSettings = newSettings;
	this.skipAlliedVictoryCheck = false;

	Engine.BroadcastMessage(MT_GameTypeChanged, {});
};

EndGameManager.prototype.MarkPlayerAsWon = function(playerID)
{
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let numPlayers = cmpPlayerManager.GetNumPlayers();

	this.skipAlliedVictoryCheck = true;

	// Group win/defeat messages
	for (let won of [false, true])
		for (let i = 1; i < numPlayers; ++i)
		{
			let cmpPlayer = QueryPlayerIDInterface(i);
			let hasWon = playerID == i || this.alliedVictory && cmpPlayer.IsMutualAlly(playerID);

			if (hasWon == won)
				cmpPlayer.SetState(won ? "won" : "defeated");
		}

	this.skipAlliedVictoryCheck = false;
};

EndGameManager.prototype.SetAlliedVictory = function(flag)
{
	this.alliedVictory = flag;
};

EndGameManager.prototype.AlliedVictoryCheck = function()
{
	if (this.skipAlliedVictoryCheck)
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
		for (let playerID of allies)
		{
			let cmpPlayer = QueryPlayerIDInterface(playerID);
			if (cmpPlayer)
				cmpPlayer.SetState("won");
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
