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
	this.formationMembersWithAura = []; // Members with a formation aura
	this.width = 0;
	this.depth = 0;
	this.oldOrientation = {"sin": 0, "cos": 0};
};

Formation.prototype.GetSize = function()
{
	return {"width": this.width, "depth": this.depth};
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
		
		var cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		if (cmpAuras && cmpAuras.HasFormationAura())
		{
			this.formationMembersWithAura.push(ent);
			cmpAuras.ApplyFormationBonus(ents);
		}
	}

	this.offsets = undefined;
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
		cmpUnitAI.UpdateWorkOrders();
		cmpUnitAI.SetFormationController(INVALID_ENTITY);
	}

	for each (var ent in this.formationMembersWithAura)
	{
		var cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		cmpAuras.RemoveFormationBonus(ents);

		// the unit with the aura is also removed from the formation
		if (ents.indexOf(ent) !== -1)
			cmpAuras.RemoveFormationBonus(this.members);
	}

	this.formationMembersWithAura = this.formationMembersWithAura.filter(function(e) { return ents.indexOf(e) == -1; });

	// If there's nobody left, destroy the formation
	if (this.members.length == 0)
	{
		Engine.DestroyEntity(this.entity);
		return;
	}

	if (!this.rearrange)
		return;

	this.offsets = undefined;
	this.ComputeMotionParameters();

	// Rearrange the remaining members
	this.MoveMembersIntoFormation(true, true);
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

	for each (var ent in this.formationMembersWithAura)
	{
		var cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		cmpAuras.RemoveFormationBonus(this.members);
	}


	this.members = [];
	this.inPosition = [];
	this.formationMembersWithAura = [];
	this.offsets = undefined;

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
 * Call obj.functname(args) on UnitAI components of all members,
 * and return true if all calls return true.
 */
Formation.prototype.TestAllMemberFunction = function(funcname, args)
{
	for each (var ent in this.members)
	{
		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (!cmpUnitAI[funcname].apply(cmpUnitAI, args))
			return false;
	}
	return true;
};

Formation.prototype.GetMaxAttackRangeFunction = function(target)
{
	var result = 0;
	var range = 0;
	for each (var ent in this.members)
	{
		var cmpAttack = Engine.QueryInterface(ent, IID_Attack);
		if (!cmpAttack)
			continue;

		var type = cmpAttack.GetBestAttackAgainst(target);
		if (!type)
			continue;

		range = cmpAttack.GetRange(type);
		if (range.max > result)
			result = range.max;
	}
	return result;
};

/**
 * Set all members to form up into the formation shape.
 * If moveCenter is true, the formation center will be reinitialised
 * to the center of the units.
 * If force is true, all individual orders of the formation units are replaced,
 * otherwise the order to walk into formation is just pushed to the front.
 */
Formation.prototype.MoveMembersIntoFormation = function(moveCenter, force)
{
	var active = [];
	var positions = [];

	for each (var ent in this.members)
	{
		var cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		active.push(ent);
		// query the 2D position as exact hight calculation isn't needed
		// but bring the position to the right coordinates
		var pos = cmpPosition.GetPosition2D();
		pos.z = pos.y;
		pos.y = undefined;
		positions.push(pos);
	}

	// Work out whether this should be a column or box formation
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var walkingDistance = cmpUnitAI.ComputeWalkingDistance();
	this.columnar = (walkingDistance > g_ColumnDistanceThreshold) && this.formationName != "Column Open";


	var avgpos = this.ComputeAveragePosition(positions);

	// Reposition the formation if we're told to or if we don't already have a position
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var inWorld = cmpPosition.IsInWorld();
	if (moveCenter || !inWorld)
	{
		cmpPosition.JumpTo(avgpos.x, avgpos.z);
		// Don't make the formation controller entity show up in range queries
		if (!inWorld)
		{
			var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
			cmpRangeManager.SetEntityFlag(this.entity, "normal", false);
		}
	}

	var newOrientation = this.GetTargetOrientation(avgpos);
	var dSin = Math.abs(newOrientation.sin - this.oldOrientation.sin);
	var dCos = Math.abs(newOrientation.cos - this.oldOrientation.cos);
	// If the formation existed, only recalculate positions if the turning agle is somewhat biggish
	if (!this.offsets || dSin > 1 || dCos > 1)
		this.offsets = this.ComputeFormationOffsets(active, positions, this.columnar);

	this.oldOrientation = newOrientation;

	var xMax = 0;
	var zMax = 0;

	for (var i = 0; i < this.offsets.length; ++i)
	{
		var offset = this.offsets[i];

		var cmpUnitAI = Engine.QueryInterface(offset.ent, IID_UnitAI);
		
		if (force)
		{
			cmpUnitAI.ReplaceOrder("FormationWalk", {
				"target": this.entity,
				"x": offset.x,
				"z": offset.z
			});
		}
		else
		{
			cmpUnitAI.PushOrderFront("FormationWalk", {
				"target": this.entity,
				"x": offset.x,
				"z": offset.z
			});
		}
		xMax = Math.max(xMax, offset.x);
		zMax = Math.max(zMax, offset.z);
	}
	this.width = xMax * 2;
	this.depth = zMax * 2;
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
	var inWorld = cmpPosition.IsInWorld();
	cmpPosition.JumpTo(avgpos.x, avgpos.z);

	// Don't make the formation controller show up in range queries
	if (!inWorld)
	{
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		cmpRangeManager.SetEntityFlag(this.entity, "normal", false);
	}
};

