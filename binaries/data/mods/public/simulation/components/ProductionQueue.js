var g_ProgressInterval = 1000;
const MAX_QUEUE_SIZE = 16;

function ProductionQueue() {}

ProductionQueue.prototype.Schema =
	"<a:help>Allows the building to train new units and research technologies</a:help>" +
	"<a:example>" +
		"<Entities datatype='tokens'>" +
			"\n    units/{civ}_support_female_citizen\n    units/{civ}_support_trader\n    units/celt_infantry_spearman_b\n  " +
		"</Entities>" +
	"</a:example>" +
	"<optional>" + 
		"<element name='Entities' a:help='Space-separated list of entity template names that this building can train. The special string \"{civ}\" will be automatically replaced by the building&apos;s four-character civ code'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" + 
	"<optional>" + 
		"<element name='Technologies' a:help='Space-separated list of technology names that this building can research.'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>";

ProductionQueue.prototype.Init = function()
{
	this.nextID = 1;

	this.queue = [];
	// Queue items are:
	//   {
	//     "id": 1,
	//     "player": 1, // who paid for this batch; we need this to cope with refunds cleanly
	//     "unitTemplate": "units/example",
	//     "count": 10,
	//     "resources": { "wood": 100, ... },	// resources per unit, multiply by count to get total
	//     "population": 1,	// population per unit, multiply by count to get total
	//     "productionStarted": false, // true iff we have reserved population
	//     "timeTotal": 15000, // msecs
	//     "timeRemaining": 10000, // msecs
	//   }
	//   
	//   {
	//     "id": 1,
	//     "player": 1, // who paid for this research; we need this to cope with refunds cleanly
	//     "technologyTemplate": "example_tech",
	//     "resources": { "wood": 100, ... },	// resources needed for research
	//     "productionStarted": false, // true iff production has started
	//     "timeTotal": 15000, // msecs
	//     "timeRemaining": 10000, // msecs
	//   }
	
	this.timer = undefined; // g_ProgressInterval msec timer, active while the queue is non-empty
	
	this.entityCache = [];
	this.spawnNotified = false;
};

/*
 * Returns list of entities that can be trained by this building.
 */
ProductionQueue.prototype.GetEntitiesList = function()
{
	if (!this.template.Entities)
		return [];
	
	var string = this.template.Entities._string;
	
	// Replace the "{civ}" codes with this entity's civ ID
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		string = string.replace(/\{civ\}/g, cmpIdentity.GetCiv());
	
	return string.split(/\s+/);
};

/*
 * Returns list of technologies that can be researched by this building.
 */
ProductionQueue.prototype.GetTechnologiesList = function()
{
	if (!this.template.Technologies)
		return [];
	
	var string = this.template.Technologies._string;
	
	var cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	if (!cmpTechnologyManager)
		return [];
	
	var techs = string.split(/\s+/);
	var techList = [];
	var superseded = {}; // Stores the tech which supersedes the key 
	
	// Add any top level technologies to an array which corresponds to the displayed icons
	// Also store what a technology is superceded by in the superceded object {"tech1":"techWhichSupercedesTech1", ...}
	for (var i in techs)
	{
		var tech = techs[i];
		var template = cmpTechnologyManager.GetTechnologyTemplate(tech);
		if (!template.supersedes || techs.indexOf(template.supersedes) === -1)
			techList.push(tech);
		else
			superseded[template.supersedes] = tech;
	}
	
	// Now make researched/in progress techs invisible
	for (var i in techList)
	{
		var tech = techList[i];
		while (this.IsTechnologyResearchedOrInProgress(tech))
		{
			tech = superseded[tech];
		}
		
		techList[i] = tech;
	}
	
	var ret = []
	
	// This inserts the techs into the correct positions to line up the tehnology pairs
	for (var i = 0; i < techList.length; i++)
	{
		var tech = techList[i];
		if (!tech)
		{
			ret[i] = undefined;
			continue;
		}
		
		var template = cmpTechnologyManager.GetTechnologyTemplate(tech);
		if (template.top)
			ret[i] = {"pair": true, "top": template.top, "bottom": template.bottom};
		else
			ret[i] = tech;
	}
	
	return ret;
};

