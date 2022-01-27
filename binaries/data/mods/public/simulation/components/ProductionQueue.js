function ProductionQueue() {}

ProductionQueue.prototype.Schema =
	"<a:help>Helps the building to train new units and research technologies.</a:help>" +
	"<empty/>";

ProductionQueue.prototype.ProgressInterval = 1000;
ProductionQueue.prototype.MaxQueueSize = 16;

/**
 * This object represents an item in the queue.
 *
 * @param {number} producer - The entity ID of our producer.
 * @param {string} metadata - Optionally any metadata attached to us.
 */
ProductionQueue.prototype.Item = function(producer, metadata)
{
	this.producer = producer;
	this.metadata = metadata;
};

/**
 * @param {string} type - The type of queue to use.
 * @param {string} templateName - The template to queue.
 * @param {number} count - The amount of template to queue. Only applicable for type == "unit".
 *
 * @return {boolean} - Whether the item could be queued.
 */
ProductionQueue.prototype.Item.prototype.Queue = function(type, templateName, count)
{
	if (type == "unit")
		return this.QueueEntity(templateName, count);

	if (type == "technology")
		return this.QueueTechnology(templateName);

	warn("Tried to add invalid item of type \"" + type + "\" and template \"" + templateName + "\" to a production queue (entity: " + this.producer + ").");
	return false;
};

/**
 * @param {string} templateName - The name of the entity to queue.
 * @param {number} count - The number of entities that should be produced.
 * @return {boolean} - Whether the batch was successfully created.
 */
ProductionQueue.prototype.Item.prototype.QueueEntity = function(templateName, count)
{
	const cmpTrainer = Engine.QueryInterface(this.producer, IID_Trainer);
	if (!cmpTrainer)
		return false;
	this.entity = cmpTrainer.QueueBatch(templateName, count, this.metadata);
	if (this.entity == -1)
		return false;
	this.originalItem = {
		"templateName": templateName,
		"count": count,
		"metadata": this.metadata
	};

	return true;
};

/**
 * @param {string} templateName - The name of the technology to queue.
 * @return {boolean} - Whether the technology was successfully queued.
 */
ProductionQueue.prototype.Item.prototype.QueueTechnology = function(templateName)
{
	const cmpResearcher = Engine.QueryInterface(this.producer, IID_Researcher);
	if (!cmpResearcher)
		return false;
	this.technology = cmpResearcher.QueueTechnology(templateName, this.metadata);
	return this.technology != -1;
};

/**
 * @param {number} id - The id this item needs to get.
 */
ProductionQueue.prototype.Item.prototype.SetID = function(id)
{
	this.id = id;
};

ProductionQueue.prototype.Item.prototype.Stop = function()
{
	if (this.entity > 0)
		Engine.QueryInterface(this.producer, IID_Trainer)?.StopBatch(this.entity);

	if (this.technology > 0)
		Engine.QueryInterface(this.producer, IID_Researcher)?.StopResearching(this.technology);
};

/**
 * Called when the first work is performed.
 */
ProductionQueue.prototype.Item.prototype.Start = function()
{
	this.started = true;
};

/**
 * @return {boolean} - Whether there is work done on the item.
 */
ProductionQueue.prototype.Item.prototype.IsStarted = function()
{
	return !!this.started;
};

/**
 * @return {boolean} - Whether this item is finished.
 */
ProductionQueue.prototype.Item.prototype.IsFinished = function()
{
	return !!this.finished;
};

/**
 * @param {number} allocatedTime - The time allocated to this item.
 * @return {number} - The time used for this item.
 */
ProductionQueue.prototype.Item.prototype.Progress = function(allocatedTime)
{
	if (this.paused)
		this.Unpause();
	if (this.entity)
	{
		const cmpTrainer = Engine.QueryInterface(this.producer, IID_Trainer);
		allocatedTime -= cmpTrainer.Progress(this.entity, allocatedTime);
		if (!cmpTrainer.HasBatch(this.entity))
			delete this.entity;
	}
	if (this.technology)
	{
		const cmpResearcher = Engine.QueryInterface(this.producer, IID_Researcher);
		allocatedTime -= cmpResearcher.Progress(this.technology, allocatedTime);
		if (!cmpResearcher.HasItem(this.technology))
			delete this.technology;
	}
	if (!this.entity && !this.technology)
		this.finished = true;

	return allocatedTime;
};

ProductionQueue.prototype.Item.prototype.Pause = function()
{
	this.paused = true;
	if (this.entity)
		Engine.QueryInterface(this.producer, IID_Trainer).PauseBatch(this.entity);
	if (this.technology)
		Engine.QueryInterface(this.producer, IID_Researcher).PauseTechnology(this.technology);
};

