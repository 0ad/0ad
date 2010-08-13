var g_ProgressInterval = 1000;

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
	//     "template": "units/example",
	//     "count": 10,
	//     "resources": { "wood": 100, ... },
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

TrainingQueue.prototype.AddBatch = function(player, templateName, count)
{
	// TODO: there should probably be a limit on the number of queued batches
	// TODO: there should be a way for the GUI to determine whether it's going
	// to be possible to add a batch (based on resource costs and length limits)

	// Find the template data so we can determine the build costs
	var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTempMan.GetTemplate(templateName);
	if (!template)
		return;

	var costMult = count;

	// Apply a time discount to larger batches.
	// TODO: work out what equation we should use here.
	var timeMult = Math.pow(count, 0.7);

	var time = timeMult * (template.Cost.BuildTime || 1);
	var costs = {};
	for each (var r in ["food", "wood", "stone", "metal"])
	{
		if (template.Cost.Resources[r])
			costs[r] = Math.floor(costMult * template.Cost.Resources[r]);
		else
			costs[r] = 0;
	}

	// Find the player
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var playerEnt = cmpPlayerMan.GetPlayerByID(player);
	var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);

	if (!cmpPlayer.TrySubtractResources(costs))
	{
		// TODO: report error to player (they ran out of resources)
		return;
	}

	this.queue.push({
		"id": this.nextID++,
		"template": templateName,
		"count": count,
		"resources": costs,
		"timeTotal": time*1000,
		"timeRemaining": time*1000,
	});

	// If this is the first item in the queue, start the timer
	if (!this.timer)
	{
 		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_TrainingQueue, "ProgressTimeout", g_ProgressInterval, {});
	}
};

TrainingQueue.prototype.RemoveBatch = function(player, id)
{
	for (var i = 0; i < this.queue.length; ++i)
	{
		var item = this.queue[i];
		if (item.id != id)
			continue;

		// Now we've found the item to remove

		// Find the player
		var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
		var playerEnt = cmpPlayerMan.GetPlayerByID(player);
		var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);

		// Refund the resource cost for this batch
		cmpPlayer.AddResources(item.resources);

		// Remove from the queue
		// (We don't need to remove the timer - it'll expire if it discovers the queue is empty)
		this.queue.splice(i, 1);
		return;
	}
}

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
		});
	}
	return out;
};

TrainingQueue.prototype.OnDestroy = function()
{
	// If the building is destroyed while it's got a large training queue,
	// you lose all the resources invested in that queue. That'll teach you
	// to be so reckless with your buildings.

	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
	}
};


TrainingQueue.prototype.SpawnUnits = function(templateName, count)
{
	var cmpFootprint = Engine.QueryInterface(this.entity, IID_Footprint);
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpRallyPoint = Engine.QueryInterface(this.entity, IID_RallyPoint);
	
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

		// If a rally point is set, walk towards it
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (cmpUnitAI && cmpRallyPoint)
		{
			var rallyPos = cmpRallyPoint.GetPosition();
			if (rallyPos)
				cmpUnitAI.Walk(rallyPos.x, rallyPos.z, false);
		}

		// Play a sound, but only for the first in the batch (to avoid nasty phasing effects)
		if (i == 0)
			PlaySound("trained", ent);
	}
};

TrainingQueue.prototype.ProgressTimeout = function(data)
{
	// Allocate the 1000msecs to as many queue items as it takes
	// until we've used up all the time (so that we work accurately
	// with items that take fractions of a second)
	var time = g_ProgressInterval;

	while (time > 0 && this.queue.length)
	{
		var item = this.queue[0];
		if (item.timeRemaining > time)
		{
			item.timeRemaining -= time;
			break;
		}

		// This item is finished now
		time -= item.timeRemaining;
		this.SpawnUnits(item.template, item.count);
		this.queue.shift();
	}

	// If the queue's empty, delete the timer, else repeat it
	if (this.queue.length == 0)
	{
		this.timer = undefined;
	}
	else
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_TrainingQueue, "ProgressTimeout", g_ProgressInterval, data);
	}
}

Engine.RegisterComponentType(IID_TrainingQueue, "TrainingQueue", TrainingQueue);
