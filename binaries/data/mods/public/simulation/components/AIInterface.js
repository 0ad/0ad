function AIInterface() {}

AIInterface.prototype.Schema =
	"<a:component type='system'/><empty/>";

AIInterface.prototype.EventNames = [
	"Create",
	"Destroy",
	"Attacked",
	"ConstructionFinished",
	"TrainingStarted",
	"TrainingFinished",
	"AIMetadata",
	"PlayerDefeated",
	"EntityRenamed",
	"OwnershipChanged",
	"Garrison",
	"UnGarrison",
	"TerritoryDecayChanged",
	"TributeExchanged",
	"AttackRequest"
];

AIInterface.prototype.Init = function()
{
	this.events = {};
	for (let name of this.EventNames)
		this.events[name] = [];

	this.changedEntities = {};

	// cache for technology changes;
	// this one is PlayerID->TemplateName->{StringForTheValue, ActualValue}
	this.changedTemplateInfo = {};
	// this is for auras and is EntityID->{StringForTheValue, ActualValue}
	this.changedEntityTemplateInfo = {};
	this.enabled = true;
};

AIInterface.prototype.Serialize = function()
{
	var state = {};
	for (var key in this)
	{
		if (!this.hasOwnProperty(key))
			continue;
		if (typeof this[key] == "function")
			continue;
		state[key] = this[key];
	}
	return state;
};

AIInterface.prototype.Deserialize = function(data)
{
	for (var key in data)
	{
		if (!data.hasOwnProperty(key))
			continue;
		this[key] = data[key];
	}
	if (!this.enabled)
		this.Disable();
};

/**
 * Disable all registering functions for this component
 * Gets called in case no AI players are present to save resources
 */
AIInterface.prototype.Disable = function()
{
	this.enabled = false;
	var nop = function(){};
	this.ChangedEntity = nop;
	this.PushEvent = nop;
	this.OnGlobalPlayerDefeated = nop;
	this.OnGlobalEntityRenamed = nop;
	this.OnGlobalTributeExchanged = nop;
	this.OnTemplateModification = nop;
	this.OnGlobalValueModification = nop;
};

AIInterface.prototype.GetNonEntityRepresentation = function()
{
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	// Return the same game state as the GUI uses
	var state = cmpGuiInterface.GetSimulationState(-1);
	
	// Add some extra AI-specific data
	// add custom events and reset them for the next turn
	state.events = {};
	for (let name of this.EventNames)
	{
		state.events[name] = this.events[name];
		this.events[name] = [];
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
		for (let name of this.EventNames)
			state.events[name] = [];

	// Add entity representations
	Engine.ProfileStart("proxy representations");
	state.entities = {};
	// all entities are changed in the initial state.
	for (let id of Engine.GetEntitiesWithInterface(IID_AIProxy))
		state.entities[id] = Engine.QueryInterface(id, IID_AIProxy).GetFullRepresentation();
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
	var cmpMirage = Engine.QueryInterface(msg.entity, IID_Mirage);
	if (!cmpMirage)
		this.events["EntityRenamed"].push(msg);
};

AIInterface.prototype.OnGlobalTributeExchanged = function(msg)
{
	this.events["TributeExchanged"].push(msg);
};

// When a new technology is researched, check which templates it affects,
// and send the updated values to the AI.
// this relies on the fact that any "value" in a technology can only ever change
// one template value, and that the naming is the same (with / in place of .)
// it's not incredibly fast but it's not incredibly slow.
AIInterface.prototype.OnTemplateModification = function(msg)
{
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	if (!this.templates)
	{
		this.templates = cmpTemplateManager.FindAllTemplates(false);
		for (let i = 0; i < this.templates.length; ++i)
		{
			// remove templates that we obviously don't care about.
			if (this.templates[i].startsWith("skirmish/"))
				this.templates.splice(i--,1);
			else
			{
				let template = cmpTemplateManager.GetTemplateWithoutValidation(this.templates[i]);
				if (!template || !template.Identity || !template.Identity.Civ)
					this.templates.splice(i--,1);
			}
		}
	}

	for (let name of this.templates)
	{
		let template = cmpTemplateManager.GetTemplateWithoutValidation(name);

		for (let valName of msg.valueNames)
		{
			// let's get the base template value.
			let strings = valName.split("/");
			let item = template;
			let ended = true;
			for (let str of strings)
			{
				if (item !== undefined && item[str] !== undefined)
					item = item[str];
				else
					ended = false;
			}
			if (!ended)
				continue;
			// item now contains the template value for this.
			let oldValue = +item;
			let newValue = ApplyValueModificationsToTemplate(valName, oldValue, msg.player, template);
			// Apply the same roundings as in the components
			if (valName === "Player/MaxPopulation" || valName === "Cost/Population" || valName === "Cost/PopulationBonus")
				newValue = Math.round(newValue);

			// TODO in some cases, we can have two opposite changes which bring us to the old value,
			// and we should keep it. But how to distinguish it ?
			if(newValue == oldValue)
				continue;
			if (!this.changedTemplateInfo[msg.player])
				this.changedTemplateInfo[msg.player] = {};
			if (!this.changedTemplateInfo[msg.player][name])
				this.changedTemplateInfo[msg.player][name] = [{"variable": valName, "value": newValue}];
			else
				this.changedTemplateInfo[msg.player][name].push({"variable": valName, "value": newValue});
		}
	}
};

AIInterface.prototype.OnGlobalValueModification = function(msg)
{
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	for (let ent of msg.entities)
	{
		let templateName = cmpTemplateManager.GetCurrentTemplateName(ent);
		// if there's no template name, the unit is probably killed, ignore it.
		if (!templateName || !templateName.length)
			continue;
		let template = cmpTemplateManager.GetTemplateWithoutValidation(templateName);
		for (let valName of msg.valueNames)
		{
			// let's get the base template value.
			let strings = valName.split("/");
			let item = template;
			let ended = true;
			for (let str of strings)
			{
				if (item !== undefined && item[str] !== undefined)
					item = item[str];
				else
					ended = false;
			}
			if (!ended)
				continue;
			// "item" now contains the unmodified template value for this.
			let oldValue = +item;
			let newValue = ApplyValueModificationsToEntity(valName, oldValue, ent);
			// Apply the same roundings as in the components
			if (valName === "Player/MaxPopulation" || valName === "Cost/Population" || valName === "Cost/PopulationBonus")
				newValue = Math.round(newValue);
			// TODO in some cases, we can have two opposite changes which bring us to the old value,
			// and we should keep it. But how to distinguish it ?
			if (newValue == oldValue)
				continue;
			if (!this.changedEntityTemplateInfo[ent])
				this.changedEntityTemplateInfo[ent] = [{"variable": valName, "value": newValue}];
			else
				this.changedEntityTemplateInfo[ent].push({"variable": valName, "value": newValue});
		}
	}
};

Engine.RegisterSystemComponentType(IID_AIInterface, "AIInterface", AIInterface);
