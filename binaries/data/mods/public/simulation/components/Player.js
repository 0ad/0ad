function Player() {}

Player.prototype.Schema =
	"<element name='BarterMultiplier' a:help='Multipliers for barter prices.'>" +
		"<interleave>" +
			"<element name='Buy' a:help='Multipliers for the buy prices.'>" +
				Resources.BuildSchema("positiveDecimal") +
			"</element>" +
			"<element name='Sell' a:help='Multipliers for the sell prices.'>" +
				Resources.BuildSchema("positiveDecimal") +
			"</element>" +
		"</interleave>" +
	"</element>" +
	"<element name='SharedLosTech' a:help='Allies will share los when this technology is researched. Leave empty to never share LOS.'>" +
		"<text/>" +
	"</element>" +
	"<element name='SharedDropsitesTech' a:help='Allies will share dropsites when this technology is researched. Leave empty to never share dropsites.'>" +
		"<text/>" +
	"</element>" +
	"<element name='SpyCostMultiplier'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

/**
 * Which units will be shown with special icons at the top.
 */
var panelEntityClasses = "Hero Relic";

Player.prototype.Init = function()
{
	this.playerID = undefined;
	this.name = undefined;	// define defaults elsewhere (supporting other languages)
	this.civ = undefined;
	this.color = undefined;
	this.popUsed = 0; // population of units owned or trained by this player
	this.popBonuses = 0; // sum of population bonuses of player's entities
	this.maxPop = 300; // maximum population
	this.trainingBlocked = false; // indicates whether any training queue is currently blocked
	this.resourceCount = {};
	this.tradingGoods = []; // goods for next trade-route and its proba in % (the sum of probas must be 100)
	this.team = -1;	// team number of the player, players on the same team will always have ally diplomatic status - also this is useful for team emblems, scoring, etc.
	this.teamsLocked = false;
	this.state = "active"; // game state - one of "active", "defeated", "won"
	this.diplomacy = [];	// array of diplomatic stances for this player with respect to other players (including gaia and self)
	this.sharedDropsites = false;
	this.formations = [];
	this.startCam = undefined;
	this.controlAllUnits = false;
	this.isAI = false;
	this.gatherRateMultiplier = 1;
	this.tradeRateMultiplier = 1;
	this.cheatsEnabled = false;
	this.cheatTimeMultiplier = 1;
	this.panelEntities = [];
	this.resourceNames = {};
	this.disabledTemplates = {};
	this.disabledTechnologies = {};
	this.startingTechnologies = [];
	this.spyCostMultiplier = +this.template.SpyCostMultiplier;
	this.barterMultiplier = {
		"buy": clone(this.template.BarterMultiplier.Buy),
		"sell": clone(this.template.BarterMultiplier.Sell)
	};

	// Initial resources and trading goods probability in steps of 5
	let resCodes = Resources.GetCodes();
	let quotient = Math.floor(20 / resCodes.length);
	let remainder = 20 % resCodes.length;
	for (let i in resCodes)
	{
		let res = resCodes[i];
		this.resourceCount[res] = 300;
		this.resourceNames[res] = Resources.GetResource(res).name;
		this.tradingGoods.push({
			"goods":  res,
			"proba": 5 * (quotient + (+i < remainder ? 1 : 0))
		});
	}
};

Player.prototype.SetPlayerID = function(id)
{
	this.playerID = id;
};

Player.prototype.GetPlayerID = function()
{
	return this.playerID;
};

Player.prototype.SetName = function(name)
{
	this.name = name;
};

Player.prototype.GetName = function()
{
	return this.name;
};

Player.prototype.SetCiv = function(civcode)
{
	var oldCiv = this.civ;
	this.civ = civcode;
	// Normally, the civ is only set once
	// But in Atlas, the map designers can change civs at any time
	var playerID = this.GetPlayerID();
	if (oldCiv && playerID && oldCiv != civcode)
		Engine.BroadcastMessage(MT_CivChanged, {
			"player": playerID,
			"from": oldCiv,
			"to": civcode
		});
};

Player.prototype.GetCiv = function()
{
	return this.civ;
};

Player.prototype.SetColor = function(r, g, b)
{
	var colorInitialized = !!this.color;

	this.color = { "r": r/255.0, "g": g/255.0, "b": b/255.0, "a": 1.0 };

	// Used in Atlas
	if (colorInitialized)
		Engine.BroadcastMessage(MT_PlayerColorChanged, {
			"player": this.playerID
		});
};