ProductionQueue.prototype.IsTechnologyResearchedOrInProgress = function(tech)
{
	if (!tech)
		return false;
	
	var cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	
	var template = cmpTechnologyManager.GetTechnologyTemplate(tech);
	if (template.top)
	{
		return (cmpTechnologyManager.IsTechnologyResearched(template.top) || cmpTechnologyManager.IsInProgress(template.top)
			|| cmpTechnologyManager.IsTechnologyResearched(template.bottom) || cmpTechnologyManager.IsInProgress(template.bottom))
	}
	else
	{
		return (cmpTechnologyManager.IsTechnologyResearched(tech) || cmpTechnologyManager.IsInProgress(tech))
	}
};

/*
 * Adds a new batch of identical units to train or a technology to research to the production queue.
 */
ProductionQueue.prototype.AddBatch = function(templateName, type, count, metadata)
{
	// TODO: there should probably be a limit on the number of queued batches
	// TODO: there should be a way for the GUI to determine whether it's going
	// to be possible to add a batch (based on resource costs and length limits)
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	if (this.queue.length < MAX_QUEUE_SIZE)
	{
		
		if (type == "unit")
		{
			// Find the template data so we can determine the build costs
			var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
			var template = cmpTempMan.GetTemplate(templateName);
			if (!template)
				return;
			
			// Apply a time discount to larger batches.
			// TODO: work out what equation we should use here.
			var timeMult = Math.pow(count, 0.7) * cmpPlayer.cheatTimeMultiplier;
			
			// We need the costs after tech modifications
			// Obviously we don't have the entities yet, so we must use template data
			var costs = {};
			var totalCosts = {};
			var buildTime = +template.Cost.BuildTime;
			
			var cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
			if (cmpTechnologyManager)
				buildTime = cmpTechnologyManager.ApplyModificationsTemplate("Cost/BuildTime", buildTime, template);
			var time = timeMult * buildTime;

			for (var r in template.Cost.Resources)
			{
				costs[r] = +template.Cost.Resources[r];
				if (cmpTechnologyManager)
					costs[r] = cmpTechnologyManager.ApplyModificationsTemplate("Cost/Resources/"+r, costs[r], template);
				totalCosts[r] = Math.floor(count * costs[r]);
			}

			var population = +template.Cost.Population;

			// TrySubtractResources should report error to player (they ran out of resources)
			if (!cmpPlayer.TrySubtractResources(totalCosts))
				return;

			// Update entity count in the EntityLimits component
			if (template.TrainingRestrictions)
			{
				var unitCategory = template.TrainingRestrictions.Category;
				var cmpPlayerEntityLimits = QueryOwnerInterface(this.entity, IID_EntityLimits);
				cmpPlayerEntityLimits.IncreaseCount(unitCategory, count);
			}

			this.queue.push({
				"id": this.nextID++,
				"player": cmpPlayer.GetPlayerID(),
				"unitTemplate": templateName,
				"count": count,
				"metadata": metadata,
				"resources": costs,
				"population": population,
				"productionStarted": false,
				"timeTotal": time*1000,
				"timeRemaining": time*1000,
			});
		}
		else if (type == "technology")
		{
			// Load the technology template
			var cmpTechTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TechnologyTemplateManager);
			var template = cmpTechTempMan.GetTemplate(templateName);
			if (!template)
				return;
			var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
			var time = template.researchTime * cmpPlayer.cheatTimeMultiplier;

			var cost = {};
			for each (var r in ["food", "wood", "stone", "metal"])
				cost[r] = Math.floor(template.cost[r]);
			
			// TrySubtractResources should report error to player (they ran out of resources)
			if (!cmpPlayer.TrySubtractResources(cost))
				return;
			
			// Tell the technology manager that we have started researching this so that people can't research the same 
			// thing twice.
			var cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
			cmpTechnologyManager.QueuedResearch(templateName, this.entity);
			if (this.queue.length == 0)
				cmpTechnologyManager.StartedResearch(templateName);

			this.queue.push({
				"id": this.nextID++,
				"player": cmpPlayer.GetPlayerID(),
				"count": 1,
				"technologyTemplate": templateName,
				"resources": deepcopy(template.cost), // need to copy to avoid serialization problems
				"productionStarted": false,
				"timeTotal": time*1000,
				"timeRemaining": time*1000,
			});
		}
		else
		{
			warn("Tried to add invalid item of type \"" + type + "\" and template \"" + templateName + "\" to a production queue");
			return;
		}
		
		Engine.PostMessage(this.entity, MT_ProductionQueueChanged, { });

		// If this is the first item in the queue, start the timer
		if (!this.timer)
		{
			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			this.timer = cmpTimer.SetTimeout(this.entity, IID_ProductionQueue, "ProgressTimeout", g_ProgressInterval, {});
		}
	}
	else
	{
		var notification = {"player": cmpPlayer.GetPlayerID(), "message": "The production queue is full."};
		var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification(notification);
	}
};

