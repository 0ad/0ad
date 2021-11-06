function Trigger() {}

Trigger.prototype.Schema =
	"<a:component type='system'/><empty/>";

/**
 * Events we're able to receive and call handlers for.
 */
Trigger.prototype.eventNames =
[
	"OnCinemaPathEnded",
	"OnCinemaQueueEnded",
	"OnConstructionStarted",
	"OnDiplomacyChanged",
	"OnDeserialized",
	"OnInitGame",
	"OnInterval",
	"OnEntityRenamed",
	"OnOwnershipChanged",
	"OnPlayerCommand",
	"OnPlayerDefeated",
	"OnPlayerWon",
	"OnRange",
	"OnResearchFinished",
	"OnResearchQueued",
	"OnStructureBuilt",
	"OnTrainingFinished",
	"OnTrainingQueued",
	"OnTreasureCollected"
];

Trigger.prototype.Init = function()
{
	// Difficulty used by trigger scripts (as defined in data/settings/trigger_difficulties.json).
	this.difficulty = undefined;

	this.triggerPoints = {};

	// Each event has its own set of actions determined by the map maker.
	this.triggers = {};
	for (const eventName of this.eventNames)
		this.triggers[eventName] = {};
};

Trigger.prototype.RegisterTriggerPoint = function(ref, ent)
{
	if (!this.triggerPoints[ref])
		this.triggerPoints[ref] = [];
	this.triggerPoints[ref].push(ent);
};

Trigger.prototype.RemoveRegisteredTriggerPoint = function(ref, ent)
{
	if (!this.triggerPoints[ref])
	{
		warn("no trigger points found with ref "+ref);
		return;
	}
	const i = this.triggerPoints[ref].indexOf(ent);
	if (i == -1)
	{
		warn("entity " + ent + " wasn't found under the trigger points with ref "+ref);
		return;
	}
	this.triggerPoints[ref].splice(i, 1);
};

Trigger.prototype.GetTriggerPoints = function(ref)
{
	return this.triggerPoints[ref] || [];
};

/**
 * Create a trigger listening on a specific event.
 *
 * @param {string} event - One of eventNames
 * @param {string} name - Name of the trigger.
 *     If no action is specified in triggerData, the action will be the trigger name.
 * @param {Object} triggerData - f.e. enabled or not, delay for timers, range for range triggers.
 * @param {Object} customData - User-defined data that will be forwarded to the action.
 *
 * @example
 * triggerData = { enabled: true, interval: 1000, delay: 500 }
 *
 * General settings:
 *     enabled = false       * If the trigger is enabled by default.
 *     action = name         * The function (on Trigger) to call. Defaults to the trigger name.
 *
 * Range trigger:
 *     entities = [id1, id2] * Ids of the source
 *     players = [1,2,3,...] * list of player ids
 *     minRange = 0          * Minimum range for the query
 *     maxRange = -1         * Maximum range for the query (-1 = no maximum)
 *     requiredComponent = 0 * Required component id the entities will have
 */
Trigger.prototype.RegisterTrigger = function(event, name, triggerData, customData = undefined)
{
	if (!this.triggers[event])
	{
		warn("Trigger.js: Invalid trigger event \"" + event + "\".");
		return;
	}
	if (this.triggers[event][name])
	{
		warn("Trigger.js: Trigger \"" + name + "\" has been registered before. Aborting...");
		return;
	}
	// clone the data to be sure it's only modified locally
	// We could run into triggers overwriting each other's data otherwise.
	// F.e. getting the wrong timer tag
	triggerData = clone(triggerData) || { "enabled": false };
	if (!triggerData.action)
		triggerData.action = name;

	this.triggers[event][name] = { "triggerData": triggerData, "customData": customData };

	// setup range query
	if (event == "OnRange")
	{
		if (!triggerData.entities)
		{
			warn("Trigger.js: Range triggers should carry extra data");
			return;
		}
		triggerData.queries = [];
		for (const ent of triggerData.entities)
		{
			const cmpTriggerPoint = Engine.QueryInterface(ent, IID_TriggerPoint);
			if (!cmpTriggerPoint)
			{
				warn("Trigger.js: Range triggers must be defined on trigger points");
				continue;
			}
			triggerData.queries.push(cmpTriggerPoint.RegisterRangeTrigger(name, triggerData));
		}
	}

	if (triggerData.enabled)
		this.EnableTrigger(event, name);
};

