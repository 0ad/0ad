function GuiInterface() {}

GuiInterface.prototype.Init = function()
{
	// TODO: need to not serialise this value
	this.placementEntity = undefined; // = undefined or [templateName, entityID]
};

GuiInterface.prototype.GetSimulationState = function(player)
{
	var ret = {
		players: []
	};
	
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var n = cmpPlayerMan.GetNumPlayers();
	for (var i = 0; i < n; ++i)
	{
		var playerEnt = cmpPlayerMan.GetPlayerByID(i);
		var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);
		var player = {
			popCount: cmpPlayer.GetPopulationCount(),
			popLimit: cmpPlayer.GetPopulationLimit()
		};
		ret.players.push(player);
	}
	
	return ret;
};

GuiInterface.prototype.GetEntityState = function(player, ent)
{
	var cmpPosition = Engine.QueryInterface(ent, IID_Position);
	
	var ret = {
		position: cmpPosition.GetPosition()
	};

	var cmpBuilder = Engine.QueryInterface(ent, IID_Builder);
	if (cmpBuilder)
	{
		ret.buildEntities = cmpBuilder.GetEntitiesList();
	}

	return ret;
};

GuiInterface.prototype.SetSelectionHighlight = function(ent, colour)
{
	var cmpSelectable = Engine.QueryInterface(ent, IID_Selectable);
	cmpSelectable.SetSelectionHighlight(colour);
};

GuiInterface.prototype.SetBuildingPlacementPreview = function(cmd)
{
	if (!this.placementEntity || this.placementEntity[0] != cmd.template)
	{
		if (cmd.template == "")
		{
			if (this.placementEntity)
				Engine.DestroyEntity(this.placementEntity[1]);
			this.placementEntity = undefined;
		}
		else
		{
			this.placementEntity = [cmd.template, Engine.AddLocalEntity("preview|" + cmd.template)];
		}
	}

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

GuiInterface.prototype.ScriptCall = function(name, args)
{
	if (name == "SetBuildingPlacementPreview")
		this.SetBuildingPlacementPreview(args);
	else
		throw new Error("Invalid GuiInterface Call name \""+name+"\"");
};

Engine.RegisterComponentType(IID_GuiInterface, "GuiInterface", GuiInterface);