ProductionQueue.prototype.Item.prototype.Unpause = function()
{
	delete this.paused;
};

/**
 * @return {boolean} - Whether the item is currently paused.
 */
ProductionQueue.prototype.Item.prototype.IsPaused = function()
{
	return !!this.paused;
};

/**
 * @return {Object} - Some basic information of this item.
 */
ProductionQueue.prototype.Item.prototype.GetBasicInfo = function()
{
	let result;
	if (this.technology)
		result = Engine.QueryInterface(this.producer, IID_Researcher).GetResearchingTechnology(this.technology);
	else if (this.entity)
		result = Engine.QueryInterface(this.producer, IID_Trainer).GetBatch(this.entity);
	result.id = this.id;
	result.paused = this.paused;
	return result;
};

/**
 * @return {Object} - The originally queued item.
 */
ProductionQueue.prototype.Item.prototype.OriginalItem = function()
{
	return this.originalItem;
};

ProductionQueue.prototype.Item.prototype.SerializableAttributes = [
	"entity",
	"id",
	"metadata",
	"originalItem",
	"paused",
	"producer",
	"started",
	"technology"
];

ProductionQueue.prototype.Item.prototype.Serialize = function()
{
	const result = {};
	for (const att of this.SerializableAttributes)
		if (this.hasOwnProperty(att))
			result[att] = this[att];
	return result;
};

ProductionQueue.prototype.Item.prototype.Deserialize = function(data)
{
	for (const att of this.SerializableAttributes)
		if (att in data)
			this[att] = data[att];
};

ProductionQueue.prototype.Init = function()
{
	this.nextID = 1;
	this.queue = [];
};

ProductionQueue.prototype.SerializableAttributes = [
	"autoqueuing",
	"nextID",
	"paused",
	"timer"
];

ProductionQueue.prototype.Serialize = function()
{
	const result = {
		"queue": []
	};
	for (const item of this.queue)
		result.queue.push(item.Serialize());

	for (const att of this.SerializableAttributes)
		if (this.hasOwnProperty(att))
			result[att] = this[att];

	return result;
};

ProductionQueue.prototype.Deserialize = function(data)
{
	for (const att of this.SerializableAttributes)
		if (att in data)
			this[att] = data[att];

	this.queue = [];

	for (const item of data.queue)
	{
		const newItem = new this.Item();
		newItem.Deserialize(item);
		this.queue.push(newItem);
	}
};

/**
 * @return {boolean} - Whether we are automatically queuing items.
 */
ProductionQueue.prototype.IsAutoQueueing = function()
{
	return !!this.autoqueuing;
};

/**
 * Turn on Auto-Queue.
 */
ProductionQueue.prototype.EnableAutoQueue = function()
{
	this.autoqueuing = true;
};

/**
 * Turn off Auto-Queue.
 */
ProductionQueue.prototype.DisableAutoQueue = function()
{
	delete this.autoqueuing;
};

/*
 * Adds a new batch of identical units to train or a technology to research to the production queue.
 * @param {string} templateName - The template to start production on.
 * @param {string} type - The type of production (i.e. "unit" or "technology").
 * @param {number} count - The amount of units to be produced. Ignored for a tech.
 * @param {any} metadata - Optionally any metadata to be attached to the item.
 * @param {boolean} pushFront - Whether to push the item to the front of the queue and pause any item(s) currently in progress.
 *
 * @return {boolean} - Whether the addition of the item has succeeded.
 */
ProductionQueue.prototype.AddItem = function(templateName, type, count, metadata, pushFront = false)
{
	// TODO: there should be a way for the GUI to determine whether it's going
	// to be possible to add a batch (based on resource costs and length limits).

	if (!this.queue.length)
	{
		const cmpPlayer = QueryOwnerInterface(this.entity);
		if (!cmpPlayer)
			return false;
		const player = cmpPlayer.GetPlayerID();
		const cmpUpgrade = Engine.QueryInterface(this.entity, IID_Upgrade);
		if (cmpUpgrade && cmpUpgrade.IsUpgrading())
		{
			let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
			cmpGUIInterface.PushNotification({
				"players": [player],
				"message": markForTranslation("Entity is being upgraded. Cannot start production."),
				"translateMessage": true
			});
			return false;
		}
	}
	else if (this.queue.length >= this.MaxQueueSize)
	{
		const cmpPlayer = QueryOwnerInterface(this.entity);
		if (!cmpPlayer)
			return false;
		const player = cmpPlayer.GetPlayerID();
		const cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification({
		    "players": [player],
		    "message": markForTranslation("The production queue is full."),
		    "translateMessage": true,
		});
		return false;
	}

	const item = new this.Item(this.entity, metadata);
	if (!item.Queue(type, templateName, count))
		return false;

	item.SetID(this.nextID++);
	if (pushFront)
	{
		this.queue[0]?.Pause();
		this.queue.unshift(item);
	}
	else
		this.queue.push(item);

	Engine.PostMessage(this.entity, MT_ProductionQueueChanged, null);

	if (!this.timer)
		this.StartTimer();
	return true;
};

