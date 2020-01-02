function ProductionQueue() {}

ProductionQueue.prototype.Schema =
	"<a:help>Allows the building to train new units and research technologies</a:help>" +
	"<a:example>" +
		"<BatchTimeModifier>0.7</BatchTimeModifier>" +
		"<Entities datatype='tokens'>" +
			"\n    units/{civ}_support_female_citizen\n    units/{native}_support_trader\n    units/athen_infantry_spearman_b\n  " +
		"</Entities>" +
	"</a:example>" +
	"<element name='BatchTimeModifier' a:help='Modifier that influences the time benefit for batch training'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Entities' a:help='Space-separated list of entity template names that this entity can train. The special string \"{civ}\" will be automatically replaced by the civ code of the entity&apos;s owner, while the string \"{native}\" will be automatically replaced by the entity&apos;s civ code.'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Technologies' a:help='Space-separated list of technology names that this building can research. When present, the special string \"{civ}\" will be automatically replaced either by the civ code of the building&apos;s owner if such a tech exists, or by \"generic\".'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<element name='TechCostMultiplier' a:help='Multiplier to modify ressources cost and research time of technologies searched in this building.'>" +
		Resources.BuildSchema("nonNegativeDecimal", ["time"]) +
	"</element>";

ProductionQueue.prototype.ProgressInterval = 1000;
ProductionQueue.prototype.MaxQueueSize = 16;

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
	//     "neededSlots": 3, // number of population slots missing for production to begin
	//     "resources": { "wood": 100, ... }, // resources per unit, multiply by count to get total
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
	//     "resources": { "wood": 100, ... }, // resources needed for research
	//     "productionStarted": false, // true iff production has started
	//     "timeTotal": 15000, // msecs
	//     "timeRemaining": 10000, // msecs
	//   }

	this.timer = undefined; // this.ProgressInterval msec timer, active while the queue is non-empty
	this.paused = false;

	this.entityCache = [];
	this.spawnNotified = false;
};

/*
 * Returns list of entities that can be trained by this building.
 */
ProductionQueue.prototype.GetEntitiesList = function()
{
	return this.entitiesList;
};

ProductionQueue.prototype.CalculateEntitiesList = function()
{
	this.entitiesList = [];
	if (!this.template.Entities)
		return;

	let string = this.template.Entities._string;
	if (!string)
		return;

	// Replace the "{civ}" and "{native}" codes with the owner's civ ID and entity's civ ID.
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return;

	let cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		string = string.replace(/\{native\}/g, cmpIdentity.GetCiv());

	let entitiesList = string.replace(/\{civ\}/g, cmpPlayer.GetCiv()).split(/\s+/);

	// Filter out disabled and invalid entities.
	let disabledEntities = cmpPlayer.GetDisabledTemplates();
	entitiesList = entitiesList.filter(ent => !disabledEntities[ent] && cmpTemplateManager.TemplateExists(ent));

	// Check if some templates need to show their advanced or elite version.
	let upgradeTemplate = function(templateName)
	{
		let template = cmpTemplateManager.GetTemplate(templateName);
		while (template && template.Promotion !== undefined)
		{
			let requiredXp = ApplyValueModificationsToTemplate(
			    "Promotion/RequiredXp",
			    +template.Promotion.RequiredXp,
			    cmpPlayer.GetPlayerID(),
			    template);
			if (requiredXp > 0)
				break;
			templateName = template.Promotion.Entity;
			template = cmpTemplateManager.GetTemplate(templateName);
		}
		return templateName;
	};

	for (let templateName of entitiesList)
		this.entitiesList.push(upgradeTemplate(templateName));

	for (let item of this.queue)
		if (item.unitTemplate)
			item.unitTemplate = upgradeTemplate(item.unitTemplate);
};

/*
 * Returns list of technologies that can be researched by this building.
 */