/*
 * Removes an existing batch of units from the production queue.
 * Refunds resource costs and population reservations.
 */
ProductionQueue.prototype.RemoveBatch = function(id)
{
	// Destroy any cached entities (those which didn't spawn for some reason)
	for (var i = 0; i < this.entityCache.length; ++i)
	{
		Engine.DestroyEntity(this.entityCache[i]);
	}
	this.entityCache = [];
	
	for (var i = 0; i < this.queue.length; ++i)
	{
		var item = this.queue[i];
		if (item.id != id)
			continue;

		// Now we've found the item to remove
		
		var cmpPlayer = QueryPlayerIDInterface(item.player, IID_Player);

		// Update entity count in the EntityLimits component
		if (item.unitTemplate)
		{
			var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
			var template = cmpTempMan.GetTemplate(item.unitTemplate);
			if (template.TrainingRestrictions)
			{
				var unitCategory = template.TrainingRestrictions.Category;
				var cmpPlayerEntityLimits = QueryOwnerInterface(this.entity, IID_EntityLimits);
				cmpPlayerEntityLimits.DecreaseCount(unitCategory, item.count);
			}
		}

		// Refund the resource cost for this batch
		var totalCosts = {};
		var cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
		for each (var r in ["food", "wood", "stone", "metal"])
		{
			totalCosts[r] = Math.floor(item.count * item.resources[r]);
			if (cmpStatisticsTracker)
				cmpStatisticsTracker.IncreaseResourceUsedCounter(r, -totalCosts[r]);
		}
		
		cmpPlayer.AddResources(totalCosts);
		
		// Remove reserved population slots if necessary
		if (item.productionStarted && item.unitTemplate)
			cmpPlayer.UnReservePopulationSlots(item.population * item.count);
		
		// Mark the research as stopped if we cancel it
		if (item.technologyTemplate)
		{
			// item.player is used as this.entity's owner may be invalid (deletion, etc.)
			var cmpTechnologyManager = QueryPlayerIDInterface(item.player, IID_TechnologyManager);
			cmpTechnologyManager.StoppedResearch(item.technologyTemplate);
		}
		
		// Remove from the queue
		// (We don't need to remove the timer - it'll expire if it discovers the queue is empty)
		this.queue.splice(i, 1);
		Engine.PostMessage(this.entity, MT_ProductionQueueChanged, { });

		return;
	}
};

/*
 * Returns basic data from all batches in the production queue.
 */
