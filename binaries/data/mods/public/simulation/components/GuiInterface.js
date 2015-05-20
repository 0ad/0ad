function GuiInterface() {}

GuiInterface.prototype.Schema =
	"<a:component type='system'/><empty/>";

GuiInterface.prototype.Serialize = function()
{
	// This component isn't network-synchronised for the biggest part
	// So most of the attributes shouldn't be serialized
	// Return an object with a small selection of deterministic data
	return {
		"timeNotifications": this.timeNotifications,
		"timeNotificationID": this.timeNotificationID
	};
};

GuiInterface.prototype.Deserialize = function(data)
{
	this.Init();
	this.timeNotifications = data.timeNotifications;
	this.timeNotificationID = data.timeNotificationID;
};

GuiInterface.prototype.Init = function()
{
	this.placementEntity = undefined; // = undefined or [templateName, entityID]
	this.placementWallEntities = undefined;
	this.placementWallLastAngle = 0;
	this.notifications = [];
	this.renamedEntities = [];
	this.miragedEntities = [];
	this.timeNotificationID = 1;
	this.timeNotifications = [];
	this.entsRallyPointsDisplayed = [];
	this.entsWithAuraAndStatusBars = new Set();
};

/*
 * All of the functions defined below are called via Engine.GuiInterfaceCall(name, arg)
 * from GUI scripts, and executed here with arguments (player, arg).
 * 
 * CAUTION: The input to the functions in this module is not network-synchronised, so it 
 * mustn't affect the simulation state (i.e. the data that is serialised and can affect 
 * the behaviour of the rest of the simulation) else it'll cause out-of-sync errors.
 */

/**
 * Returns global information about the current game state.
 * This is used by the GUI and also by AI scripts.
 */
GuiInterface.prototype.GetSimulationState = function(player)
{
	var ret = {
		"players": []
	};

	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var n = cmpPlayerMan.GetNumPlayers();
	for (var i = 0; i < n; ++i)
	{
		var playerEnt = cmpPlayerMan.GetPlayerByID(i);
		var cmpPlayerEntityLimits = Engine.QueryInterface(playerEnt, IID_EntityLimits);
		var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);

		// Work out what phase we are in
		var phase = "";
		var cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
		if (cmpTechnologyManager)
		{
			if (cmpTechnologyManager.IsTechnologyResearched("phase_city"))
				phase = "city";
			else if (cmpTechnologyManager.IsTechnologyResearched("phase_town"))
				phase = "town";
			else if (cmpTechnologyManager.IsTechnologyResearched("phase_village"))
				phase = "village";
		}

		// store player ally/neutral/enemy data as arrays
		var allies = [];
		var mutualAllies = [];
		var neutrals = [];
		var enemies = [];
		for (var j = 0; j < n; ++j)
		{
			allies[j] = cmpPlayer.IsAlly(j);
			mutualAllies[j] = cmpPlayer.IsMutualAlly(j);
			neutrals[j] = cmpPlayer.IsNeutral(j);
			enemies[j] = cmpPlayer.IsEnemy(j);
		}
		var playerData = {
			"name": cmpPlayer.GetName(),
			"civ": cmpPlayer.GetCiv(),
			"color": cmpPlayer.GetColor(),
			"popCount": cmpPlayer.GetPopulationCount(),
			"popLimit": cmpPlayer.GetPopulationLimit(),
			"popMax": cmpPlayer.GetMaxPopulation(),
			"heroes": cmpPlayer.GetHeroes(),
			"resourceCounts": cmpPlayer.GetResourceCounts(),
			"trainingBlocked": cmpPlayer.IsTrainingBlocked(),
			"state": cmpPlayer.GetState(),
			"team": cmpPlayer.GetTeam(),
			"teamsLocked": cmpPlayer.GetLockTeams(),
			"cheatsEnabled": cmpPlayer.GetCheatsEnabled(),
			"disabledTemplates": cmpPlayer.GetDisabledTemplates(),
			"phase": phase,
			"isAlly": allies,
			"isMutualAlly": mutualAllies,
			"isNeutral": neutrals,
			"isEnemy": enemies,
			"entityLimits": cmpPlayerEntityLimits ? cmpPlayerEntityLimits.GetLimits() : null,
			"entityCounts": cmpPlayerEntityLimits ? cmpPlayerEntityLimits.GetCounts() : null,
			"entityLimitChangers": cmpPlayerEntityLimits ? cmpPlayerEntityLimits.GetLimitChangers() : null,
			"researchQueued": cmpTechnologyManager ? cmpTechnologyManager.GetQueuedResearch() : null,
			"researchStarted": cmpTechnologyManager ? cmpTechnologyManager.GetStartedResearch() : null,
			"researchedTechs": cmpTechnologyManager ? cmpTechnologyManager.GetResearchedTechs() : null,
			"classCounts": cmpTechnologyManager ? cmpTechnologyManager.GetClassCounts() : null,
			"typeCountsByClass": cmpTechnologyManager ? cmpTechnologyManager.GetTypeCountsByClass() : null
		};
		ret.players.push(playerData);
	}
	
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (cmpRangeManager)
		ret.circularMap = cmpRangeManager.GetLosCircular();

	var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
	if (cmpTerrain)
		ret.mapSize = 4 * cmpTerrain.GetTilesPerSide();

	// Add timeElapsed
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	ret.timeElapsed = cmpTimer.GetTime();
	
	// Add ceasefire info
	var cmpCeasefireManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_CeasefireManager);
	if (cmpCeasefireManager)
	{
		ret.ceasefireActive = cmpCeasefireManager.IsCeasefireActive();
		ret.ceasefireTimeRemaining = cmpCeasefireManager.GetCeasefireStartedTime() + cmpCeasefireManager.GetCeasefireTime() - ret.timeElapsed;
	}
	
	// Add the game type
	var cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	ret.gameType = cmpEndGameManager.GetGameType();

	// Add bartering prices
	var cmpBarter = Engine.QueryInterface(SYSTEM_ENTITY, IID_Barter);
	ret.barterPrices = cmpBarter.GetPrices();

	// Add basic statistics to each player
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var n = cmpPlayerMan.GetNumPlayers();
	for (var i = 0; i < n; ++i)
	{
		var playerEnt = cmpPlayerMan.GetPlayerByID(i);
		var cmpPlayerStatisticsTracker = Engine.QueryInterface(playerEnt, IID_StatisticsTracker);
		if (cmpPlayerStatisticsTracker)
			ret.players[i].statistics = cmpPlayerStatisticsTracker.GetBasicStatistics();
	}

	return ret;
};

/**
 * Returns global information about the current game state, plus statistics.
 * This is used by the GUI at the end of a game, in the summary screen.
 * Note: Amongst statistics, the team exploration map percentage is computed from
 * scratch, so the extended simulation state should not be requested too often.
 */
GuiInterface.prototype.GetExtendedSimulationState = function(player)
{
	// Get basic simulation info
	var ret = this.GetSimulationState();

	// Add statistics to each player
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var n = cmpPlayerMan.GetNumPlayers();
	for (var i = 0; i < n; ++i)
	{
		var playerEnt = cmpPlayerMan.GetPlayerByID(i);
		var cmpPlayerStatisticsTracker = Engine.QueryInterface(playerEnt, IID_StatisticsTracker);
		if (cmpPlayerStatisticsTracker)
			ret.players[i].statistics = cmpPlayerStatisticsTracker.GetStatistics();
	}

	return ret;
};

GuiInterface.prototype.GetRenamedEntities = function(player)
{
	if (this.miragedEntities[player])
		return this.renamedEntities.concat(this.miragedEntities[player]);
	else
		return this.renamedEntities;
};

GuiInterface.prototype.ClearRenamedEntities = function(player)
{
	this.renamedEntities = [];
	this.miragedEntities = [];
};

GuiInterface.prototype.AddMiragedEntity = function(player, entity, mirage)
{
	if (!this.miragedEntities[player])
		this.miragedEntities[player] = [];

	this.miragedEntities[player].push({"entity": entity, "newentity": mirage});
};

/**
 * Get common entity info, often used in the gui
 */
