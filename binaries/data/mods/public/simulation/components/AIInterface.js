function AIInterface() {}

AIInterface.prototype.Schema =
	"<a:component type='system'/><empty/>";

AIInterface.prototype.Init = function()
{
	this.events = {};
	this.events["Create"] = [];
	this.events["Destroy"] = [];
	this.events["Attacked"] = [];
	this.events["RangeUpdate"] = [];
	this.events["ConstructionFinished"] = [];
	this.events["TrainingFinished"] = [];
	this.events["AIMetadata"] = [];
	this.events["PlayerDefeated"] = [];
	this.events["EntityRenamed"] = [];
	this.events["OwnershipChanged"] = [];

	this.changedEntities = {};
};

AIInterface.prototype.GetRepresentation = function()
{
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	// Return the same game state as the GUI uses
	var state = cmpGuiInterface.GetExtendedSimulationState(-1);

	// Add some extra AI-specific data
	state.events = {};
	state.events["Create"] = this.events["Create"];
	state.events["Destroy"] = this.events["Destroy"];
	state.events["Attacked"] = this.events["Attacked"];
	state.events["RangeUpdate"] = this.events["RangeUpdate"];
	state.events["ConstructionFinished"] = this.events["ConstructionFinished"];
	state.events["TrainingFinished"] = this.events["TrainingFinished"];
	state.events["AIMetadata"] = this.events["AIMetadata"];
	state.events["PlayerDefeated"] = this.events["PlayerDefeated"];
	state.events["EntityRenamed"] = this.events["EntityRenamed"];
	state.events["OwnershipChanged"] = this.events["OwnershipChanged"];

	// Reset the event list for the next turn
	this.events["Create"] = [];
	this.events["Destroy"] = [];
	this.events["Attacked"] = [];
	this.events["RangeUpdate"] = [];
	this.events["ConstructionFinished"] = [];
	this.events["TrainingFinished"] = [];
	this.events["AIMetadata"] = [];
	this.events["PlayerDefeated"] = [];
	this.events["EntityRenamed"] = [];
	this.events["OwnershipChanged"] = [];
	
	// Add entity representations
	Engine.ProfileStart("proxy representations");
	state.entities = {};
	for (var id in this.changedEntities)
	{
		var aiProxy = Engine.QueryInterface(+id, IID_AIProxy);
		if (aiProxy)
			state.entities[id] = aiProxy.GetRepresentation();
	}
	this.changedEntities = {};
	Engine.ProfileStop();

	return state;
};
// Intended to be called first, during the map initialization: no caching
AIInterface.prototype.GetFullRepresentation = function(flushEvents)
{
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	
	// Return the same game state as the GUI uses
	var state = cmpGuiInterface.GetExtendedSimulationState(-1);
	
	// Add some extra AI-specific data
	state.events = {};
	state.events["Create"] = this.events["Create"];
	state.events["Destroy"] = this.events["Destroy"];
	state.events["Attacked"] = this.events["Attacked"];
	state.events["RangeUpdate"] = this.events["RangeUpdate"];
	state.events["ConstructionFinished"] = this.events["ConstructionFinished"];
	state.events["TrainingFinished"] = this.events["TrainingFinished"];
	state.events["AIMetadata"] = this.events["AIMetadata"];
	state.events["PlayerDefeated"] = this.events["PlayerDefeated"];
	state.events["EntityRenamed"] = this.events["EntityRenamed"];
	state.events["OwnershipChanged"] = this.events["OwnershipChanged"];
	

	if (flushEvents)
	{
		state.events["Create"] = [];
		state.events["Destroy"] = [];
		state.events["Attacked"] = [];
		state.events["RangeUpdate"] = [];
		state.events["ConstructionFinished"] = [];
		state.events["TrainingFinished"] = [];
		state.events["AIMetadata"] = [];
		state.events["PlayerDefeated"] = [];
		state.events["EntityRenamed"] = [];
		state.events["OwnershipChanged"] = [];
	}

	// Reset the event list for the next turn
	this.events["Create"] = [];
	this.events["Destroy"] = [];
	this.events["Attacked"] = [];
	this.events["RangeUpdate"] = [];
	this.events["ConstructionFinished"] = [];
	this.events["TrainingFinished"] = [];
	this.events["AIMetadata"] = [];
	this.events["PlayerDefeated"] = [];
	this.events["EntityRenamed"] = [];
	this.events["OwnershipChanged"] = [];


	// Add entity representations
	Engine.ProfileStart("proxy representations");
	state.entities = {};
	// all entities are changed in the initial state.
	for (var id in this.changedEntities)
	{
		var aiProxy = Engine.QueryInterface(+id, IID_AIProxy);
		if (aiProxy)
			state.entities[id] = aiProxy.GetFullRepresentation();
	}
	Engine.ProfileStop();
	
	return state;
};

AIInterface.prototype.ChangedEntity = function(ent)
{
	this.changedEntities[ent] = 1;
};

// AIProxy sets up a load of event handlers to capture interesting things going on
// in the world, which we will report to AI. Handle those, and add a few more handlers
// for events that AIProxy won't capture.

AIInterface.prototype.PushEvent = function(type, msg)
{
	if (this.events[type] === undefined)
		warn("Tried to push unknown event type " + type +", please add it to AIInterface.js");
	this.events[type].push(msg);
};

AIInterface.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.events["PlayerDefeated"].push(msg);
};

AIInterface.prototype.OnGlobalEntityRenamed = function(msg)
{
	this.events["EntityRenamed"].push(msg);
};

Engine.RegisterComponentType(IID_AIInterface, "AIInterface", AIInterface);
