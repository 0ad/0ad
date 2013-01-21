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

	this.team = -1;	// team number of the player, players on the same team will always have ally diplomatic status - also this is useful for team emblems, scoring, etc.
	this.teamsLocked = false;
	this.state = "active"; // game state - one of "active", "defeated", "won"
	this.diplomacy = [];	// array of diplomatic stances for this player with respect to other players (including gaia and self)
	this.conquestCriticalEntitiesCount = 0; // number of owned units with ConquestCritical class
	this.phase = "village";
	this.formations = [];
	this.startCam = undefined;
	this.controlAllUnits = false;
	this.isAI = false;
	this.cheatsEnabled = true;
	this.cheatTimeMultiplier = 1;
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

Player.prototype.TryReservePopulationSlots = function(num)
{
	if (num != 0 && num > (this.GetPopulationLimit() - this.GetPopulationCount()))
		return false;

	this.popUsed += num;
	return true;
};

Player.prototype.UnReservePopulationSlots = function(num)
{
	this.popUsed -= num;
};

Player.prototype.GetPopulationCount = function()
{
	return this.popUsed;
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
	return Math.round(ApplyTechModificationsToPlayer("Player/MaxPopulation", this.maxPop, this.entity));
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
 * @param amount Amount of resource, whick should be added (integer)
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
			amountsNeeded[type] = amounts[type] - this.resourceCount[type];

	if (Object.keys(amountsNeeded).length == 0)
		return undefined;
	return amountsNeeded;
};

Player.prototype.TrySubtractResources = function(amounts)
{
	var amountsNeeded = this.GetNeededResources(amounts);

	// If we don't have enough resources, send a notification to the player
	if (amountsNeeded)
	{
		var formatted = [];
		for (var type in amountsNeeded)
			formatted.push(amountsNeeded[type] + " " + type[0].toUpperCase() + type.substr(1) );
		var notification = {"player": this.playerID, "message": "Insufficient resources - " + formatted.join(", ")};
		var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification(notification);
		return false;
	}
	else
	{
		// Subtract the resources
		var cmpStatisticsTracker = QueryPlayerIDInterface(this.playerID, IID_StatisticsTracker);
		for (var type in amounts)
		{
			this.resourceCount[type] -= amounts[type];
			if (cmpStatisticsTracker)
				cmpStatisticsTracker.IncreaseResourceUsedCounter(type, amounts[type]);
		}
	}

	return true;
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
		if (this.team == -1 || cmpPlayer && this.team != cmpPlayer.GetTeam())
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
		if (this.IsAlly(i))
		{
			var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(i), IID_Player);
			if (cmpPlayer && cmpPlayer.IsAlly(this.playerID))
				sharedLos.push(i);
		}

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
	var isConquestCritical = false;
	// Load class list only if we're going to need it
	if (msg.from == this.playerID || msg.to == this.playerID)
	{
		var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
		if (cmpIdentity)
		{
			isConquestCritical = cmpIdentity.HasClass("ConquestCritical");
		}
	}
	if (msg.from == this.playerID)
	{
		if (isConquestCritical)
			this.conquestCriticalEntitiesCount--;
		var cost = Engine.QueryInterface(msg.entity, IID_Cost);
		if (cost)
		{
			this.popUsed -= cost.GetPopCost();
			this.popBonuses -= cost.GetPopBonus();
		}
	}
	if (msg.to == this.playerID)
	{
		if (isConquestCritical)
			this.conquestCriticalEntitiesCount++;
		var cost = Engine.QueryInterface(msg.entity, IID_Cost);
		if (cost)
		{
			this.popUsed += cost.GetPopCost();
			this.popBonuses += cost.GetPopBonus();
		}
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
};

Player.prototype.SetCheatEnabled = function(flag)
{
	this.cheatsEnabled = flag;
};

Player.prototype.GetCheatEnabled = function(flag)
{
	return this.cheatsEnabled;
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

	if (!this.GetNeededResources(amounts))
	{
		for (var type in amounts)
			this.resourceCount[type] -= amounts[type];

		cmpPlayer.AddResources(amounts);
		// TODO: notify the receiver
	}
	// else not enough resources... TODO: send gui notification
};

Engine.RegisterComponentType(IID_Player, "Player", Player);