Player.prototype.GetColor = function()
{
	return this.color;
};

// Try reserving num population slots. Returns 0 on success or number of missing slots otherwise.
Player.prototype.TryReservePopulationSlots = function(num)
{
	if (num != 0 && num > (this.GetPopulationLimit() - this.GetPopulationCount()))
		return num - (this.GetPopulationLimit() - this.GetPopulationCount());

	this.popUsed += num;
	return 0;
};

Player.prototype.UnReservePopulationSlots = function(num)
{
	this.popUsed -= num;
};

Player.prototype.GetPopulationCount = function()
{
	return this.popUsed;
};

Player.prototype.AddPopulation = function(num)
{
	this.popUsed += num;
};

Player.prototype.SetPopulationBonuses = function(num)
{
	this.popBonuses = num;
};

Player.prototype.AddPopulationBonuses = function(num)
{
	this.popBonuses += num;
};

Player.prototype.GetPopulationLimit = function()
{
	return Math.min(this.GetMaxPopulation(), this.popBonuses);
};

Player.prototype.SetMaxPopulation = function(max)
{
	this.maxPop = max;
};

Player.prototype.GetMaxPopulation = function()
{
	return Math.round(ApplyValueModificationsToEntity("Player/MaxPopulation", this.maxPop, this.entity));
};

Player.prototype.GetBarterMultiplier = function()
{
	return this.barterMultiplier;
};

Player.prototype.SetGatherRateMultiplier = function(value)
{
	this.gatherRateMultiplier = value;
};

Player.prototype.GetGatherRateMultiplier = function()
{
	return this.gatherRateMultiplier;
};

Player.prototype.SetTradeRateMultiplier = function(value)
{
	this.tradeRateMultiplier = value;
};

Player.prototype.GetTradeRateMultiplier = function()
{
	return this.tradeRateMultiplier;
};

Player.prototype.GetSpyCostMultiplier = function()
{
	return this.spyCostMultiplier;
};

Player.prototype.GetPanelEntities = function()
{
	return this.panelEntities;
};

Player.prototype.IsTrainingBlocked = function()
{
	return this.trainingBlocked;
};

Player.prototype.BlockTraining = function()
{
	this.trainingBlocked = true;
};

Player.prototype.UnBlockTraining = function()
{
	this.trainingBlocked = false;
};

Player.prototype.SetResourceCounts = function(resources)
{
	for (let res in resources)
		this.resourceCount[res] = resources[res];
};

Player.prototype.GetResourceCounts = function()
{
	return this.resourceCount;
};

/**
 * Add resource of specified type to player
 * @param type Generic type of resource (string)
 * @param amount Amount of resource, which should be added (integer)
 */
Player.prototype.AddResource = function(type, amount)
{
	this.resourceCount[type] += +amount;
};

/**
 * Add resources to player
 */
Player.prototype.AddResources = function(amounts)
{
	for (var type in amounts)
		this.resourceCount[type] += +amounts[type];
};

Player.prototype.GetNeededResources = function(amounts)
{
	// Check if we can afford it all
	var amountsNeeded = {};
	for (var type in amounts)
		if (this.resourceCount[type] != undefined && amounts[type] > this.resourceCount[type])
			amountsNeeded[type] = amounts[type] - Math.floor(this.resourceCount[type]);

	if (Object.keys(amountsNeeded).length == 0)
		return undefined;
	return amountsNeeded;
};

