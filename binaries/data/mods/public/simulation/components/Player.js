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
	"<element name='Formations' a:help='Space-separated list of formations this player can use.'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
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

// The GUI expects these strings.
Player.prototype.STATE_ACTIVE = "active";
Player.prototype.STATE_DEFEATED = "defeated";
Player.prototype.STATE_WON = "won";

/**
 * Don't serialize diplomacyColor or displayDiplomacyColor since they're modified by the GUI.
 */
Player.prototype.Serialize = function()
{
	let state = {};
	for (let key in this)
		if (this.hasOwnProperty(key))
			state[key] = this[key];

	state.diplomacyColor = undefined;
	state.displayDiplomacyColor = false;
	return state;
};

Player.prototype.Deserialize = function(state)
{
	for (let prop in state)
		this[prop] = state[prop];
};

/**
 * Which units will be shown with special icons at the top.
 */
var panelEntityClasses = "Hero Relic";

Player.prototype.Init = function()
{
	this.playerID = undefined;
	this.color = undefined;
	this.diplomacyColor = undefined;
	this.displayDiplomacyColor = false;
	this.popUsed = 0; // Population of units owned or trained by this player.
	this.popBonuses = 0; // Sum of population bonuses of player's entities.
	this.maxPop = 300; // Maximum population.
	this.trainingBlocked = false; // Indicates whether any training queue is currently blocked.
	this.resourceCount = {};
	this.resourceGatherers = {};
	this.tradingGoods = []; // Goods for next trade-route and its probabilities * 100.
	this.team = -1;	// Team number of the player, players on the same team will always have ally diplomatic status. Also this is useful for team emblems, scoring, etc.
	this.teamsLocked = false;
	this.state = this.STATE_ACTIVE;
	this.diplomacy = [];	// Array of diplomatic stances for this player with respect to other players (including gaia and self).
	this.sharedDropsites = false;
	this.formations = this.template.Formations._string.split(" ");
	this.startCam = undefined;
	this.controlAllUnits = false;
	this.isAI = false;
	this.cheatsEnabled = false;
	this.panelEntities = [];
	this.resourceNames = {};
	this.disabledTemplates = {};
	this.disabledTechnologies = {};
	this.spyCostMultiplier = +this.template.SpyCostMultiplier;
	this.barterEntities = [];
	this.barterMultiplier = {
		"buy": clone(this.template.BarterMultiplier.Buy),
		"sell": clone(this.template.BarterMultiplier.Sell)
	};

	// Initial resources.
	let resCodes = Resources.GetCodes();
	for (let res of resCodes)
	{
		this.resourceCount[res] = 300;
		this.resourceNames[res] = Resources.GetResource(res).name;
		this.resourceGatherers[res] = 0;
	}
	// Trading goods probability in steps of 5.
	let resTradeCodes = Resources.GetTradableCodes();
	let quotient = Math.floor(20 / resTradeCodes.length);
	let remainder = 20 % resTradeCodes.length;
	for (let i in resTradeCodes)
		this.tradingGoods.push({
			"goods": resTradeCodes[i],
			"proba": 5 * (quotient + (+i < remainder ? 1 : 0))
		});
};

Player.prototype.SetPlayerID = function(id)
{
	this.playerID = id;
};

Player.prototype.GetPlayerID = function()
{
	return this.playerID;
};

Player.prototype.SetColor = function(r, g, b)
{
	let colorInitialized = !!this.color;

	this.color = { "r": r / 255, "g": g / 255, "b": b / 255, "a": 1 };

	// Used in Atlas.
	if (colorInitialized)
		Engine.BroadcastMessage(MT_PlayerColorChanged, {
			"player": this.playerID
		});
};

Player.prototype.SetDiplomacyColor = function(color)
{
	this.diplomacyColor = { "r": color.r / 255, "g": color.g / 255, "b": color.b / 255, "a": 1 };
};

Player.prototype.SetDisplayDiplomacyColor = function(displayDiplomacyColor)
{
	this.displayDiplomacyColor = displayDiplomacyColor;
};

Player.prototype.GetColor = function()
{
	return this.color;
};

Player.prototype.GetDisplayedColor = function()
{
	return this.displayDiplomacyColor ? this.diplomacyColor : this.color;
};

