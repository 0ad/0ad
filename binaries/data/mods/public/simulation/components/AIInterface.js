function AIInterface() {}

AIInterface.prototype.Schema =
	"<a:component type='system'/><empty/>";

AIInterface.prototype.EventNames = [
	"Create",
	"Destroy",
	"Attacked",
	"RangeUpdate",
	"ConstructionFinished",
	"TrainingFinished",
	"AIMetadata",
	"PlayerDefeated",
	"EntityRenamed",
	"OwnershipChanged",
	"Garrison",
	"UnGarrison"
];

AIInterface.prototype.Init = function()
{
	this.events = {};
	for each (var i in this.EventNames)
		this.events[i] = [];

	this.changedEntities = {};

	// cache for technology changes;
	// this one is PlayerID->TemplateName->{StringForTheValue, ActualValue}
	this.changedTemplateInfo = {};
	// this is for auras and is EntityID->{StringForTheValue, ActualValue}
	this.changedEntityTemplateInfo = {};
};

AIInterface.prototype.GetNonEntityRepresentation = function()
{
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	
	// Return the same game state as the GUI uses
	var state = cmpGuiInterface.GetExtendedSimulationState(-1);
	
	// Add some extra AI-specific data
	// add custom events and reset them for the next turn
	state.events = {};
	for each (var i in this.EventNames)
	{
		state.events[i] = this.events[i];
		this.events[i] = [];
	}

	return state;
};

AIInterface.prototype.GetRepresentation = function()
{
	var state = this.GetNonEntityRepresentation();

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

	state.changedTemplateInfo = this.changedTemplateInfo;
	this.changedTemplateInfo = {};
	state.changedEntityTemplateInfo = this.changedEntityTemplateInfo;
	this.changedEntityTemplateInfo = {};

	return state;
};

// Intended to be called first, during the map initialization: no caching
AIInterface.prototype.GetFullRepresentation = function(flushEvents)
{	
	var state = this.GetNonEntityRepresentation();

	if (flushEvents)
		for each (var i in this.EventNames)
			state.events[i] = [];

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
	
	state.changedTemplateInfo = this.changedTemplateInfo;
	this.changedTemplateInfo = {};
	state.changedEntityTemplateInfo = this.changedEntityTemplateInfo;
	this.changedEntityTemplateInfo = {};

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

// When a new technology is researched, check which templates it affects,
// and send the updated values to the AI.
// this relies on the fact that any "value" in a technology can only ever change
// one template value, and that the naming is the same (with / in place of .)
// it's not incredibly fast but it's not incredibly slow.
AIInterface.prototype.OnTemplateModification = function(msg)
{
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	if (!this.templates)
		this.templates = cmpTemplateManager.FindAllTemplates(false);

	for each (var value in msg.valueNames)
	{
		for (var o = 0; o < this.templates.length; ++o)
		{
			var tmp = this.templates[o];
			var template = cmpTemplateManager.GetTemplateWithoutValidation(this.templates[o]);
			// remove templates that we obviously don't care about.
			if (!template || !template.Identity || ! template.Identity.Civ)
			{
				this.templates.splice(o--,1);
				continue;
			}

			// let's get the base template value.
			var strings = value.split("/");
			var item = template;
			var ended = true;
			for (var i = 0; i < strings.length; ++i)
			{
				if (item !== undefined && item[strings[i]] !== undefined)
					item = item[strings[i]];
				else
					ended = false;
			}
			if (!ended)
				continue;
			// item now contains the template value for this.
			
			// check for numerals, they need to be handled properly
			item = !isNaN(+item) ? +item : item;
			var newValue = ApplyValueModificationsToTemplate(value, item, msg.player, template);
			// round the value to 5th decimal or so.
			newValue = !isNaN(+newValue) ? (Math.abs((+newValue) - Math.round(+newValue)) < 0.0001 ? Math.round(+newValue) : +newValue) : newValue;

			if(item != newValue)
			{
				if (!this.changedTemplateInfo[msg.player])
					this.changedTemplateInfo[msg.player] = {};
				if (!this.changedTemplateInfo[msg.player][this.templates[o]])
					this.changedTemplateInfo[msg.player][this.templates[o]] = [ { "variable" : value, "value" : newValue} ];
				else
					this.changedTemplateInfo[msg.player][this.templates[o]].push({ "variable" : value, "value" : newValue });
			}
		}
	}
};

AIInterface.prototype.OnGlobalValueModification = function(msg)
{
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	for each (var ent in msg.entities)
	{
		var templateName = cmpTemplateManager.GetCurrentTemplateName(ent);
		// if there's no template name, the unit is probably killed, ignore it.
		if (!templateName || !templateName.length)
			continue;
		var template = cmpTemplateManager.GetTemplateWithoutValidation(templateName);
		for each (var value in msg.valueNames)
		{
			// let's get the base template value.
			var strings = value.split("/");
			var item = template;
			var ended = true;
			for (var i = 0; i < strings.length; ++i)
			{
				if (item !== undefined && item[strings[i]] !== undefined)
					item = item[strings[i]];
				else
					ended = false;
			}
			if (!ended)
				continue;
			// "item" now contains the unmodified template value for this.
			var newValue = ApplyValueModificationsToEntity(value, +item, ent);
			newValue = typeof(newValue) === "Number" ? Math.round(newValue) : newValue;
			if(item != newValue)
			{
				if (!this.changedEntityTemplateInfo[ent])
					this.changedEntityTemplateInfo[ent] = [{ "variable" : value, "value" : newValue }];
				else
					this.changedEntityTemplateInfo[ent].push({ "variable" : value, "value" : newValue });
			}
		}
	}
};

Engine.RegisterSystemComponentType(IID_AIInterface, "AIInterface", AIInterface);
