/**
 * System component which regularly checks victory/defeat conditions
 * and if they are satisfied then it marks the player as victorious/defeated.
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

	Engine.BroadcastMessage(MT_GameTypeChanged, {});
};

EndGameManager.prototype.MarkPlayerAsWon = function(playerID)
{
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let numPlayers = cmpPlayerManager.GetNumPlayers();

	for (let i = 1; i < numPlayers; ++i)
	{
		let playerEntityId = cmpPlayerManager.GetPlayerByID(i);
		let cmpPlayer = Engine.QueryInterface(playerEntityId, IID_Player);

		if (cmpPlayer.GetState() != "active")
			continue;

		if (playerID == cmpPlayer.GetPlayerID() || this.alliedVictory && cmpPlayer.IsMutualAlly(playerID))
			cmpPlayer.SetState("won");
		else
			Engine.PostMessage(playerEntityId, MT_PlayerDefeated, {
				"playerId": i,
				"skipAlliedVictoryCheck": true
			});
	}

	// Reveal the map to all players
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.SetLosRevealAll(-1, true);
};

EndGameManager.prototype.SetAlliedVictory = function(flag)
{
	this.alliedVictory = flag;
};

EndGameManager.prototype.AlliedVictoryCheck = function()
{
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
				cmpPlayer.SetState("won");
		}

		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetLosRevealAll(-1, true);
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
	if (!msg.skipAlliedVictoryCheck)
		this.AlliedVictoryCheck();
};

EndGameManager.prototype.OnGlobalPlayerDefeated = function(msg)
{
	if (!msg.skipAlliedVictoryCheck)
		this.AlliedVictoryCheck();
};

Engine.RegisterSystemComponentType(IID_EndGameManager, "EndGameManager", EndGameManager);
