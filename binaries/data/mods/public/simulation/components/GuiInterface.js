function GuiInterface() {}

GuiInterface.prototype.Init = function()
{
	// TODO: need to not serialise this value
	this.placementEntity = undefined; // = undefined or [templateName, entityID]
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
		var player = {
			"popCount": cmpPlayer.GetPopulationCount(),
			"popLimit": cmpPlayer.GetPopulationLimit(),
			"resourceCounts": cmpPlayer.GetResourceCounts()
		};
		ret.players.push(player);
	}
	
	return ret;
};

GuiInterface.prototype.GetEntityState = function(player, ent)
{
	var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var cmpPosition = Engine.QueryInterface(ent, IID_Position);
	
	var ret = {
		"template": cmpTempMan.GetCurrentTemplateName(ent),
		"position": cmpPosition.GetPosition()
	};

	var cmpHealth = Engine.QueryInterface(ent, IID_Health);
	if (cmpHealth)
	{
		ret.hitpoints = cmpHealth.GetHitpoints();
		ret.maxHitpoints = cmpHealth.GetMaxHitpoints();
	}

	var cmpAttack = Engine.QueryInterface(ent, IID_Attack);
	if (cmpAttack)
	{
		ret.attack = cmpAttack.GetAttackStrengths();
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

	return ret;
};

GuiInterface.prototype.GetTemplateData = function(player, name)
{
	var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTempMan.GetTemplate(name);

	var ret = {};

	if (template.Identity)
	{
		ret.name = {
			"specific": (template.Identity.SpecificName || template.Identity.GenericName),
			"generic": template.Identity.GenericName
		};
		ret.icon_cell = template.Identity.IconCell;
	}

	return ret;
};

GuiInterface.prototype.SetSelectionHighlight = function(player, cmd)
{
	var cmpSelectable = Engine.QueryInterface(cmd.entity, IID_Selectable);
	cmpSelectable.SetSelectionHighlight(cmd.colour);
};

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

	// Move the preview into the right location
	if (this.placementEntity)
	{
		var pos = Engine.QueryInterface(this.placementEntity[1], IID_Position);
		if (pos)
		{
			pos.JumpTo(cmd.x, cmd.z);
			pos.SetYRotation(cmd.angle);
		}
	}
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
	"SetSelectionHighlight": 1,
	"SetBuildingPlacementPreview": 1
};

GuiInterface.prototype.ScriptCall = function(player, name, args)
{
	if (exposedFunctions[name])
		return this[name](player, args);
	else
		throw new Error("Invalid GuiInterface Call name \""+name+"\"");
};

Engine.RegisterComponentType(IID_GuiInterface, "GuiInterface", GuiInterface);