GuiInterface.prototype.GetEntityState = function(player, ent)
{
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);

	// All units must have a template; if not then it's a nonexistent entity id
	var template = cmpTemplateManager.GetCurrentTemplateName(ent);
	if (!template)
		return null;

	var ret = {
		"id": ent,
		"template": template,

		"alertRaiser": null,
		"builder": null,
		"identity": null,
		"fogging": null,
		"foundation": null,
		"garrisonHolder": null,
		"gate": null,
		"guard": null,
		"mirage": null,
		"pack": null,
		"player": -1,
		"position": null,
		"production": null,
		"rallyPoint": null,
		"resourceCarrying": null,
		"rotation": null,
		"trader": null,
		"unitAI": null,
		"visibility": null,
	};

	var cmpMirage = Engine.QueryInterface(ent, IID_Mirage);
	if (cmpMirage)
		ret.mirage = true;

	var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
	if (cmpIdentity)
	{
		ret.identity = {
			"rank": cmpIdentity.GetRank(),
			"classes": cmpIdentity.GetClassesList(),
			"visibleClasses": cmpIdentity.GetVisibleClassesList(),
			"selectionGroupName": cmpIdentity.GetSelectionGroupName()
		};
	}
	
	var cmpPosition = Engine.QueryInterface(ent, IID_Position);
	if (cmpPosition && cmpPosition.IsInWorld())
	{
		ret.position = cmpPosition.GetPosition();
		ret.rotation = cmpPosition.GetRotation();
	}

	var cmpHealth = QueryMiragedInterface(ent, IID_Health);
	if (cmpHealth)
	{
		ret.hitpoints = Math.ceil(cmpHealth.GetHitpoints());
		ret.maxHitpoints = cmpHealth.GetMaxHitpoints();
		ret.needsRepair = cmpHealth.IsRepairable() && (cmpHealth.GetHitpoints() < cmpHealth.GetMaxHitpoints());
		ret.needsHeal = !cmpHealth.IsUnhealable();
	}

	var cmpCapturable = QueryMiragedInterface(ent, IID_Capturable);
	if (cmpCapturable)
	{
		ret.capturePoints = cmpCapturable.GetCapturePoints();
		ret.maxCapturePoints = cmpCapturable.GetMaxCapturePoints();
	}

	var cmpBuilder = Engine.QueryInterface(ent, IID_Builder);
	if (cmpBuilder)
		ret.builder = true;

	var cmpPack = Engine.QueryInterface(ent, IID_Pack);
	if (cmpPack)
	{
		ret.pack = {
			"packed": cmpPack.IsPacked(),
			"progress": cmpPack.GetProgress(),
		};
	}

	var cmpProductionQueue = Engine.QueryInterface(ent, IID_ProductionQueue);
	if (cmpProductionQueue)
	{
		ret.production = {
			"entities": cmpProductionQueue.GetEntitiesList(),
			"technologies": cmpProductionQueue.GetTechnologiesList(),
			"queue": cmpProductionQueue.GetQueue(),
		};
	}

	var cmpTrader = Engine.QueryInterface(ent, IID_Trader);
	if (cmpTrader)
	{
		ret.trader = {
			"goods": cmpTrader.GetGoods(),
			"requiredGoods": cmpTrader.GetRequiredGoods()
		};
	}

	var cmpFogging = Engine.QueryInterface(ent, IID_Fogging);
	if (cmpFogging)
	{
		if (cmpFogging.IsMiraged(player))
			ret.fogging = {"mirage": cmpFogging.GetMirage(player)};
		else
			ret.fogging = {"mirage": null};
	}

	var cmpFoundation = QueryMiragedInterface(ent, IID_Foundation);
	if (cmpFoundation)
	{
		ret.foundation = {
			"progress": cmpFoundation.GetBuildPercentage(),
			"numBuilders": cmpFoundation.GetNumBuilders()
		};
	}

	var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (cmpOwnership)
	{
		ret.player = cmpOwnership.GetOwner();
	}

	var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
	if (cmpRallyPoint)
	{
		ret.rallyPoint = {'position': cmpRallyPoint.GetPositions()[0]}; // undefined or {x,z} object
	}

	var cmpGarrisonHolder = Engine.QueryInterface(ent, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
	{
		ret.garrisonHolder = {
			"entities": cmpGarrisonHolder.GetEntities(),
			"allowedClasses": cmpGarrisonHolder.GetAllowedClasses(),
			"capacity": cmpGarrisonHolder.GetCapacity(),
			"garrisonedEntitiesCount": cmpGarrisonHolder.GetGarrisonedEntitiesCount()
		};
	}

	var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
	if (cmpUnitAI)
	{
		ret.unitAI = {
			"state": cmpUnitAI.GetCurrentState(),
			"orders": cmpUnitAI.GetOrders(),
			"hasWorkOrders": cmpUnitAI.HasWorkOrders(),
			"canGuard": cmpUnitAI.CanGuard(),
			"isGuarding": cmpUnitAI.IsGuardOf(),
			"possibleStances": cmpUnitAI.GetPossibleStances(),
			"isIdle":cmpUnitAI.IsIdle(),
		};
		// Add some information needed for ungarrisoning
		if (cmpUnitAI.IsGarrisoned() && ret.player !== undefined)
			ret.template = "p" + ret.player + "&" + ret.template;
	}

	var cmpGuard = Engine.QueryInterface(ent, IID_Guard);
	if (cmpGuard)
	{
		ret.guard = {
			"entities": cmpGuard.GetEntities(),
		};
	}

	var cmpResourceGatherer = Engine.QueryInterface(ent, IID_ResourceGatherer);
	if (cmpResourceGatherer)
	{
		ret.resourceCarrying = cmpResourceGatherer.GetCarryingStatus();
	}

	var cmpGate = Engine.QueryInterface(ent, IID_Gate);
	if (cmpGate)
	{
		ret.gate = {
			"locked": cmpGate.IsLocked(),
		};
	}

	var cmpAlertRaiser = Engine.QueryInterface(ent, IID_AlertRaiser);
	if (cmpAlertRaiser)
	{
		ret.alertRaiser = {
			"level": cmpAlertRaiser.GetLevel(),
			"canIncreaseLevel": cmpAlertRaiser.CanIncreaseLevel(),
			"hasRaisedAlert": cmpAlertRaiser.HasRaisedAlert(),
		};
	}

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	ret.visibility = cmpRangeManager.GetLosVisibility(ent, player);

	return ret;
};

/**
 * Get additionnal entity info, rarely used in the gui
 */