ProductionQueue.prototype.GetTechnologiesList = function()
{
	if (!this.template.Technologies)
		return [];

	let string = this.template.Technologies._string;
	if (!string)
		return [];

	let cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	if (!cmpTechnologyManager)
		return [];

	let cmpPlayer = QueryOwnerInterface(this.entity);
	let cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (!cmpPlayer || !cmpIdentity)
		return [];

	let techs = string.split(/\s+/);

	// Replace the civ specific technologies.
	for (let i = 0; i < techs.length; ++i)
	{
		let tech = techs[i];
		if (tech.indexOf("{civ}") == -1)
			continue;
		let civTech = tech.replace("{civ}", cmpPlayer.GetCiv());
		techs[i] = TechnologyTemplates.Has(civTech) ? civTech : tech.replace("{civ}", "generic");
	}

	// Remove any technologies that can't be researched by this civ.
	techs = techs.filter(tech =>
		cmpTechnologyManager.CheckTechnologyRequirements(
			DeriveTechnologyRequirements(TechnologyTemplates.Get(tech), cmpPlayer.GetCiv()),
			true));

	let techList = [];
	// Stores the tech which supersedes the key.
	let superseded = {};

	let disabledTechnologies = cmpPlayer.GetDisabledTechnologies();

	// Add any top level technologies to an array which corresponds to the displayed icons.
	// Also store what technology is superseded in the superseded object { "tech1":"techWhichSupercedesTech1", ... }.
	for (let tech of techs)
	{
		if (disabledTechnologies && disabledTechnologies[tech])
			continue;

		let template = TechnologyTemplates.Get(tech);
		if (!template.supersedes || techs.indexOf(template.supersedes) === -1)
			techList.push(tech);
		else
			superseded[template.supersedes] = tech;
	}

	// Now make researched/in progress techs invisible.
	for (let i in techList)
	{
		let tech = techList[i];
		while (this.IsTechnologyResearchedOrInProgress(tech))
			tech = superseded[tech];

		techList[i] = tech;
	}

	let ret = [];

	// This inserts the techs into the correct positions to line up the technology pairs.
	for (let i = 0; i < techList.length; ++i)
	{
		let tech = techList[i];
		if (!tech)
		{
			ret[i] = undefined;
			continue;
		}

		let template = TechnologyTemplates.Get(tech);
		if (template.top)
			ret[i] = { "pair": true, "top": template.top, "bottom": template.bottom };
		else
			ret[i] = tech;
	}

	return ret;
};

ProductionQueue.prototype.GetTechCostMultiplier = function()
{
	let techCostMultiplier = {};
	for (let res in this.template.TechCostMultiplier)
		techCostMultiplier[res] = ApplyValueModificationsToEntity(
		    "ProductionQueue/TechCostMultiplier/" + res,
		    +this.template.TechCostMultiplier[res],
		    this.entity);

	return techCostMultiplier;
};

ProductionQueue.prototype.IsTechnologyResearchedOrInProgress = function(tech)
{
	if (!tech)
		return false;

	let cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);

	let template = TechnologyTemplates.Get(tech);
	if (template.top)
		return cmpTechnologyManager.IsTechnologyResearched(template.top) ||
		    cmpTechnologyManager.IsInProgress(template.top) ||
		    cmpTechnologyManager.IsTechnologyResearched(template.bottom) ||
		    cmpTechnologyManager.IsInProgress(template.bottom);

	return cmpTechnologyManager.IsTechnologyResearched(tech) || cmpTechnologyManager.IsInProgress(tech);
};

/*
 * Adds a new batch of identical units to train or a technology to research to the production queue.
 */
