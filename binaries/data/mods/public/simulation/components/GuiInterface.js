function GuiInterface() {}

GuiInterface.prototype.Schema =
	"<a:component type='system'/><empty/>";

GuiInterface.prototype.Serialize = function()
{
	// This component isn't network-synchronised so we mustn't serialise
	// its non-deterministic data. Instead just return an empty object.
	return {};
};

GuiInterface.prototype.Deserialize = function(obj)
{
	this.Init();
};

GuiInterface.prototype.Init = function()
{
	this.placementEntity = undefined; // = undefined or [templateName, entityID]
	this.rallyPoints = undefined;
	this.notifications = [];
	this.renamedEntities = [];
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
		var cmpPlayerBuildLimits = Engine.QueryInterface(playerEnt, IID_BuildLimits);
		var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);
		
		// Work out what phase we are in
		var cmpTechMan = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
		var phase = "";
		if (cmpTechMan.IsTechnologyResearched("phase_city"))
			phase = "city";
		else if (cmpTechMan.IsTechnologyResearched("phase_town"))
			phase = "town";
		else if (cmpTechMan.IsTechnologyResearched("phase_village"))
			phase = "village";
		
		// store player ally/enemy data as arrays
		var allies = [];
		var enemies = [];
		for (var j = 0; j <= n; ++j)
		{
			allies[j] = cmpPlayer.IsAlly(j);
			enemies[j] = cmpPlayer.IsEnemy(j);
		}
		var playerData = {
			"name": cmpPlayer.GetName(),
			"civ": cmpPlayer.GetCiv(),
			"colour": cmpPlayer.GetColour(),
			"popCount": cmpPlayer.GetPopulationCount(),
			"popLimit": cmpPlayer.GetPopulationLimit(),
			"popMax": cmpPlayer.GetMaxPopulation(),
			"resourceCounts": cmpPlayer.GetResourceCounts(),
			"trainingBlocked": cmpPlayer.IsTrainingBlocked(),
			"state": cmpPlayer.GetState(),
			"team": cmpPlayer.GetTeam(),
			"phase": phase,
			"isAlly": allies,
			"isEnemy": enemies,
			"buildLimits": cmpPlayerBuildLimits.GetLimits(),
			"buildCounts": cmpPlayerBuildLimits.GetCounts()
		};
		ret.players.push(playerData);
	}

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (cmpRangeManager)
	{
		ret.circularMap = cmpRangeManager.GetLosCircular();
	}
	
	// Add timeElapsed
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	ret.timeElapsed = cmpTimer.GetTime();

	return ret;
};

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
		ret.players[i].statistics = cmpPlayerStatisticsTracker.GetStatistics();
	}

	return ret;
};

GuiInterface.prototype.GetRenamedEntities = function(player)
{
	return this.renamedEntities;
};

GuiInterface.prototype.ClearRenamedEntities = function(player)
{
	this.renamedEntities = [];
};

