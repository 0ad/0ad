function AIInterface() {}

AIInterface.prototype.Schema =
	"<a:component type='system'/><empty/>";

AIInterface.prototype.EventNames = [
	"Create",
	"Destroy",
	"Attacked",
	"ConstructionFinished",
	"DiplomacyChanged",
	"TrainingStarted",
	"TrainingFinished",
	"AIMetadata",
	"PlayerDefeated",
	"EntityRenamed",
	"OwnershipChanged",
	"Garrison",
	"UnGarrison",
	"TerritoriesChanged",
	"TerritoryDecayChanged",
	"TributeExchanged",
	"AttackRequest",
	"CeasefireEnded",
	"DiplomacyRequest",
	"TributeRequest"
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
	let state = {};
	for (var key in this)
	{
		if (!this.hasOwnProperty(key))
			continue;
		if (typeof this[key] == "function")
			continue;
		if (key == "templates")
			continue;
		state[key] = this[key];
	}
	return state;
};

AIInterface.prototype.Deserialize = function(data)
{
	for (let key in data)
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
	let nop = function(){};
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
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	// Return the same game state as the GUI uses
	let state = cmpGuiInterface.GetSimulationState();

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
	let state = this.GetNonEntityRepresentation();

	// Add entity representations
	Engine.ProfileStart("proxy representations");
	state.entities = {};
	for (let id in this.changedEntities)
	{
		let cmpAIProxy = Engine.QueryInterface(+id, IID_AIProxy);
		if (cmpAIProxy)
			state.entities[id] = cmpAIProxy.GetRepresentation();
	}
	this.changedEntities = {};
	Engine.ProfileStop();

	state.changedTemplateInfo = this.changedTemplateInfo;
	this.changedTemplateInfo = {};
	state.changedEntityTemplateInfo = this.changedEntityTemplateInfo;
	this.changedEntityTemplateInfo = {};

	return state;
};

/**
 * Intended to be called first, during the map initialization: no caching
 */
AIInterface.prototype.GetFullRepresentation = function(flushEvents)
{
	let state = this.GetNonEntityRepresentation();

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

/**
 * AIProxy sets up a load of event handlers to capture interesting things going on
 * in the world, which we will report to AI. Handle those, and add a few more handlers
 * for events that AIProxy won't capture.
 */
AIInterface.prototype.PushEvent = function(type, msg)
{
	if (this.events[type] === undefined)
		warn("Tried to push unknown event type " + type +", please add it to AIInterface.js");
	this.events[type].push(msg);
};

AIInterface.prototype.OnDiplomacyChanged = function(msg)
{
	this.events.DiplomacyChanged.push(msg);
};

AIInterface.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.events.PlayerDefeated.push(msg);
};

AIInterface.prototype.OnGlobalEntityRenamed = function(msg)
{
	if (!Engine.QueryInterface(msg.entity, IID_Mirage))
		this.events.EntityRenamed.push(msg);
};

AIInterface.prototype.OnGlobalTributeExchanged = function(msg)
{
	this.events.TributeExchanged.push(msg);
};

AIInterface.prototype.OnTerritoriesChanged = function(msg)
{
	this.events.TerritoriesChanged.push(msg);
};

AIInterface.prototype.OnCeasefireEnded = function(msg)
{
	this.events.CeasefireEnded.push(msg);
};

/**
 * When a new technology is researched, check which templates it affects,
 * and send the updated values to the AI.
 * this relies on the fact that any "value" in a technology can only ever change
 * one template value, and that the naming is the same (with / in place of .)
 */
AIInterface.prototype.OnTemplateModification = function(msg)
{
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	if (!this.templates)
	{
		this.templates = [];
		for (let templateName of cmpTemplateManager.FindAllTemplates(false))
		{
			// Remove templates that we obviously don't care about.
			if (templateName.startsWith("campaigns/") || templateName.startsWith("rubble/") ||
			    templateName.startsWith("skirmish/"))
				continue;
			let template = cmpTemplateManager.GetTemplateWithoutValidation(templateName);
			if (!template || !template.Identity || !template.Identity.Civ)
				continue;
			this.templates.push(templateName);
		}
	}

	for (let name of this.templates)
	{
		let template = cmpTemplateManager.GetTemplateWithoutValidation(name);
		if (!template || !template[msg.component])
			continue;
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
			if (valName === "Player/MaxPopulation" || valName === "Cost/Population" ||
			    valName === "Cost/PopulationBonus")
				newValue = Math.round(newValue);
			// TODO in some cases, we can have two opposite changes which bring us to the old value,
			// and we should keep it. But how to distinguish it ?
			if(newValue == oldValue)
				continue;
			if (!this.changedTemplateInfo[msg.player])
				this.changedTemplateInfo[msg.player] = {};
			if (!this.changedTemplateInfo[msg.player][name])
				this.changedTemplateInfo[msg.player][name] = [{ "variable": valName, "value": newValue }];
			else
				this.changedTemplateInfo[msg.player][name].push({ "variable": valName, "value": newValue });
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
		if (!template || !template[msg.component])
			continue;
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
			if (valName === "Player/MaxPopulation" || valName === "Cost/Population" ||
			    valName === "Cost/PopulationBonus")
				newValue = Math.round(newValue);
			// TODO in some cases, we can have two opposite changes which bring us to the old value,
			// and we should keep it. But how to distinguish it ?
			if (newValue == oldValue)
				continue;
			if (!this.changedEntityTemplateInfo[ent])
				this.changedEntityTemplateInfo[ent] = [{ "variable": valName, "value": newValue }];
			else
				this.changedEntityTemplateInfo[ent].push({ "variable": valName, "value": newValue });
		}
	}
};

Engine.RegisterSystemComponentType(IID_AIInterface, "AIInterface", AIInterface);