// Try reserving num population slots. Returns 0 on success or number of missing slots otherwise.
Player.prototype.TryReservePopulationSlots = function(num)
{
	if (num != 0 && num > (this.GetPopulationLimit() - this.popUsed))
		return num - (this.GetPopulationLimit() - this.popUsed);

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

Player.prototype.CanBarter = function()
{
	return this.barterEntities.length > 0;
};

Player.prototype.GetBarterMultiplier = function()
{
	return this.barterMultiplier;
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

Player.prototype.GetResourceGatherers = function()
{
	return this.resourceGatherers;
};

/**
 * @param {string} type - The generic type of resource to add the gatherer for.
 */
Player.prototype.AddResourceGatherer = function(type)
{
	++this.resourceGatherers[type];
};

/**
 * @param {string} type - The generic type of resource to remove the gatherer from.
 */
Player.prototype.RemoveResourceGatherer = function(type)
{
	--this.resourceGatherers[type];
};

/**
 * Add resource of specified type to player.
 * @param {string} type - Generic type of resource.
 * @param {number} amount - Amount of resource, which should be added.
 */
Player.prototype.AddResource = function(type, amount)
{
	this.resourceCount[type] += +amount;
};

/**
 * Add resources to player.
 */
Player.prototype.AddResources = function(amounts)
{
	for (let type in amounts)
		this.resourceCount[type] += +amounts[type];
};

Player.prototype.GetNeededResources = function(amounts)
{
	// Check if we can afford it all.
	let amountsNeeded = {};
	for (let type in amounts)
		if (this.resourceCount[type] != undefined && amounts[type] > this.resourceCount[type])
			amountsNeeded[type] = amounts[type] - Math.floor(this.resourceCount[type]);

	if (Object.keys(amountsNeeded).length == 0)
		return undefined;
	return amountsNeeded;
};

Player.prototype.SubtractResourcesOrNotify = function(amounts)
{
	let amountsNeeded = this.GetNeededResources(amounts);

	// If we don't have enough resources, send a notification to the player.
	if (amountsNeeded)
	{
		let parameters = {};
		let i = 0;
		for (let type in amountsNeeded)
		{
			++i;
			parameters["resourceType" + i] = this.resourceNames[type];
			parameters["resourceAmount" + i] = amountsNeeded[type];
		}

		let msg = "";
		// When marking strings for translations, you need to include the actual string,
		// not some way to derive the string.
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

		// Send as time-notification.
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
				"resourceType4": "withinSentence"
			}
		});
		return false;
	}

	for (let type in amounts)
		this.resourceCount[type] -= amounts[type];

	return true;
};