GuiInterface.prototype.GetExtendedEntityState = function(player, ent)
{
	var ret = {
		"armour": null,
		"attack": null,
		"barterMarket": null,
		"buildingAI": null,
		"healer": null,
		"obstruction": null,
		"turretParent":null,
		"promotion": null,
		"resourceDropsite": null,
		"resourceGatherRates": null,
		"resourceSupply": null,
	};

	var cmpMirage = Engine.QueryInterface(ent, IID_Mirage);

	var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);

	var cmpAttack = Engine.QueryInterface(ent, IID_Attack);
	if (cmpAttack)
	{
		var types = cmpAttack.GetAttackTypes();
		if (types.length)
			ret.attack = {};
		for (var type of types)
		{
			ret.attack[type] = cmpAttack.GetAttackStrengths(type);
			var range = cmpAttack.GetRange(type);
			ret.attack[type].minRange = range.min;
			ret.attack[type].maxRange = range.max;
			var timers = cmpAttack.GetTimers(type);
			ret.attack[type].prepareTime = timers.prepare;
			ret.attack[type].repeatTime = timers.repeat;
			if (type != "Ranged")
			{
				// not a ranged attack, set some defaults
				ret.attack[type].elevationBonus = 0;
				ret.attack[type].elevationAdaptedRange = ret.attack.maxRange;
				continue;
			}

			ret.attack[type].elevationBonus = range.elevationBonus;
			var cmpPosition = Engine.QueryInterface(ent, IID_Position);
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
			if (cmpUnitAI && cmpPosition && cmpPosition.IsInWorld())
			{
				// For units, take the rage in front of it, no spread. So angle = 0
				ret.attack[type].elevationAdaptedRange = cmpRangeManager.GetElevationAdaptedRange(cmpPosition.GetPosition(), cmpPosition.GetRotation(), range.max, range.elevationBonus, 0);
			}
			else if(cmpPosition && cmpPosition.IsInWorld())
			{
				// For buildings, take the average elevation around it. So angle = 2*pi
				ret.attack[type].elevationAdaptedRange = cmpRangeManager.GetElevationAdaptedRange(cmpPosition.GetPosition(), cmpPosition.GetRotation(), range.max, range.elevationBonus, 2*Math.PI);
			}
			else
			{
				// not in world, set a default?
				ret.attack[type].elevationAdaptedRange = ret.attack.maxRange;
			}
		}
	}

	var cmpArmour = Engine.QueryInterface(ent, IID_DamageReceiver);
	if (cmpArmour)
	{
		ret.armour = cmpArmour.GetArmourStrengths();
	}

	var cmpAuras = Engine.QueryInterface(ent, IID_Auras)
	if (cmpAuras)
	{
		ret.auras = cmpAuras.GetDescriptions();
	}

	var cmpBuildingAI = Engine.QueryInterface(ent, IID_BuildingAI);
	if (cmpBuildingAI)
	{
		ret.buildingAI = {
			"defaultArrowCount": cmpBuildingAI.GetDefaultArrowCount(),
			"garrisonArrowMultiplier": cmpBuildingAI.GetGarrisonArrowMultiplier(),
			"garrisonArrowClasses": cmpBuildingAI.GetGarrisonArrowClasses(),
			"arrowCount": cmpBuildingAI.GetArrowCount()
		};
	}

	var cmpObstruction = Engine.QueryInterface(ent, IID_Obstruction);
	if (cmpObstruction)
	{
		ret.obstruction = {
			"controlGroup": cmpObstruction.GetControlGroup(),
			"controlGroup2": cmpObstruction.GetControlGroup2(),
		};
	}

	var cmpPosition = Engine.QueryInterface(ent, IID_Position);
	if (cmpPosition && cmpPosition.GetTurretParent() != INVALID_ENTITY)
		ret.turretParent = cmpPosition.GetTurretParent();

	var cmpResourceSupply = QueryMiragedInterface(ent, IID_ResourceSupply);
	if (cmpResourceSupply)
	{
		ret.resourceSupply = {
			"isInfinite": cmpResourceSupply.IsInfinite(),
			"max": cmpResourceSupply.GetMaxAmount(),
			"amount": cmpResourceSupply.GetCurrentAmount(),
			"type": cmpResourceSupply.GetType(),
			"killBeforeGather": cmpResourceSupply.GetKillBeforeGather(),
			"maxGatherers": cmpResourceSupply.GetMaxGatherers(),
			"numGatherers": cmpResourceSupply.GetNumGatherers()
		};
	}

	var cmpResourceGatherer = Engine.QueryInterface(ent, IID_ResourceGatherer);
	if (cmpResourceGatherer)
	{
		ret.resourceGatherRates = cmpResourceGatherer.GetGatherRates();
	}
	
	var cmpResourceDropsite = Engine.QueryInterface(ent, IID_ResourceDropsite);
	if (cmpResourceDropsite)
	{
		ret.resourceDropsite = {
			"types": cmpResourceDropsite.GetTypes()
		};
	}

	var cmpPromotion = Engine.QueryInterface(ent, IID_Promotion);
	if (cmpPromotion)
	{
		ret.promotion = {
			"curr": cmpPromotion.GetCurrentXp(),
			"req": cmpPromotion.GetRequiredXp()
		};
	}

	var cmpFoundation = Engine.QueryInterface(ent, IID_Foundation);
	if (!cmpFoundation && cmpIdentity && cmpIdentity.HasClass("BarterMarket"))
	{
		var cmpBarter = Engine.QueryInterface(SYSTEM_ENTITY, IID_Barter);
		ret.barterMarket = { "prices": cmpBarter.GetPrices() };
	}

	var cmpHeal = Engine.QueryInterface(ent, IID_Heal);
	if (cmpHeal)
	{
		ret.healer = { 
			"unhealableClasses": cmpHeal.GetUnhealableClasses(),
			"healableClasses": cmpHeal.GetHealableClasses(),
		};
	}

	return ret;
};

GuiInterface.prototype.GetAverageRangeForBuildings = function(player, cmd)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
	var rot = {x:0, y:0, z:0};
	var pos = {x:cmd.x,z:cmd.z};
	pos.y = cmpTerrain.GetGroundLevel(cmd.x, cmd.z);
	var elevationBonus = cmd.elevationBonus || 0;
	var range = cmd.range;

	return cmpRangeManager.GetElevationAdaptedRange(pos, rot, range, elevationBonus, 2*Math.PI);
};

GuiInterface.prototype.GetTemplateData = function(player, extendedName)
{
	var name = extendedName;
	// Special case for garrisoned units which have a extended template
	if (extendedName.indexOf("&") != -1)
		name = extendedName.slice(extendedName.indexOf("&")+1);

	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTemplateManager.GetTemplate(name);

	if (!template)
		return null;

	return GetTemplateDataHelper(template, player);
};

GuiInterface.prototype.GetTechnologyData = function(player, name)
{
	var cmpTechTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TechnologyTemplateManager);
	var template = cmpTechTempMan.GetTemplate(name);

	if (!template)
	{
		warn("Tried to get data for invalid technology: " + name);
		return null;
	}

	var cmpPlayer = QueryPlayerIDInterface(player, IID_Player);
	return GetTechnologyDataHelper(template, cmpPlayer.GetCiv());
};

GuiInterface.prototype.IsTechnologyResearched = function(player, tech)
{
	if (!tech)
		return true;

	var cmpTechnologyManager = QueryPlayerIDInterface(player, IID_TechnologyManager);
	
	if (!cmpTechnologyManager)
		return false;

	return cmpTechnologyManager.IsTechnologyResearched(tech);
};

// Checks whether the requirements for this technology have been met
GuiInterface.prototype.CheckTechnologyRequirements = function(player, tech)
{
	var cmpTechnologyManager = QueryPlayerIDInterface(player, IID_TechnologyManager);

	if (!cmpTechnologyManager)
		return false;

	return cmpTechnologyManager.CanResearch(tech);
};

// Returns technologies that are being actively researched, along with
// which entity is researching them and how far along the research is.
GuiInterface.prototype.GetStartedResearch = function(player)
{
	var cmpTechnologyManager = QueryPlayerIDInterface(player, IID_TechnologyManager);
	if (!cmpTechnologyManager)
		return false;

	var ret = {};
	for (var tech in cmpTechnologyManager.GetTechsStarted())
	{
		ret[tech] = { "researcher": cmpTechnologyManager.GetResearcher(tech) };
		var cmpProductionQueue = Engine.QueryInterface(ret[tech].researcher, IID_ProductionQueue);
		if (cmpProductionQueue)
			ret[tech].progress = cmpProductionQueue.GetQueue()[0].progress;
		else
			ret[tech].progress = 0;
	}
	return ret;
};

// Returns the battle state of the player.
GuiInterface.prototype.GetBattleState = function(player)
{
	var cmpBattleDetection = QueryPlayerIDInterface(player, IID_BattleDetection);
	if (cmpBattleDetection)
		return cmpBattleDetection.GetState();
	else
		return false;
};

// Returns a list of ongoing attacks against the player.
GuiInterface.prototype.GetIncomingAttacks = function(player)
{
	var cmpAttackDetection = QueryPlayerIDInterface(player, IID_AttackDetection);
	return cmpAttackDetection.GetIncomingAttacks();
};

// Used to show a red square over GUI elements you can't yet afford.
GuiInterface.prototype.GetNeededResources = function(player, amounts)
{
	var cmpPlayer = QueryPlayerIDInterface(player, IID_Player);
	return cmpPlayer.GetNeededResources(amounts);
};

/**
 * Add a timed notification.
 * Warning: timed notifacations are serialised
 * (to also display them on saved games or after a rejoin)
 * so they should allways be added and deleted in a deterministic way.
 */
GuiInterface.prototype.AddTimeNotification = function(notification, duration = 10000)
{
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	notification.endTime = duration + cmpTimer.GetTime();
	notification.id = ++this.timeNotificationID;

	// Let all players and observers receive the notification by default
	if (notification.players == undefined)
	{
		var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
		var numPlayers = cmpPlayerManager.GetNumPlayers();
		notification.players = [-1];
		for (var i = 1; i < numPlayers; ++i)
			notification.players.push(i);
	}
	this.timeNotifications.push(notification);
	this.timeNotifications.sort(function (n1, n2){return n2.endTime - n1.endTime});

	cmpTimer.SetTimeout(this.entity, IID_GuiInterface, "DeleteTimeNotification", duration, this.timeNotificationID);

	return this.timeNotificationID;
};

GuiInterface.prototype.DeleteTimeNotification = function(notificationID)
{
	this.timeNotifications = this.timeNotifications.filter(n => n.id != notificationID);
};

GuiInterface.prototype.GetTimeNotifications = function(playerID)
{
	var time = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime();
	// filter on players and time, since the delete timer might be executed with a delay
	return this.timeNotifications.filter(n => n.players.indexOf(playerID) != -1 && n.endTime > time);
};

GuiInterface.prototype.PushNotification = function(notification)
{
	if (!notification.type || notification.type == "text")
		this.AddTimeNotification(notification);
	else
		this.notifications.push(notification);
};

GuiInterface.prototype.GetNotifications = function()
{
	var n = this.notifications;
	this.notifications = [];
	return n;
};

