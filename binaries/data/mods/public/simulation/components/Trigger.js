function Trigger() {}

Trigger.prototype.Schema =
	"<a:component type='system'/><empty/>";

/**
 * Events we're able to receive and call handlers for
 */
Trigger.prototype.eventNames = 
[ 
	"StructureBuilt",  
	"ConstructionStarted",  
	"TrainingFinished",  
	"TrainingQueued",  
	"ResearchFinished",  
	"ResearchQueued",  
	"OwnershipChanged",
	"PlayerCommand",
	"Interval",
	"Range",
	"TreasureCollected",
]; 

Trigger.prototype.Init = function()
{
	this.triggerPoints = {};

	// Each event has its own set of actions determined by the map maker.
	for each (var eventName in this.eventNames) 
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
	var i = this.triggerPoints[ref].indexOf(ent);
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

/**  Binds the "action" function to one of the implemented events.
 *
 * @param event Name of the event (see the list in init)  
 * @param action Functionname of a function available under this object
 * @param Extra Data for the trigger (enabled or not, delay for timers, range for range triggers ...)
 *
 * Interval triggers example:
 * data = {enabled: true, interval: 1000, delay: 500}
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
	var eventString = event + "Actions";
	if (!this[eventString])
	{
		warn("Trigger.js: Invalid trigger event \"" + event + "\".") 
		return;
	}
	if (this[eventString][action])
	{
		warn("Trigger.js: Trigger has been registered before. Aborting...");
		return;
	}
	// clone the data to be sure it's only modified locally
	// We could run into triggers overwriting each other's data otherwise.
	// F.e. getting the wrong timer tag
	data = clone(data) || {"enabled": false};

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
		for (var ent of data.entities)
		{
			var cmpTriggerPoint = Engine.QueryInterface(ent, IID_TriggerPoint);
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

// Disable trigger
Trigger.prototype.DisableTrigger = function(event, action)
{
	var eventString = event + "Actions";
	if (!this[eventString][action])
	{
		warn("Trigger.js: Disabling unknown trigger");
		return;
	}
	var data = this[eventString][action];
	// special casing interval and range triggers for performance
	if (event == "OnInterval")
	{
		if (!data.timer) // don't disable it a second time
			return;
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
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
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		for (var query of data.queries)
			cmpRangeManager.DisableActiveQuery(query);
	}

	data.enabled = false;
};

// Enable trigger
Trigger.prototype.EnableTrigger = function(event, action)
{
	var eventString = event + "Actions";
	if (!this[eventString][action])
	{
		warn("Trigger.js: Enabling unknown trigger");
		return;
	}
	var data = this[eventString][action];
	// special casing interval and range triggers for performance
	if (event == "OnInterval")
	{
		if (data.timer) // don't enable it a second time
			return;
		if (!data.interval)
		{
			warn("Trigger.js: An interval trigger should have an intervel in its data")
			return;
		}
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		var timer = cmpTimer.SetInterval(this.entity, IID_Trigger, "DoAction", data.delay || 0, data.interval, {"action" : action});
		data.timer = timer;
	}
	else if (event == "OnRange")
	{
		if (!data.queries)
		{
			warn("Trigger.js: Range query wasn't set up before");
			return;
		}
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		for (var query of data.queries)
			cmpRangeManager.EnableActiveQuery(query);
	}

	data.enabled = true;
};

/** 
 * This function executes the actions bound to the events 
 * It's either called directlty from other simulation scripts,  
 * or from message listeners in this file 
 * 
 * @param event Name of the event (see the list in init) 
 * @param data Data object that will be passed to the actions 
 */ 
Trigger.prototype.CallEvent = function(event, data)
{
	var eventString = "On" + event + "Actions";
	
	if (!this[eventString])
	{
		warn("Trigger.js: Unknown trigger event called:\"" + event + "\".");
		return;
	}
	
	for (var action in this[eventString])
	{
		if (this[eventString][action].enabled)
			this.DoAction({"action": action, "data":data});
	}
};

// Handles "OnStructureBuilt" event.
Trigger.prototype.OnGlobalConstructionFinished = function(msg)
{
	this.CallEvent("StructureBuilt", {"building": msg.newentity}); // The data for this one is {"building": constructedBuilding}
};

// Handles "OnTrainingFinished" event.
Trigger.prototype.OnGlobalTrainingFinished = function(msg)
{
	this.CallEvent("TrainingFinished", msg);
	// The data for this one is {"entities": createdEnts,
	//							 "owner": cmpOwnership.GetOwner(),
	//							 "metadata": metadata}
	// See function "SpawnUnits" in ProductionQueue for more details
};

Trigger.prototype.OnGlobalOwnershipChanged = function(msg)
{
	this.CallEvent("OwnershipChanged", msg);
	// data is {"entity": ent, "from": playerId, "to": playerId}
};

/** 
 * Execute a function after a certain delay 
 * @param time The delay expressed in milleseconds 
 * @param action Name of the action function 
 * @param data Data object that will be passed to the action function 
 */ 
Trigger.prototype.DoAfterDelay = function(miliseconds, action, data)
{
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	return cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_Trigger, "DoAction", miliseconds, {"action": action, "data": data});
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
