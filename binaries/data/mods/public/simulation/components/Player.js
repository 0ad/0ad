function Player() {}

Player.prototype.Schema =
	"<a:component type='system'/><empty/>";

Player.prototype.Init = function()
{
	this.playerID = undefined;
	this.name = undefined;	// define defaults elsewhere (supporting other languages)
	this.civ = undefined;
	this.colour = { "r": 0.0, "g": 0.0, "b": 0.0, "a": 1.0 };
	this.popUsed = 0; // population of units owned or trained by this player
	this.popBonuses = 0; // sum of population bonuses of player's entities
	this.maxPop = 300; // maximum population
	this.trainingBlocked = false; // indicates whether any training queue is currently blocked
	this.resourceCount = {
		"food": 300,
		"wood": 300,
		"metal": 300,
		"stone": 300
	};
	this.tradingGoods = [                      // goods for next trade-route and its proba in % (the sum of probas must be 100)
		{ "goods":  "wood", "proba": 30 },
		{ "goods": "stone", "proba": 35 },
		{ "goods": "metal", "proba": 35 } ];
	this.team = -1;	// team number of the player, players on the same team will always have ally diplomatic status - also this is useful for team emblems, scoring, etc.
	this.teamsLocked = false;
	this.state = "active"; // game state - one of "active", "defeated", "won"
	this.diplomacy = [];	// array of diplomatic stances for this player with respect to other players (including gaia and self)
	this.conquestCriticalEntitiesCount = 0; // number of owned units with ConquestCritical class
	this.formations = [];
	this.startCam = undefined;
	this.controlAllUnits = false;
	this.isAI = false;
	this.gatherRateMultiplier = 1;
	this.cheatsEnabled = false;
	this.cheatTimeMultiplier = 1;
	this.heroes = [];
	this.resourceNames = {
		"food": markForTranslation("Food"),
		"wood": markForTranslation("Wood"),
		"metal": markForTranslation("Metal"),
		"stone": markForTranslation("Stone"),
	}
	Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager).CheckConquestCriticalEntities();
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
	this.civ = civcode;
};

Player.prototype.GetCiv = function()
{
	return this.civ;
};

Player.prototype.SetColour = function(r, g, b)
{
	this.colour = { "r": r/255.0, "g": g/255.0, "b": b/255.0, "a": 1.0 };
};

Player.prototype.GetColour = function()
{
	return this.colour;
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
	return Math.round(ApplyValueModificationsToPlayer("Player/MaxPopulation", this.maxPop, this.entity));
};

Player.prototype.SetGatherRateMultiplier = function(value)
{
	this.gatherRateMultiplier = value;
};

Player.prototype.GetGatherRateMultiplier = function()
{
	return this.gatherRateMultiplier;
};

Player.prototype.GetHeroes = function()
{
	return this.heroes;
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
	if (resources.food !== undefined)
		this.resourceCount.food = resources.food;
	if (resources.wood !== undefined)
		this.resourceCount.wood = resources.wood;
	if (resources.stone !== undefined)
		this.resourceCount.stone = resources.stone;
	if (resources.metal !== undefined)
		this.resourceCount.metal = resources.metal;
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
	this.resourceCount[type] += (+amount);
};

/**
 * Add resources to player
 */
Player.prototype.AddResources = function(amounts)
{
	for (var type in amounts)
	{
		this.resourceCount[type] += (+amounts[type]);
	}
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
			i++;
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

		var notification = {
			"player": this.playerID,
			"message": msg,
			"parameters": parameters,
			"translateMessage": true,
			"translateParameters": {
				"resourceType1": "withinSentence",
				"resourceType2": "withinSentence",
				"resourceType3": "withinSentence",
				"resourceType4": "withinSentence",
			},
		};
		var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification(notification);
		return false;
	}

	// Subtract the resources
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
	var value = 100*Math.random();
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
	for each (var resource in this.tradingGoods)
		tradingGoods[resource.goods] = resource.proba;

	return tradingGoods;
};