GuiInterface.prototype.GetAvailableFormations = function(player, wantedPlayer)
{
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerMan.GetPlayerByID(wantedPlayer), IID_Player);
	return cmpPlayer.GetFormations();
};

GuiInterface.prototype.GetFormationRequirements = function(player, data)
{
	return GetFormationRequirements(data.formationTemplate);
};

GuiInterface.prototype.CanMoveEntsIntoFormation = function(player, data)
{
	return CanMoveEntsIntoFormation(data.ents, data.formationTemplate);
};

GuiInterface.prototype.GetFormationInfoFromTemplate = function(player, data)
{
	var r = {};
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTemplateManager.GetTemplate(data.templateName);
	if (!template || !template.Formation)
		return r;
	r.name = template.Formation.FormationName;
	r.tooltip = template.Formation.DisabledTooltip || "";
	r.icon = template.Formation.Icon;
	return r;
};

GuiInterface.prototype.IsFormationSelected = function(player, data)
{
	for each (var ent in data.ents)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (cmpUnitAI)
		{
			// GetLastFormationName is named in a strange way as it (also) is
			// the value of the current formation (see Formation.js LoadFormation)
			if (cmpUnitAI.GetLastFormationTemplate() == data.formationTemplate)
				return true;
		}
	}
	return false;
};

GuiInterface.prototype.IsStanceSelected = function(player, data)
{
	for each (var ent in data.ents)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (cmpUnitAI)
		{
			if (cmpUnitAI.GetStanceName() == data.stance)
				return true;
		}
	}
	return false;
};

GuiInterface.prototype.GetAllBuildableEntities = function(player, cmd)
{
	var buildableEnts = [];
	for each (var ent in cmd.entities)
	{
		var cmpBuilder = Engine.QueryInterface(ent, IID_Builder);
		if (!cmpBuilder)
			continue;

		for (var building of cmpBuilder.GetEntitiesList())
			if (buildableEnts.indexOf(building) == -1)
				buildableEnts.push(building);
	}
	return buildableEnts;
};

GuiInterface.prototype.SetSelectionHighlight = function(player, cmd)
{
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var playerColors = {}; // cache of owner -> color map
	
	for each (var ent in cmd.entities)
	{
		var cmpSelectable = Engine.QueryInterface(ent, IID_Selectable);
		if (!cmpSelectable)
			continue;

		// Find the entity's owner's color:
		var owner = -1;
		var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (cmpOwnership)
			owner = cmpOwnership.GetOwner();

		var color = playerColors[owner];
		if (!color)
		{
			color = {"r":1, "g":1, "b":1};
			var cmpPlayer = Engine.QueryInterface(cmpPlayerMan.GetPlayerByID(owner), IID_Player);
			if (cmpPlayer)
				color = cmpPlayer.GetColor();
			playerColors[owner] = color;
		}

		cmpSelectable.SetSelectionHighlight({"r":color.r, "g":color.g, "b":color.b, "a":cmd.alpha}, cmd.selected);
	}
};

GuiInterface.prototype.GetEntitiesWithStatusBars = function()
{
	return [...this.entsWithAuraAndStatusBars];
};

GuiInterface.prototype.SetStatusBars = function(player, cmd)
{
	let affectedEnts = new Set();
	for (let ent of cmd.entities)
	{
		let cmpStatusBars = Engine.QueryInterface(ent, IID_StatusBars);
		if (!cmpStatusBars)
			continue;
		cmpStatusBars.SetEnabled(cmd.enabled);

		let cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		if (!cmpAuras)
			continue;

		for (let name of cmpAuras.GetAuraNames())
		{
			if (!cmpAuras.GetOverlayIcon(name))
				continue;
			for (let e of cmpAuras.GetAffectedEntities(name))
				affectedEnts.add(e);
			if (cmd.enabled)
				this.entsWithAuraAndStatusBars.add(ent);
			else
				this.entsWithAuraAndStatusBars.delete(ent);
		}
	}

	for (let ent of affectedEnts)
	{
		let cmpStatusBars = Engine.QueryInterface(ent, IID_StatusBars)
		if (cmpStatusBars)
			cmpStatusBars.RegenerateSprites();
	}
};

GuiInterface.prototype.GetPlayerEntities = function(player)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return cmpRangeManager.GetEntitiesByPlayer(player);
};

/**
 * Displays the rally points of a given list of entities (carried in cmd.entities).
 * 
 * The 'cmd' object may carry its own x/z coordinate pair indicating the location where the rally point should 
 * be rendered, in order to support instantaneously rendering a rally point marker at a specified location 
 * instead of incurring a delay while PostNetworkCommand processes the set-rallypoint command (see input.js).
 * If cmd doesn't carry a custom location, then the position to render the marker at will be read from the 
 * RallyPoint component.
 */
GuiInterface.prototype.DisplayRallyPoint = function(player, cmd)
{
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerMan.GetPlayerByID(player), IID_Player);

	// If there are some rally points already displayed, first hide them
	for each (var ent in this.entsRallyPointsDisplayed)
	{
		var cmpRallyPointRenderer = Engine.QueryInterface(ent, IID_RallyPointRenderer);
		if (cmpRallyPointRenderer)
			cmpRallyPointRenderer.SetDisplayed(false);
	}
	
	this.entsRallyPointsDisplayed = [];
	
	// Show the rally points for the passed entities
	for each (var ent in cmd.entities)
	{
		var cmpRallyPointRenderer = Engine.QueryInterface(ent, IID_RallyPointRenderer);
		if (!cmpRallyPointRenderer)
			continue;
		
		// entity must have a rally point component to display a rally point marker
		// (regardless of whether cmd specifies a custom location)
		var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
		if (!cmpRallyPoint)
			continue;

		// Verify the owner
		var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (!(cmpPlayer && cmpPlayer.CanControlAllUnits()))
			if (!cmpOwnership || cmpOwnership.GetOwner() != player)
				continue;

		// If the command was passed an explicit position, use that and
		// override the real rally point position; otherwise use the real position
		var pos;
		if (cmd.x && cmd.z)
			pos = cmd; 
		else
			pos = cmpRallyPoint.GetPositions()[0]; // may return undefined if no rally point is set

		if (pos)
		{
			// Only update the position if we changed it (cmd.queued is set)
			if ("queued" in cmd)
				if (cmd.queued == true)
					cmpRallyPointRenderer.AddPosition({'x': pos.x, 'y': pos.z}); // AddPosition takes a CFixedVector2D which has X/Y components, not X/Z
				else
					cmpRallyPointRenderer.SetPosition({'x': pos.x, 'y': pos.z}); // SetPosition takes a CFixedVector2D which has X/Y components, not X/Z
			// rebuild the renderer when not set (when reading saved game or in case of building update)
			else if (!cmpRallyPointRenderer.IsSet())
				for each (var posi in cmpRallyPoint.GetPositions())
					cmpRallyPointRenderer.AddPosition({'x': posi.x, 'y': posi.z});

			cmpRallyPointRenderer.SetDisplayed(true);
			
			// remember which entities have their rally points displayed so we can hide them again
			this.entsRallyPointsDisplayed.push(ent);
		}
	}
};

/**
 * Display the building placement preview.
 * cmd.template is the name of the entity template, or "" to disable the preview.
 * cmd.x, cmd.z, cmd.angle give the location.
 * 
 * Returns result object from CheckPlacement:
 * 	{
 *		"success":             true iff the placement is valid, else false
 *		"message":             message to display in UI for invalid placement, else ""
 *		"parameters":          parameters to use in the message
 *		"translateMessage":    localisation info
 *		"translateParameters": localisation info
 *  }
 */
GuiInterface.prototype.SetBuildingPlacementPreview = function(player, cmd)
{
	var result = {
		"success": false,
		"message": "",
		"parameters": {},
		"translateMessage": false,
		"translateParameters": [],
	}

	// See if we're changing template
	if (!this.placementEntity || this.placementEntity[0] != cmd.template)
	{
		// Destroy the old preview if there was one
		if (this.placementEntity)
			Engine.DestroyEntity(this.placementEntity[1]);

		// Load the new template
		if (cmd.template == "")
		{
			this.placementEntity = undefined;
		}
		else
		{
			this.placementEntity = [cmd.template, Engine.AddLocalEntity("preview|" + cmd.template)];
		}
	}

	if (this.placementEntity)
	{
		var ent = this.placementEntity[1];

		// Move the preview into the right location
		var pos = Engine.QueryInterface(ent, IID_Position);
		if (pos)
		{
			pos.JumpTo(cmd.x, cmd.z);
			pos.SetYRotation(cmd.angle);
		}

		var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		cmpOwnership.SetOwner(player);

		// Check whether building placement is valid
		var cmpBuildRestrictions = Engine.QueryInterface(ent, IID_BuildRestrictions);
		if (!cmpBuildRestrictions)
			error("cmpBuildRestrictions not defined");
		else
			result = cmpBuildRestrictions.CheckPlacement();

		// Set it to a red shade if this is an invalid location
		var cmpVisual = Engine.QueryInterface(ent, IID_Visual);
		if (cmpVisual)
		{
			if (cmd.actorSeed !== undefined)
				cmpVisual.SetActorSeed(cmd.actorSeed);

			if (!result.success)
				cmpVisual.SetShadingColor(1.4, 0.4, 0.4, 1);
			else
				cmpVisual.SetShadingColor(1, 1, 1, 1);
		}
	}

	return result;
};