ProductionQueue.prototype.AddBatch = function(templateName, type, count, metadata)
{
	// TODO: there should probably be a limit on the number of queued batches.
	// TODO: there should be a way for the GUI to determine whether it's going
	// to be possible to add a batch (based on resource costs and length limits).
	let cmpPlayer = QueryOwnerInterface(this.entity);

	if (this.queue.length < this.MaxQueueSize)
	{

		if (type == "unit")
		{
			if (!Number.isInteger(count) || count <= 0)
			{
				error("Invalid batch count " + count);
				return;
			}

			// Find the template data so we can determine the build costs.
			let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
			let template = cmpTemplateManager.GetTemplate(templateName);
			if (!template)
				return;
			if (template.Promotion &&
			  !ApplyValueModificationsToTemplate(
			    "Promotion/RequiredXp",
			    +template.Promotion.RequiredXp,
			    cmpPlayer.GetPlayerID(),
			    template))
			{
				this.AddBatch(template.Promotion.Entity, type, count, metadata);
				return;
			}

			// We need the costs after tech modifications.
			// Obviously we don't have the entities yet, so we must use template data.
			let costs = {};
			let totalCosts = {};

			for (let res in template.Cost.Resources)
			{
				costs[res] = ApplyValueModificationsToTemplate(
				    "Cost/Resources/" + res,
				    +template.Cost.Resources[res],
				    cmpPlayer.GetPlayerID(),
				    template);

				totalCosts[res] = Math.floor(count * costs[res]);
			}

			// TrySubtractResources should report error to player (they ran out of resources).
			if (!cmpPlayer.TrySubtractResources(totalCosts))
				return;

			// Update entity count in the EntityLimits component.
			if (template.TrainingRestrictions)
			{
				let unitCategory = template.TrainingRestrictions.Category;
				let cmpPlayerEntityLimits = QueryOwnerInterface(this.entity, IID_EntityLimits);
				cmpPlayerEntityLimits.ChangeCount(unitCategory, count);
			}

			let buildTime = ApplyValueModificationsToTemplate(
			    "Cost/BuildTime",
			    +template.Cost.BuildTime,
			    cmpPlayer.GetPlayerID(),
			    template);
			// Apply a time discount to larger batches.
			let time = this.GetBatchTime(count) * buildTime * 1000;
			this.queue.push({
				"id": this.nextID++,
				"player": cmpPlayer.GetPlayerID(),
				"unitTemplate": templateName,
				"count": count,
				"metadata": metadata,
				"resources": costs,
				"population": ApplyValueModificationsToTemplate(
				    "Cost/Population",
				    +template.Cost.Population,
				    cmpPlayer.GetPlayerID(),
				    template),
				"productionStarted": false,
				"timeTotal": time,
				"timeRemaining": time
			});

			// Call the related trigger event.
			let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
			cmpTrigger.CallEvent("TrainingQueued", {
			    "playerid": cmpPlayer.GetPlayerID(),
			    "unitTemplate": templateName,
			    "count": count,
			    "metadata": metadata,
			    "trainerEntity": this.entity
			});
		}
		else if (type == "technology")
		{
			if (!TechnologyTemplates.Has(templateName))
				return;

			if (!this.GetTechnologiesList().some(tech =>
				tech &&
					(tech == templateName ||
						tech.pair &&
						(tech.top == templateName || tech.bottom == templateName))))
			{
				error("This entity cannot research " + templateName);
				return;
			}

			let template = TechnologyTemplates.Get(templateName);
			let techCostMultiplier = this.GetTechCostMultiplier();

			let cost = {};
			for (let res in template.cost)
				cost[res] = Math.floor((techCostMultiplier[res] || 1) * template.cost[res]);

			// TrySubtractResources should report error to player (they ran out of resources).
			if (!cmpPlayer.TrySubtractResources(cost))
				return;

			// Tell the technology manager that we have started researching this so that people can't research the same
			// thing twice.
			let cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
			cmpTechnologyManager.QueuedResearch(templateName, this.entity);
			if (!this.queue.length)
			{
				cmpTechnologyManager.StartedResearch(templateName, false);
				this.SetAnimation("researching");
			}

			let time = techCostMultiplier.time * template.researchTime * 1000;
			this.queue.push({
			    "id": this.nextID++,
			    "player": cmpPlayer.GetPlayerID(),
			    "count": 1,
			    "technologyTemplate": templateName,
			    "resources": cost,
			    "productionStarted": false,
			    "timeTotal": time,
			    "timeRemaining": time
			});

			// Call the related trigger event.
			let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
			cmpTrigger.CallEvent("ResearchQueued", {
			    "playerid": cmpPlayer.GetPlayerID(),
			    "technologyTemplate": templateName,
			    "researcherEntity": this.entity
			});
		}
		else
		{
			warn("Tried to add invalid item of type \"" + type + "\" and template \"" + templateName + "\" to a production queue");
			return;
		}

		Engine.PostMessage(this.entity, MT_ProductionQueueChanged, {});

		// If this is the first item in the queue, start the timer.
		if (!this.timer)
		{
			let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			this.timer = cmpTimer.SetTimeout(this.entity, IID_ProductionQueue, "ProgressTimeout", this.ProgressInterval, {});
		}
	}
	else
	{
		let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification({
		    "players": [cmpPlayer.GetPlayerID()],
		    "message": markForTranslation("The production queue is full."),
		    "translateMessage": true,
		});
	}
};

/*
 * Removes an existing batch of units from the production queue.
 * Refunds resource costs and population reservations.
 */
