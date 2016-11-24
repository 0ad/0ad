function RallyPoint() {}

RallyPoint.prototype.Schema =
	"<a:component/><empty/>";

RallyPoint.prototype.Init = function()
{
	this.pos = [];
	this.data = [];
};

RallyPoint.prototype.AddPosition = function(x, z)
{
	this.pos.push({
		"x": x,
		"z": z
	});
};

RallyPoint.prototype.GetPositions = function()
{
	// Update positions for moving target entities

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);

	// We must not affect the simulation state here (modifications of the
	// RallyPointRenderer are allowed though), so copy the state
	var ret = [];
	for (var i = 0; i < this.pos.length; i++)
	{
		ret.push(this.pos[i]);

		// Update the rallypoint coordinates if the target is alive
		if (!this.data[i] || !this.data[i].target || !this.TargetIsAlive(this.data[i].target))
			continue;

		// and visible
		if (cmpRangeManager && cmpOwnership &&
				cmpRangeManager.GetLosVisibility(this.data[i].target, cmpOwnership.GetOwner()) != "visible")
			continue;

		// Get the actual position of the target entity
		var cmpPosition = Engine.QueryInterface(this.data[i].target, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		var targetPosition = cmpPosition.GetPosition2D();
		if (!targetPosition)
			continue;

		if (this.pos[i].x == targetPosition.x && this.pos[i].z == targetPosition.y)
			continue;

		ret[i] = { "x": targetPosition.x, "z": targetPosition.y };
		var cmpRallyPointRenderer = Engine.QueryInterface(this.entity, IID_RallyPointRenderer);
		if (cmpRallyPointRenderer)
			cmpRallyPointRenderer.UpdatePosition(i, targetPosition);
	}

	return ret;
};

// Extra data for the rally point, should have a command property and then helpful data for that command
// See getActionInfo in gui/input.js
RallyPoint.prototype.AddData = function(data)
{
	this.data.push(data);
};

// Returns an array with the data associated with this rally point.  Each element has the structure:
// {"type": "walk/gather/garrison/...", "target": targetEntityId, "resourceType": "tree/fruit/ore/..."} where target
// and resourceType (specific resource type) are optional, also target may be an invalid entity, check for existence.
RallyPoint.prototype.GetData = function()
{
	return this.data;
};

RallyPoint.prototype.Unset = function()
{
	this.pos = [];
	this.data = [];
};

RallyPoint.prototype.Reset = function()
{
	this.Unset();
	var cmpRallyPointRenderer = Engine.QueryInterface(this.entity, IID_RallyPointRenderer);
	if (cmpRallyPointRenderer)
		cmpRallyPointRenderer.Reset();
};

RallyPoint.prototype.OnGlobalEntityRenamed = function(msg)
{
	for (var data of this.data)
	{
		if (!data)
			continue;
		if (data.target && data.target == msg.entity)
			data.target = msg.newentity;
		if (data.source && data.source == msg.entity)
			data.source = msg.newentity;
	}
};

RallyPoint.prototype.OnOwnershipChanged = function(msg)
{
	// No need to reset when constructing or destructing the entity
	// Don't reset upon defeat to improve performance
	if (msg.from == -1 || msg.to < 1)
		return;

	this.Reset();
};

/**
 * Returns true if the target exists and has non-zero hitpoints.
 */
RallyPoint.prototype.TargetIsAlive = function(ent)
{
	var cmpFormation = Engine.QueryInterface(ent, IID_Formation);
	if (cmpFormation)
		return true;

	var cmpHealth = QueryMiragedInterface(ent, IID_Health);
	return cmpHealth && cmpHealth.GetHitpoints() != 0;
};

Engine.RegisterComponentType(IID_RallyPoint, "RallyPoint", RallyPoint);
