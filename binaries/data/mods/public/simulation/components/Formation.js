function Formation() {}

Formation.prototype.Schema =
	"<a:component type='system'/><empty/>";

Formation.prototype.Init = function()
{
	this.members = [];
};

Formation.prototype.GetMemberCount = function()
{
	return this.members.length;
};

/**
 * Initialise the members of this formation.
 * Must only be called once.
 * All members must implement UnitAI.
 */
Formation.prototype.SetMembers = function(ents)
{
	this.members = ents;

	for each (var ent in this.members)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI.SetFormationController(this.entity);
	}

	// Locate this formation controller in the middle of its members
	this.MoveToMembersCenter();

	this.ComputeMotionParameters();
};

/**
 * Remove the given list of entities.
 * The entities must already be members of this formation.
 */
Formation.prototype.RemoveMembers = function(ents)
{
	this.members = this.members.filter(function(e) { return ents.indexOf(e) == -1; });

	for each (var ent in ents)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI.SetFormationController(INVALID_ENTITY);
	}

	// If there's nobody left, destroy the formation
	if (this.members.length == 0)
	{
		Engine.DestroyEntity(this.entity);
		return;
	}

	this.ComputeMotionParameters();

	// Rearrange the remaining members
	this.MoveMembersIntoFormation();
};

/**
 * Remove all members and destroy the formation.
 */
Formation.prototype.Disband = function()
{
	for each (var ent in this.members)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI.SetFormationController(INVALID_ENTITY);
	}

	this.members = [];

	Engine.DestroyEntity(this.entity);
};

/**
 * Call obj.funcname(args) on UnitAI components of all members.
 */
Formation.prototype.CallMemberFunction = function(funcname, args)
{
	for each (var ent in this.members)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI[funcname].apply(cmpUnitAI, args);
	}
};

/**
 * Set all members to form up into the formation shape.
 */
Formation.prototype.MoveMembersIntoFormation = function()
{
	var active = [];
	var positions = [];

	var types = { "Unknown": 0 }; // TODO: make this work so we put ranged behind infantry etc

	for each (var ent in this.members)
	{
		var cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		active.push(ent);
		positions.push(cmpPosition.GetPosition());

		types["Unknown"] += 1; // TODO
	}

	var offsets = this.ComputeFormationOffsets(types);

	var avgpos = this.ComputeAveragePosition(positions);
	var avgoffset = this.ComputeAveragePosition(offsets);

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	cmpPosition.JumpTo(avgpos.x, avgpos.z);

	// TODO: assign to minimise worst-case distances or whatever

	for (var i = 0; i < active.length; ++i)
	{
		var offset = offsets[i];

		var cmpUnitAI = Engine.QueryInterface(active[i], IID_UnitAI);
		cmpUnitAI.ReplaceOrder("FormationWalk", {
			"target": this.entity,
			"x": offset.x - avgoffset.x,
			"z": offset.z - avgoffset.z
		});
	}
};

Formation.prototype.MoveToMembersCenter = function()
{
	var positions = [];

	for each (var ent in this.members)
	{
		var cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		positions.push(cmpPosition.GetPosition());
	}

	var avgpos = this.ComputeAveragePosition(positions);

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	cmpPosition.JumpTo(avgpos.x, avgpos.z);
};

Formation.prototype.ComputeFormationOffsets = function(types)
{
	var separation = 4; // TODO: don't hardcode this

	var count = types["Unknown"];

	// Choose a sensible width for the basic box formation
	var cols;
	if (count <= 4)
		cols = count;
	if (count <= 8)
		cols = 4;
	else if (count <= 16)
		cols = Math.ceil(count / 2);
	else
		cols = 8;

	var ranks = Math.ceil(count / cols);

	var offsets = [];

	var left = count;
	for (var r = 0; r < ranks; ++r)
	{
		var n = Math.min(left, cols);
		for (var c = 0; c < n; ++c)
		{
			var x = ((n-1)/2 - c) * separation;
			var z = -r * separation;
			offsets.push({"x": x, "z": z});
		}
		left -= n;
	}

	return offsets;
};

Formation.prototype.ComputeAveragePosition = function(positions)
{
	var sx = 0;
	var sz = 0;
	for each (var pos in positions)
	{
		sx += pos.x;
		sz += pos.z;
	}
	return { "x": sx / positions.length, "z": sz / positions.length };
};

/**
 * Set formation controller's radius and speed based on its current members.
 */
Formation.prototype.ComputeMotionParameters = function()
{
	var maxRadius = 0;
	var minSpeed = Infinity;

	for each (var ent in this.members)
	{
		var cmpObstruction = Engine.QueryInterface(ent, IID_Obstruction);
		if (cmpObstruction)
			maxRadius = Math.max(maxRadius, cmpObstruction.GetUnitRadius());

		var cmpUnitMotion = Engine.QueryInterface(ent, IID_UnitMotion);
		if (cmpUnitMotion)
			minSpeed = Math.min(minSpeed, cmpUnitMotion.GetWalkSpeed());
	}

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpUnitMotion.SetUnitRadius(maxRadius);
	cmpUnitMotion.SetSpeed(minSpeed);

	// TODO: we also need to do something about PassabilityClass, CostClass
};

Formation.prototype.OnGlobalOwnershipChanged = function(msg)
{
	// When an entity is captured or destroyed, it should no longer be
	// controlled by this formation

	if (this.members.indexOf(msg.entity) != -1)
		this.RemoveMembers([msg.entity]);
};

Engine.RegisterComponentType(IID_Formation, "Formation", Formation);