ProductionQueue.prototype.RemoveBatch = function(id)
{
	// Destroy any cached entities (those which didn't spawn for some reason).
	for (let ent of this.entityCache)
		Engine.DestroyEntity(ent);

	this.entityCache = [];

	for (let i = 0; i < this.queue.length; ++i)
	{
		let item = this.queue[i];
		if (item.id != id)
			continue;

		// Now we've found the item to remove.
		let cmpPlayer = QueryPlayerIDInterface(item.player);

		// Update entity count in the EntityLimits component.
		if (item.unitTemplate)
		{
			let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
			let template = cmpTemplateManager.GetTemplate(item.unitTemplate);
			if (template.TrainingRestrictions)
			{
				let cmpPlayerEntityLimits = QueryPlayerIDInterface(item.player, IID_EntityLimits);
				cmpPlayerEntityLimits.ChangeCount(template.TrainingRestrictions.Category, -item.count);
			}
		}

		// Refund the resource cost for this batch.
		let totalCosts = {};
		let cmpStatisticsTracker = QueryPlayerIDInterface(item.player, IID_StatisticsTracker);
		for (let r in item.resources)
		{
			totalCosts[r] = Math.floor(item.count * item.resources[r]);
			if (cmpStatisticsTracker)
				cmpStatisticsTracker.IncreaseResourceUsedCounter(r, -totalCosts[r]);
		}

		cmpPlayer.AddResources(totalCosts);

		// Remove reserved population slots if necessary.
		if (item.productionStarted && item.unitTemplate)
			cmpPlayer.UnReservePopulationSlots(item.population * item.count);

		// Mark the research as stopped if we cancel it.
		if (item.technologyTemplate)
		{
			// item.player is used as this.entity's owner may be invalid (deletion, etc.)
			let cmpTechnologyManager = QueryPlayerIDInterface(item.player, IID_TechnologyManager);
			cmpTechnologyManager.StoppedResearch(item.technologyTemplate, true);
			this.SetAnimation("idle");
		}

		// Remove from the queue.
		// (We don't need to remove the timer - it'll expire if it discovers the queue is empty.)
		this.queue.splice(i, 1);
		Engine.PostMessage(this.entity, MT_ProductionQueueChanged, {});

		return;
	}
};

ProductionQueue.prototype.SetAnimation = function(name)
{
	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation(name, false, 1);
};

/*
 * Returns basic data from all batches in the production queue.
 */
ProductionQueue.prototype.GetQueue = function()
{
	return this.queue.map(item => ({
	    "id": item.id,
	    "unitTemplate": item.unitTemplate,
	    "technologyTemplate": item.technologyTemplate,
	    "count": item.count,
	    "neededSlots": item.neededSlots,
	    "progress": 1 - (item.timeRemaining / (item.timeTotal || 1)),
	    "timeRemaining": item.timeRemaining,
	    "metadata": item.metadata
	}));
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

/*
 * Returns batch build time.
 */
ProductionQueue.prototype.GetBatchTime = function(batchSize)
{
	// TODO: work out what equation we should use here.
	return Math.pow(batchSize, ApplyValueModificationsToEntity(
	    "ProductionQueue/BatchTimeModifier",
	    +this.template.BatchTimeModifier,
	    this.entity));
};

ProductionQueue.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.from != INVALID_PLAYER)
	{
		// Unset flag that previous owner's training may be blocked.
		let cmpPlayer = QueryPlayerIDInterface(msg.from);
		if (cmpPlayer && this.queue.length)
			cmpPlayer.UnBlockTraining();
	}
	if (msg.to != INVALID_PLAYER)
		this.CalculateEntitiesList();

	// Reset the production queue whenever the owner changes.
	// (This should prevent players getting surprised when they capture
	// an enemy building, and then loads of the enemy's civ's soldiers get
	// created from it. Also it means we don't have to worry about
	// updating the reserved pop slots.)
	this.ResetQueue();
};

ProductionQueue.prototype.OnCivChanged = function()
{
	this.CalculateEntitiesList();
};

ProductionQueue.prototype.OnDestroy = function()
{
	// Reset the queue to refund any resources.
	this.ResetQueue();

	if (this.timer)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
	}
};

/*
 * This function creates the entities and places them in world if possible
 * and returns the number of successfully created entities.
 * (some of these entities may be garrisoned directly if autogarrison, the others are spawned).
 */
