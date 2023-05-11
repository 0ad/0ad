function Trainer() {}

Trainer.prototype.Schema =
	"<a:help>Allows the entity to train new units.</a:help>" +
	"<a:example>" +
		"<BatchTimeModifier>0.7</BatchTimeModifier>" +
		"<Entities datatype='tokens'>" +
			"\n    units/{civ}/support_female_citizen\n    units/{native}/support_trader\n    units/athen/infantry_spearman_b\n  " +
		"</Entities>" +
	"</a:example>" +
	"<optional>" +
		"<element name='BatchTimeModifier' a:help='Modifier that influences the time benefit for batch training. Defaults to 1, which means no benefit.'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Entities' a:help='Space-separated list of entity template names that this entity can train. The special string \"{civ}\" will be automatically replaced by the civ code of the entity&apos;s owner, while the string \"{native}\" will be automatically replaced by the entity&apos;s civ code.'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>";

/**
 * This object represents a batch of entities being trained.
 * @param {string} templateName - The name of the template we ought to train.
 * @param {number} count - The size of the batch to train.
 * @param {number} trainer - The entity ID of our trainer.
 * @param {string} metadata - Optionally any metadata to attach to us.
 */
Trainer.prototype.Item = function(templateName, count, trainer, metadata)
{
	this.count = count;
	this.templateName = templateName;
	this.trainer = trainer;
	this.metadata = metadata;
};

/**
 * Prepare for the queue.
 * @param {Object} trainCostMultiplier - The multipliers to use when calculating costs.
 * @param {number} batchTimeMultiplier - The factor to use when training this batches.
 *
 * @return {boolean} - Whether the item was successfully initiated.
 */
Trainer.prototype.Item.prototype.Queue = function(trainCostMultiplier, batchTimeMultiplier)
{
	if (!Number.isInteger(this.count) || this.count <= 0)
	{
		error("Invalid batch count " + this.count + ".");
		return false;
	}
	const cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	const template = cmpTemplateManager.GetTemplate(this.templateName);
	if (!template)
		return false;

	const cmpPlayer = QueryOwnerInterface(this.trainer);
	if (!cmpPlayer)
		return false;
	this.player = cmpPlayer.GetPlayerID();

	this.resources = {};
	const totalResources = {};

	for (const res in template.Cost.Resources)
	{
		this.resources[res] = trainCostMultiplier[res] *
			ApplyValueModificationsToTemplate(
				"Cost/Resources/" + res,
				+template.Cost.Resources[res],
				this.player,
				template);

		totalResources[res] = Math.floor(this.count * this.resources[res]);
	}
	// TrySubtractResources should report error to player (they ran out of resources).
	if (!cmpPlayer.TrySubtractResources(totalResources))
		return false;

	this.population = ApplyValueModificationsToTemplate("Cost/Population", +template.Cost.Population, this.player, template);

	if (template.TrainingRestrictions)
	{
		const unitCategory = template.TrainingRestrictions.Category;
		const cmpPlayerEntityLimits = QueryPlayerIDInterface(this.player, IID_EntityLimits);
		if (cmpPlayerEntityLimits)
		{
			if (!cmpPlayerEntityLimits.AllowedToTrain(unitCategory, this.count, this.templateName, template.TrainingRestrictions.MatchLimit))
			// Already warned, return.
			{
				cmpPlayer.RefundResources(totalResources);
				return false;
			}
			// ToDo: Should warn here v and return?
			cmpPlayerEntityLimits.ChangeCount(unitCategory, this.count);
			if (template.TrainingRestrictions.MatchLimit)
				cmpPlayerEntityLimits.ChangeMatchCount(this.templateName, this.count);
		}
	}

	const buildTime = ApplyValueModificationsToTemplate("Cost/BuildTime", +template.Cost.BuildTime, this.player, template);

	const time = batchTimeMultiplier * trainCostMultiplier.time * buildTime * 1000;
	this.timeRemaining = time;
	this.timeTotal = time;

	const cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.CallEvent("OnTrainingQueued", {
		"playerid": this.player,
		"unitTemplate": this.templateName,
		"count": this.count,
		"metadata": this.metadata,
		"trainerEntity": this.trainer
	});

	return true;
};

/**
 * Destroy cached entities, refund resources and free (population) limits.
 */
