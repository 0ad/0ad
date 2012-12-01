function Formation() {}

Formation.prototype.Schema =
	"<a:component type='system'/><empty/>";

var g_ColumnDistanceThreshold = 128; // distance at which we'll switch between column/box formations

Formation.prototype.Init = function()
{
	this.members = []; // entity IDs currently belonging to this formation
	this.inPosition = []; // entities that have reached their final position
	this.columnar = false; // whether we're travelling in column (vs box) formation
	this.formationName = "Line Closed";
	this.rearrange = true; // whether we should rearrange all formation members
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
 * Permits formation members to register that they've reached their
 * destination, and automatically disbands the formation if all members
 * are at their final positions and no controller orders remain.
 */
Formation.prototype.SetInPosition = function(ent)
{
	if (this.inPosition.indexOf(ent) != -1)
		return;

	// Only consider automatically disbanding if there are no orders left.
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (cmpUnitAI.GetOrders().length)
	{
		this.inPosition = [];
		return;
	}

	this.inPosition.push(ent);
	if (this.inPosition.length >= this.members.length)
		this.Disband();
};

/**
 * Called by formation members upon entering non-walking states.
 */
Formation.prototype.UnsetInPosition = function(ent)
{
	var ind = this.inPosition.indexOf(ent);
	if (ind != -1)
		this.inPosition.splice(ind, 1);
}

/**
 * Set whether we should rearrange formation members if
 * units are removed from the formation.
 */
Formation.prototype.SetRearrange = function(rearrange)
{
	this.rearrange = rearrange;
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
	this.inPosition = this.inPosition.filter(function(e) { return ents.indexOf(e) == -1; });

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

	if (!this.rearrange)
		return;

	this.ComputeMotionParameters();

	// Rearrange the remaining members
	this.MoveMembersIntoFormation(true);
};

/**
 * Called when the formation stops moving in order to detect
 * units that have already reached their final positions.
 */
Formation.prototype.FindInPosition = function()
{
	for (var i = 0; i < this.members.length; ++i)
	{
		var cmpUnitMotion = Engine.QueryInterface(this.members[i], IID_UnitMotion);
		if (!cmpUnitMotion.IsMoving())
		{
			// Verify that members are stopped in FORMATIONMEMBER.WALKING
			var cmpUnitAI = Engine.QueryInterface(this.members[i], IID_UnitAI);
			if (cmpUnitAI.IsWalking())
				this.SetInPosition(this.members[i]);
		}
	}
}

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
	this.inPosition = [];

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

	for each (var ent in this.members)
	{
		var cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		active.push(ent);
		positions.push(cmpPosition.GetPosition());
	}

	// Work out whether this should be a column or box formation
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var walkingDistance = cmpUnitAI.ComputeWalkingDistance();
	this.columnar = (walkingDistance > g_ColumnDistanceThreshold) && this.formationName != "Column Open";

	var offsets = this.ComputeFormationOffsets(active, this.columnar);

	var avgpos = this.ComputeAveragePosition(positions);
	var avgoffset = this.ComputeAveragePosition(offsets);

	// Reposition the formation if we're told to or if we don't already have a position
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (moveCenter || !cmpPosition.IsInWorld())
		cmpPosition.JumpTo(avgpos.x, avgpos.z);

	for (var i = 0; i < offsets.length; ++i)
	{
		var offset = offsets[i];

		var cmpUnitAI = Engine.QueryInterface(offset.ent, IID_UnitAI);
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

Formation.prototype.ComputeFormationOffsets = function(active, columnar)
{
	var separation = 4; // TODO: don't hardcode this

	var types = {
		"Cavalry" : [],
		"Hero" : [],
		"Melee" : [],
		"Ranged" : [],
		"Support" : [],
		"Unknown": []
	}; // TODO: make this work so we put ranged behind infantry etc

	for each (var ent in active)
	{
		var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		var classes = cmpIdentity.GetClassesList();
		var done = false;
		for each (var cla in classes)
		{
			if (cla in types)
			{
				types[cla].push(ent);
				done = true;
				break;
			}
		}
		if (!done)
		{
			types["Unknown"].push(ent);
		}
	}

	var count = active.length;

	var shape = undefined;
	var ordering = "default";

	var offsets = [];

	// Choose a sensible size/shape for the various formations, depending on number of units
	var cols;

	if (columnar)
		this.formationName = "Column Closed";

	switch(this.formationName)
	{
	case "Column Closed":
		// Have at most 3 files
		if (count <= 3)
			cols = count;
		else
			cols = 3;
		shape = "square";
		break;
	case "Phalanx":
		// Try to have at least 5 files (so batch training gives a single line),
		// and at most 8
		if (count <= 5)
			cols = count;
		else if (count <= 10)
			cols = 5;
		else if (count <= 16)
			cols = Math.ceil(count/2);
		else if (count <= 48)
			cols = 8;
		else
			cols = Math.ceil(count/6);
		shape = "square";
		break;
	case "Line Closed":
		if (count <= 3)
			cols = count;
		else if (count < 30)
			cols = Math.max(Math.ceil(count/2), 3);
		else
			cols = Math.ceil(count/3);
		shape = "square";
		break;
	case "Testudo":
		cols = Math.ceil(Math.sqrt(count));
		shape = "square";
		break;
	case "Column Open":
		cols = 2;
		shape = "opensquare";
		break;
	case "Line Open":
		if (count <= 5)
			cols = 3;
		else if (count <= 11)
			cols = 4;
		else if (count <= 18)
			cols = 5;
		else
			cols = 6;
		shape = "opensquare";
		break;
	case "Scatter":
		var width = Math.sqrt(count) * separation * 5;

		for (var i = 0; i < count; ++i)
		{
			offsets.push({"x": Math.random()*width, "z": Math.random()*width});
		}
		break;
	case "Circle":
		var depth;
		var pop;
		if (count <= 36)
		{
			pop = 12;
			depth = Math.ceil(count / pop);
		}
		else
		{
			depth = 3;
			pop = Math.ceil(count / depth);
		}

		var left = count;
		var radius = Math.min(left, pop) * separation / (2 * Math.PI);
		for (var c = 0; c < depth; ++c)
		{
			var ctodo = Math.min(left, pop);
			var cradius = radius - c * separation / 2;
			var delta = 2 * Math.PI / ctodo;
			for (var alpha = 0; ctodo; alpha += delta)
			{
				var x = Math.cos(alpha) * cradius;
				var z = Math.sin(alpha) * cradius;
				offsets.push({"x": x, "z": z});
				ctodo--;
				left--;
			}
		}
		break;
	case "Box":
		var root = Math.ceil(Math.sqrt(count));

		var left = count;
		var meleeleft = types["Melee"].length;
		for (var sq = Math.floor(root/2); sq >= 0; --sq)
		{
			var width = sq * 2 + 1;
			var stodo;
			if (sq == 0)
			{
				stodo = left;
			}
			else
			{
				if (meleeleft >= width*width - (width-2)*(width-2)) // form a complete box
				{
					stodo = width*width - (width-2)*(width-2);
					meleeleft -= stodo;
				}
				else	// compact
				{
					stodo = Math.max(0, left - (width-2)*(width-2));
				}
			}

			for (var r = -sq; r <= sq && stodo; ++r)
			{
				for (var c = -sq; c <= sq && stodo; ++c)
				{
					if (Math.abs(r) == sq || Math.abs(c) == sq)
					{
						var x = c * separation;
						var z = -r * separation;
						offsets.push({"x": x, "z": z});
						stodo--;
						left--;
					}
				}
			}
		}
		break;
	case "Skirmish":
		cols = Math.ceil(count/2);
		shape = "opensquare";
		break;
	case "Wedge":
		var depth = Math.ceil(Math.sqrt(count));

		var left = count;
		var width = 2 * depth - 1;
		for (var p = 0; p < depth && left; ++p)
		{
			for (var r = p; r < depth && left; ++r)
			{
				var c1 = depth - r + p;
				var c2 = depth + r - p;

				if (left)
				{
						var x = c1 * separation;
						var z = -r * separation;
						offsets.push({"x": x, "z": z});
						left--;
				}
				if (left && c1 != c2)
				{
						var x = c2 * separation;
						var z = -r * separation;
						offsets.push({"x": x, "z": z});
						left--;
				}
			}
		}
		break;
	case "Flank":
		cols = 3;
		var leftside = [];
		leftside[0] = Math.ceil(count/2);
		leftside[1] = Math.floor(count/2);
		ranks = Math.ceil(leftside[0] / cols);
		var off = - separation * 4;
		for (var side = 0; side < 2; ++side)
		{
			var left = leftside[side];
			off += side * separation * 8;
			for (var r = 0; r < ranks; ++r)
			{
				var n = Math.min(left, cols);
				for (var c = 0; c < n; ++c)
				{
					var x = off + ((n-1)/2 - c) * separation;
					var z = -r * separation;
					offsets.push({"x": x, "z": z});
				}
				left -= n;
			}
		}
		break;
	case "Syntagma":
		cols = Math.ceil(Math.sqrt(count));
		shape = "square";
		break;
	case "Battle Line":
		if (count <= 5)
			cols = count;
		else if (count <= 10)
			cols = 5;
		else if (count <= 16)
			cols = Math.ceil(count/2);
		else if (count <= 48)
			cols = 8;
		else
			cols = Math.ceil(count/6);
		shape = "opensquare";
		separation /= 1.5;
		ordering = "cavalryOnTheSides";
		break;
	default:
		warn("Unknown formation: " + this.formationName);
		break;
	}

	if (shape == "square")
	{
		var ranks = Math.ceil(count / cols);

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
	}
	else if (shape == "opensquare")
	{
		var left = count;
		for (var r = 0; left; ++r)
		{
			var n = Math.min(left, cols - (r&1?1:0));
			for (var c = 0; c < 2*n; c+=2)
			{
				var x = (- c - (r&1)) * separation;
				var z = -r * separation;
				offsets.push({"x": x, "z": z});
			}
			left -= n;
		}
	}

	// TODO: assign to minimise worst-case distances or whatever
	if (ordering == "default")
	{
		for each (var offset in offsets)
		{
			var ent = undefined;
			for (var t in types)
			{
				if (types[t].length)
				{
					ent = types[t].pop();
					offset.ent = ent;
					break;
				}
			}
		}
	}
	else if (ordering == "cavalryOnTheSides")
	{
		var noffset = [];
		var cavalrycount = types["Cavalry"].length;
		offsets.sort(function (a, b) {
			if (a.x < b.x) return -1;
			else if (a.x == b.x && a.z < b.z) return -1;
			return 1;
		});

		while (offsets.length && types["Cavalry"].length && types["Cavalry"].length > cavalrycount/2)
		{
			var offset = offsets.pop();
			offset.ent = types["Cavalry"].pop();
			noffset.push(offset);
		}

		offsets.sort(function (a, b) {
			if (a.x > b.x) return -1;
			else if (a.x == b.x && a.z < b.z) return -1;
			return 1;
		});

		while (offsets.length && types["Cavalry"].length)
		{
			var offset = offsets.pop();
			offset.ent = types["Cavalry"].pop();
			noffset.push(offset);
		}

		offsets.sort(function (a, b) {
			if (a.z < b.z) return -1;
			else if (a.z == b.z && a.x < b.x) return -1;
			return 1;
		});

		while (offsets.length)
		{
			var offset = offsets.pop();
			for (var t in types)
			{
				if (types[t].length)
				{
					offset.ent = types[t].pop();
					break;
				}
			}
			noffset.push(offset);
		}
		offsets = noffset;
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

Formation.prototype.OnGlobalEntityRenamed = function(msg)
{
	if (this.members.indexOf(msg.entity) != -1)
	{
		var cmpNewUnitAI = Engine.QueryInterface(msg.newentity, IID_UnitAI);
		if (cmpNewUnitAI)
			this.members[this.members.indexOf(msg.entity)] = msg.newentity;

		var cmpOldUnitAI = Engine.QueryInterface(msg.entity, IID_UnitAI);
		cmpOldUnitAI.SetFormationController(INVALID_ENTITY);

		if (cmpNewUnitAI)
			cmpNewUnitAI.SetFormationController(this.entity);

		// Because the renamed entity might have different characteristics,
		// (e.g. packed vs. unpacked siege), we need to recompute motion parameters
		this.ComputeMotionParameters();
	}
}

Formation.prototype.LoadFormation = function(formationName)
{
	this.formationName = formationName;
	for each (var ent in this.members)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI.SetLastFormationName(this.formationName);
	}
};

Engine.RegisterComponentType(IID_Formation, "Formation", Formation);