Trigger.prototype.DisableTrigger = function(event, name)
{
	if (!this.triggers[event][name])
	{
		warn("Trigger.js: Disabling unknown trigger " + name);
		return;
	}

	const triggerData = this.triggers[event][name].triggerData;
	// special casing interval and range triggers for performance
	if (event == "OnInterval")
	{
		if (!triggerData.timer) // don't disable it a second time
			return;
		const cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(triggerData.timer);
		triggerData.timer = null;
	}
	else if (event == "OnRange")
	{
		if (!triggerData.queries)
		{
			warn("Trigger.js: Range query wasn't set up before trying to disable it.");
			return;
		}
		const cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		for (const query of triggerData.queries)
			cmpRangeManager.DisableActiveQuery(query);
	}

	triggerData.enabled = false;
};

Trigger.prototype.EnableTrigger = function(event, name)
{
	if (!this.triggers[event][name])
	{
		warn("Trigger.js: Enabling unknown trigger " + name);
		return;
	}
	const triggerData = this.triggers[event][name].triggerData;
	// special casing interval and range triggers for performance
	if (event == "OnInterval")
	{
		if (triggerData.timer) // don't enable it a second time
			return;
		if (!triggerData.interval)
		{
			warn("Trigger.js: An interval trigger should have an intervel in its data");
			return;
		}
		const cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		triggerData.timer = cmpTimer.SetInterval(this.entity, IID_Trigger, "DoAction",
			triggerData.delay || 0, triggerData.interval, { "action": name });
	}
	else if (event == "OnRange")
	{
		if (!triggerData.queries)
		{
			warn("Trigger.js: Range query wasn't set up before");
			return;
		}
		const cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		for (const query of triggerData.queries)
			cmpRangeManager.EnableActiveQuery(query);
	}

	triggerData.enabled = true;
};

Trigger.prototype.OnGlobalInitGame = function(msg)
{
	this.CallEvent("OnInitGame", {});
};

Trigger.prototype.OnGlobalConstructionFinished = function(msg)
{
	this.CallEvent("OnStructureBuilt", { "building": msg.newentity, "foundation": msg.entity });
};

Trigger.prototype.OnGlobalTrainingFinished = function(msg)
{
	this.CallEvent("OnTrainingFinished", msg);
	// The data for this one is {"entities": createdEnts,
	//							 "owner": cmpOwnership.GetOwner(),
	//							 "metadata": metadata}
	// See function "SpawnUnits" in ProductionQueue for more details
};

Trigger.prototype.OnGlobalResearchFinished = function(msg)
{
	this.CallEvent("OnResearchFinished", msg);
	// The data for this one is { "player": playerID, "tech": tech }
};

Trigger.prototype.OnGlobalCinemaPathEnded = function(msg)
{
	this.CallEvent("OnCinemaPathEnded", msg);
};

Trigger.prototype.OnGlobalCinemaQueueEnded = function(msg)
{
	this.CallEvent("OnCinemaQueueEnded", msg);
};

Trigger.prototype.OnGlobalDeserialized = function(msg)
{
	this.CallEvent("OnDeserialized", msg);
};

Trigger.prototype.OnGlobalEntityRenamed = function(msg)
{
	this.CallEvent("OnEntityRenamed", msg);
};

Trigger.prototype.OnGlobalOwnershipChanged = function(msg)
{
	this.CallEvent("OnOwnershipChanged", msg);
	// data is {"entity": ent, "from": playerId, "to": playerId}
};

