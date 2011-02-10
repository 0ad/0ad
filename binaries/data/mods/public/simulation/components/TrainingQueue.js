var g_ProgressInterval = 1000;
const MAX_QUEUE_SIZE = 16;

function TrainingQueue() {}

TrainingQueue.prototype.Schema =
	"<a:help>Allows the building to train new units.</a:help>" +
	"<a:example>" +
		"<Entities datatype='tokens'>" +
			"\n    units/{civ}_support_female_citizen\n    units/{civ}_support_trader\n    units/celt_infantry_spearman_b\n  " +
		"</Entities>" +
	"</a:example>" +
	"<element name='Entities' a:help='Space-separated list of entity template names that this building can train. The special string \"{civ}\" will be automatically replaced by the building&apos;s four-character civ code'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>";

TrainingQueue.prototype.Init = function()
{
	this.nextID = 1;

	this.queue = [];
	// Queue items are:
	//   {
	//     "id": 1,
	//     "player": 1, // who paid for this batch; we need this to cope with refunds cleanly
	//     "template": "units/example",
	//     "count": 10,
	//     "resources": { "wood": 100, ... },
	//     "population": 10,
	//     "trainingStarted": false, // true iff we have reserved population
	//     "timeTotal": 15000, // msecs
	//     "timeRemaining": 10000, // msecs
	//   }
	
	this.timer = undefined; // g_ProgressInterval msec timer, active while the queue is non-empty
};

TrainingQueue.prototype.GetEntitiesList = function()
{
	var string = this.template.Entities._string;
	
	// Replace the "{civ}" codes with this entity's civ ID
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		string = string.replace(/\{civ\}/g, cmpIdentity.GetCiv());
	
	return string.split(/\s+/);
};

TrainingQueue.prototype.AddBatch = function(templateName, count, metadata)
{
	// TODO: there should probably be a limit on the number of queued batches
	// TODO: there should be a way for the GUI to determine whether it's going
	// to be possible to add a batch (based on resource costs and length limits)
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	if (this.queue.length < MAX_QUEUE_SIZE)
	{
		// Find the template data so we can determine the build costs
		var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var template = cmpTempMan.GetTemplate(templateName);
		if (!template)
			return;

		var costMult = count;

		// Apply a time discount to larger batches.
		// TODO: work out what equation we should use here.
		var timeMult = Math.pow(count, 0.7);

		var time = timeMult * template.Cost.BuildTime;

		var costs = {};
		for each (var r in ["food", "wood", "stone", "metal"])
			costs[r] = Math.floor(costMult * template.Cost.Resources[r]);

		var population = template.Cost.Population * count;
	
		// TrySubtractResources should report error to player (they ran out of resources)
		if (!cmpPlayer.TrySubtractResources(costs))
			return;

		this.queue.push({
			"id": this.nextID++,
			"player": cmpPlayer.GetPlayerID(),
			"template": templateName,
			"count": count,
			"metadata": metadata,
			"resources": costs,
			"population": population,
			"trainingStarted": false,
			"timeTotal": time*1000,
			"timeRemaining": time*1000,
		});
		Engine.PostMessage(this.entity, MT_TrainingQueueChanged, { });

		// If this is the first item in the queue, start the timer
		if (!this.timer)
		{
			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			this.timer = cmpTimer.SetTimeout(this.entity, IID_TrainingQueue, "ProgressTimeout", g_ProgressInterval, {});
		}
	}
	else
	{
		var notification = {"player": cmpPlayer.GetPlayerID(), "message": "The training queue is full."};
		var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification(notification);
	}
};

TrainingQueue.prototype.RemoveBatch = function(id)
{
	for (var i = 0; i < this.queue.length; ++i)
	{
		var item = this.queue[i];
		if (item.id != id)
			continue;

		// Now we've found the item to remove

		var cmpPlayer = QueryPlayerIDInterface(item.player, IID_Player);

		// Refund the resource cost for this batch
		cmpPlayer.AddResources(item.resources);

		// Remove reserved population slots if necessary
		if (item.trainingStarted)
			cmpPlayer.UnReservePopulationSlots(item.population);

		// Remove from the queue
		// (We don't need to remove the timer - it'll expire if it discovers the queue is empty)
		this.queue.splice(i, 1);
		Engine.PostMessage(this.entity, MT_TrainingQueueChanged, { });

		return;
	}
};

TrainingQueue.prototype.GetQueue = function()
{
	var out = [];
	for each (var item in this.queue)
	{
		out.push({
			"id": item.id,
			"template": item.template,
			"count": item.count,
			"progress": 1-(item.timeRemaining/item.timeTotal),
			"metadata": item.metadata,
		});
	}
	return out;
};

