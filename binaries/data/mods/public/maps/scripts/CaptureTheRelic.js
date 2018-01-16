Trigger.prototype.InitCaptureTheRelic = function()
{
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let catafalqueTemplates = shuffleArray(cmpTemplateManager.FindAllTemplates(false).filter(
		name => GetIdentityClasses(cmpTemplateManager.GetTemplate(name).Identity || {}).indexOf("Relic") != -1));

	let potentialSpawnPoints = TriggerHelper.GetLandSpawnPoints();
	if (!potentialSpawnPoints.length)
	{
		error("No gaia entities found on this map that could be used as spawn points!");
		return;
	}

	let cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	let numSpawnedRelics = cmpEndGameManager.GetGameTypeSettings().relicCount;
	this.playerRelicsCount = new Array(TriggerHelper.GetNumberOfPlayers()).fill(0, 1);
	this.playerRelicsCount[0] = numSpawnedRelics;

	for (let i = 0; i < numSpawnedRelics; ++i)
	{
		this.relics[i] = TriggerHelper.SpawnUnits(pickRandom(potentialSpawnPoints), catafalqueTemplates[i], 1, 0)[0];

		let cmpDamageReceiver = Engine.QueryInterface(this.relics[i], IID_DamageReceiver);
		cmpDamageReceiver.SetInvulnerability(true);

		let cmpPositionRelic = Engine.QueryInterface(this.relics[i], IID_Position);
		cmpPositionRelic.SetYRotation(randomAngle());
	}
};

Trigger.prototype.CheckCaptureTheRelicVictory = function(data)
{
	let cmpIdentity = Engine.QueryInterface(data.entity, IID_Identity);
	if (!cmpIdentity || !cmpIdentity.HasClass("Relic") || data.from == -1)
		return;

	--this.playerRelicsCount[data.from];

	if (data.to == -1)
	{
		warn("Relic entity " + data.entity + " has been destroyed");
		this.relics.splice(this.relics.indexOf(data.entity), 1);
	}
	else
		++this.playerRelicsCount[data.to];

	this.CheckCaptureTheRelicCountdown();
};

/**
 * Check if an individual player or team has acquired all relics.
 * Also check if the countdown needs to be stopped if a player/team no longer has all relics.
 * Reset the countdown if any of the original allies tries to change their diplomacy with one of these allies.
 */
Trigger.prototype.CheckCaptureTheRelicCountdown = function(data)
{
	let cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);

	for (let playerID = 1; playerID < TriggerHelper.GetNumberOfPlayers(); ++playerID)
	{
		let playerAndAllies = cmpEndGameManager.GetAlliedVictory() ?
			QueryPlayerIDInterface(playerID).GetMutualAllies() : [playerID];

		let teamRelicsOwned = 0;

		for (let ally of playerAndAllies)
			teamRelicsOwned += this.playerRelicsCount[ally];

		if (teamRelicsOwned == this.relics.length)
		{
			if (!data ||
			    !this.relicsVictoryCountdownPlayers.length ||
			    this.relicsVictoryCountdownPlayers.indexOf(data.player) != -1 &&
			    this.relicsVictoryCountdownPlayers.indexOf(data.otherPlayer) != -1)
			{
				this.relicsVictoryCountdownPlayers = playerAndAllies;
				this.StartCaptureTheRelicCountdown(playerAndAllies);
			}
			return;
		}
	}

	this.DeleteCaptureTheRelicVictoryMessages();
};

Trigger.prototype.DeleteCaptureTheRelicVictoryMessages = function()
{
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.relicsVictoryTimer);

	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGuiInterface.DeleteTimeNotification(this.ownRelicsVictoryMessage);
	cmpGuiInterface.DeleteTimeNotification(this.othersRelicsVictoryMessage);
	this.relicsVictoryCountdownPlayers = [];
};

Trigger.prototype.StartCaptureTheRelicCountdown = function(playerAndAllies)
{
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	if (this.relicsVictoryTimer)
	{
		cmpTimer.CancelTimer(this.relicsVictoryTimer);
		cmpGuiInterface.DeleteTimeNotification(this.ownRelicsVictoryMessage);
		cmpGuiInterface.DeleteTimeNotification(this.othersRelicsVictoryMessage);
	}

	if (!this.relics.length)
		return;

	let others = [-1];
	for (let playerID = 1; playerID < TriggerHelper.GetNumberOfPlayers(); ++playerID)
	{
		let cmpPlayer = QueryPlayerIDInterface(playerID);
		if (cmpPlayer.GetState() == "won")
			return;

		if (playerAndAllies.indexOf(playerID) == -1)
			others.push(playerID);
	}

	let cmpPlayer = QueryOwnerInterface(this.relics[0], IID_Player);
	let cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	let captureTheRelicDuration = cmpEndGameManager.GetGameTypeSettings().relicDuration;

	let isTeam = playerAndAllies.length > 1;
	this.ownRelicsVictoryMessage = cmpGuiInterface.AddTimeNotification({
		"message": isTeam ?
			markForTranslation("%(_player_)s's team has captured all relics and will win in %(time)s.") :
			markForTranslation("%(_player_)s has captured all relics and will win in %(time)s."),
		"players": others,
		"parameters": {
			"_player_": cmpPlayer.GetPlayerID()
		},
		"translateMessage": true,
		"translateParameters": []
	}, captureTheRelicDuration);

	this.othersRelicsVictoryMessage = cmpGuiInterface.AddTimeNotification({
		"message": isTeam ?
			markForTranslation("Your team has captured all relics and will win in %(time)s.") :
			markForTranslation("You have captured all relics and will win in %(time)s."),
		"players": playerAndAllies,
		"translateMessage": true
	}, captureTheRelicDuration);

	this.relicsVictoryTimer = cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_Trigger,
		"CaptureTheRelicVictorySetWinner", captureTheRelicDuration, playerAndAllies[0]);
};

Trigger.prototype.CaptureTheRelicVictorySetWinner = function(playerID)
{
	let cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	cmpEndGameManager.MarkPlayerAsWon(
		playerID,
		n => markForPluralTranslation(
			"%(lastPlayer)s has won (Capture the Relic).",
			"%(players)s and %(lastPlayer)s have won (Capture the Relic).",
			n),
		n => markForPluralTranslation(
			"%(lastPlayer)s has been defeated (Capture the Relic).",
			"%(players)s and %(lastPlayer)s have been defeated (Capture the Relic).",
			n));
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.relics = [];
	cmpTrigger.playerRelicsCount = [];
	cmpTrigger.relicsVictoryTimer = undefined;
	cmpTrigger.ownRelicsVictoryMessage = undefined;
	cmpTrigger.othersRelicsVictoryMessage = undefined;
	cmpTrigger.relicsVictoryCountdownPlayers = [];

	cmpTrigger.DoAfterDelay(0, "InitCaptureTheRelic", {});
	cmpTrigger.RegisterTrigger("OnDiplomacyChanged", "CheckCaptureTheRelicCountdown", { "enabled": true });
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "CheckCaptureTheRelicVictory", { "enabled": true });
	cmpTrigger.RegisterTrigger("OnPlayerWon", "DeleteCaptureTheRelicVictoryMessages", { "enabled": true });
}