Trainer.prototype.Item.prototype.Stop = function()
{
	// Destroy any cached entities (those which didn't spawn for some reason).
	if (this.entities?.length)
	{
		for (const ent of this.entities)
			Engine.DestroyEntity(ent);

		delete this.entities;
	}

	const cmpPlayer = QueryPlayerIDInterface(this.player);

	const cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	const template = cmpTemplateManager.GetTemplate(this.templateName);
	if (template.TrainingRestrictions)
	{
		const cmpPlayerEntityLimits = QueryPlayerIDInterface(this.player, IID_EntityLimits);
		if (cmpPlayerEntityLimits)
			cmpPlayerEntityLimits.ChangeCount(template.TrainingRestrictions.Category, -this.count);
		if (template.TrainingRestrictions.MatchLimit)
			cmpPlayerEntityLimits.ChangeMatchCount(this.templateName, -this.count);
	}

	if (cmpPlayer)
	{
		if (this.started)
			cmpPlayer.UnReservePopulationSlots(this.population * this.count);

		const totalCosts = {};
		for (const resource in this.resources)
			totalCosts[resource] = Math.floor(this.count * this.resources[resource]);

		cmpPlayer.RefundResources(totalCosts);
		cmpPlayer.UnBlockTraining();
	}

	delete this.resources;
};

/**
 * This starts the item, reserving population.
 * @return {boolean} - Whether the item was started successfully.
 */
Trainer.prototype.Item.prototype.Start = function()
{
	const cmpPlayer = QueryPlayerIDInterface(this.player);
	if (!cmpPlayer)
		return false;

	const template = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetTemplate(this.templateName);
	this.population = ApplyValueModificationsToTemplate(
		"Cost/Population",
		+template.Cost.Population,
		this.player,
		template);

	this.missingPopSpace = cmpPlayer.TryReservePopulationSlots(this.population * this.count);
	if (this.missingPopSpace)
	{
		cmpPlayer.BlockTraining();
		return false;
	}
	cmpPlayer.UnBlockTraining();

	Engine.PostMessage(this.trainer, MT_TrainingStarted, { "entity": this.trainer });

	this.started = true;
	return true;
};

Trainer.prototype.Item.prototype.Finish = function()
{
	this.Spawn();
	if (!this.count)
		this.finished = true;
};

/**
 * @return {boolean} -
 */
Trainer.prototype.Item.prototype.IsFinished = function()
{
	return !!this.finished;
};

/*
 * This function creates the entities and places them in world if possible
 * (some of these entities may be garrisoned directly if autogarrison, the others are spawned).
 */
