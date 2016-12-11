function Trigger() {}

Trigger.prototype.Schema =
	"<a:component type='system'/><empty/>";

/**
 * Events we're able to receive and call handlers for.
 */
Trigger.prototype.eventNames =
[
	"CinemaPathEnded",
	"CinemaQueueEnded",
	"ConstructionStarted",
	"InitGame",
	"Interval",
	"OwnershipChanged",
	"PlayerCommand",
	"PlayerDefeated",
	"PlayerWon",
	"Range",
	"ResearchFinished",
	"ResearchQueued",
	"StructureBuilt",
	"TrainingFinished",
	"TrainingQueued",
	"TreasureCollected"
];

Trigger.prototype.Init = function()
{
	this.triggerPoints = {};

	// Each event has its own set of actions determined by the map maker.
	for (let eventName of this.eventNames)
		this["On" + eventName + "Actions"] = {};
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
	let i = this.triggerPoints[ref].indexOf(ent);
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
 * Binds a function to the specified event.
 *
 * @param {string} event - One of eventNames
 * @param {string} action - Name of a function available to this object
 * @param {Object} data - f.e. enabled or not, delay for timers, range for range triggers
 *
 * @example
 * data = { enabled: true, interval: 1000, delay: 500 }
 *
 * Range trigger:
 * data.entities = [id1, id2] * Ids of the source
 * data.players = [1,2,3,...] * list of player ids
 * data.minRange = 0          * Minimum range for the query
 * data.maxRange = -1         * Maximum range for the query (-1 = no maximum)
 * data.requiredComponent = 0 * Required component id the entities will have
 * data.enabled = false       * If the query is enabled by default
 */
Trigger.prototype.RegisterTrigger = function(event, action, data)
{
	let eventString = event + "Actions";
	if (!this[eventString])
	{
		warn("Trigger.js: Invalid trigger event \"" + event + "\".");
		return;
	}
	if (this[eventString][action])
	{
		warn("Trigger.js: Trigger \"" + action + "\" has been registered before. Aborting...");
		return;
	}
	// clone the data to be sure it's only modified locally
	// We could run into triggers overwriting each other's data otherwise.
	// F.e. getting the wrong timer tag
	data = clone(data) || { "enabled": false };

	this[eventString][action] = data;

	// setup range query
	if (event == "OnRange")
	{
		if (!data.entities)
		{
			warn("Trigger.js: Range triggers should carry extra data");
			return;
		}
		data.queries = [];
		for (let ent of data.entities)
		{
			let cmpTriggerPoint = Engine.QueryInterface(ent, IID_TriggerPoint);
			if (!cmpTriggerPoint)
			{
				warn("Trigger.js: Range triggers must be defined on trigger points");
				continue;
			}
			data.queries.push(cmpTriggerPoint.RegisterRangeTrigger(action, data));
		}
	}

	if (data.enabled)
		this.EnableTrigger(event, action);
};

Trigger.prototype.DisableTrigger = function(event, action)
{
	let eventString = event + "Actions";
	if (!this[eventString][action])
	{
		warn("Trigger.js: Disabling unknown trigger");
		return;
	}
	let data = this[eventString][action];
	// special casing interval and range triggers for performance
	if (event == "OnInterval")
	{
		if (!data.timer) // don't disable it a second time
			return;
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(data.timer);
		data.timer = null;
	}
	else if (event == "OnRange")
	{
		if (!data.queries)
		{
			warn("Trigger.js: Range query wasn't set up before trying to disable it.");
			return;
		}
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		for (let query of data.queries)
			cmpRangeManager.DisableActiveQuery(query);
	}

	data.enabled = false;
};

Trigger.prototype.EnableTrigger = function(event, action)
{
	let eventString = event + "Actions";
	if (!this[eventString][action])
	{
		warn("Trigger.js: Enabling unknown trigger");
		return;
	}
	let data = this[eventString][action];
	// special casing interval and range triggers for performance
	if (event == "OnInterval")
	{
		if (data.timer) // don't enable it a second time
			return;
		if (!data.interval)
		{
			warn("Trigger.js: An interval trigger should have an intervel in its data");
			return;
		}
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		data.timer = cmpTimer.SetInterval(this.entity, IID_Trigger, "DoAction",
			data.delay || 0, data.interval, { "action" : action });
	}
	else if (event == "OnRange")
	{
		if (!data.queries)
		{
			warn("Trigger.js: Range query wasn't set up before");
			return;
		}
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		for (let query of data.queries)
			cmpRangeManager.EnableActiveQuery(query);
	}

	data.enabled = true;
};

/**
 * This function executes the actions bound to the events.
 * It's either called directlty from other simulation scripts,
 * or from message listeners in this file
 *
 * @param {string} event - One of eventNames
 * @param {Object} data - will be passed to the actions
 */
Trigger.prototype.CallEvent = function(event, data)
{
	let eventString = "On" + event + "Actions";

	if (!this[eventString])
	{
		warn("Trigger.js: Unknown trigger event called:\"" + event + "\".");
		return;
	}

	for (let action in this[eventString])
		if (this[eventString][action].enabled)
			this.DoAction({ "action": action, "data":data });
};

Trigger.prototype.OnGlobalInitGame = function(msg)
{
	this.CallEvent("InitGame", {});
};

Trigger.prototype.OnGlobalConstructionFinished = function(msg)
{
	this.CallEvent("StructureBuilt", { "building": msg.newentity });
};

Trigger.prototype.OnGlobalTrainingFinished = function(msg)
{
	this.CallEvent("TrainingFinished", msg);
	// The data for this one is {"entities": createdEnts,
	//							 "owner": cmpOwnership.GetOwner(),
	//							 "metadata": metadata}
	// See function "SpawnUnits" in ProductionQueue for more details
};

Trigger.prototype.OnGlobalResearchFinished = function(msg)
{
	this.CallEvent("ResearchFinished", msg);
	// The data for this one is { "player": playerID, "tech": tech }
};

Trigger.prototype.OnGlobalCinemaPathEnded = function(msg)
{
	this.CallEvent("CinemaPathEnded", msg);
};

Trigger.prototype.OnGlobalCinemaQueueEnded = function(msg)
{
	this.CallEvent("CinemaQueueEnded", msg);
};

Trigger.prototype.OnGlobalOwnershipChanged = function(msg)
{
	this.CallEvent("OwnershipChanged", msg);
	// data is {"entity": ent, "from": playerId, "to": playerId}
};

Trigger.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.CallEvent("PlayerDefeated", msg);
};

Trigger.prototype.OnGlobalPlayerWon = function(msg)
{
	this.CallEvent("PlayerWon", msg);
};

/**
 * Execute a function after a certain delay.
 *
 * @param {Number} time - delay in milleseconds
 * @param {String} action - Name of the action function
 * @param {Object} data - will be passed to the action function
 */
Trigger.prototype.DoAfterDelay = function(miliseconds, action, data)
{
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	return cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_Trigger, "DoAction", miliseconds, {
		"action": action,
		"data": data
	});
};

/**
 * Called by the trigger listeners to exucute the actual action. Including sanity checks.
 */
Trigger.prototype.DoAction = function(msg)
{
	if (this[msg.action])
		this[msg.action](msg.data || null);
	else
		warn("Trigger.js: called a trigger action '" + msg.action + "' that wasn't found");
};

Engine.RegisterSystemComponentType(IID_Trigger, "Trigger", Trigger);