GuiInterface.prototype.GetEntityState = function(player, ent)
{
	var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);

	// All units must have a template; if not then it's a nonexistent entity id
	var template = cmpTempMan.GetCurrentTemplateName(ent);
	if (!template)
		return null;

	var ret = {
		"id": ent,
		"template": template
	}

	var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
	if (cmpIdentity)
	{
		ret.identity = {
			"rank": cmpIdentity.GetRank(),
			"classes": cmpIdentity.GetClassesList(),
			"selectionGroupName": cmpIdentity.GetSelectionGroupName()
		};
	}
	
	var cmpPosition = Engine.QueryInterface(ent, IID_Position);
	if (cmpPosition && cmpPosition.IsInWorld())
	{
		ret.position = cmpPosition.GetPosition();
	}

	var cmpHealth = Engine.QueryInterface(ent, IID_Health);
	if (cmpHealth)
	{
		ret.hitpoints = cmpHealth.GetHitpoints();
		ret.maxHitpoints = cmpHealth.GetMaxHitpoints();
		ret.needsRepair = cmpHealth.IsRepairable() && (cmpHealth.GetHitpoints() < cmpHealth.GetMaxHitpoints());
		ret.needsHeal = !cmpHealth.IsUnhealable();
	}

	var cmpAttack = Engine.QueryInterface(ent, IID_Attack);
	if (cmpAttack)
	{
		var type = cmpAttack.GetBestAttack(); // TODO: how should we decide which attack to show?
		ret.attack = cmpAttack.GetAttackStrengths(type);
	}

	var cmpArmour = Engine.QueryInterface(ent, IID_DamageReceiver);
	if (cmpArmour)
	{
		ret.armour = cmpArmour.GetArmourStrengths();
	}

	var cmpBuilder = Engine.QueryInterface(ent, IID_Builder);
	if (cmpBuilder)
	{
		ret.buildEntities = cmpBuilder.GetEntitiesList();
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
			"preferredGoods": cmpTrader.GetPreferredGoods()
		};
	}

	var cmpFoundation = Engine.QueryInterface(ent, IID_Foundation);
	if (cmpFoundation)
	{
		ret.foundation = {
			"progress": cmpFoundation.GetBuildPercentage()
		};
	}

	var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (cmpOwnership)
	{
		ret.player = cmpOwnership.GetOwner();
	}

	var cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
	if (cmpResourceSupply)
	{
		ret.resourceSupply = {
			"max": cmpResourceSupply.GetMaxAmount(),
			"amount": cmpResourceSupply.GetCurrentAmount(),
			"type": cmpResourceSupply.GetType()
		};
	}

	var cmpResourceGatherer = Engine.QueryInterface(ent, IID_ResourceGatherer);
	if (cmpResourceGatherer)
	{
		ret.resourceGatherRates = cmpResourceGatherer.GetGatherRates();
		ret.resourceCarrying = cmpResourceGatherer.GetCarryingStatus();
	}

	var cmpResourceDropsite = Engine.QueryInterface(ent, IID_ResourceDropsite);
	if (cmpResourceDropsite)
	{
		ret.resourceDropsite = {
			"types": cmpResourceDropsite.GetTypes()
		};
	}

	var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
	if (cmpRallyPoint)
	{
		ret.rallyPoint = {'position': cmpRallyPoint.GetPosition()}; // undefined or {x,z} object
	}

	var cmpGarrisonHolder = Engine.QueryInterface(ent, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
	{
		ret.garrisonHolder = {
			"entities": cmpGarrisonHolder.GetEntities(),
			"allowedClasses": cmpGarrisonHolder.GetAllowedClassesList()
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
	
	var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
	if (cmpUnitAI)
	{
		ret.unitAI = {
			// TODO: reading properties directly is kind of violating abstraction
			"state": cmpUnitAI.fsmStateName,
			"orders": cmpUnitAI.orderQueue,
		};
	}

	if (!cmpFoundation && cmpIdentity && cmpIdentity.HasClass("BarterMarket"))
	{
		var cmpBarter = Engine.QueryInterface(SYSTEM_ENTITY, IID_Barter);
		ret.barterMarket = { "prices": cmpBarter.GetPrices() };
	}

	var cmpHeal = Engine.QueryInterface(ent, IID_Heal);
	if (cmpHeal)
	{
		ret.Healer = { 
			"unhealableClasses": cmpHeal.GetUnhealableClasses(),
			"healableClasses": cmpHeal.GetHealableClasses(),
		};
	}

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	ret.visibility = cmpRangeManager.GetLosVisibility(ent, player, false);

	return ret;
};

GuiInterface.prototype.GetTemplateData = function(player, name)
{
	var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTempMan.GetTemplate(name);

	if (!template)
		return null;

	var ret = {};

	if (template.Armour)
	{
		ret.armour = {
			"hack": +template.Armour.Hack,
			"pierce": +template.Armour.Pierce,
			"crush": +template.Armour.Crush,
		};
	}
	
	if (template.Attack)
	{
		ret.attack = {};
		for (var type in template.Attack)
		{
			ret.attack[type] = {
				"hack": (+template.Attack[type].Hack || 0),
				"pierce": (+template.Attack[type].Pierce || 0),
				"crush": (+template.Attack[type].Crush || 0),
			};
		}
	}
	
	if (template.Cost)
	{
		ret.cost = {};
		if (template.Cost.Resources.food) ret.cost.food = +template.Cost.Resources.food;
		if (template.Cost.Resources.wood) ret.cost.wood = +template.Cost.Resources.wood;
		if (template.Cost.Resources.stone) ret.cost.stone = +template.Cost.Resources.stone;
		if (template.Cost.Resources.metal) ret.cost.metal = +template.Cost.Resources.metal;
		if (template.Cost.Population) ret.cost.population = +template.Cost.Population;
		if (template.Cost.PopulationBonus) ret.cost.populationBonus = +template.Cost.PopulationBonus;
	}
	
	if (template.Health)
	{
		ret.health = +template.Health.Max;
	}

	if (template.Identity)
	{
		ret.selectionGroupName = template.Identity.SelectionGroupName;
		ret.name = {
			"specific": (template.Identity.SpecificName || template.Identity.GenericName),
			"generic": template.Identity.GenericName
		};
		ret.icon = template.Identity.Icon;
		ret.tooltip =  template.Identity.Tooltip;
		ret.requiredTechnology = template.Identity.RequiredTechnology;
	}

	if (template.UnitMotion)
	{
		ret.speed = {
			"walk": +template.UnitMotion.WalkSpeed,
		};
		if (template.UnitMotion.Run) ret.speed.run = +template.UnitMotion.Run.Speed;
	}

	return ret;
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
	
	var ret = {};
	
	// Get specific name for this civ or else the generic specific name 
	var cmpPlayer = QueryPlayerIDInterface(player, IID_Player);
	var specific = undefined;
	if (template.specificName)
	{
		if (template.specificName[cmpPlayer.GetCiv()])
			specific = template.specificName[cmpPlayer.GetCiv()];
		else
			specific = template.specificName['generic'];
	}
	
	ret.name = {
		"specific": specific,
		"generic": template.genericName,
	};
	ret.icon = "technologies/" + template.icon;
	ret.cost = {
		"food": +template.cost.food,
		"wood": +template.cost.wood,
		"metal": +template.cost.metal,
		"stone": +template.cost.stone,
	}
	ret.tooltip = template.tooltip;
	
	if (template.requirementsTooltip)
		ret.requirementsTooltip = template.requirementsTooltip;
	else
		ret.requirementsTooltip = "";
	
	ret.description = template.description;
	
	return ret;
};

GuiInterface.prototype.IsTechnologyResearched = function(player, tech)
{
	var cmpTechMan = QueryPlayerIDInterface(player, IID_TechnologyManager);
	
	if (!cmpTechMan)
		return false;
	
	return cmpTechMan.IsTechnologyResearched(tech);
};

// Checks whether the requirements for this technology have been met
GuiInterface.prototype.CheckTechnologyRequirements = function(player, tech)
{
	var cmpTechMan = QueryPlayerIDInterface(player, IID_TechnologyManager);
	
	if (!cmpTechMan)
		return false;
	
	return cmpTechMan.CanResearch(tech);
};

GuiInterface.prototype.PushNotification = function(notification)
{
	this.notifications.push(notification);
};

GuiInterface.prototype.GetNextNotification = function()
{
	if (this.notifications.length)
		return this.notifications.pop();
	else
		return "";
};

GuiInterface.prototype.GetFormationRequirements = function(player, data)
{
	return GetFormationRequirements(data.formationName);
};

GuiInterface.prototype.CanMoveEntsIntoFormation = function(player, data)
{
	return CanMoveEntsIntoFormation(data.ents, data.formationName);
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
			if (cmpUnitAI.GetLastFormationName() == data.formationName)
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

GuiInterface.prototype.SetSelectionHighlight = function(player, cmd)
{
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var playerColours = {}; // cache of owner -> colour map
	
	for each (var ent in cmd.entities)
	{
		var cmpSelectable = Engine.QueryInterface(ent, IID_Selectable);
		if (!cmpSelectable)
			continue;

		// Find the entity's owner's colour:
		var owner = -1;
		var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (cmpOwnership)
			owner = cmpOwnership.GetOwner();

		var colour = playerColours[owner];
		if (!colour)
		{
			colour = {"r":1, "g":1, "b":1};
			var cmpPlayer = Engine.QueryInterface(cmpPlayerMan.GetPlayerByID(owner), IID_Player);
			if (cmpPlayer)
				colour = cmpPlayer.GetColour();
			playerColours[owner] = colour;
		}

		cmpSelectable.SetSelectionHighlight({"r":colour.r, "g":colour.g, "b":colour.b, "a":cmd.alpha});
	}
};

GuiInterface.prototype.SetStatusBars = function(player, cmd)
{
	for each (var ent in cmd.entities)
	{
		var cmpStatusBars = Engine.QueryInterface(ent, IID_StatusBars);
		if (cmpStatusBars)
			cmpStatusBars.SetEnabled(cmd.enabled);
	}
};

/**
 * Displays the rally point of a given list of entities (carried in cmd.entities).
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
			pos = cmpRallyPoint.GetPosition(); // may return undefined if no rally point is set

		if (pos)
		{
			cmpRallyPointRenderer.SetPosition({'x': pos.x, 'y': pos.z}); // SetPosition takes a CFixedVector2D which has X/Y components, not X/Z
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
 * Returns true if the placement is okay (everything is valid and the entity is not obstructed by others).
 */
GuiInterface.prototype.SetBuildingPlacementPreview = function(player, cmd)
{
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

		// Check whether it's in a visible or fogged region
		//	tell GetLosVisibility to force RetainInFog because preview entities set this to false,
		//	which would show them as hidden instead of fogged
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		var visible = (cmpRangeManager && cmpRangeManager.GetLosVisibility(ent, player, true) != "hidden");
		var validPlacement = false;

		if (visible)
		{	// Check whether it's obstructed by other entities or invalid terrain
			var cmpBuildRestrictions = Engine.QueryInterface(ent, IID_BuildRestrictions);
			if (!cmpBuildRestrictions)
				error("cmpBuildRestrictions not defined");

			validPlacement = (cmpBuildRestrictions && cmpBuildRestrictions.CheckPlacement(player));
		}

		var ok = (visible && validPlacement);

		// Set it to a red shade if this is an invalid location
		var cmpVisual = Engine.QueryInterface(ent, IID_Visual);
		if (cmpVisual)
		{
			if (!ok)
				cmpVisual.SetShadingColour(1.4, 0.4, 0.4, 1);
			else
				cmpVisual.SetShadingColour(1, 1, 1, 1);
		}

		return ok;
	}

	return false;
};

GuiInterface.prototype.GetFoundationSnapData = function(player, data)
{
	var cmpTemplateMgr = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTemplateMgr.GetTemplate(data.template);

	if (template.BuildRestrictions.Category == "Dock")
	{
		var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
		var cmpWaterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
		if (!cmpTerrain || !cmpWaterManager)
		{
			return false;
		}
		
		// Get footprint size
		var halfSize = 0;
		if (template.Footprint.Square)
		{
			halfSize = Math.max(template.Footprint.Square["@depth"], template.Footprint.Square["@width"])/2;
		}
		else if (template.Footprint.Circle)
		{
			halfSize = template.Footprint.Circle["@radius"];
		}
		
		/* Find direction of most open water, algorithm:
		 *	1. Pick points in a circle around dock
		 *	2. If point is in water, add to array
		 *	3. Scan array looking for consecutive points
		 *	4. Find longest sequence of consecutive points
		 *	5. If sequence equals all points, no direction can be determined,
		 *		expand search outward and try (1) again
		 *	6. Calculate angle using average of sequence
		 */
		const numPoints = 16;
		for (var dist = 0; dist < 4; ++dist)
		{
			var waterPoints = [];
			for (var i = 0; i < numPoints; ++i)
			{
				var angle = (i/numPoints)*2*Math.PI;
				var d = halfSize*(dist+1);
				var nx = data.x - d*Math.sin(angle);
				var nz = data.z + d*Math.cos(angle);
				
				if (cmpTerrain.GetGroundLevel(nx, nz) < cmpWaterManager.GetWaterLevel(nx, nz))
				{
					waterPoints.push(i);
				}
			}
			var consec = [];
			var length = waterPoints.length;
			for (var i = 0; i < length; ++i)
			{
				var count = 0;
				for (var j = 0; j < (length-1); ++j)
				{
					if (((waterPoints[(i + j) % length]+1) % numPoints) == waterPoints[(i + j + 1) % length])
					{
						++count;
					}
					else
					{
						break;
					}
				}
				consec[i] = count;
			}
			var start = 0;
			var count = 0;
			for (var c in consec)
			{
				if (consec[c] > count)
				{
					start = c;
					count = consec[c];
				}
			}
			
			// If we've found a shoreline, stop searching
			if (count != numPoints-1)
			{
				return {"x": data.x, "z": data.z, "angle": -(((waterPoints[start] + consec[start]/2) % numPoints)/numPoints*2*Math.PI)};
			}
		}
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

function isIdleUnit(ent, idleClass)
{
	var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
	var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
	
	// TODO: Do something with garrisoned idle units
	return (cmpUnitAI && cmpIdentity && cmpUnitAI.IsIdle() && !cmpUnitAI.IsGarrisoned() && idleClass && cmpIdentity.HasClass(idleClass));
}

GuiInterface.prototype.FindIdleUnit = function(player, data)
{
	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var playerEntities = rangeMan.GetEntitiesByPlayer(player);

	// Find the first matching entity that is after the previous selection,
	// so that we cycle around in a predictable order
	for each (var ent in playerEntities)
	{
		if (ent > data.prevUnit && isIdleUnit(ent, data.idleClass))
			return ent;
	}

	// No idle entities left in the class
	return 0;
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
			"goods": cmpEntityTrader.GetPreferredGoods(),
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
			"goods": cmpEntityTrader.GetPreferredGoods()
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
			"goods": cmpEntityTrader.GetPreferredGoods()
		};
	}
	else
	{
		// Else both markets are not null and target is different from them
		result = {"type": "set first"};
	}
	return result;
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

GuiInterface.prototype.OnGlobalEntityRenamed = function(msg)
{
	this.renamedEntities.push(msg);
}

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
	"GetTemplateData": 1,
	"GetTechnologyData": 1,
	"IsTechnologyResearched": 1,
	"CheckTechnologyRequirements": 1,
	"GetNextNotification": 1,

	"GetFormationRequirements": 1,
	"CanMoveEntsIntoFormation": 1,
	"IsFormationSelected": 1,
	"IsStanceSelected": 1,

	"SetSelectionHighlight": 1,
	"SetStatusBars": 1,
	"DisplayRallyPoint": 1,
	"SetBuildingPlacementPreview": 1,
	"GetFoundationSnapData": 1,
	"PlaySound": 1,
	"FindIdleUnit": 1,
	"GetTradingDetails": 1,

	"SetPathfinderDebugOverlay": 1,
	"SetObstructionDebugOverlay": 1,
	"SetMotionDebugOverlay": 1,
	"SetRangeDebugOverlay": 1,
};

GuiInterface.prototype.ScriptCall = function(player, name, args)
{
	if (exposedFunctions[name])
		return this[name](player, args);
	else
		throw new Error("Invalid GuiInterface Call name \""+name+"\"");
};

Engine.RegisterComponentType(IID_GuiInterface, "GuiInterface", GuiInterface);