Trainer.prototype.Item.prototype.Spawn = function()
{
	const createdEnts = [];
	const spawnedEnts = [];

	// We need entities to test spawning, but we don't want to waste resources,
	// so only create them once and use as needed.
	if (!this.entities)
	{
		this.entities = [];
		for (let i = 0; i < this.count; ++i)
			this.entities.push(Engine.AddEntity(this.templateName));
	}

	let autoGarrison;
	const cmpRallyPoint = Engine.QueryInterface(this.trainer, IID_RallyPoint);
	if (cmpRallyPoint)
	{
		const data = cmpRallyPoint.GetData()[0];
		if (data?.target && data.target == this.trainer && data.command == "garrison")
			autoGarrison = true;
	}

	const cmpFootprint = Engine.QueryInterface(this.trainer, IID_Footprint);
	const cmpPosition = Engine.QueryInterface(this.trainer, IID_Position);
	const positionTrainer = cmpPosition && cmpPosition.GetPosition();

	const cmpPlayerEntityLimits = QueryPlayerIDInterface(this.player, IID_EntityLimits);
	const cmpPlayerStatisticsTracker = QueryPlayerIDInterface(this.player, IID_StatisticsTracker);
	while (this.entities.length)
	{
		const ent = this.entities[0];
		const cmpNewOwnership = Engine.QueryInterface(ent, IID_Ownership);
		let garrisoned = false;

		if (autoGarrison)
		{
			const cmpGarrisonable = Engine.QueryInterface(ent, IID_Garrisonable);
			if (cmpGarrisonable)
			{
				// Temporary owner affectation needed for GarrisonHolder checks.
				cmpNewOwnership.SetOwnerQuiet(this.player);
				garrisoned = cmpGarrisonable.Garrison(this.trainer);
				cmpNewOwnership.SetOwnerQuiet(INVALID_PLAYER);
			}
		}

		if (!garrisoned)
		{
			const pos = cmpFootprint.PickSpawnPoint(ent);
			if (pos.y < 0)
				break;

			const cmpNewPosition = Engine.QueryInterface(ent, IID_Position);
			cmpNewPosition.JumpTo(pos.x, pos.z);

			if (positionTrainer)
				cmpNewPosition.SetYRotation(positionTrainer.horizAngleTo(pos));

			spawnedEnts.push(ent);
		}

		// Decrement entity count in the EntityLimits component
		// since it will be increased by EntityLimits.OnGlobalOwnershipChanged,
		// i.e. we replace a 'trained' entity by 'alive' one.
		// Must be done after spawn check so EntityLimits decrements only if unit spawns.
		if (cmpPlayerEntityLimits)
		{
			const cmpTrainingRestrictions = Engine.QueryInterface(ent, IID_TrainingRestrictions);
			if (cmpTrainingRestrictions)
				cmpPlayerEntityLimits.ChangeCount(cmpTrainingRestrictions.GetCategory(), -1);
		}
		cmpNewOwnership.SetOwner(this.player);

		if (cmpPlayerStatisticsTracker)
			cmpPlayerStatisticsTracker.IncreaseTrainedUnitsCounter(ent);

		this.count--;
		this.entities.shift();
		createdEnts.push(ent);
	}

	if (spawnedEnts.length && cmpRallyPoint)
		for (const com of GetRallyPointCommands(cmpRallyPoint, spawnedEnts))
			ProcessCommand(this.player, com);

	const cmpPlayer = QueryOwnerInterface(this.trainer);
	if (createdEnts.length)
	{
		if (this.population)
			cmpPlayer.UnReservePopulationSlots(this.population * createdEnts.length);
		// Play a sound, but only for the first in the batch (to avoid nasty phasing effects).
		PlaySound("trained", createdEnts[0]);
		Engine.PostMessage(this.trainer, MT_TrainingFinished, {
		    "entities": createdEnts,
		    "owner": this.player,
		    "metadata": this.metadata
		});
	}
	if (this.count)
	{
		cmpPlayer.BlockTraining();

		if (!this.spawnNotified)
		{
			Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).PushNotification({
			    "players": [cmpPlayer.GetPlayerID()],
			    "message": markForTranslation("Can't find free space to spawn trained units."),
			    "translateMessage": true
			});
			this.spawnNotified = true;
		}
	}
	else
	{
		cmpPlayer.UnBlockTraining();
		delete this.spawnNotified;
	}
};

/**
 * @param {number} allocatedTime - The time allocated to this item.
 * @return {number} - The time used for this item.
 */
Trainer.prototype.Item.prototype.Progress = function(allocatedTime)
{
	if (this.paused)
		this.Unpause();
	// We couldn't start this timeout, try again later.
	if (!this.started && !this.Start())
		return allocatedTime;

	if (this.timeRemaining > allocatedTime)
	{
		this.timeRemaining -= allocatedTime;
		return allocatedTime;
	}
	this.Finish();
	return this.timeRemaining;
};

Trainer.prototype.Item.prototype.Pause = function()
{
	if (this.started)
		this.paused = true;
	else if (this.missingPopSpace)
	{
		delete this.missingPopSpace;
		QueryOwnerInterface(this.trainer)?.UnBlockTraining();
	}
};

Trainer.prototype.Item.prototype.Unpause = function()
{
	delete this.paused;
};

/**
 * @return {Object} - Some basic information of this batch.
 */
Trainer.prototype.Item.prototype.GetBasicInfo = function()
{
	return {
		"unitTemplate": this.templateName,
		"count": this.count,
		"neededSlots": this.missingPopSpace,
		"progress": 1 - (this.timeRemaining / (this.timeTotal || 1)),
		"timeRemaining": this.timeRemaining,
		"paused": this.paused,
		"metadata": this.metadata
	};
};

Trainer.prototype.Item.prototype.SerializableAttributes = [
	"count",
	"entities",
	"metadata",
	"missingPopSpace",
	"paused",
	"player",
	"population",
	"trainer",
	"resources",
	"started",
	"templateName",
	"timeRemaining",
	"timeTotal"
];

Trainer.prototype.Item.prototype.Serialize = function(id)
{
	const result = {
		"id": id
	};
	for (const att of this.SerializableAttributes)
		if (this.hasOwnProperty(att))
			result[att] = this[att];
	return result;
};

