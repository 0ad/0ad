function GuiInterface() {}

GuiInterface.prototype.Init = function() {};

GuiInterface.prototype.GetSimulationState = function(player)
{
//	print("GetSimulationState "+player+"\n");

	return { test: "simulation state" };
};

GuiInterface.prototype.GetEntityState = function(player, ent)
{
//	print("GetEntityState "+player+" "+ent+"\n");

	var cmpPosition = Engine.QueryInterface(ent, IID_Position);
	
	var ret = {
		position: cmpPosition.GetPosition()
	};
	
	return ret;
};

GuiInterface.prototype.SetSelectionHighlight = function(ent, colour)
{
	var cmpSelectable = Engine.QueryInterface(ent, IID_Selectable);
	cmpSelectable.SetSelectionHighlight(colour);
};

Engine.RegisterComponentType(IID_GuiInterface, "GuiInterface", GuiInterface);