Trigger.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.CallEvent("OnPlayerDefeated", msg);
};

Trigger.prototype.OnGlobalPlayerWon = function(msg)
{
	this.CallEvent("OnPlayerWon", msg);
};

Trigger.prototype.OnGlobalDiplomacyChanged = function(msg)
{
	this.CallEvent("OnDiplomacyChanged", msg);
};

/**
 * Execute a function after a certain delay.
 *
 * @param {number} time - Delay in milliseconds.
 * @param {string} action - Name of the action function.
 * @param {Object} eventData - Arbitrary object that will be passed to the action function.
 * @return {number} The ID of the timer, so it can be stopped later.
 */
Trigger.prototype.DoAfterDelay = function(time, action, eventData)
{
	const cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	return cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_Trigger, "DoAction", time, {
		"action": action,
		"eventData": eventData
	});
};

/**
 * Execute a function each time a certain delay has passed.
 *
 * @param {number} interval - Interval in milleseconds between consecutive calls.
 * @param {string} action - Name of the action function.
 * @param {Object} eventData - Arbitrary object that will be passed to the action function.
 * @param {number} [start] - Optional initial delay in milleseconds before starting the calls.
 *                           If not given, interval will be used.
 * @return {number} the ID of the timer, so it can be stopped later.
 */
Trigger.prototype.DoRepeatedly = function(time, action, eventData, start)
{
	const cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	return cmpTimer.SetInterval(SYSTEM_ENTITY, IID_Trigger, "DoAction", start !== undefined ? start : time, time, {
		"action": action,
		"eventData": eventData
	});
};

/**
 * This function executes the actions bound to the events.
 * It's either called directlty from other simulation scripts,
 * or from message listeners in this file
 *
 * @param {string} event - One of eventNames
 * @param {Object} data - will be passed to the actions
 */
Trigger.prototype.CallEvent = function(event, eventData)
{
	if (!this.triggers[event])
	{
		warn("Trigger.js: Unknown trigger event called:\"" + event + "\".");
		return;
	}

	for (const name in this.triggers[event])
		if (this.triggers[event][name].triggerData.enabled)
			this.DoAction({
				"action": this.triggers[event][name].triggerData.action,
				"eventData": eventData,
				"customData": this.triggers[event][name].customData,
				"triggerData": this.triggers[event][name].triggerData
			});
};

/**
 * Call the action method of a trigger with the given event Data.
 * By default, call the trigger even if it is currently disabled.
 */
Trigger.prototype.CallTrigger = function(event, name, eventData, evenIfDisabled = true)
{
	if (!this.triggers[event]?.[name])
	{
		warn(`Trigger.js: called a trigger '${name}' for event '${event}' that wasn't found`);
		return;
	}

	if (!evenIfDisabled && !this.triggers[event][name].triggerData.enabled)
		return;

	this.DoAction({
		"action": this.triggers[event][name].triggerData.action,
		"eventData": eventData,
		"customData": this.triggers[event][name].customData,
		"triggerData": this.triggers[event][name].triggerData
	});
};


/**
 * Called by the trigger listeners to execute the actual action. Including sanity checks.
 * Intended for internal use, prefer CallEvent or CallTrigger.
 */
Trigger.prototype.DoAction = function(msg)
{
	if (this[msg.action])
		this[msg.action](msg?.eventData, msg?.customData, msg?.triggerData);
	else
		warn("Trigger.js: called a trigger action '" + msg.action + "' that wasn't found");
};

/**
 * Level of difficulty used by trigger scripts.
 */
Trigger.prototype.GetDifficulty = function()
{
	return this.difficulty;
};

Trigger.prototype.SetDifficulty = function(diff)
{
	this.difficulty = diff;
};

Engine.RegisterSystemComponentType(IID_Trigger, "Trigger", Trigger);