Trainer.prototype.Item.prototype.Deserialize = function(data)
{
	for (const att of this.SerializableAttributes)
		if (att in data)
			this[att] = data[att];
};

Trainer.prototype.Init = function()
{
	this.nextID = 1;
	this.queue = new Map();
	this.trainCostMultiplier = {};
};

Trainer.prototype.SerializableAttributes = [
	"entitiesMap",
	"nextID",
	"trainCostMultiplier"
];

Trainer.prototype.Serialize = function()
{
	const queue = [];
	for (const [id, item] of this.queue)
		queue.push(item.Serialize(id));

	const result = {
		"queue": queue
	};
	for (const att of this.SerializableAttributes)
		if (this.hasOwnProperty(att))
			result[att] = this[att];

	return result;
};

Trainer.prototype.Deserialize = function(data)
{
	for (const att of this.SerializableAttributes)
		if (att in data)
			this[att] = data[att];

	this.queue = new Map();
	for (const item of data.queue)
	{
		const newItem = new this.Item();
		newItem.Deserialize(item);
		this.queue.set(item.id, newItem);
	}
};

/*
 * Returns list of entities that can be trained by this entity.
 */
Trainer.prototype.GetEntitiesList = function()
{
	return Array.from(this.entitiesMap.values());
};

/**
 * Calculate the new list of producible entities
 * and update any entities currently being produced.
 */
Trainer.prototype.CalculateEntitiesMap = function()
{
	// Don't reset the map, it's used below to update entities.
	if (!this.entitiesMap)
		this.entitiesMap = new Map();

	const string = this.template?.Entities?._string || "";
	// Tokens can be added -> process an empty list to get them.
	let addedTokens = ApplyValueModificationsToEntity("Trainer/Entities/_string", "", this.entity);
	if (!addedTokens && !string)
		return;

	addedTokens = addedTokens == "" ? [] : addedTokens.split(/\s+/);

	const cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	const cmpPlayer = QueryOwnerInterface(this.entity);

	const disabledEntities = cmpPlayer ? cmpPlayer.GetDisabledTemplates() : {};

	/**
	 * Process tokens:
	 * - process token modifiers (this is a bit tricky).
	 * - replace the "{civ}" and "{native}" codes with the owner's civ ID and entity's civ ID
	 * - remove disabled entities
	 * - upgrade templates where necessary
	 * This also updates currently queued production (it's more convenient to do it here).
	 */

	const removeAllQueuedTemplate = (token) => {
		const queue = clone(this.queue);
		const template = this.entitiesMap.get(token);
		for (const [id, item] of queue)
			if (item.templateName == template)
				this.StopBatch(id);
	};

	// ToDo: Notice this doesn't account for entity limits changing due to the template change.
	const updateAllQueuedTemplate = (token, updateTo) => {
		const template = this.entitiesMap.get(token);
		for (const [id, item] of this.queue)
			if (item.templateName === template)
				item.templateName = updateTo;
	};

	const toks = string.split(/\s+/);
	for (const tok of addedTokens)
		toks.push(tok);

	const nativeCiv = Engine.QueryInterface(this.entity, IID_Identity)?.GetCiv();
	const playerCiv = QueryOwnerInterface(this.entity, IID_Identity)?.GetCiv();

	const addedDict = addedTokens.reduce((out, token) => { out[token] = true; return out; }, {});
	this.entitiesMap = toks.reduce((entMap, token) => {
		const rawToken = token;
		if (!(token in addedDict))
		{
			// This is a bit wasteful but I can't think of a simpler/better way.
			// The list of token is unlikely to be a performance bottleneck anyways.
			token = ApplyValueModificationsToEntity("Trainer/Entities/_string", token, this.entity);
			token = token.split(/\s+/);
			if (token.every(tok => addedTokens.indexOf(tok) !== -1))
			{
				removeAllQueuedTemplate(rawToken);
				return entMap;
			}
			token = token[0];
		}
		// Replace the "{civ}" and "{native}" codes with the owner's civ ID and entity's civ ID.
		if (nativeCiv)
			token = token.replace(/\{native\}/g, nativeCiv);
		if (playerCiv)
			token = token.replace(/\{civ\}/g, playerCiv);

		// Filter out disabled and invalid entities.
		if (disabledEntities[token] || !cmpTemplateManager.TemplateExists(token))
		{
			removeAllQueuedTemplate(rawToken);
			return entMap;
		}

		token = GetUpgradedTemplate(cmpPlayer.GetPlayerID(), token);
		entMap.set(rawToken, token);
		updateAllQueuedTemplate(rawToken, token);
		return entMap;
	}, new Map());

	this.CalculateTrainCostMultiplier();
};