ProductionQueue.prototype.GetQueue = function()
{
	var out = [];
	for each (var item in this.queue)
	{
		out.push({
			"id": item.id,
			"unitTemplate": item.unitTemplate,
			"technologyTemplate": item.technologyTemplate,
			"count": item.count,
			"progress": 1-(item.timeRemaining/item.timeTotal),
			"metadata": item.metadata,
		});
	}
	return out;
};

/*
 * Removes all existing batches from the queue.
 */
ProductionQueue.prototype.ResetQueue = function()
{
	// Empty the production queue and refund all the resource costs
	// to the player. (This is to avoid players having to micromanage their
	// buildings' queues when they're about to be destroyed or captured.)

	while (this.queue.length)
		this.RemoveBatch(this.queue[0].id);
};

ProductionQueue.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.from != -1)
	{
		// Unset flag that previous owner's training may be blocked
		var cmpPlayer = QueryPlayerIDInterface(msg.from, IID_Player);
		if (cmpPlayer && this.queue.length > 0)
			cmpPlayer.UnBlockTraining();
	}

	// Reset the production queue whenever the owner changes.
	// (This should prevent players getting surprised when they capture
	// an enemy building, and then loads of the enemy's civ's soldiers get
	// created from it. Also it means we don't have to worry about
	// updating the reserved pop slots.)
	this.ResetQueue();
};

ProductionQueue.prototype.OnDestroy = function()
{
	// Reset the queue to refund any resources
	this.ResetQueue();

	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
	}
};

/*
 * This function creates the entities and places them in world if possible.
 * returns the number of successfully spawned entities.
 */
ProductionQueue.prototype.SpawnUnits = function(templateName, count, metadata)
{
	var cmpFootprint = Engine.QueryInterface(this.entity, IID_Footprint);
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpRallyPoint = Engine.QueryInterface(this.entity, IID_RallyPoint);
	
	var spawnedEnts = [];
	
	if (this.entityCache.length == 0)
	{
		// We need entities to test spawning, but we don't want to waste resources,
		//	so only create them once and use as needed
		for (var i = 0; i < count; ++i)
		{
			var ent = Engine.AddEntity(templateName);
			this.entityCache.push(ent);

			// Decrement entity count in the EntityLimits component
			// since it will be increased by EntityLimits.OnGlobalOwnershipChanged function,
			// i.e. we replace a 'trained' entity to an 'alive' one
			var cmpTrainingRestrictions = Engine.QueryInterface(ent, IID_TrainingRestrictions);
			if (cmpTrainingRestrictions)
			{
				var unitCategory = cmpTrainingRestrictions.GetCategory();
				var cmpPlayerEntityLimits = QueryOwnerInterface(this.entity, IID_EntityLimits);
				cmpPlayerEntityLimits.DecrementCount(unitCategory);
			}
		}
	}

	for (var i = 0; i < count; ++i)
	{
		var ent = this.entityCache[0];
		var pos = cmpFootprint.PickSpawnPoint(ent);
		if (pos.y < 0)
		{
			// Fail: there wasn't any space to spawn the unit
			break;
		}
		else
		{
			// Successfully spawned
			var cmpNewPosition = Engine.QueryInterface(ent, IID_Position);
			cmpNewPosition.JumpTo(pos.x, pos.z);
			// TODO: what direction should they face in?

			var cmpNewOwnership = Engine.QueryInterface(ent, IID_Ownership);
			cmpNewOwnership.SetOwner(cmpOwnership.GetOwner());
			
			var cmpPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
			cmpPlayerStatisticsTracker.IncreaseTrainedUnitsCounter();

			// Play a sound, but only for the first in the batch (to avoid nasty phasing effects)
			if (spawnedEnts.length == 0)
				PlaySound("trained", ent);
			
			this.entityCache.shift();
			spawnedEnts.push(ent);
		}
	}

	if (spawnedEnts.length > 0)
	{
		// If a rally point is set, walk towards it (in formation) using a suitable command based on where the 
		// rally point is placed.
		if (cmpRallyPoint)
		{
			var rallyPos = cmpRallyPoint.GetPositions()[0];
			if (rallyPos)
			{
				var commands = GetRallyPointCommands(cmpRallyPoint, spawnedEnts);
				for each(var com in commands)
				{
					ProcessCommand(cmpOwnership.GetOwner(), com);
				}
			}
		}

		Engine.PostMessage(this.entity, MT_TrainingFinished, {
			"entities": spawnedEnts,
			"owner": cmpOwnership.GetOwner(),
			"metadata": metadata,
		});
	}
	
	return spawnedEnts.length;
};

