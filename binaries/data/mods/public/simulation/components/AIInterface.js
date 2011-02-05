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

// AIProxy sets up a load of event handlers to capture interesting things going on
// in the world, which we will report to AI. Handle those, and add a few more handlers
// for events that AIProxy won't capture.

AIInterface.prototype.PushEvent = function(type, msg)
{
	this.events.push({"type": type, "msg": msg});
};

AIInterface.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.events.push({"type": "PlayerDefeated", "msg": msg});
};

Engine.RegisterComponentType(IID_AIInterface, "AIInterface", AIInterface);