Formation.prototype.GetAvgObstructionRange = function(active)
{
	var obstructions = [];
	for each (var ent in active)
	{
		var cmpObstruction = Engine.QueryInterface(ent, IID_Obstruction);
		if (cmpObstruction)
			obstructions.push(cmpObstruction.GetUnitRadius());
	}
	if (!obstructions.length)
		return 1;
	return obstructions.reduce(function(m,v) {return m + v;}, 0) / obstructions.length;
};

Formation.prototype.ComputeFormationOffsets = function(active, positions, columnar)
{
	// TODO give some sense to this number
	var separation = this.GetAvgObstructionRange(active) * 4; 

	// the entities will be assigned to positions in the formation in 
	// the same order as the types list is ordered
	var types = {
		"Cavalry" : [],
		"Hero" : [],
		"Melee" : [],
		"Ranged" : [],
		"Support" : [],
		"Unknown": []
	}; 

	for (var i in active)
	{
		var cmpIdentity = Engine.QueryInterface(active[i], IID_Identity);
		var classes = cmpIdentity.GetClassesList();
		var done = false;
		for each (var cla in classes)
		{
			if (cla in types)
			{
				types[cla].push({"ent": active[i], "pos": positions[i]});
				done = true;
				break;
			}
		}
		if (!done)
		{
			types["Unknown"].push({"ent": active[i], "pos": positions[i]});
		}
	}

	var count = active.length;

	var shape = undefined;
	var ordering = [];

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
		ordering = ["FillFromTheCenter", "FillFromTheFront"];
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
		ordering = ["FillFromTheCenter", "FillFromTheFront"];
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
		ordering.push("FillFromTheCenter");
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
		ordering.push("FillFromTheSides");
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
	// make sure the average offset is zero, as the formation is centered around that
	// calculating offset distances without a zero average makes no sense, as the formation
	// will jump to a different position any time
	var avgoffset = this.ComputeAveragePosition(offsets);
	for each (var offset in offsets)
	{
		offset.x -= avgoffset.x;
		offset.z -= avgoffset.z;
	}

	// sort the available places in certain ways
	// the places first in the list will contain the heaviest units as defined by the order
	// of the types list
	for each (var order in ordering)
	{
		if (order == "FillFromTheSides")
			offsets.sort(function(o1, o2) { return Math.abs(o1.x) < Math.abs(o2.x);});
		else if (order == "FillFromTheCenter")
			offsets.sort(function(o1, o2) { return Math.abs(o1.x) > Math.abs(o2.x);});
		else if (order == "FillFromTheFront")
			offsets.sort(function(o1, o2) { return o1.z < o2.z;});
	}

	// query the 2D position of the formation, and bring to the right coordinate system
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var formationPos = cmpPosition.GetPosition2D();
	formationPos.z = formationPos.y;
	formationPos.y = undefined;

	// use realistic place assignment,
	// every soldier searches the closest available place in the formation
	var newOffsets = [];
	var realPositions = this.GetRealOffsetPositions(offsets, formationPos);
	for each (var t in types)
	{
		var usedOffsets = offsets.splice(0,t.length);
		var usedRealPositions = realPositions.splice(0, t.length);
		for each (var entPos in t)
		{
			var closestOffsetId = this.TakeClosestOffset(entPos, usedRealPositions);
			usedRealPositions.splice(closestOffsetId, 1);
			newOffsets.push(usedOffsets.splice(closestOffsetId, 1)[0]);
			newOffsets[newOffsets.length - 1].ent = entPos.ent;
		}
	}

	return newOffsets;
};

/**
 * Search the closest position in the realPositions list to the given entity
 * @param ent, the queried entity
 * @param realPositions, the world coordinates of the available offsets
 * @return the index of the closest offset position
 */
Formation.prototype.TakeClosestOffset = function(entPos, realPositions)
{
	var pos = entPos.pos;
	var closestOffsetId = -1;
	var offsetDistanceSq = Infinity;
	for (var i = 0; i < realPositions.length; i++)
	{
		var dx = realPositions[i].x - pos.x;
		var dz = realPositions[i].z - pos.z;
		var distSq = dx * dx + dz * dz;
		if (distSq < offsetDistanceSq)
		{
			offsetDistanceSq = distSq;
			closestOffsetId = i;
		}
	}
	return closestOffsetId;
};

/**
 * Get the world positions for a list of offsets in this formation
 */
Formation.prototype.GetRealOffsetPositions = function(offsets, pos)
{
	var offsetPositions = [];
	var {sin, cos} = this.GetTargetOrientation(pos);
	// calculate the world positions
	for each (var o in offsets)
		offsetPositions.push({
			"x" : pos.x + o.z * sin + o.x * cos, 
			"z" : pos.z + o.z * cos - o.x * sin
		});

	return offsetPositions;
};

/**
 * calculate the estimated rotation of the formation 
 * based on the first unitAI target position
 * Return the sine and cosine of the angle
 */
Formation.prototype.GetTargetOrientation = function(pos)
{
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var targetPos = cmpUnitAI.GetTargetPositions();
	var sin = 1;
	var cos = 0;
	if (targetPos.length)
	{
		var dx = targetPos[0].x - pos.x;
		var dz = targetPos[0].z - pos.z;
		if (dx || dz)
		{
			var dist = Math.sqrt(dx * dx + dz * dz);
			cos = dz / dist;
			sin = dx / dist;
		}
	}
	return {"sin": sin, "cos": cos};
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
		this.MoveMembersIntoFormation(false, true);
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
	this.offsets = undefined;
};

Engine.RegisterComponentType(IID_Formation, "Formation", Formation);