Player.prototype.SubtractResourcesOrNotify = function(amounts)
{
	var amountsNeeded = this.GetNeededResources(amounts);

	// If we don't have enough resources, send a notification to the player
	if (amountsNeeded)
	{
		var parameters = {};
		var i = 0;
		for (var type in amountsNeeded)
		{
			++i;
			parameters["resourceType"+i] = this.resourceNames[type];
			parameters["resourceAmount"+i] = amountsNeeded[type];
		}

		var msg = "";
		// when marking strings for translations, you need to include the actual string,
		// not some way to derive the string
		if (i < 1)
			warn("Amounts needed but no amounts given?");
		else if (i == 1)
			msg = markForTranslation("Insufficient resources - %(resourceAmount1)s %(resourceType1)s");
		else if (i == 2)
			msg = markForTranslation("Insufficient resources - %(resourceAmount1)s %(resourceType1)s, %(resourceAmount2)s %(resourceType2)s");
		else if (i == 3)
			msg = markForTranslation("Insufficient resources - %(resourceAmount1)s %(resourceType1)s, %(resourceAmount2)s %(resourceType2)s, %(resourceAmount3)s %(resourceType3)s");
		else if (i == 4)
			msg = markForTranslation("Insufficient resources - %(resourceAmount1)s %(resourceType1)s, %(resourceAmount2)s %(resourceType2)s, %(resourceAmount3)s %(resourceType3)s, %(resourceAmount4)s %(resourceType4)s");
		else
			warn("Localisation: Strings are not localised for more than 4 resources");

		// Send as time-notification
		let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification({
			"players": [this.playerID],
			"message": msg,
			"parameters": parameters,
			"translateMessage": true,
			"translateParameters": {
				"resourceType1": "withinSentence",
				"resourceType2": "withinSentence",
				"resourceType3": "withinSentence",
				"resourceType4": "withinSentence",
			},
		});
		return false;
	}

	for (var type in amounts)
		this.resourceCount[type] -= amounts[type];

	return true;
};

Player.prototype.TrySubtractResources = function(amounts)
{
	if (!this.SubtractResourcesOrNotify(amounts))
		return false;

	var cmpStatisticsTracker = QueryPlayerIDInterface(this.playerID, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		for (var type in amounts)
			cmpStatisticsTracker.IncreaseResourceUsedCounter(type, amounts[type]);

	return true;
};

Player.prototype.GetNextTradingGoods = function()
{
	var value = randFloat(0, 100);
	var last = this.tradingGoods.length - 1;
	var sumProba = 0;
	for (var i = 0; i < last; ++i)
	{
		sumProba += this.tradingGoods[i].proba;
		if (value < sumProba)
			return this.tradingGoods[i].goods;
	}
	return this.tradingGoods[last].goods;
};

Player.prototype.GetTradingGoods = function()
{
	var tradingGoods = {};
	for (let resource of this.tradingGoods)
		tradingGoods[resource.goods] = resource.proba;

	return tradingGoods;
};

Player.prototype.SetTradingGoods = function(tradingGoods)
{
	let resCodes = Resources.GetCodes();
	let sumProba = 0;
	for (let resource in tradingGoods)
	{
		if (resCodes.indexOf(resource) == -1 || tradingGoods[resource] < 0)
		{
			error("Invalid trading goods: " + uneval(tradingGoods));
			return;
		}
		sumProba += tradingGoods[resource];
	}

	if (sumProba != 100)
	{
		error("Invalid trading goods: " + uneval(tradingGoods));
		return;
	}

	this.tradingGoods = [];
	for (let resource in tradingGoods)
		this.tradingGoods.push({
			"goods": resource,
			"proba": tradingGoods[resource]
		});
};

Player.prototype.GetState = function()
{
	return this.state;
};

/**
 * @param {string} newState - Either "defeated" or "won".
 * @param {string|undefined} message - A string to be shown in chat, for example
 *     markForTranslation("%(player)s has been defeated (failed objective).").
 *     If it is undefined, the caller MUST send that GUI notification manually.
 */
Player.prototype.SetState = function(newState, message)
{
	if (this.state != "active")
		return;

	if (newState != "won" && newState != "defeated")
	{
		warn("Can't change playerstate to " + this.state);
		return;
	}

	this.state = newState;

	let won = newState == "won";
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (won)
		cmpRangeManager.SetLosRevealAll(this.playerID, true);
	else
	{
		// Reassign all player's entities to Gaia
		let entities = cmpRangeManager.GetEntitiesByPlayer(this.playerID);

		// The ownership change is done in two steps so that entities don't hit idle
		// (and thus possibly look for "enemies" to attack) before nearby allies get
		// converted to Gaia as well.
		for (let entity of entities)
		{
			let cmpOwnership = Engine.QueryInterface(entity, IID_Ownership);
			cmpOwnership.SetOwnerQuiet(0);
		}

		// With the real ownership change complete, send OwnershipChanged messages.
		for (let entity of entities)
			Engine.PostMessage(entity, MT_OwnershipChanged, {
				"entity": entity,
				"from": this.playerID,
				"to": 0
			});
	}

	Engine.BroadcastMessage(won ? MT_PlayerWon : MT_PlayerDefeated, { "playerId": this.playerID });

	if (message)
	{
		let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification({
			"type": won ? "won" : "defeat",
			"players": [this.playerID],
			"allies": [this.playerID],
			"message": message
		});
	}
};

Player.prototype.GetTeam = function()
{
	return this.team;
};

Player.prototype.SetTeam = function(team)
{
	if (this.teamsLocked)
		return;

	this.team = team;

	// Set all team members as allies
	if (this.team != -1)
	{
		let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
		for (let i = 0; i < numPlayers; ++i)
		{
			let cmpPlayer = QueryPlayerIDInterface(i);
			if (this.team != cmpPlayer.GetTeam())
				continue;

			this.SetAlly(i);
			cmpPlayer.SetAlly(this.playerID);
		}
	}

	Engine.BroadcastMessage(MT_DiplomacyChanged, {
		"player": this.playerID,
		"otherPlayer": null
	});
};

Player.prototype.SetLockTeams = function(value)
{
	this.teamsLocked = value;
};

Player.prototype.GetLockTeams = function()
{
	return this.teamsLocked;
};

Player.prototype.GetDiplomacy = function()
{
	return this.diplomacy.slice();
};

Player.prototype.SetDiplomacy = function(dipl)
{
	this.diplomacy = dipl.slice();

	Engine.BroadcastMessage(MT_DiplomacyChanged, {
		"player": this.playerID,
		"otherPlayer": null
	});
};

Player.prototype.SetDiplomacyIndex = function(idx, value)
{
	let cmpPlayer = QueryPlayerIDInterface(idx);
	if (!cmpPlayer)
		return;

	if (this.state != "active" || cmpPlayer.state != "active")
		return;

	this.diplomacy[idx] = value;

	Engine.BroadcastMessage(MT_DiplomacyChanged, {
		"player": this.playerID,
		"otherPlayer": cmpPlayer.GetPlayerID()
	});

	// Mutual worsening of relations
	if (cmpPlayer.diplomacy[this.playerID] > value)
		cmpPlayer.SetDiplomacyIndex(this.playerID, value);
};

Player.prototype.UpdateSharedLos = function()
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let cmpTechnologyManager = Engine.QueryInterface(this.entity, IID_TechnologyManager);
	if (!cmpRangeManager || !cmpTechnologyManager)
		return;

	if (!cmpTechnologyManager.IsTechnologyResearched(this.template.SharedLosTech))
	{
		cmpRangeManager.SetSharedLos(this.playerID, [this.playerID]);
		return;
	}

	cmpRangeManager.SetSharedLos(this.playerID, this.GetMutualAllies());
};