/**
 * Previews the placement of a wall between cmd.start and cmd.end, or just the starting piece of a wall if cmd.end is not 
 * specified. Returns an object with information about the list of entities that need to be newly constructed to complete
 * at least a part of the wall, or false if there are entities required to build at least part of the wall but none of
 * them can be validly constructed.
 * 
 * It's important to distinguish between three lists of entities that are at play here, because they may be subsets of one
 * another depending on things like snapping and whether some of the entities inside them can be validly positioned.
 * We have:
 *    - The list of entities that previews the wall. This list is usually equal to the entities required to construct the
 *      entire wall. However, if there is snapping to an incomplete tower (i.e. a foundation), it includes extra entities
 *      to preview the completed tower on top of its foundation.
 *      
 *    - The list of entities that need to be newly constructed to build the entire wall. This list is regardless of whether
 *      any of them can be validly positioned. The emphasishere here is on 'newly'; this list does not include any existing
 *      towers at either side of the wall that we snapped to. Or, more generally; it does not include any _entities_ that we
 *      snapped to; we might still snap to e.g. terrain, in which case the towers on either end will still need to be newly
 *      constructed.
 *      
 *    - The list of entities that need to be newly constructed to build at least a part of the wall. This list is the same
 *      as the one above, except that it is truncated at the first entity that cannot be validly positioned. This happens
 *      e.g. if the player tries to build a wall straight through an obstruction. Note that any entities that can be validly
 *      constructed but come after said first invalid entity are also truncated away.
 * 
 * With this in mind, this method will return false if the second list is not empty, but the third one is. That is, if there
 * were entities that are needed to build the wall, but none of them can be validly constructed. False is also returned in
 * case of unexpected errors (typically missing components), and when clearing the preview by passing an empty wallset
 * argument (see below). Otherwise, it will return an object with the following information:
 * 
 * result: {
 *   'startSnappedEnt':   ID of the entity that we snapped to at the starting side of the wall. Currently only supports towers.
 *   'endSnappedEnt':     ID of the entity that we snapped to at the (possibly truncated) ending side of the wall. Note that this
 *                        can only be set if no truncation of the second list occurs; if we snapped to an entity at the ending side
 *                        but the wall construction was truncated before we could reach it, it won't be set here. Currently only
 *                        supports towers.
 *   'pieces':            Array with the following data for each of the entities in the third list:
 *    [{
 *       'template':      Template name of the entity. 
 *       'x':             X coordinate of the entity's position.
 *       'z':             Z coordinate of the entity's position.
 *       'angle':         Rotation around the Y axis of the entity (in radians).
 *     },
 *     ...]
 *   'cost': {            The total cost required for constructing all the pieces as listed above.
 *     'food': ...,
 *     'wood': ...,
 *     'stone': ...,
 *     'metal': ...,
 *     'population': ...,
 *     'populationBonus': ...,
 *   }
 * }
 * 
 * @param cmd.wallSet Object holding the set of wall piece template names. Set to an empty value to clear the preview.
 * @param cmd.start Starting point of the wall segment being created.
 * @param cmd.end (Optional) Ending point of the wall segment being created. If not defined, it is understood that only
 *                 the starting point of the wall is available at this time (e.g. while the player is still in the process
 *                 of picking a starting point), and that therefore only the first entity in the wall (a tower) should be
 *                 previewed.
 * @param cmd.snapEntities List of candidate entities to snap the start and ending positions to.
 */