/*
 * @param {number} - The ID of the item to remove from the queue.
 */
ProductionQueue.prototype.RemoveItem = function(id)
{
	let itemIndex = this.queue.findIndex(item => item.id == id);
	if (itemIndex == -1)
		return;

	this.queue.splice(itemIndex, 1)[0].Stop();

	Engine.PostMessage(this.entity, MT_ProductionQueueChanged, null);

	if (!this.queue.length)
		this.StopTimer();
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
	return this.queue.map(item => item.GetBasicInfo());
};

/*
 * Removes all existing batches from the queue.
 */
ProductionQueue.prototype.ResetQueue = function()
{
	while (this.queue.length)
		this.RemoveItem(this.queue[0].id);

	this.DisableAutoQueue();
};

/*
 * Increments progress on the first item in the production queue.
 * @param {Object} data - Unused in this case.
 * @param {number} lateness - The time passed since the expected time to fire the function.
 */
ProductionQueue.prototype.ProgressTimeout = function(data, lateness)
{
	if (this.paused)
		return;

	// Allocate available time to as many queue items as it takes
	// until we've used up all the time (so that we work accurately
	// with items that take fractions of a second).
	let time = this.ProgressInterval + lateness;

	while (this.queue.length)
	{
		let item = this.queue[0];
		if (!item.IsStarted())
		{
			if (item.entity)
				this.SetAnimation("training");
			if (item.technology)
				this.SetAnimation("researching");

			item.Start();
		}
		time -= item.Progress(time);
		if (!item.IsFinished())
		{
			Engine.PostMessage(this.entity, MT_ProductionQueueChanged, null);
			return;
		}

		this.queue.shift();
		Engine.PostMessage(this.entity, MT_ProductionQueueChanged, null);

		// If autoqueuing, push a new unit on the queue immediately,
		// but don't start right away. This 'wastes' some time, making
		// autoqueue slightly worse than regular queuing, and also ensures
		// that autoqueue doesn't train more than one item per turn,
		// if the units would take fewer than ProgressInterval ms to train.
		if (this.autoqueuing)
		{
			const autoqueueData = item.OriginalItem();
			if (!autoqueueData)
				continue;

			if (!this.AddItem(autoqueueData.templateName, "unit", autoqueueData.count, autoqueueData.metadata))
			{
				this.DisableAutoQueue();
				const cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
				cmpGUIInterface.PushNotification({
					"players": [QueryOwnerInterface(this.entity).GetPlayerID()],
					"message": markForTranslation("Could not auto-queue unit, de-activating."),
					"translateMessage": true
				});
			}
			break;
		}
	}

	if (!this.queue.length)
		this.StopTimer();
};

ProductionQueue.prototype.PauseProduction = function()
{
	this.StopTimer();
	this.paused = true;
	this.queue[0]?.Pause();
};

ProductionQueue.prototype.UnpauseProduction = function()
{
	delete this.paused;
	this.StartTimer();
};

ProductionQueue.prototype.StartTimer = function()
{
	if (this.timer)
		return;

	this.timer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).SetInterval(
		this.entity,
		IID_ProductionQueue,
		"ProgressTimeout",
		this.ProgressInterval,
		this.ProgressInterval,
		null
	);
};

ProductionQueue.prototype.StopTimer = function()
{
	if (!this.timer)
		return;

	this.SetAnimation("idle");
	Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).CancelTimer(this.timer);
	delete this.timer;
};

/**
 * @return {boolean} - Whether this entity is currently producing.
 */
ProductionQueue.prototype.HasQueuedProduction = function()
{
	return this.queue.length > 0;
};

ProductionQueue.prototype.OnOwnershipChanged = function(msg)
{
	// Reset the production queue whenever the owner changes.
	// (This should prevent players getting surprised when they capture
	// an enemy building, and then loads of the enemy's civ's soldiers get
	// created from it. Also it means we don't have to worry about
	// updating the reserved pop slots.)
	this.ResetQueue();
};

ProductionQueue.prototype.OnGarrisonedStateChanged = function(msg)
{
	if (msg.holderID != INVALID_ENTITY)
		this.PauseProduction();
	else
		this.UnpauseProduction();
};

Engine.RegisterComponentType(IID_ProductionQueue, "ProductionQueue", ProductionQueue);