Player.prototype.TrySubtractResources = function(amounts)
{
	if (!this.SubtractResourcesOrNotify(amounts))
		return false;

	let cmpStatisticsTracker = QueryPlayerIDInterface(this.playerID, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		for (let type in amounts)
			cmpStatisticsTracker.IncreaseResourceUsedCounter(type, amounts[type]);

	return true;
};

Player.prototype.RefundResources = function(amounts)
{
	const cmpStatisticsTracker = QueryPlayerIDInterface(this.playerID, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		for (const type in amounts)
			cmpStatisticsTracker.IncreaseResourceUsedCounter(type, -amounts[type]);

	this.AddResources(amounts);
};

Player.prototype.GetNextTradingGoods = function()
{
	let value = randFloat(0, 100);
	let last = this.tradingGoods.length - 1;
	let sumProba = 0;
	for (let i = 0; i < last; ++i)
	{
		sumProba += this.tradingGoods[i].proba;
		if (value < sumProba)
			return this.tradingGoods[i].goods;
	}
	return this.tradingGoods[last].goods;
};

Player.prototype.GetTradingGoods = function()
{
	let tradingGoods = {};
	for (let resource of this.tradingGoods)
		tradingGoods[resource.goods] = resource.proba;

	return tradingGoods;
};

Player.prototype.SetTradingGoods = function(tradingGoods)
{
	let resTradeCodes = Resources.GetTradableCodes();
	let sumProba = 0;
	for (let resource in tradingGoods)
	{
		if (resTradeCodes.indexOf(resource) == -1 || tradingGoods[resource] < 0)
		{
			error("Invalid trading goods: " + uneval(tradingGoods));
			return;
		}
		sumProba += tradingGoods[resource];
	}

	if (sumProba != 100)
	{
		error("Invalid trading goods probability: " + uneval(sumProba));
		return;
	}

	this.tradingGoods = [];
	for (let resource in tradingGoods)
		this.tradingGoods.push({
			"goods": resource,
			"proba": tradingGoods[resource]
		});
};

/**
 * @param {string} message - The message to send in the chat. May be undefined.
 */
Player.prototype.Win = function(message)
{
	this.SetState(this.STATE_WON, message);
};

/**
 * @param {string} message - The message to send in the chat. May be undefined.
 */
Player.prototype.Defeat = function(message)
{
	this.SetState(this.STATE_DEFEATED, message);
};

/**
 * @return {string} - The string identified with the current state.
 */
Player.prototype.GetState = function()
{
	return this.state;
};

/**
 * @return {boolean} -
 */
Player.prototype.IsActive = function()
{
	return this.state === this.STATE_ACTIVE;
};

/**
 * @return {boolean} -
 */
Player.prototype.IsDefeated = function()
{
	return this.state === this.STATE_DEFEATED;
};

/**
 * @return {boolean} -
 */
Player.prototype.HasWon = function()
{
	return this.state === this.STATE_WON;
};

/**
 * @param {string} newState - Either "defeated" or "won".
 * @param {string|undefined} message - A string to be shown in chat, for example
 *     markForTranslation("%(player)s has been defeated (failed objective).").
 *     If it is undefined, the caller MUST send that GUI notification manually.
 */
Player.prototype.SetState = function(newState, message)
{
	if (!this.IsActive())
		return;

	if (newState != this.STATE_WON && newState != this.STATE_DEFEATED)
	{
		warn("Can't change playerstate to " + newState);
		return;
	}

	if (!this.playerID)
	{
		warn("Gaia can't change state.");
		return;
	}

	this.state = newState;

	const won = this.HasWon();
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (won)
		cmpRangeManager.SetLosRevealAll(this.playerID, true);
	else
	{
		// Reassign all player's entities to Gaia.
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

	if (message)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).PushNotification({
			"type": won ? "won" : "defeat",
			"players": [this.playerID],
			"allies": [this.playerID],
			"message": message
		});

	Engine.PostMessage(this.entity, won ? MT_PlayerWon : MT_PlayerDefeated, { "playerId": this.playerID });
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

	// Set all team members as allies.
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

	if (!this.IsActive() || !cmpPlayer.IsActive())
		return;

	this.diplomacy[idx] = value;

	Engine.BroadcastMessage(MT_DiplomacyChanged, {
		"player": this.playerID,
		"otherPlayer": cmpPlayer.GetPlayerID()
	});

	// Mutual worsening of relations.
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
	let players = [];
	for (let i = 0; i < this.diplomacy.length; ++i)
		if (this[func](i))
			players.push(i);
	return players;
};

Player.prototype.SetAlly = function(id)
{
	this.SetDiplomacyIndex(id, 1);
};

/**
 * Check if given player is our ally.
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
	let cmpPlayer = QueryPlayerIDInterface(id);
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
	// Replace the "{civ}" code with this civ ID.
	let disabledTemplates = this.disabledTemplates;
	this.disabledTemplates = {};
	const civ = Engine.QueryInterface(this.entity, IID_Identity).GetCiv();
	for (let template in disabledTemplates)
		if (disabledTemplates[template])
			this.disabledTemplates[template.replace(/\{civ\}/g, civ)] = true;
};

/**
 * Keep track of population effects of all entities that
 * become owned or unowned by this player.
 */
Player.prototype.OnGlobalOwnershipChanged = function(msg)
{
	if (msg.from != this.playerID && msg.to != this.playerID)
		return;

	let cmpCost = Engine.QueryInterface(msg.entity, IID_Cost);

	if (msg.from == this.playerID)
	{
		if (cmpCost)
			this.popUsed -= cmpCost.GetPopCost();

		let panelIndex = this.panelEntities.indexOf(msg.entity);
		if (panelIndex >= 0)
			this.panelEntities.splice(panelIndex, 1);

		let barterIndex = this.barterEntities.indexOf(msg.entity);
		if (barterIndex >= 0)
			this.barterEntities.splice(barterIndex, 1);
	}
	if (msg.to == this.playerID)
	{
		if (cmpCost)
			this.popUsed += cmpCost.GetPopCost();

		let cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
		if (!cmpIdentity)
			return;

		if (MatchesClassList(cmpIdentity.GetClassesList(), panelEntityClasses))
			this.panelEntities.push(msg.entity);

		if (cmpIdentity.HasClass("Barter") && !Engine.QueryInterface(msg.entity, IID_Foundation))
			this.barterEntities.push(msg.entity);
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

Player.prototype.TributeResource = function(player, amounts)
{
	let cmpPlayer = QueryPlayerIDInterface(player);
	if (!cmpPlayer)
		return;

	if (!this.IsActive() || !cmpPlayer.IsActive())
		return;

	let resTribCodes = Resources.GetTributableCodes();
	for (let resCode in amounts)
		if (resTribCodes.indexOf(resCode) == -1 ||
		    !Number.isInteger(amounts[resCode]) ||
		    amounts[resCode] < 0)
		{
			warn("Invalid tribute amounts: " + uneval(resCode) + ": " + uneval(amounts));
			return;
		}

	if (!this.SubtractResourcesOrNotify(amounts))
		return;
	cmpPlayer.AddResources(amounts);

	let total = Object.keys(amounts).reduce((sum, type) => sum + amounts[type], 0);
	let cmpOurStatisticsTracker = QueryPlayerIDInterface(this.playerID, IID_StatisticsTracker);
	if (cmpOurStatisticsTracker)
		cmpOurStatisticsTracker.IncreaseTributesSentCounter(total);
	let cmpTheirStatisticsTracker = QueryPlayerIDInterface(player, IID_StatisticsTracker);
	if (cmpTheirStatisticsTracker)
		cmpTheirStatisticsTracker.IncreaseTributesReceivedCounter(total);

	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
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
	Engine.BroadcastMessage(MT_DisabledTemplatesChanged, { "player": this.playerID });
};

Player.prototype.RemoveDisabledTemplate = function(template)
{
	this.disabledTemplates[template] = false;
	Engine.BroadcastMessage(MT_DisabledTemplatesChanged, { "player": this.playerID });
};

Player.prototype.SetDisabledTemplates = function(templates)
{
	this.disabledTemplates = {};
	for (let template of templates)
		this.disabledTemplates[template] = true;
	Engine.BroadcastMessage(MT_DisabledTemplatesChanged, { "player": this.playerID });
};

Player.prototype.GetDisabledTemplates = function()
{
	return this.disabledTemplates;
};

Player.prototype.AddDisabledTechnology = function(tech)
{
	this.disabledTechnologies[tech] = true;
	Engine.BroadcastMessage(MT_DisabledTechnologiesChanged, { "player": this.playerID });
};

Player.prototype.RemoveDisabledTechnology = function(tech)
{
	this.disabledTechnologies[tech] = false;
	Engine.BroadcastMessage(MT_DisabledTechnologiesChanged, { "player": this.playerID });
};

Player.prototype.SetDisabledTechnologies = function(techs)
{
	this.disabledTechnologies = {};
	for (let tech of techs)
		this.disabledTechnologies[tech] = true;
	Engine.BroadcastMessage(MT_DisabledTechnologiesChanged, { "player": this.playerID });
};

Player.prototype.GetDisabledTechnologies = function()
{
	return this.disabledTechnologies;
};

Player.prototype.OnGlobalPlayerDefeated = function(msg)
{
	let cmpSound = Engine.QueryInterface(this.entity, IID_Sound);
	if (!cmpSound)
		return;

	const soundGroup = cmpSound.GetSoundGroup(this.playerID === msg.playerId ? "defeated" : this.IsAlly(msg.playerId) ? "defeated_ally" : this.HasWon() ? "won" : "defeated_enemy");
	if (soundGroup)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_SoundManager).PlaySoundGroupForPlayer(soundGroup, this.playerID);
};

Engine.RegisterComponentType(IID_Player, "Player", Player);