GuiInterface.prototype.SetWallPlacementPreview = function(player, cmd)
{
	var wallSet = cmd.wallSet;
	
	var start = {
		"pos": cmd.start,
		"angle": 0,
		"snapped": false,                       // did the start position snap to anything?
		"snappedEnt": INVALID_ENTITY,           // if we snapped, was it to an entity? if yes, holds that entity's ID
	};
	
	var end = {
		"pos": cmd.end,
		"angle": 0,
		"snapped": false,                       // did the start position snap to anything?
		"snappedEnt": INVALID_ENTITY,           // if we snapped, was it to an entity? if yes, holds that entity's ID
	};
	
	// --------------------------------------------------------------------------------
	// do some entity cache management and check for snapping
	
	if (!this.placementWallEntities)
		this.placementWallEntities = {};
	
	if (!wallSet)
	{
		// we're clearing the preview, clear the entity cache and bail
		var numCleared = 0;
		for (var tpl in this.placementWallEntities)
		{
			for each (var ent in this.placementWallEntities[tpl].entities)
				Engine.DestroyEntity(ent);
			
			this.placementWallEntities[tpl].numUsed = 0;
			this.placementWallEntities[tpl].entities = [];
			// keep template data around
		}
		
		return false;
	}
	else
	{
		// Move all existing cached entities outside of the world and reset their use count
		for (var tpl in this.placementWallEntities)
		{
			for each (var ent in this.placementWallEntities[tpl].entities)
			{
				var pos = Engine.QueryInterface(ent, IID_Position);
				if (pos)
					pos.MoveOutOfWorld();
			}
			
			this.placementWallEntities[tpl].numUsed = 0;
		}
		
		// Create cache entries for templates we haven't seen before
		for each (var tpl in wallSet.templates)
		{
			if (!(tpl in this.placementWallEntities))
			{
				this.placementWallEntities[tpl] = {
					"numUsed": 0,
					"entities": [],
					"templateData": this.GetTemplateData(player, tpl),
				};
				
				// ensure that the loaded template data contains a wallPiece component
				if (!this.placementWallEntities[tpl].templateData.wallPiece)
				{
					error("[SetWallPlacementPreview] No WallPiece component found for wall set template '" + tpl + "'");
					return false;
				}
			}
		}
	}
	
	// prevent division by zero errors further on if the start and end positions are the same
	if (end.pos && (start.pos.x === end.pos.x && start.pos.z === end.pos.z))
		end.pos = undefined;
	
	// See if we need to snap the start and/or end coordinates to any of our list of snap entities. Note that, despite the list
	// of snapping candidate entities, it might still snap to e.g. terrain features. Use the "ent" key in the returned snapping
	// data to determine whether it snapped to an entity (if any), and to which one (see GetFoundationSnapData).
	if (cmd.snapEntities)
	{
		var snapRadius = this.placementWallEntities[wallSet.templates.tower].templateData.wallPiece.length * 0.5; // determined through trial and error
		var startSnapData = this.GetFoundationSnapData(player, {
			"x": start.pos.x,
			"z": start.pos.z,
			"template": wallSet.templates.tower,
			"snapEntities": cmd.snapEntities,
			"snapRadius": snapRadius,
		});
		
		if (startSnapData)
		{
			start.pos.x = startSnapData.x;
			start.pos.z = startSnapData.z;
			start.angle = startSnapData.angle;
			start.snapped = true;
			
			if (startSnapData.ent)
				start.snappedEnt = startSnapData.ent; 
		}
		
		if (end.pos)
		{
			var endSnapData = this.GetFoundationSnapData(player, {
				"x": end.pos.x,
				"z": end.pos.z,
				"template": wallSet.templates.tower,
				"snapEntities": cmd.snapEntities,
				"snapRadius": snapRadius,
			});
			
			if (endSnapData)
			{
				end.pos.x = endSnapData.x;
				end.pos.z = endSnapData.z;
				end.angle = endSnapData.angle;
				end.snapped = true;
				
				if (endSnapData.ent)
					end.snappedEnt = endSnapData.ent;
			}
		}
	}
	
	// clear the single-building preview entity (we'll be rolling our own)
	this.SetBuildingPlacementPreview(player, {"template": ""});
	
	// --------------------------------------------------------------------------------
	// calculate wall placement and position preview entities
	
	var result = {
		"pieces": [],
		"cost": {"food": 0, "wood": 0, "stone": 0, "metal": 0, "population": 0, "populationBonus": 0, "time": 0},
	};
	
	var previewEntities = [];
	if (end.pos)
		previewEntities = GetWallPlacement(this.placementWallEntities, wallSet, start, end); // see helpers/Walls.js
	
	// For wall placement, we may (and usually do) need to have wall pieces overlap each other more than would 
	// otherwise be allowed by their obstruction shapes. However, during this preview phase, this is not so much of
	// an issue, because all preview entities have their obstruction components deactivated, meaning that their 
	// obstruction shapes do not register in the simulation and hence cannot affect it. This implies that the preview
	// entities cannot be found to obstruct each other, which largely solves the issue of overlap between wall pieces.
	
	// Note that they will still be obstructed by existing shapes in the simulation (that have the BLOCK_FOUNDATION
	// flag set), which is what we want. The only exception to this is when snapping to existing towers (or
	// foundations thereof); the wall segments that connect up to these will be found to be obstructed by the
	// existing tower/foundation, and be shaded red to indicate that they cannot be placed there. To prevent this,
	// we manually set the control group of the outermost wall pieces equal to those of the snapped-to towers, so
	// that they are free from mutual obstruction (per definition of obstruction control groups). This is done by
	// assigning them an extra "controlGroup" field, which we'll then set during the placement loop below.
	
	// Additionally, in the situation that we're snapping to merely a foundation of a tower instead of a fully
	// constructed one, we'll need an extra preview entity for the starting tower, which also must not be obstructed
	// by the foundation it snaps to.
	
	if (start.snappedEnt && start.snappedEnt != INVALID_ENTITY)
	{
		var startEntObstruction = Engine.QueryInterface(start.snappedEnt, IID_Obstruction);
		if (previewEntities.length > 0 && startEntObstruction)
			previewEntities[0].controlGroups = [startEntObstruction.GetControlGroup()];
		
		// if we're snapping to merely a foundation, add an extra preview tower and also set it to the same control group
		var startEntState = this.GetEntityState(player, start.snappedEnt);
		if (startEntState.foundation)
		{
			var cmpPosition = Engine.QueryInterface(start.snappedEnt, IID_Position);
			if (cmpPosition)
			{
				previewEntities.unshift({
					"template": wallSet.templates.tower,
					"pos": start.pos,
					"angle": cmpPosition.GetRotation().y,
					"controlGroups": [(startEntObstruction ? startEntObstruction.GetControlGroup() : undefined)],
					"excludeFromResult": true, // preview only, must not appear in the result
				});
			}
		}
	}
	else
	{
		// Didn't snap to an existing entity, add the starting tower manually. To prevent odd-looking rotation jumps 
		// when shift-clicking to build a wall, reuse the placement angle that was last seen on a validly positioned 
		// wall piece.
		
		// To illustrate the last point, consider what happens if we used some constant instead, say, 0. Issuing the
		// build command for a wall is asynchronous, so when the preview updates after shift-clicking, the wall piece
		// foundations are not registered yet in the simulation. This means they cannot possibly be picked in the list
		// of candidate entities for snapping. In the next preview update, we therefore hit this case, and would rotate
		// the preview to 0 radians. Then, after one or two simulation updates or so, the foundations register and 
		// onSimulationUpdate in session.js updates the preview again. It first grabs a new list of snapping candidates,
		// which this time does include the new foundations; so we snap to the entity, and rotate the preview back to
		// the foundation's angle.
		
		// The result is a noticeable rotation to 0 and back, which is undesirable. So, for a split second there until
		// the simulation updates, we fake it by reusing the last angle and hope the player doesn't notice.
		previewEntities.unshift({
			"template": wallSet.templates.tower,
			"pos": start.pos,
			"angle": (previewEntities.length > 0 ? previewEntities[0].angle : this.placementWallLastAngle)
		});
	}
	
	if (end.pos)
	{
		// Analogous to the starting side case above
		if (end.snappedEnt && end.snappedEnt != INVALID_ENTITY)
		{
			var endEntObstruction = Engine.QueryInterface(end.snappedEnt, IID_Obstruction);
			
			// Note that it's possible for the last entity in previewEntities to be the same as the first, i.e. the
			// same wall piece snapping to both a starting and an ending tower. And it might be more common than you would
			// expect; the allowed overlap between wall segments and towers facilitates this to some degree. To deal with
			// the possibility of dual initial control groups, we use a '.controlGroups' array rather than a single
			// '.controlGroup' property. Note that this array can only ever have 0, 1 or 2 elements (checked at a later time).
			if (previewEntities.length > 0 && endEntObstruction)
			{
				previewEntities[previewEntities.length-1].controlGroups = (previewEntities[previewEntities.length-1].controlGroups || []);
				previewEntities[previewEntities.length-1].controlGroups.push(endEntObstruction.GetControlGroup());
			}
			
			// if we're snapping to a foundation, add an extra preview tower and also set it to the same control group
			var endEntState = this.GetEntityState(player, end.snappedEnt);
			if (endEntState.foundation)
			{
				var cmpPosition = Engine.QueryInterface(end.snappedEnt, IID_Position);
				if (cmpPosition)
				{
					previewEntities.push({
						"template": wallSet.templates.tower,
						"pos": end.pos,
						"angle": cmpPosition.GetRotation().y,
						"controlGroups": [(endEntObstruction ? endEntObstruction.GetControlGroup() : undefined)],
						"excludeFromResult": true
					});
				}
			}
		}
		else
		{
			previewEntities.push({
				"template": wallSet.templates.tower,
				"pos": end.pos,
				"angle": (previewEntities.length > 0 ? previewEntities[previewEntities.length-1].angle : this.placementWallLastAngle)
			});
		}
	}
	
	var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
	if (!cmpTerrain)
	{
		error("[SetWallPlacementPreview] System Terrain component not found");
		return false;
	}
	
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (!cmpRangeManager)
	{
		error("[SetWallPlacementPreview] System RangeManager component not found");
		return false;
	}
	
	// Loop through the preview entities, and construct the subset of them that need to be, and can be, validly constructed
	// to build at least a part of the wall (meaning that the subset is truncated after the first entity that needs to be,
	// but cannot validly be, constructed). See method-level documentation for more details.
	
	var allPiecesValid = true;
	var numRequiredPieces = 0; // number of entities that are required to build the entire wall, regardless of validity
	
	for (var i = 0; i < previewEntities.length; ++i)
	{
		var entInfo = previewEntities[i];
		
		var ent = null;
		var tpl = entInfo.template;
		var tplData = this.placementWallEntities[tpl].templateData;
		var entPool = this.placementWallEntities[tpl];
		
		if (entPool.numUsed >= entPool.entities.length)
		{
			// allocate new entity
			ent = Engine.AddLocalEntity("preview|" + tpl);
			entPool.entities.push(ent);
		}
		else
		{
			 // reuse an existing one
			ent = entPool.entities[entPool.numUsed];
		}
		
		if (!ent)
		{
			error("[SetWallPlacementPreview] Failed to allocate or reuse preview entity of template '" + tpl + "'");
			continue;
		}
		
		// move piece to right location
		// TODO: consider reusing SetBuildingPlacementReview for this, enhanced to be able to deal with multiple entities
		var cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (cmpPosition)
		{
			cmpPosition.JumpTo(entInfo.pos.x, entInfo.pos.z);
			cmpPosition.SetYRotation(entInfo.angle);
			
			// if this piece is a tower, then it should have a Y position that is at least as high as its surrounding pieces
			if (tpl === wallSet.templates.tower)
			{
				var terrainGroundPrev = null;
				var terrainGroundNext = null;
				
				if (i > 0)
					terrainGroundPrev = cmpTerrain.GetGroundLevel(previewEntities[i-1].pos.x, previewEntities[i-1].pos.z);
				if (i < previewEntities.length - 1)
					terrainGroundNext = cmpTerrain.GetGroundLevel(previewEntities[i+1].pos.x, previewEntities[i+1].pos.z);
				
				if (terrainGroundPrev != null || terrainGroundNext != null)
				{
					var targetY = Math.max(terrainGroundPrev, terrainGroundNext);
					cmpPosition.SetHeightFixed(targetY);
				}
			}
		}
		
		var cmpObstruction = Engine.QueryInterface(ent, IID_Obstruction);
		if (!cmpObstruction)
		{
			error("[SetWallPlacementPreview] Preview entity of template '" + tpl + "' does not have an Obstruction component");
			continue;
		}
		
		// Assign any predefined control groups. Note that there can only be 0, 1 or 2 predefined control groups; if there are
		// more, we've made a programming error. The control groups are assigned from the entInfo.controlGroups array on a
		// first-come first-served basis; the first value in the array is always assigned as the primary control group, and
		// any second value as the secondary control group.
		
		// By default, we reset the control groups to their standard values. Remember that we're reusing entities; if we don't
		// reset them, then an ending wall segment that was e.g. at one point snapped to an existing tower, and is subsequently
		// reused as a non-snapped ending wall segment, would no longer be capable of being obstructed by the same tower it was
		// once snapped to.
		
		var primaryControlGroup = ent;
		var secondaryControlGroup = INVALID_ENTITY;
		
		if (entInfo.controlGroups && entInfo.controlGroups.length > 0)
		{
			if (entInfo.controlGroups.length > 2)
			{
				error("[SetWallPlacementPreview] Encountered preview entity of template '" + tpl + "' with more than 2 initial control groups");
				break;
			}
			
			primaryControlGroup = entInfo.controlGroups[0];
			if (entInfo.controlGroups.length > 1)
				secondaryControlGroup = entInfo.controlGroups[1];
		}
		
		cmpObstruction.SetControlGroup(primaryControlGroup);
		cmpObstruction.SetControlGroup2(secondaryControlGroup);
		
		// check whether this wall piece can be validly positioned here
		var validPlacement = false;
		
		var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		cmpOwnership.SetOwner(player);
		
		// Check whether it's in a visible or fogged region
		// TODO: should definitely reuse SetBuildingPlacementPreview, this is just straight up copy/pasta
		var visible = (cmpRangeManager.GetLosVisibility(ent, player) != "hidden");
		if (visible)
		{
			var cmpBuildRestrictions = Engine.QueryInterface(ent, IID_BuildRestrictions);
			if (!cmpBuildRestrictions)
			{
				error("[SetWallPlacementPreview] cmpBuildRestrictions not defined for preview entity of template '" + tpl + "'");
				continue;
			}
			
			// TODO: Handle results of CheckPlacement
			validPlacement = (cmpBuildRestrictions && cmpBuildRestrictions.CheckPlacement().success);

			// If a wall piece has two control groups, it's likely a segment that spans
			// between two existing towers. To avoid placing a duplicate wall segment,
			// check for collisions with entities that share both control groups.
			if (validPlacement && entInfo.controlGroups && entInfo.controlGroups.length > 1)
				validPlacement = cmpObstruction.CheckDuplicateFoundation();
		}

		allPiecesValid = allPiecesValid && validPlacement;
		
		// The requirement below that all pieces so far have to have valid positions, rather than only this single one,
		// ensures that no more foundations will be placed after a first invalidly-positioned piece. (It is possible
		// for pieces past some invalidly-positioned ones to still have valid positions, e.g. if you drag a wall 
		// through and past an existing building).
		
		// Additionally, the excludeFromResult flag is set for preview entities that were manually added to be placed
		// on top of foundations of incompleted towers that we snapped to; they must not be part of the result.
		
		if (!entInfo.excludeFromResult)
			numRequiredPieces++;
		
		if (allPiecesValid && !entInfo.excludeFromResult)
		{
			result.pieces.push({
				"template": tpl,
				"x": entInfo.pos.x,
				"z": entInfo.pos.z,
				"angle": entInfo.angle,
			});
			this.placementWallLastAngle = entInfo.angle;
			
			// grab the cost of this wall piece and add it up (note; preview entities don't have their Cost components
			// copied over, so we need to fetch it from the template instead).
			// TODO: we should really use a Cost object or at least some utility functions for this, this is mindless
			// boilerplate that's probably duplicated in tons of places.
			result.cost.food += tplData.cost.food;
			result.cost.wood += tplData.cost.wood;
			result.cost.stone += tplData.cost.stone;
			result.cost.metal += tplData.cost.metal;
			result.cost.population += tplData.cost.population;
			result.cost.populationBonus += tplData.cost.populationBonus;
			result.cost.time += tplData.cost.time;
		}

		var canAfford = true;
		var cmpPlayer = QueryPlayerIDInterface(player, IID_Player);
		if (cmpPlayer && cmpPlayer.GetNeededResources(result.cost))
			var canAfford = false;

		var cmpVisual = Engine.QueryInterface(ent, IID_Visual);
		if (cmpVisual)
		{
			if (!allPiecesValid || !canAfford)
				cmpVisual.SetShadingColor(1.4, 0.4, 0.4, 1);
			else
				cmpVisual.SetShadingColor(1, 1, 1, 1);
		}

		entPool.numUsed++;
	}
	
	// If any were entities required to build the wall, but none of them could be validly positioned, return failure
	// (see method-level documentation).
	if (numRequiredPieces > 0 && result.pieces.length == 0)
		return false;
	
	if (start.snappedEnt && start.snappedEnt != INVALID_ENTITY)
		result.startSnappedEnt = start.snappedEnt;
	
	// We should only return that we snapped to an entity if all pieces up until that entity can be validly constructed,
	// i.e. are included in result.pieces (see docs for the result object).
	if (end.pos && end.snappedEnt && end.snappedEnt != INVALID_ENTITY && allPiecesValid)
		result.endSnappedEnt = end.snappedEnt;
	
	return result;
};

