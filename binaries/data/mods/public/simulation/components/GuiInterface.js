function GuiInterface()
{
	this.notifications = [];
}

GuiInterface.prototype.Schema =
	"<a:component type='system'/><empty/>";

GuiInterface.prototype.Serialize = function()
{
	return {};
};

GuiInterface.prototype.Init = function()
{
	this.placementEntity = undefined; // = undefined or [templateName, entityID]
	this.rallyPoints = undefined;
};

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
		var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);
		var playerData = {
			"name": cmpPlayer.GetName(),
			"civ": cmpPlayer.GetCiv(),
			"colour": cmpPlayer.GetColour(),
			"popCount": cmpPlayer.GetPopulationCount(),
			"popLimit": cmpPlayer.GetPopulationLimit(),
			"resourceCounts": cmpPlayer.GetResourceCounts(),
			"trainingQueueBlocked": cmpPlayer.IsTrainingQueueBlocked(),
			"state": cmpPlayer.GetState(),
			"team": cmpPlayer.GetTeam(),
			"diplomacy": cmpPlayer.GetDiplomacy(),
			"phase": cmpPlayer.GetPhase(),
		};
		ret.players.push(playerData);
	}

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (cmpRangeManager)
	{
		ret.circularMap = cmpRangeManager.GetLosCircular();
	}

	return ret;
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
			"classes": cmpIdentity.GetClassesList()
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

	var cmpTrainingQueue = Engine.QueryInterface(ent, IID_TrainingQueue);
	if (cmpTrainingQueue)
	{
		ret.training = {
			"entities": cmpTrainingQueue.GetEntitiesList(),
			"queue": cmpTrainingQueue.GetQueue(),
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
	}

	var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
	if (cmpRallyPoint)
	{
		ret.rallyPoint = { };
	}

	var cmpGarrisonHolder = Engine.QueryInterface(ent, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
	{
		ret.garrisonHolder = {
				"entities": cmpGarrisonHolder.GetEntities()
		};
	}
	
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	ret.visibility = cmpRangeManager.GetLosVisibility(ent, player);

	return ret;
};

GuiInterface.prototype.GetTemplateData = function(player, name)
{
	var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTempMan.GetTemplate(name);

	if (!template)
		return null;

	var ret = {};

	if (template.Identity)
	{
		ret.name = {
			"specific": (template.Identity.SpecificName || template.Identity.GenericName),
			"generic": template.Identity.GenericName
		};
		ret.icon = template.Identity.Icon;
		ret.tooltip =  template.Identity.Tooltip;
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

	return ret;
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

GuiInterface.prototype.SetSelectionHighlight = function(player, cmd)
{
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	var playerColours = {}; // cache of owner -> colour map
	
	for each (var ent in cmd.entities)
	{
		var cmpSelectable = Engine.QueryInterface(ent, IID_Selectable);
		if (!cmpSelectable)
			continue;

		if (cmd.alpha == 0)
		{
			cmpSelectable.SetSelectionHighlight({"r":0, "g":0, "b":0, "a":0});
			continue;
		}

		// Find the entity's owner's colour:

		var owner = -1;
		var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (cmpOwnership)
			owner = cmpOwnership.GetOwner();

		var colour = playerColours[owner];
		if (!colour)
		{
			colour = [1, 1, 1];
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
 * Displays the rally point of a building
 */
GuiInterface.prototype.DisplayRallyPoint = function(player, cmd)
{
	// If there are rally points already displayed, destroy them
	for each (var ent in this.rallyPoints)
	{
		// Hide it first (the destruction won't be instantaneous)
		var cmpPosition = Engine.QueryInterface(ent, IID_Position);
		cmpPosition.MoveOutOfWorld();

		Engine.DestroyEntity(ent);
	}

	this.rallyPoints = [];

	var positions = [];
	// DisplayRallyPoints is called passing a list of entities for which
	// rally points must be displayed
	for each (var ent in cmd.entities)
	{
		var cmpRallyPoint = Engine.QueryInterface(ent, IID_RallyPoint);
		if (!cmpRallyPoint)
			continue;

		// Verify the owner
		var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (!cmpOwnership || cmpOwnership.GetOwner() != player)
			continue;

		// If the command was passed an explicit position, use that and
		// override the real rally point position; otherwise use the real position
		var pos;
		if (cmd.x && cmd.z)
			pos = {"x": cmd.x, "z": cmd.z};
		else
			pos = cmpRallyPoint.GetPosition();

		if (pos)
		{
			// TODO: it'd probably be nice if we could draw some kind of line
			// between the building and pos, to make the marker easy to find even
			// if it's a long way from the building

			positions.push(pos);
		}
	}

	// Add rally point entity for each building
	for each (var pos in positions)
	{
		var rallyPoint = Engine.AddLocalEntity("actor|props/special/common/waypoint_flag.xml");
		var cmpPosition = Engine.QueryInterface(rallyPoint, IID_Position);
		cmpPosition.JumpTo(pos.x, pos.z);
		this.rallyPoints.push(rallyPoint);
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

		// Check whether it's obstructed by other entities
		var cmpObstruction = Engine.QueryInterface(ent, IID_Obstruction);
		var colliding = (cmpObstruction && cmpObstruction.CheckCollisions());

		// Check whether it's in a visible region
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		var visible = (cmpRangeManager.GetLosVisibility(ent, player) == "visible");

		var ok = (!colliding && visible);

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

GuiInterface.prototype.PlaySound = function(player, data)
{
	// Ignore if no entity was passed
	if (!data.entity)
		return;

	PlaySound(data.name, data.entity);
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

// List the GuiInterface functions that can be safely called by GUI scripts.
// (GUI scripts are non-deterministic and untrusted, so these functions must be
// appropriately careful. They are called with a first argument "player", which is
// trusted and indicates the player associated with the current client; no data should
// be returned unless this player is meant to be able to see it.)
var exposedFunctions = {
	
	"GetSimulationState": 1,
	"GetEntityState": 1,
	"GetTemplateData": 1,
	"GetNextNotification": 1,

	"SetSelectionHighlight": 1,
	"SetStatusBars": 1,
	"DisplayRallyPoint": 1,
	"SetBuildingPlacementPreview": 1,
	"PlaySound": 1,

	"SetPathfinderDebugOverlay": 1,
	"SetObstructionDebugOverlay": 1,
	"SetMotionDebugOverlay": 1,
	"SetRangeDebugOverlay": 1
};

GuiInterface.prototype.ScriptCall = function(player, name, args)
{
	if (exposedFunctions[name])
		return this[name](player, args);
	else
		throw new Error("Invalid GuiInterface Call name \""+name+"\"");
};

Engine.RegisterComponentType(IID_GuiInterface, "GuiInterface", GuiInterface);