Trainer.prototype.CalculateTrainCostMultiplier = function()
{
	for (const res of Resources.GetCodes().concat(["time"]))
		this.trainCostMultiplier[res] = ApplyValueModificationsToEntity(
		    "Trainer/TrainCostMultiplier/" + res,
		    +(this.template?.TrainCostMultiplier?.[res] || 1),
		    this.entity);
};

/**
 * @return {Object} - The multipliers to change the costs of any training activity with.
 */
Trainer.prototype.TrainCostMultiplier = function()
{
	return this.trainCostMultiplier;
};

/*
 * Returns batch build time.
 */
Trainer.prototype.GetBatchTime = function(batchSize)
{
	// TODO: work out what equation we should use here.
	return Math.pow(batchSize, ApplyValueModificationsToEntity(
	    "Trainer/BatchTimeModifier",
	    +(this.template?.BatchTimeModifier || 1),
	    this.entity));
};

/**
 * @param {string} templateName - The template name to check.
 * @return {boolean} - Whether we can train this template.
 */
Trainer.prototype.CanTrain = function(templateName)
{
	return this.GetEntitiesList().includes(templateName);
};

/**
 * @param {string} templateName - The entity to queue.
 * @param {number} count - The batch size.
 * @param {string} metadata - Any metadata attached to the item.
 *
 * @return {number} - The ID of the item. -1 if the item could not be queued.
 */
Trainer.prototype.QueueBatch = function(templateName, count, metadata)
{
	const item = new this.Item(templateName, count, this.entity, metadata);
	if (!item.Queue(this.TrainCostMultiplier(), this.GetBatchTime(count)))
		return -1;

	const id = this.nextID++;
	this.queue.set(id, item);
	return id;
};

/**
 * @param {number} id - The ID of the batch being trained here we need to stop.
 */
Trainer.prototype.StopBatch = function(id)
{
	this.queue.get(id).Stop();
	this.queue.delete(id);
};

/**
 * @param {number} id - The ID of the training.
 */
Trainer.prototype.PauseBatch = function(id)
{
	this.queue.get(id).Pause();
};

/**
 * @param {number} id - The ID of the batch to check.
 * @return {boolean} - Whether we are currently training the batch.
 */
Trainer.prototype.HasBatch = function(id)
{
	return this.queue.has(id);
};

/**
 * @parameter {number} id - The id of the training.
 * @return {Object} - Some basic information about the training.
 */
Trainer.prototype.GetBatch = function(id)
{
	const item = this.queue.get(id);
	return item?.GetBasicInfo();
};

/**
 * @param {number} id - The ID of the item we spent time on.
 * @param {number} allocatedTime - The time we spent on the given item.
 * @return {number} - The time we've actually used.
 */
Trainer.prototype.Progress = function(id, allocatedTime)
{
	const item = this.queue.get(id);
	const usedTime = item.Progress(allocatedTime);
	if (item.IsFinished())
		this.queue.delete(id);
	return usedTime;
};

Trainer.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != INVALID_PLAYER)
		this.CalculateEntitiesMap();
};

Trainer.prototype.OnValueModification = function(msg)
{
	// If the promotion requirements of units is changed,
	// update the entities list so that automatically promoted units are shown
	// appropriately in the list.
	if (msg.component != "Promotion" && (msg.component != "Trainer" ||
	        !msg.valueNames.some(val => val.startsWith("Trainer/Entities/"))))
		return;

	if (msg.entities.indexOf(this.entity) === -1)
		return;

	// This also updates the queued production if necessary.
	this.CalculateEntitiesMap();

	// Inform the GUI that it'll need to recompute the selection panel.
	// TODO: it would be better to only send the message if something actually changing
	// for the current training queue.
	const cmpPlayer = QueryOwnerInterface(this.entity);
	if (cmpPlayer)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).SetSelectionDirty(cmpPlayer.GetPlayerID());
};

Trainer.prototype.OnDisabledTemplatesChanged = function(msg)
{
	this.CalculateEntitiesMap();
};

Engine.RegisterComponentType(IID_Trainer, "Trainer", Trainer);