/*
 * Increments progress on the first batch in the production queue, and blocks the
 * queue if population limit is reached or some units failed to spawn.
 */
ProductionQueue.prototype.ProgressTimeout = function(data)
{
	// Allocate the 1000msecs to as many queue items as it takes
	// until we've used up all the time (so that we work accurately
	// with items that take fractions of a second)
	var time = g_ProgressInterval;
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	while (time > 0 && this.queue.length)
	{
		var item = this.queue[0];
		if (!item.productionStarted)
		{
			// If the item is a unit then do population checks
			if (item.unitTemplate)
			{
				// Batch's training hasn't started yet.
				// Try to reserve the necessary population slots
				if (item.unitTemplate && !cmpPlayer.TryReservePopulationSlots(item.population * item.count))
				{
					// No slots available - don't train this batch now
					// (we'll try again on the next timeout)

					// Set flag that training is blocked
					cmpPlayer.BlockTraining();
					break;
				}
				
				// Unset flag that training is blocked
				cmpPlayer.UnBlockTraining();
			}

			if (item.technologyTemplate)
			{
				// Mark the research as started.
				var cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
				cmpTechnologyManager.StartedResearch(item.technologyTemplate);
			}

			item.productionStarted = true;
		}

		// If we won't finish the batch now, just update its timer
		if (item.timeRemaining > time)
		{
			item.timeRemaining -= time;
			break;
		}

		if (item.unitTemplate)
		{
			var numSpawned = this.SpawnUnits(item.unitTemplate, item.count, item.metadata);
			if (numSpawned == item.count)
			{
				// All entities spawned, this batch finished
				cmpPlayer.UnReservePopulationSlots(item.population * numSpawned);
				time -= item.timeRemaining;
				this.queue.shift();
				// Unset flag that training is blocked
				cmpPlayer.UnBlockTraining();
				this.spawnNotified = false;
				Engine.PostMessage(this.entity, MT_ProductionQueueChanged, { });
			}
			else
			{
				if (numSpawned > 0)
				{
					// Only partially finished
					cmpPlayer.UnReservePopulationSlots(item.population * numSpawned);
					item.count -= numSpawned;
					Engine.PostMessage(this.entity, MT_ProductionQueueChanged, { });
				}

				// Some entities failed to spawn
				// Set flag that training is blocked
				cmpPlayer.BlockTraining();
				
				if (!this.spawnNotified)
				{
					var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
					var notification = {"player": cmpPlayer.GetPlayerID(), "message": "Can't find free space to spawn trained units" };
					var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
					cmpGUIInterface.PushNotification(notification);
					this.spawnNotified = true;
				}
				break;
			}
		}
		else if (item.technologyTemplate)
		{
			var cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
			cmpTechnologyManager.ResearchTechnology(item.technologyTemplate);
			
			time -= item.timeRemaining;
			
			this.queue.shift();
			Engine.PostMessage(this.entity, MT_ProductionQueueChanged, { });
		}
	}

	// If the queue's empty, delete the timer, else repeat it
	if (this.queue.length == 0)
	{
		this.timer = undefined;

		// Unset flag that training is blocked
		// (This might happen when the player unqueues all batches)
		cmpPlayer.UnBlockTraining();
	}
	else
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_ProductionQueue, "ProgressTimeout", g_ProgressInterval, data);
	}
}

Engine.RegisterComponentType(IID_ProductionQueue, "ProductionQueue", ProductionQueue);