Player.prototype.GetFormations = function()
{
	return this.formations;
};

Player.prototype.SetFormations = function(formations)
{
	this.formations = formations;
};

Player.prototype.GetStartingCameraPos = function()
{
	return this.startCam.position;
};

Player.prototype.GetStartingCameraRot = function()
{
	return this.startCam.rotation;
};

Player.prototype.SetStartingCamera = function(pos, rot)
{
	this.startCam = { "position": pos, "rotation": rot };
};

Player.prototype.HasStartingCamera = function()
{
	return this.startCam !== undefined;
};

Player.prototype.HasSharedLos = function()
{
	let cmpTechnologyManager = Engine.QueryInterface(this.entity, IID_TechnologyManager);
	return cmpTechnologyManager && cmpTechnologyManager.IsTechnologyResearched(this.template.SharedLosTech);
};
Player.prototype.HasSharedDropsites = function()
{
	return this.sharedDropsites;
};

Player.prototype.SetControlAllUnits = function(c)
{
	this.controlAllUnits = c;
};

Player.prototype.CanControlAllUnits = function()
{
	return this.controlAllUnits;
};

Player.prototype.SetAI = function(flag)
{
	this.isAI = flag;
};

Player.prototype.IsAI = function()
{
	return this.isAI;
};

Player.prototype.GetPlayersByDiplomacy = function(func)
{
	var players = [];
	for (var i = 0; i < this.diplomacy.length; ++i)
		if (this[func](i))
			players.push(i);
	return players;
};

Player.prototype.SetAlly = function(id)
{
	this.SetDiplomacyIndex(id, 1);
};

/**
 * Check if given player is our ally
 */