TrainingQueue.prototype.ResetQueue = function()
{
	// Empty the training queue and refund all the resource costs
	// to the player. (This is to avoid players having to micromanage their
	// buildings' queues when they're about to be destroyed or captured.)

	while (this.queue.length)
		this.RemoveBatch(this.queue[0].id);
};

TrainingQueue.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.from != -1)
	{
		// Unset flag that previous owner's training queue may be blocked
		var cmpPlayer = QueryPlayerIDInterface(msg.from, IID_Player);
		if (cmpPlayer && this.queue.length > 0)
			cmpPlayer.UnBlockTrainingQueue();
	}

	// Reset the training queue whenever the owner changes.
	// (This should prevent players getting surprised when they capture
	// an enemy building, and then loads of the enemy's civ's soldiers get
	// created from it. Also it means we don't have to worry about
	// updating the reserved pop slots.)
	this.ResetQueue();
};

TrainingQueue.prototype.OnDestroy = function()
{
	// Reset the queue to refund any resources
	this.ResetQueue();

	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
	}
};


TrainingQueue.prototype.SpawnUnits = function(templateName, count, metadata)
{
	var cmpFootprint = Engine.QueryInterface(this.entity, IID_Footprint);
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpRallyPoint = Engine.QueryInterface(this.entity, IID_RallyPoint);
	
	var ents = [];

	for (var i = 0; i < count; ++i)
	{
		var ent = Engine.AddEntity(templateName);

		var pos = cmpFootprint.PickSpawnPoint(ent);
		if (pos.y < 0)
		{
			// Whoops, something went wrong (maybe there wasn't any space to spawn the unit).
			// What should we do here?
			// For now, just move the unit into the middle of the building where it'll probably get stuck
			pos = cmpPosition.GetPosition();
			warn("Can't find free space to spawn trained unit");
		}

		var cmpNewPosition = Engine.QueryInterface(ent, IID_Position);
		cmpNewPosition.JumpTo(pos.x, pos.z);
		// TODO: what direction should they face in?

		var cmpNewOwnership = Engine.QueryInterface(ent, IID_Ownership);
		cmpNewOwnership.SetOwner(cmpOwnership.GetOwner());
		
		var cmpPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
		cmpPlayerStatisticsTracker.IncreaseTrainedUnitsCounter();
		
		ents.push(ent);

		// Play a sound, but only for the first in the batch (to avoid nasty phasing effects)
		if (i == 0)
			PlaySound("trained", ent);
	}

	// If a rally point is set, walk towards it (in formation)
	if (cmpRallyPoint)
	{
		var rallyPos = cmpRallyPoint.GetPosition();
		if (rallyPos)
		{
			ProcessCommand(cmpOwnership.GetOwner(), {
				"type": "walk",
				"entities": ents,
				"x": rallyPos.x,
				"z": rallyPos.z,
				"queued": false
			});
		}
	}

	Engine.PostMessage(this.entity, MT_TrainingFinished, {
		"entities": ents,
		"owner": cmpOwnership.GetOwner(),
		"metadata": metadata,
	});
};

TrainingQueue.prototype.ProgressTimeout = function(data)
{
	// Allocate the 1000msecs to as many queue items as it takes
	// until we've used up all the time (so that we work accurately
	// with items that take fractions of a second)
	var time = g_ProgressInterval;
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	while (time > 0 && this.queue.length)
	{
		var item = this.queue[0];
		if (!item.trainingStarted)
		{
			// Batch's training hasn't started yet.
			// Try to reserve the necessary population slots
			if (!cmpPlayer.TryReservePopulationSlots(item.population))
			{
				// No slots available - don't train this batch now
				// (we'll try again on the next timeout)

				// Set flag that training queue is blocked
				cmpPlayer.BlockTrainingQueue();

				break;
			}

			// Unset flag that training queue is blocked
			cmpPlayer.UnBlockTrainingQueue();

			item.trainingStarted = true;
		}

		// If we won't finish the batch now, just update its timer
		if (item.timeRemaining > time)
		{
			item.timeRemaining -= time;
			break;
		}

		// This item is finished now
		time -= item.timeRemaining;
		cmpPlayer.UnReservePopulationSlots(item.population);
		this.SpawnUnits(item.template, item.count, item.metadata);
		this.queue.shift();
		Engine.PostMessage(this.entity, MT_TrainingQueueChanged, { });
	}

	// If the queue's empty, delete the timer, else repeat it
	if (this.queue.length == 0)
	{
		this.timer = undefined;

		// Unset flag that training queue is blocked
		// (This might happen when the player unqueues all batches)
		cmpPlayer.UnBlockTrainingQueue();
	}
	else
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_TrainingQueue, "ProgressTimeout", g_ProgressInterval, data);
	}
}

Engine.RegisterComponentType(IID_TrainingQueue, "TrainingQueue", TrainingQueue);
