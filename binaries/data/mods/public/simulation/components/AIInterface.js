function AIInterface() {}

AIInterface.prototype.Schema =
	"<a:component type='system'/><empty/>";

AIInterface.prototype.Init = function()
{
	this.events = [];
};

AIInterface.prototype.GetRepresentation = function()
{
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	// Return the same game state as the GUI uses
	var state = cmpGuiInterface.GetSimulationState(-1);

	// Add some extra AI-specific data
	state.events = this.events;

	// Reset the event list for the next turn
	this.events = [];

	// Add entity representations
	state.entities = {};
	for each (var proxy in Engine.GetComponentsWithInterface(IID_AIProxy))
	{
		var rep = proxy.GetRepresentation();
		if (rep !== null)
			state.entities[proxy.entity] = rep;
	}

	return state;
};

// Set up a load of event handlers to capture interesting things going on
// in the world, which we will report to AI:
// (This shouldn't include extremely high-frequency events, like PositionChanged,
// because that would be very expensive and AI will rarely care about all those
// events.)

AIInterface.prototype.OnGlobalCreate = function(msg)
{
	this.events.push({"type": "Create", "msg": msg});
};

AIInterface.prototype.OnGlobalDestroy = function(msg)
{
	this.events.push({"type": "Destroy", "msg": msg});
};

AIInterface.prototype.OnGlobalOwnershipChanged = function(msg)
{
	this.events.push({"type": "OwnershipChanged", "msg": msg});
};

AIInterface.prototype.OnGlobalAttacked = function(msg)
{
	this.events.push({"type": "Attacked", "msg": msg});
};

AIInterface.prototype.OnGlobalConstructionFinished = function(msg)
{
	this.events.push({"type": "ConstructionFinished", "msg": msg});
};

AIInterface.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.events.push({"type": "PlayerDefeated", "msg": msg});
};

Engine.RegisterComponentType(IID_AIInterface, "AIInterface", AIInterface);