Player.prototype.IsAlly = function(id)
{
	return this.diplomacy[id] > 0;
};

Player.prototype.GetAllies = function()
{
	return this.GetPlayersByDiplomacy("IsAlly");
};

/**
 * Check if given player is our ally excluding ourself
 */
Player.prototype.IsExclusiveAlly = function(id)
{
	return this.playerID != id && this.IsAlly(id);
};

/**
 * Check if given player is our ally, and we are its ally
 */
Player.prototype.IsMutualAlly = function(id)
{
	var cmpPlayer = QueryPlayerIDInterface(id);
	return this.IsAlly(id) && cmpPlayer && cmpPlayer.IsAlly(this.playerID);
};

Player.prototype.GetMutualAllies = function()
{
	return this.GetPlayersByDiplomacy("IsMutualAlly");
};

/**
 * Check if given player is our ally, and we are its ally, excluding ourself
 */
Player.prototype.IsExclusiveMutualAlly = function(id)
{
	return this.playerID != id && this.IsMutualAlly(id);
};

Player.prototype.SetEnemy = function(id)
{
	this.SetDiplomacyIndex(id, -1);
};

/**
 * Check if given player is our enemy
 */
Player.prototype.IsEnemy = function(id)
{
	return this.diplomacy[id] < 0;
};

Player.prototype.GetEnemies = function()
{
	return this.GetPlayersByDiplomacy("IsEnemy");
};

Player.prototype.SetNeutral = function(id)
{
	this.SetDiplomacyIndex(id, 0);
};

/**
 * Check if given player is neutral
 */
Player.prototype.IsNeutral = function(id)
{
	return this.diplomacy[id] == 0;
};

/**
 * Do some map dependant initializations
 */
Player.prototype.OnGlobalInitGame = function(msg)
{
	let cmpTechnologyManager = Engine.QueryInterface(this.entity, IID_TechnologyManager);
	if (cmpTechnologyManager)
		for (let tech of this.startingTechnologies)
			cmpTechnologyManager.ResearchTechnology(tech);

	// Replace the "{civ}" code with this civ ID
	let disabledTemplates = this.disabledTemplates;
	this.disabledTemplates = {};
	for (let template in disabledTemplates)
		if (disabledTemplates[template])
			this.disabledTemplates[template.replace(/\{civ\}/g, this.civ)] = true;
};

/**
 * Keep track of population effects of all entities that
 * become owned or unowned by this player
 */
Player.prototype.OnGlobalOwnershipChanged = function(msg)
{
	if (msg.from != this.playerID && msg.to != this.playerID)
		return;

	var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
	var cmpCost = Engine.QueryInterface(msg.entity, IID_Cost);

	if (msg.from == this.playerID)
	{
		if (cmpCost)
			this.popUsed -= cmpCost.GetPopCost();

		if (cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), panelEntityClasses))
		{
			let index = this.panelEntities.indexOf(msg.entity);
			if (index >= 0)
				this.panelEntities.splice(index, 1);
		}
	}
	if (msg.to == this.playerID)
	{
		if (cmpCost)
			this.popUsed += cmpCost.GetPopCost();

		if (cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), panelEntityClasses))
			this.panelEntities.push(msg.entity);
	}
};

Player.prototype.OnResearchFinished = function(msg)
{
	if (msg.tech == this.template.SharedLosTech)
		this.UpdateSharedLos();
	else if (msg.tech == this.template.SharedDropsitesTech)
		this.sharedDropsites = true;
};

Player.prototype.OnDiplomacyChanged = function()
{
	this.UpdateSharedLos();
};

Player.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Player")
		return;

	if (msg.valueNames.indexOf("Player/SpyCostMultiplier") != -1)
		this.spyCostMultiplier = ApplyValueModificationsToEntity("Player/SpyCostMultiplier", +this.template.SpyCostMultiplier, this.entity);

	if (msg.valueNames.some(mod => mod.startsWith("Player/BarterMultiplier/")))
		for (let res in this.template.BarterMultiplier.Buy)
		{
			this.barterMultiplier.buy[res] = ApplyValueModificationsToEntity("Player/BarterMultiplier/Buy/"+res, +this.template.BarterMultiplier.Buy[res], this.entity);
			this.barterMultiplier.sell[res] = ApplyValueModificationsToEntity("Player/BarterMultiplier/Sell/"+res, +this.template.BarterMultiplier.Sell[res], this.entity);
		}
};