ProductionQueue.prototype.SpawnUnits = function(templateName, count, metadata)
{
	let cmpFootprint = Engine.QueryInterface(this.entity, IID_Footprint);
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let cmpRallyPoint = Engine.QueryInterface(this.entity, IID_RallyPoint);
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	let cmpPlayerEntityLimits = QueryOwnerInterface(this.entity, IID_EntityLimits);
	let cmpPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);

	let createdEnts = [];
	let spawnedEnts = [];

	// We need entities to test spawning, but we don't want to waste resources,
	// so only create them once and use as needed.
	if (!this.entityCache.length)
		for (let i = 0; i < count; ++i)
			this.entityCache.push(Engine.AddEntity(templateName));

	let cmpAutoGarrison;
	if (cmpRallyPoint)
	{
		let data = cmpRallyPoint.GetData()[0];
		if (data && data.target && data.target == this.entity && data.command == "garrison")
			cmpAutoGarrison = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	}

	for (let i = 0; i < count; ++i)
	{
		let ent = this.entityCache[0];
		let cmpNewOwnership = Engine.QueryInterface(ent, IID_Ownership);
		let garrisoned = false;

		if (cmpAutoGarrison)
		{
			// Temporary owner affectation needed for GarrisonHolder checks.
			cmpNewOwnership.SetOwnerQuiet(cmpOwnership.GetOwner());
			garrisoned = cmpAutoGarrison.PerformGarrison(ent);
			cmpNewOwnership.SetOwnerQuiet(INVALID_PLAYER);
		}

		if (garrisoned)
		{
			let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			if (cmpUnitAI)
				cmpUnitAI.Autogarrison(this.entity);
		}
		else
		{
			let pos = cmpFootprint.PickSpawnPoint(ent);
			if (pos.y < 0)
				break;

			let cmpNewPosition = Engine.QueryInterface(ent, IID_Position);
			cmpNewPosition.JumpTo(pos.x, pos.z);

			if (cmpPosition)
				cmpNewPosition.SetYRotation(cmpPosition.GetPosition().horizAngleTo(pos));

			spawnedEnts.push(ent);
		}

		// Decrement entity count in the EntityLimits component
		// since it will be increased by EntityLimits.OnGlobalOwnershipChanged function,
		// i.e. we replace a 'trained' entity by 'alive' one.
		// Must be done after spawn check so EntityLimits decrements only if unit spawns.
		let cmpTrainingRestrictions = Engine.QueryInterface(ent, IID_TrainingRestrictions);
		if (cmpTrainingRestrictions)
		{
			let unitCategory = cmpTrainingRestrictions.GetCategory();
			cmpPlayerEntityLimits.ChangeCount(unitCategory, -1);
		}

		cmpNewOwnership.SetOwner(cmpOwnership.GetOwner());

		if (cmpPlayerStatisticsTracker)
			cmpPlayerStatisticsTracker.IncreaseTrainedUnitsCounter(ent);

		// Play a sound, but only for the first in the batch (to avoid nasty phasing effects).
		if (!createdEnts.length)
			PlaySound("trained", ent);

		this.entityCache.shift();
		createdEnts.push(ent);
	}

	if (spawnedEnts.length && !cmpAutoGarrison)
	{
		// If a rally point is set, walk towards it (in formation) using a suitable command based on where the
		// rally point is placed.
		if (cmpRallyPoint)
		{
			let rallyPos = cmpRallyPoint.GetPositions()[0];
			if (rallyPos)
			{
				let commands = GetRallyPointCommands(cmpRallyPoint, spawnedEnts);
				for (let com of commands)
					ProcessCommand(cmpOwnership.GetOwner(), com);
			}
		}
	}

	if (createdEnts.length)
		Engine.PostMessage(this.entity, MT_TrainingFinished, {
		    "entities": createdEnts,
		    "owner": cmpOwnership.GetOwner(),
		    "metadata": metadata
		});

	return createdEnts.length;
};

/*
 * Increments progress on the first batch in the production queue, and blocks the
 * queue if population limit is reached or some units failed to spawn.
 */
