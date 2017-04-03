Trigger.prototype.InitCaptureTheRelic = function()
{
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let catafalqueTemplates = shuffleArray(cmpTemplateManager.FindAllTemplates(false).filter(
			name => name.startsWith("other/catafalque/")));

	// Attempt to spawn relics using gaia entities in neutral territory
	// If there are none, try to spawn using gaia entities in non-neutral territory
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let cmpWaterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
	let cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);

	let potentialGaiaSpawnPoints = [];

	let potentialSpawnPoints = cmpRangeManager.GetEntitiesByPlayer(0).filter(entity => {
		let cmpPosition = Engine.QueryInterface(entity, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			return false;

		let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
		if (!cmpIdentity)
			return false;

		let templateName = cmpTemplateManager.GetCurrentTemplateName(entity);
		if (!templateName)
			return false;

		let template = cmpTemplateManager.GetTemplate(templateName);
		if (!template || template.UnitMotionFlying)
			return false;

		let pos = cmpPosition.GetPosition();
		if (pos.y <= cmpWaterManager.GetWaterLevel(pos.x, pos.z))
			return false;

		if (cmpTerritoryManager.GetOwner(pos.x, pos.z) == 0)
			potentialGaiaSpawnPoints.push(entity);

		return true;
	});

	if (potentialGaiaSpawnPoints.length)
		potentialSpawnPoints = potentialGaiaSpawnPoints;

	let numSpawnedRelics = Math.ceil(TriggerHelper.GetNumberOfPlayers() / 2);
	this.playerRelicsCount = new Array(TriggerHelper.GetNumberOfPlayers()).fill(0, 1);
	this.playerRelicsCount[0] = numSpawnedRelics;

	for (let i = 0; i < numSpawnedRelics; ++i)
	{
		this.relics[i] = TriggerHelper.SpawnUnits(pickRandom(potentialSpawnPoints), catafalqueTemplates[i], 1, 0)[0];

		let cmpDamageReceiver = Engine.QueryInterface(this.relics[i], IID_DamageReceiver);
		cmpDamageReceiver.SetInvulnerability(true);

		let cmpPositionRelic = Engine.QueryInterface(this.relics[i], IID_Position);
		cmpPositionRelic.SetYRotation(randFloat(0, 2 * Math.PI));
	}
};

Trigger.prototype.CheckCaptureTheRelicVictory = function(data)
{
	let cmpIdentity = Engine.QueryInterface(data.entity, IID_Identity);
	if (!cmpIdentity || !cmpIdentity.HasClass("Relic") || data.from == -1)
		return;

	if (data.to == -1)
	{
		error("Relic entity " + data.entity + " has been destroyed");
		return;
	}

	--this.playerRelicsCount[data.from];
	++this.playerRelicsCount[data.to];

	this.CheckCaptureTheRelicCountdown();
};

/**
 * Check if an individual player or team has acquired all relics.
 * Also check if the countdown needs to be stopped if a player/team no longer has all relics.
 */
Trigger.prototype.CheckCaptureTheRelicCountdown = function()
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
			this.StartCaptureTheRelicCountdown(playerAndAllies);
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
	let captureTheRelicDuration = cmpEndGameManager.GetGameTypeSettings().victoryDuration || 0;

	let isTeam = playerAndAllies.length > 1;
	this.ownRelicsVictoryMessage = cmpGuiInterface.AddTimeNotification({
		"message": isTeam ?
			markForTranslation("%(player)s's team has captured all relics and will have won in %(time)s") :
			markForTranslation("%(player)s has captured all relics and will have won in %(time)s"),
		"players": others,
		"parameters": {
			"player": cmpPlayer.GetName()
		},
		"translateMessage": true,
		"translateParameters": []
	}, captureTheRelicDuration);

	this.othersRelicsVictoryMessage = cmpGuiInterface.AddTimeNotification({
		"message": isTeam ?
			markForTranslation("Your team has captured all relics and will have won in %(time)s") :
			markForTranslation("You have captured all relics and will have won in %(time)s"),
		"players": playerAndAllies,
		"translateMessage": true
	}, captureTheRelicDuration);

	this.relicsVictoryTimer = cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_EndGameManager,
		"MarkPlayerAsWon", captureTheRelicDuration, playerAndAllies[0]);
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.relics = [];
	cmpTrigger.playerRelicsCount = [];
	cmpTrigger.relicsVictoryTimer = undefined;
	cmpTrigger.ownRelicsVictoryMessage = undefined;
	cmpTrigger.othersRelicsVictoryMessage = undefined;

	cmpTrigger.DoAfterDelay(0, "InitCaptureTheRelic", {});
	cmpTrigger.RegisterTrigger("OnDiplomacyChanged", "CheckCaptureTheRelicCountdown", { "enabled": true });
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "CheckCaptureTheRelicVictory", { "enabled": true });
	cmpTrigger.RegisterTrigger("OnPlayerWon", "DeleteCaptureTheRelicVictoryMessages", { "enabled": true });
}
