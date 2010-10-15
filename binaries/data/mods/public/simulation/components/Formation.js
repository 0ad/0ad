function Formation() {}

Formation.prototype.Schema =
	"<a:component type='system'/><empty/>";

var g_ColumnDistanceThreshold = 48; // distance at which we'll switch between column/box formations

Formation.prototype.Init = function()
{
	this.members = []; // entity IDs currently belonging to this formation
	this.columnar = false; // whether we're travelling in column (vs box) formation
};

Formation.prototype.GetMemberCount = function()
{
	return this.members.length;
};

/**
 * Returns the 'primary' member of this formation (typically the most
 * important unit type), for e.g. playing a representative sound.
 * Returns undefined if no members.
 * TODO: actually implement something like that; currently this just returns
 * the arbitrary first one.
 */
Formation.prototype.GetPrimaryMember = function()
{
	return this.members[0];
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
	this.MoveMembersIntoFormation(true);
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
 * If moveCenter is true, the formation center will be reinitialised
 * to the center of the units.
 */
Formation.prototype.MoveMembersIntoFormation = function(moveCenter)
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

	// Work out whether this should be a column or box formation
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var walkingDistance = cmpUnitAI.ComputeWalkingDistance();
	this.columnar = (walkingDistance > g_ColumnDistanceThreshold);

	var offsets = this.ComputeFormationOffsets(types, this.columnar);

	var avgpos = this.ComputeAveragePosition(positions);
	var avgoffset = this.ComputeAveragePosition(offsets);

	// Reposition the formation if we're told to or if we don't already have a position
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (moveCenter || !cmpPosition.IsInWorld())
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

Formation.prototype.ComputeFormationOffsets = function(types, columnar)
{
	var separation = 4; // TODO: don't hardcode this

	var count = types["Unknown"];

	// Choose a sensible width for the basic default formation
	var cols;
	if (columnar)
	{
		// Have at most 3 files
		if (count <= 3)
			cols = count;
		else
			cols = 3;
	}
	else
	{
		// Try to have at least 5 files (so batch training gives a single line),
		// and at most 8
		if (count <= 5)
			cols = count;
		if (count <= 10)
			cols = 5;
		else if (count <= 16)
			cols = Math.ceil(count / 2);
		else
			cols = 8;
	}

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

Formation.prototype.OnUpdate_Final = function(msg)
{
	// Switch between column and box if necessary
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var walkingDistance = cmpUnitAI.ComputeWalkingDistance();
	var columnar = (walkingDistance > g_ColumnDistanceThreshold);
	if (columnar != this.columnar)
		this.MoveMembersIntoFormation(false);
		// (disable moveCenter so we can't get stuck in a loop of switching
		// shape causing center to change causing shape to switch back)
};

Formation.prototype.OnGlobalOwnershipChanged = function(msg)
{
	// When an entity is captured or destroyed, it should no longer be
	// controlled by this formation

	if (this.members.indexOf(msg.entity) != -1)
		this.RemoveMembers([msg.entity]);
};

Engine.RegisterComponentType(IID_Formation, "Formation", Formation);