/**
 * Given the current position {data.x, data.z} of an foundation of template data.template, returns the position and angle to snap
 * it to (if necessary/useful).
 * 
 * @param data.x            The X position of the foundation to snap.
 * @param data.z            The Z position of the foundation to snap.
 * @param data.template     The template to get the foundation snapping data for.
 * @param data.snapEntities Optional; list of entity IDs to snap to if {data.x, data.z} is within a circle of radius data.snapRadius
 *                            around the entity. Only takes effect when used in conjunction with data.snapRadius.
 *                          When this option is used and the foundation is found to snap to one of the entities passed in this list
 *                            (as opposed to e.g. snapping to terrain features), then the result will contain an additional key "ent",
 *                            holding the ID of the entity that was snapped to.
 * @param data.snapRadius   Optional; when used in conjunction with data.snapEntities, indicates the circle radius around an entity that
 *                            {data.x, data.z} must be located within to have it snap to that entity.
 */
GuiInterface.prototype.GetFoundationSnapData = function(player, data)
{
	var cmpTemplateMgr = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTemplateMgr.GetTemplate(data.template);

	if (!template)
	{
		warn("[GetFoundationSnapData] Failed to load template '" + data.template + "'");
		return false;
	}
	
	if (data.snapEntities && data.snapRadius && data.snapRadius > 0)
	{
		// see if {data.x, data.z} is inside the snap radius of any of the snap entities; and if so, to which it is closest
		// (TODO: break unlikely ties by choosing the lowest entity ID)
		
		var minDist2 = -1;
		var minDistEntitySnapData = null;
		var radius2 = data.snapRadius * data.snapRadius;
		
		for each (var ent in data.snapEntities)
		{
			var cmpPosition = Engine.QueryInterface(ent, IID_Position);
			if (!cmpPosition || !cmpPosition.IsInWorld())
				continue;
			
			var pos = cmpPosition.GetPosition();
			var dist2 = (data.x - pos.x) * (data.x - pos.x) + (data.z - pos.z) * (data.z - pos.z);
			if (dist2 > radius2)
				continue;
			
			if (minDist2 < 0 || dist2 < minDist2)
			{
				minDist2 = dist2;
				minDistEntitySnapData = {"x": pos.x, "z": pos.z, "angle": cmpPosition.GetRotation().y, "ent": ent};
			}
		}
		
		if (minDistEntitySnapData != null)
			return minDistEntitySnapData;
	}

	if (template.BuildRestrictions.Category == "Dock")
	{
		var angle = GetDockAngle(template, data.x, data.z);
		if (angle !== undefined)
			return {"x": data.x, "z": data.z, "angle": angle};
	}

	return false;
};

GuiInterface.prototype.PlaySound = function(player, data)
{
	// Ignore if no entity was passed
	if (!data.entity)
		return;

	PlaySound(data.name, data.entity);
};

GuiInterface.prototype.FindIdleUnits = function(player, data)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var playerEntities = cmpRangeManager.GetEntitiesByPlayer(player).filter( function(e) {
		var cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
		if (!cmpUnitAI || !cmpUnitAI.IsIdle() || cmpUnitAI.IsGarrisoned())
			return false;
		var cmpIdentity = Engine.QueryInterface(e, IID_Identity);
		if (!cmpIdentity || !cmpIdentity.HasClass(data.idleClass))
			return false;
		return true;
	});

	var idleUnits = [];

	for (var j = 0; j < playerEntities.length; ++j)
	{
		var ent = playerEntities[j];

		if (ent <= data.prevUnit|0 || data.excludeUnits.indexOf(ent) > -1)
			continue;
		idleUnits.push(ent);
		playerEntities.splice(j--, 1);

		if (data.limit && idleUnits.length >= data.limit)
			break;
	}

	return idleUnits;
};