ProductionQueue.prototype.ProgressTimeout = function(data)
{
	// Check if the production is paused (eg the entity is garrisoned)
	if (this.paused)
		return;
	// Allocate available time to as many queue items as it takes
	// until we've used up all the time (so that we work accurately
	// with items that take fractions of a second).
	let time = this.ProgressInterval;
	let cmpPlayer = QueryOwnerInterface(this.entity);
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);

	while (time > 0 && this.queue.length)
	{
		let item = this.queue[0];
		if (!item.productionStarted)
		{
			// If the item is a unit then do population checks.
			if (item.unitTemplate)
			{
				// If something change population cost.
				let template = cmpTemplateManager.GetTemplate(item.unitTemplate);
				item.population = ApplyValueModificationsToTemplate(
				    "Cost/Population",
				    +template.Cost.Population,
				    item.player,
				    template);

				// Batch's training hasn't started yet.
				// Try to reserve the necessary population slots.
				item.neededSlots = cmpPlayer.TryReservePopulationSlots(item.population * item.count);
				if (item.neededSlots)
				{
					// Not enough slots available - don't train this batch now
					// (we'll try again on the next timeout).

					cmpPlayer.BlockTraining();
					break;
				}

				cmpPlayer.UnBlockTraining();
			}

			if (item.technologyTemplate)
			{
				// Mark the research as started.
				let cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
				cmpTechnologyManager.StartedResearch(item.technologyTemplate, true);
				this.SetAnimation("researching");
			}

			item.productionStarted = true;
			if (item.unitTemplate)
				Engine.PostMessage(this.entity, MT_TrainingStarted, { "entity": this.entity });
		}

		// If we won't finish the batch now, just update its timer.
		if (item.timeRemaining > time)
		{
			item.timeRemaining -= time;
			// send a message for the AIs.
			Engine.PostMessage(this.entity, MT_ProductionQueueChanged, {});
			break;
		}

		if (item.unitTemplate)
		{
			let numSpawned = this.SpawnUnits(item.unitTemplate, item.count, item.metadata);
			if (numSpawned == item.count)
			{
				// All entities spawned, this batch finished.
				cmpPlayer.UnReservePopulationSlots(item.population * numSpawned);
				time -= item.timeRemaining;
				this.queue.shift();
				// Unset flag that training is blocked.
				cmpPlayer.UnBlockTraining();
				this.spawnNotified = false;
				Engine.PostMessage(this.entity, MT_ProductionQueueChanged, {});
			}
			else
			{
				if (numSpawned > 0)
				{
					// Training is only partially finished.
					cmpPlayer.UnReservePopulationSlots(item.population * numSpawned);
					item.count -= numSpawned;
					Engine.PostMessage(this.entity, MT_ProductionQueueChanged, {});
				}

				// Some entities failed to spawn.
				// Set flag that training is blocked.
				cmpPlayer.BlockTraining();

				if (!this.spawnNotified)
				{
					let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
					cmpGUIInterface.PushNotification({
					    "players": [cmpPlayer.GetPlayerID()],
					    "message": markForTranslation("Can't find free space to spawn trained units"),
					    "translateMessage": true
					});
					this.spawnNotified = true;
				}
				break;
			}
		}
		else if (item.technologyTemplate)
		{
			let cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
			cmpTechnologyManager.ResearchTechnology(item.technologyTemplate);
			this.SetAnimation("idle");
			let template = TechnologyTemplates.Get(item.technologyTemplate);
			if (template && template.soundComplete)
			{
				let cmpSoundManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_SoundManager);

				if (cmpSoundManager)
					cmpSoundManager.PlaySoundGroup(template.soundComplete, this.entity);
			}

			time -= item.timeRemaining;

			this.queue.shift();
			Engine.PostMessage(this.entity, MT_ProductionQueueChanged, {});
		}
	}

	// If the queue's empty, delete the timer, else repeat it.
	if (!this.queue.length)
	{
		this.timer = undefined;

		// Unset flag that training is blocked.
		// (This might happen when the player unqueues all batches.)
		cmpPlayer.UnBlockTraining();
	}
	else
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_ProductionQueue, "ProgressTimeout", this.ProgressInterval, data);
	}
};

ProductionQueue.prototype.PauseProduction = function()
{
	this.timer = undefined;
	this.paused = true;
};

ProductionQueue.prototype.UnpauseProduction = function()
{
	this.paused = false;
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetTimeout(this.entity, IID_ProductionQueue, "ProgressTimeout", this.ProgressInterval, {});
};

ProductionQueue.prototype.OnValueModification = function(msg)
{
	// If the promotion requirements of units is changed,
	// update the entities list so that automatically promoted units are shown
	// appropriately in the list.
	if (msg.component == "Promotion")
		this.CalculateEntitiesList();
};

ProductionQueue.prototype.OnDisabledTemplatesChanged = function(msg)
{
	// If the disabled templates of the player is changed,
	// update the entities list so that this is reflected there.
	this.CalculateEntitiesList();
};

Engine.RegisterComponentType(IID_ProductionQueue, "ProductionQueue", ProductionQueue);