Player.prototype.SetTradingGoods = function(tradingGoods)
{
	var sumProba = 0;
	for (var resource in tradingGoods)
		sumProba += tradingGoods[resource];
	if (sumProba != 100)	// consistency check
	{
		error("Player.js SetTradingGoods: " + uneval(tradingGoods));
		tradingGoods = { "food": 20, "wood":20, "stone":30, "metal":30 };
	}

	this.tradingGoods = [];
	for (var resource in tradingGoods)
		this.tradingGoods.push( {"goods": resource, "proba": tradingGoods[resource]} );
};

Player.prototype.GetState = function()
{
	return this.state;
};

Player.prototype.SetState = function(newState)
{
	this.state = newState;
};

Player.prototype.GetConquestCriticalEntitiesCount = function()
{
	return this.conquestCriticalEntitiesCount;
};

Player.prototype.GetTeam = function()
{
	return this.team;
};

Player.prototype.SetTeam = function(team)
{
	if (!this.teamsLocked)
	{
		this.team = team;

		var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
		if (cmpPlayerManager && this.team != -1)
		{
			// Set all team members as allies
			for (var i = 0; i < cmpPlayerManager.GetNumPlayers(); ++i)
			{
				var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(i), IID_Player);
				if (this.team == cmpPlayer.GetTeam())
				{
					this.SetAlly(i);
					cmpPlayer.SetAlly(this.playerID);
				}
			}
		}

		Engine.BroadcastMessage(MT_DiplomacyChanged, {"player": this.playerID});
	}
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
	return this.diplomacy;
};

Player.prototype.SetDiplomacy = function(dipl)
{
	// Should we check for teamsLocked here?
	this.diplomacy = dipl;
	Engine.BroadcastMessage(MT_DiplomacyChanged, {"player": this.playerID});
};

Player.prototype.SetDiplomacyIndex = function(idx, value)
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (!cmpPlayerManager)
		return;

	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(idx), IID_Player);
	if (!cmpPlayer)
		return;

	if (this.state != "active" || cmpPlayer.state != "active")
		return;

	// You can have alliances with other players,
	if (this.teamsLocked)
	{
		// but can't stab your team members in the back
		if (this.team == -1 || this.team != cmpPlayer.GetTeam())
		{
			// Break alliance or declare war
			if (Math.min(this.diplomacy[idx],cmpPlayer.diplomacy[this.playerID]) > value)
			{
				this.diplomacy[idx] = value;
				cmpPlayer.SetDiplomacyIndex(this.playerID, value);
			}
			else
			{
				this.diplomacy[idx] = value;
			}
			Engine.BroadcastMessage(MT_DiplomacyChanged, {"player": this.playerID});
		}
	}
	else
	{
		// Break alliance or declare war (worsening of relations is mutual)
		if (Math.min(this.diplomacy[idx],cmpPlayer.diplomacy[this.playerID]) > value)
		{
			// This is duplicated because otherwise we get too much recursion
			this.diplomacy[idx] = value;
			cmpPlayer.SetDiplomacyIndex(this.playerID, value);
		}
		else
		{
			this.diplomacy[idx] = value;
		}

		Engine.BroadcastMessage(MT_DiplomacyChanged, {"player": this.playerID});
	}
};

Player.prototype.UpdateSharedLos = function()
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (!cmpRangeManager)
		return;

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (!cmpPlayerManager)
		return;

	var sharedLos = [];
	for (var i = 0; i < cmpPlayerManager.GetNumPlayers(); ++i)
		if (this.IsMutualAlly(i))
			sharedLos.push(i);

	cmpRangeManager.SetSharedLos(this.playerID, sharedLos);
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
	this.startCam = {"position": pos, "rotation": rot};
};

Player.prototype.HasStartingCamera = function()
{
	return (this.startCam !== undefined);
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

/**
 * Check if given player is our ally, and we are its ally
 */
Player.prototype.IsMutualAlly = function(id)
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (!cmpPlayerManager)
		return false;

	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(id), IID_Player);
	return this.IsAlly(id) && cmpPlayer && cmpPlayer.IsAlly(this.playerID);
};

Player.prototype.SetEnemy = function(id)
{
	this.SetDiplomacyIndex(id, -1);
};

/**
 * Get all enemies of a given player.
 */
Player.prototype.GetEnemies = function()
{
	var enemies = [];
	for (var i = 0; i < this.diplomacy.length; i++)
		if (this.diplomacy[i] < 0)
			enemies.push(i);
	return enemies;
};