GuiInterface.prototype.GetTradingRouteGain = function(player, data)
{
	if (!data.firstMarket || !data.secondMarket)
		return null;

	return CalculateTraderGain(data.firstMarket, data.secondMarket, data.template);
};

GuiInterface.prototype.GetTradingDetails = function(player, data)
{
	var cmpEntityTrader = Engine.QueryInterface(data.trader, IID_Trader);
	if (!cmpEntityTrader || !cmpEntityTrader.CanTrade(data.target))
		return null;
	var firstMarket = cmpEntityTrader.GetFirstMarket();
	var secondMarket = cmpEntityTrader.GetSecondMarket();
	var result = null;
	if (data.target === firstMarket)
	{
		result = {
			"type": "is first",
			"hasBothMarkets": cmpEntityTrader.HasBothMarkets()
		};
		if (cmpEntityTrader.HasBothMarkets())
			result.gain = cmpEntityTrader.GetGain();
	}
	else if (data.target === secondMarket)
	{
		result = {
			"type": "is second",
			"gain": cmpEntityTrader.GetGain(),
		};
	}
	else if (!firstMarket)
	{
		result = {"type": "set first"};
	}
	else if (!secondMarket)
	{
		result = {
			"type": "set second",
			"gain": cmpEntityTrader.CalculateGain(firstMarket, data.target),
		};
	}
	else
	{
		// Else both markets are not null and target is different from them
		result = {"type": "set first"};
	}
	return result;
};

GuiInterface.prototype.CanCapture = function(player, data)
{
	var cmpAttack = Engine.QueryInterface(data.entity, IID_Attack);
	if (!cmpAttack)
		return false;

	var owner = QueryOwnerInterface(data.entity).GetPlayerID();

	var cmpCapturable = QueryMiragedInterface(data.target, IID_Capturable);
	if (cmpCapturable && cmpCapturable.CanCapture(owner) && cmpAttack.GetAttackTypes().indexOf("Capture") != -1)
		return cmpAttack.CanAttack(data.target);

	return false;
};

GuiInterface.prototype.CanAttack = function(player, data)
{
	var cmpAttack = Engine.QueryInterface(data.entity, IID_Attack);
	if (!cmpAttack)
		return false;
	var cmpEntityPlayer = QueryOwnerInterface(data.entity, IID_Player);
	var cmpTargetPlayer = QueryOwnerInterface(data.target, IID_Player);
	if (!cmpEntityPlayer || !cmpTargetPlayer)
		return false;

	// if the owner is an enemy, it's up to the attack component to decide
	if (cmpEntityPlayer.IsEnemy(cmpTargetPlayer.GetPlayerID()))
		return cmpAttack.CanAttack(data.target);
	return false;
};

/*
 * Returns batch build time.
 */
GuiInterface.prototype.GetBatchTime = function(player, data)
{
	var cmpProductionQueue = Engine.QueryInterface(data.entity, IID_ProductionQueue);
	if (!cmpProductionQueue)
		return 0;

	return cmpProductionQueue.GetBatchTime(data.batchSize);
};

GuiInterface.prototype.IsMapRevealed = function(player)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return cmpRangeManager.GetLosRevealAll(player);
};

GuiInterface.prototype.SetPathfinderDebugOverlay = function(player, enabled)
{
	var cmpPathfinder = Engine.QueryInterface(SYSTEM_ENTITY, IID_Pathfinder);
	cmpPathfinder.SetDebugOverlay(enabled);
};

GuiInterface.prototype.SetObstructionDebugOverlay = function(player, enabled)
{
	var cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	cmpObstructionManager.SetDebugOverlay(enabled);
};

GuiInterface.prototype.SetMotionDebugOverlay = function(player, data)
{
	for each (var ent in data.entities)
	{
		var cmpUnitMotion = Engine.QueryInterface(ent, IID_UnitMotion);
		if (cmpUnitMotion)
			cmpUnitMotion.SetDebugOverlay(data.enabled);
	}
};

GuiInterface.prototype.SetRangeDebugOverlay = function(player, enabled)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.SetDebugOverlay(enabled);
};

GuiInterface.prototype.GetTraderNumber = function(player)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var traders = cmpRangeManager.GetEntitiesByPlayer(player).filter( function(e) {
		return Engine.QueryInterface(e, IID_Trader);
	});

	var landTrader = { "total": 0, "trading": 0, "garrisoned": 0 };
	var shipTrader = { "total": 0, "trading": 0 };
	for each (var ent in traders)
	{
		var cmpIdentity =  Engine.QueryInterface(ent, IID_Identity);
		var cmpUnitAI =  Engine.QueryInterface(ent, IID_UnitAI);
		if (!cmpIdentity || !cmpUnitAI)
			continue;
		if (cmpIdentity.HasClass("Ship"))
		{
			++shipTrader.total;
			if (cmpUnitAI.order && cmpUnitAI.order.type == "Trade")
				++shipTrader.trading;
		}
		else
		{
			++landTrader.total;
			if (cmpUnitAI.order && cmpUnitAI.order.type == "Trade")
				++landTrader.trading;
			if (cmpUnitAI.order && cmpUnitAI.order.type == "Garrison")
			{
				var holder = cmpUnitAI.order.data.target;
				var cmpHolderUnitAI = Engine.QueryInterface(holder, IID_UnitAI);
				if (cmpHolderUnitAI && cmpHolderUnitAI.order && cmpHolderUnitAI.order.type == "Trade")
					++landTrader.garrisoned;
			}
		}
	}

	return { "landTrader": landTrader, "shipTrader": shipTrader };
};

GuiInterface.prototype.GetTradingGoods = function(player, tradingGoods)
{
	var cmpPlayer = QueryPlayerIDInterface(player, IID_Player);
	return cmpPlayer.GetTradingGoods();
};

GuiInterface.prototype.OnGlobalEntityRenamed = function(msg)
{
	this.renamedEntities.push(msg);
};

// List the GuiInterface functions that can be safely called by GUI scripts.
// (GUI scripts are non-deterministic and untrusted, so these functions must be
// appropriately careful. They are called with a first argument "player", which is
// trusted and indicates the player associated with the current client; no data should
// be returned unless this player is meant to be able to see it.)
var exposedFunctions = {
	
	"GetSimulationState": 1,
	"GetExtendedSimulationState": 1,
	"GetRenamedEntities": 1,
	"ClearRenamedEntities": 1,
	"GetEntityState": 1,
	"GetExtendedEntityState": 1,
	"GetAverageRangeForBuildings": 1,
	"GetTemplateData": 1,
	"GetTechnologyData": 1,
	"IsTechnologyResearched": 1,
	"CheckTechnologyRequirements": 1,
	"GetStartedResearch": 1,
	"GetBattleState": 1,
	"GetIncomingAttacks": 1,
	"GetNeededResources": 1,
	"GetNotifications": 1,
	"GetTimeNotifications": 1,

	"GetAvailableFormations": 1,
	"GetFormationRequirements": 1,
	"CanMoveEntsIntoFormation": 1,
	"IsFormationSelected": 1,
	"GetFormationInfoFromTemplate": 1,
	"IsStanceSelected": 1,

	"SetSelectionHighlight": 1,
	"GetAllBuildableEntities": 1,
	"SetStatusBars": 1,
	"GetPlayerEntities": 1,
	"DisplayRallyPoint": 1,
	"SetBuildingPlacementPreview": 1,
	"SetWallPlacementPreview": 1,
	"GetFoundationSnapData": 1,
	"PlaySound": 1,
	"FindIdleUnits": 1,
	"GetTradingRouteGain": 1,
	"GetTradingDetails": 1,
	"CanCapture": 1,
	"CanAttack": 1,
	"GetBatchTime": 1,

	"IsMapRevealed": 1,
	"SetPathfinderDebugOverlay": 1,
	"SetObstructionDebugOverlay": 1,
	"SetMotionDebugOverlay": 1,
	"SetRangeDebugOverlay": 1,

	"GetTraderNumber": 1,
	"GetTradingGoods": 1,
};

GuiInterface.prototype.ScriptCall = function(player, name, args)
{
	if (exposedFunctions[name])
		return this[name](player, args);
	else
		throw new Error("Invalid GuiInterface Call name \""+name+"\"");
};

Engine.RegisterSystemComponentType(IID_GuiInterface, "GuiInterface", GuiInterface);