Player.prototype.SetCheatsEnabled = function(flag)
{
	this.cheatsEnabled = flag;
};

Player.prototype.GetCheatsEnabled = function()
{
	return this.cheatsEnabled;
};

Player.prototype.SetCheatTimeMultiplier = function(time)
{
	this.cheatTimeMultiplier = time;
};

Player.prototype.GetCheatTimeMultiplier = function()
{
	return this.cheatTimeMultiplier;
};

Player.prototype.TributeResource = function(player, amounts)
{
	var cmpPlayer = QueryPlayerIDInterface(player);
	if (!cmpPlayer)
		return;

	if (this.state != "active" || cmpPlayer.state != "active")
		return;

	for (let resCode in amounts)
		if (Resources.GetCodes().indexOf(resCode) == -1 ||
		    !Number.isInteger(amounts[resCode]) ||
		    amounts[resCode] < 0)
		{
			warn("Invalid tribute amounts: " + uneval(amounts));
			return;
		}

	if (!this.SubtractResourcesOrNotify(amounts))
		return;
	cmpPlayer.AddResources(amounts);

	var total = Object.keys(amounts).reduce((sum, type) => sum + amounts[type], 0);
	var cmpOurStatisticsTracker = QueryPlayerIDInterface(this.playerID, IID_StatisticsTracker);
	if (cmpOurStatisticsTracker)
		cmpOurStatisticsTracker.IncreaseTributesSentCounter(total);
	var cmpTheirStatisticsTracker = QueryPlayerIDInterface(player, IID_StatisticsTracker);
	if (cmpTheirStatisticsTracker)
		cmpTheirStatisticsTracker.IncreaseTributesReceivedCounter(total);

	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	if (cmpGUIInterface)
		cmpGUIInterface.PushNotification({
			"type": "tribute",
			"players": [player],
			"donator": this.playerID,
			"amounts": amounts
		});

	Engine.BroadcastMessage(MT_TributeExchanged, {
		"to": player,
		"from": this.playerID,
		"amounts": amounts
	});
};

Player.prototype.AddDisabledTemplate = function(template)
{
	this.disabledTemplates[template] = true;
	Engine.BroadcastMessage(MT_DisabledTemplatesChanged, {});
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGuiInterface.PushNotification({
		"type": "resetselectionpannel",
		"players": [this.GetPlayerID()]
	});
};

Player.prototype.RemoveDisabledTemplate = function(template)
{
	this.disabledTemplates[template] = false;
	Engine.BroadcastMessage(MT_DisabledTemplatesChanged, {});

	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGuiInterface.PushNotification({
		"type": "resetselectionpannel",
		"players": [this.GetPlayerID()]
	});
};

Player.prototype.SetDisabledTemplates = function(templates)
{
	this.disabledTemplates = {};
	for (let template of templates)
		this.disabledTemplates[template] = true;
	Engine.BroadcastMessage(MT_DisabledTemplatesChanged, {});

	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGuiInterface.PushNotification({
		"type": "resetselectionpannel",
		"players": [this.GetPlayerID()]
	});
};

Player.prototype.GetDisabledTemplates = function()
{
	return this.disabledTemplates;
};

Player.prototype.AddDisabledTechnology = function(tech)
{
	this.disabledTechnologies[tech] = true;
	Engine.BroadcastMessage(MT_DisabledTechnologiesChanged, {});
};

Player.prototype.RemoveDisabledTechnology = function(tech)
{
	this.disabledTechnologies[tech] = false;
	Engine.BroadcastMessage(MT_DisabledTechnologiesChanged, {});
};

Player.prototype.SetDisabledTechnologies = function(techs)
{
	this.disabledTechnologies = {};
	for (let tech of techs)
		this.disabledTechnologies[tech] = true;
	Engine.BroadcastMessage(MT_DisabledTechnologiesChanged, {});
};

Player.prototype.GetDisabledTechnologies = function()
{
	return this.disabledTechnologies;
};

Player.prototype.AddStartingTechnology = function(tech)
{
	if (this.startingTechnologies.indexOf(tech) == -1)
		this.startingTechnologies.push(tech);
};

Player.prototype.SetStartingTechnologies = function(techs)
{
	this.startingTechnologies = techs;
};

Engine.RegisterComponentType(IID_Player, "Player", Player);