/**
 * Check if given player is our enemy
 */
Player.prototype.IsEnemy = function(id)
{
	return this.diplomacy[id] < 0;
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
 * Keep track of population effects of all entities that
 * become owned or unowned by this player
 */
Player.prototype.OnGlobalOwnershipChanged = function(msg)
{
	if (msg.from != this.playerID && msg.to != this.playerID)
		return;

	var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
	var cmpCost = Engine.QueryInterface(msg.entity, IID_Cost);
	var cmpFoundation = Engine.QueryInterface(msg.entity, IID_Foundation);

	if (msg.from == this.playerID)
	{
		if (!cmpFoundation && cmpIdentity && cmpIdentity.HasClass("ConquestCritical"))
			this.conquestCriticalEntitiesCount--;

		if (this.conquestCriticalEntitiesCount == 0) // end game when needed
			Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager).CheckConquestCriticalEntities();

		if (cmpCost)
			this.popUsed -= cmpCost.GetPopCost();

		if (cmpIdentity && cmpIdentity.HasClass("Hero"))
		{
			//Remove from Heroes list
			var index = this.heroes.indexOf(msg.entity);
			if (index >= 0)
				this.heroes.splice(index, 1);
		}
	}
	if (msg.to == this.playerID)
	{
		if (!cmpFoundation && cmpIdentity && cmpIdentity.HasClass("ConquestCritical"))
			this.conquestCriticalEntitiesCount++;

		if (cmpCost)
			this.popUsed += cmpCost.GetPopCost();

		if (cmpIdentity && cmpIdentity.HasClass("Hero"))
			this.heroes.push(msg.entity);
	}
};

Player.prototype.OnPlayerDefeated = function(msg)
{
	this.state = "defeated";

	// TODO: Tribute all resources to this player's active allies (if any)

	// Reassign all player's entities to Gaia
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var entities = cmpRangeManager.GetEntitiesByPlayer(this.playerID);

	// The ownership change is done in two steps so that entities don't hit idle
	// (and thus possibly look for "enemies" to attack) before nearby allies get
	// converted to Gaia as well.
	for each (var entity in entities)
	{
		var cmpOwnership = Engine.QueryInterface(entity, IID_Ownership);
		cmpOwnership.SetOwnerQuiet(0);
	}

	// With the real ownership change complete, send OwnershipChanged messages.
	for each (var entity in entities)
		Engine.PostMessage(entity, MT_OwnershipChanged, { "entity": entity,
			"from": this.playerID, "to": 0 });

	// Reveal the map for this player.
	cmpRangeManager.SetLosRevealAll(this.playerID, true);

	// Send a chat message notifying of the player's defeat.
	var notification = {"type": "defeat", "player": this.playerID};
	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification(notification);
};

Player.prototype.OnDiplomacyChanged = function()
{
	this.UpdateSharedLos();
	Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager).CheckConquestCriticalEntities();
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
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (!cmpPlayerManager)
		return;

	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(player), IID_Player);
	if (!cmpPlayer)
		return;

	if (this.state != "active" || cmpPlayer.state != "active")
		return;

	if (!this.SubtractResourcesOrNotify(amounts))
		return;

	cmpPlayer.AddResources(amounts);

	var total = Object.keys(amounts).reduce(function (sum, type){ return sum + amounts[type]; }, 0);
	var cmpOurStatisticsTracker = QueryPlayerIDInterface(this.playerID, IID_StatisticsTracker);
	if (cmpOurStatisticsTracker)
		cmpOurStatisticsTracker.IncreaseTributesSentCounter(total);
	var cmpTheirStatisticsTracker = QueryPlayerIDInterface(player, IID_StatisticsTracker);
	if (cmpTheirStatisticsTracker)
		cmpTheirStatisticsTracker.IncreaseTributesReceivedCounter(total);

	var notification = {"type": "tribute", "player": player, "player1": this.playerID, "amounts": amounts};
	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	if (cmpGUIInterface)
		cmpGUIInterface.PushNotification(notification);
};

Engine.RegisterComponentType(IID_Player, "Player", Player);
