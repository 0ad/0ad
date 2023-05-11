function Formation() {}

Formation.prototype.Schema =
	"<element name='RequiredMemberCount' a:help='Minimum number of entities the formation should contain (at least 2).'>" +
		"<data type='integer'>" +
		  "<param name='minInclusive'>"+
		    "2"+
		  "</param>"+
		"</data>" +
	"</element>" +
	"<element name='DisabledTooltip' a:help='Tooltip shown when the formation is disabled.'>" +
		"<text/>" +
	"</element>" +
	"<element name='SpeedMultiplier' a:help='The speed of the formation is determined by the minimum speed of all members, multiplied with this number.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='FormationShape' a:help='Formation shape, currently supported are square, triangle and special, where special will be defined in the source code.'>" +
		"<text/>" +
	"</element>" +
	"<element name='MaxTurningAngle' a:help='The turning angle in radian under which the formation attempts to turn and over which the formation positions are recomputed.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='ShiftRows' a:help='Set the value to true to shift subsequent rows.'>" +
		"<text/>" +
	"</element>" +
	"<element name='SortingClasses' a:help='Classes will be added to the formation in this order. Where the classes will be added first depends on the formation.'>" +
		"<text/>" +
	"</element>" +
	"<optional>" +
	"<element name='SortingOrder' a:help='The order of sorting. This defaults to an order where the formation is filled from the first row to the last, and each row from the center to the sides. Other possible sort orders are &#x201C;fillFromTheSides&#x201D;, where the most important units are on the sides of each row, and &#x201C;fillToTheCenter&#x201D;, where the most vulnerable units are in the center of the formation.'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<element name='WidthDepthRatio' a:help='Average width-to-depth ratio, counted in number of units.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Sloppiness' a:help='The maximum difference between the actual and the perfectly aligned formation position, in meters.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='MinColumns' a:help='When possible, this number of columns will be created. Overriding the wanted width-to-depth ratio.'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='MaxColumns' a:help='When possible within the number of units, and the maximum number of rows, this will be the maximum number of columns.'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='MaxRows' a:help='The maximum number of rows in the formation.'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='CenterGap' a:help='The size of the central gap, expressed in number of units wide.'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<element name='UnitSeparationWidthMultiplier' a:help='Place the units in the formation closer or further to each other. The standard separation is the footprint size.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='UnitSeparationDepthMultiplier' a:help='Place the units in the formation closer or further to each other. The standard separation is the footprint size.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='AnimationVariants' a:help='Give a list of animation variants to use for the particular formation members, based on their positions.'>" +
		"<text a:help='example text: &#x201C;1..1,1..-1:animationVariant1;2..2,1..-1;animationVariant2&#x201D;, this will set animationVariant1 for the first row, and animation2 for the second row. The first part of the numbers (1..1 and 2..2) means the row range. Every row between (and including) those values will switch animationvariants. The second part of the numbers (1..-1) denote the columns inside those rows that will be affected. Note that in both cases, you can use -1 for the last row/column, -2 for the second to last, etc.'/>" +
	"</element>";

// Distance at which we'll switch between column/box formations.
var g_ColumnDistanceThreshold = 128;

// Distance under which the formation will not try to turn towards the target position.
var g_RotateDistanceThreshold = 1;

Formation.prototype.variablesToSerialize = [
	"lastOrderVariant",
	"members",
	"memberPositions",
	"maxRowsUsed",
	"maxColumnsUsed",
	"finishedEntities",
	"idleEntities",
	"columnar",
	"rearrange",
	"formationMembersWithAura",
	"width",
	"depth",
	"twinFormations",
	"formationSeparation",
	"offsets"
];

Formation.prototype.Init = function(deserialized = false)
{
	this.maxTurningAngle = +this.template.MaxTurningAngle;
	this.sortingClasses = this.template.SortingClasses.split(/\s+/g);
	this.shiftRows = this.template.ShiftRows == "true";
	this.separationMultiplier = {
		"width": +this.template.UnitSeparationWidthMultiplier,
		"depth": +this.template.UnitSeparationDepthMultiplier
	};
	this.sloppiness = +this.template.Sloppiness;
	this.widthDepthRatio = +this.template.WidthDepthRatio;
	this.minColumns = +(this.template.MinColumns || 0);
	this.maxColumns = +(this.template.MaxColumns || 0);
	this.maxRows = +(this.template.MaxRows || 0);
	this.centerGap = +(this.template.CenterGap || 0);

	if (this.template.AnimationVariants)
	{
		this.animationvariants = [];
		let differentAnimationVariants = this.template.AnimationVariants.split(/\s*;\s*/);
		// Loop over the different rectangulars that will map to different animation variants.
		for (let rectAnimationVariant of differentAnimationVariants)
		{
			let rect, replacementAnimationVariant;
			[rect, replacementAnimationVariant] = rectAnimationVariant.split(/\s*:\s*/);
			let rows, columns;
			[rows, columns] = rect.split(/\s*,\s*/);
			let minRow, maxRow, minColumn, maxColumn;
			[minRow, maxRow] = rows.split(/\s*\.\.\s*/);
			[minColumn, maxColumn] = columns.split(/\s*\.\.\s*/);
			this.animationvariants.push({
				"minRow": +minRow,
				"maxRow": +maxRow,
				"minColumn": +minColumn,
				"maxColumn": +maxColumn,
				"name": replacementAnimationVariant
			});
		}
	}

	this.lastOrderVariant = undefined;
	// Entity IDs currently belonging to this formation.
	this.members = [];
	this.memberPositions = {};
	this.maxRowsUsed = 0;
	this.maxColumnsUsed = [];
	// Entities that have finished the original task.
	this.finishedEntities = new Set();
	this.idleEntities = new Set();
	// Whether we're travelling in column (vs box) formation.
	this.columnar = false;
	// Whether we should rearrange all formation members.
	this.rearrange = true;
	// Members with a formation aura.
	this.formationMembersWithAura = [];
	this.width = 0;
	this.depth = 0;
	this.twinFormations = [];
	// Distance from which two twin formations will merge into one.
	this.formationSeparation = 0;

	if (deserialized)
		return;

	Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer)
		.SetInterval(this.entity, IID_Formation, "ShapeUpdate", 1000, 1000, null);
};

Formation.prototype.Serialize = function()
{
	let result = {};
	for (let key of this.variablesToSerialize)
		result[key] = this[key];

	return result;
};

Formation.prototype.Deserialize = function(data)
{
	this.Init(true);
	for (let key in data)
		this[key] = data[key];
};

/**
 * Set the value from which two twin formations will become one.
 */
Formation.prototype.SetFormationSeparation = function(value)
{
	this.formationSeparation = value;
};

Formation.prototype.GetSize = function()
{
	return { "width": this.width, "depth": this.depth };
};

Formation.prototype.GetSpeedMultiplier = function()
{
	return +this.template.SpeedMultiplier;
};

Formation.prototype.GetMemberCount = function()
{
	return this.members.length;
};

Formation.prototype.GetMembers = function()
{
	return this.members;
};

Formation.prototype.GetClosestMember = function(ent, filter)
{
	let cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
	if (!cmpEntPosition || !cmpEntPosition.IsInWorld())
		return INVALID_ENTITY;

	let entPosition = cmpEntPosition.GetPosition2D();
	let closestMember = INVALID_ENTITY;
	let closestDistance = Infinity;
	for (let member of this.members)
	{
		if (filter && !filter(ent))
			continue;

		let cmpPosition = Engine.QueryInterface(member, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		let pos = cmpPosition.GetPosition2D();
		let dist = entPosition.distanceToSquared(pos);
		if (dist < closestDistance)
		{
			closestMember = member;
			closestDistance = dist;
		}
	}
	return closestMember;
};

/**
 * Returns the 'primary' member of this formation (typically the most
 * important unit type), for e.g. playing a representative sound.
 * Returns undefined if no members.
 * TODO: Actually implement something like that. Currently this just returns
 * the arbitrary first one.
 */
Formation.prototype.GetPrimaryMember = function()
{
	return this.members[0];
};

/**
 * Get the formation animation variant for a certain member of this formation.
 * @param entity The entity ID to get the animation for.
 * @return The name of the animation variant as defined in the template,
 * e.g. "testudo_front" or undefined if does not exist.
 */
Formation.prototype.GetFormationAnimationVariant = function(entity)
{
	if (!this.animationvariants || !this.animationvariants.length || this.columnar || !this.memberPositions[entity])
		return undefined;
	let row = this.memberPositions[entity].row;
	let column = this.memberPositions[entity].column;
	for (let i = 0; i < this.animationvariants.length; ++i)
	{
		let minRow = this.animationvariants[i].minRow;
		if (minRow < 0)
			minRow += this.maxRowsUsed + 1;
		if (row < minRow)
			continue;

		let maxRow = this.animationvariants[i].maxRow;
		if (maxRow < 0)
			maxRow += this.maxRowsUsed + 1;
		if (row > maxRow)
			continue;

		let minColumn = this.animationvariants[i].minColumn;
		if (minColumn < 0)
			minColumn += this.maxColumnsUsed[row] + 1;
		if (column < minColumn)
			continue;

		let maxColumn = this.animationvariants[i].maxColumn;
		if (maxColumn < 0)
			maxColumn += this.maxColumnsUsed[row] + 1;
		if (column > maxColumn)
			continue;

		return this.animationvariants[i].name;
	}
	return undefined;
};

Formation.prototype.SetFinishedEntity = function(ent)
{
	// Rotate the entity to the correct angle.
	const cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	const cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
	if (cmpEntPosition && cmpEntPosition.IsInWorld() && cmpPosition && cmpPosition.IsInWorld())
		cmpEntPosition.TurnTo(cmpPosition.GetRotation().y);

	this.finishedEntities.add(ent);
};

Formation.prototype.UnsetFinishedEntity = function(ent)
{
	this.finishedEntities.delete(ent);
};

Formation.prototype.ResetFinishedEntities = function()
{
	this.finishedEntities.clear();
};

Formation.prototype.AreAllMembersFinished = function()
{
	return this.finishedEntities.size === this.members.length;
};

Formation.prototype.SetIdleEntity = function(ent)
{
	this.idleEntities.add(ent);
};

Formation.prototype.UnsetIdleEntity = function(ent)
{
	this.idleEntities.delete(ent);
};

Formation.prototype.ResetIdleEntities = function()
{
	this.idleEntities.clear();
};

Formation.prototype.AreAllMembersIdle = function()
{
	return this.idleEntities.size === this.members.length;
};

/**
 * Set whether we are allowed to rearrange formation members.
 */
Formation.prototype.SetRearrange = function(rearrange)
{
	this.rearrange = rearrange;
};

/**
 * Initialize the members of this formation.
 * Must only be called once.
 * All members must implement UnitAI.
 */
Formation.prototype.SetMembers = function(ents)
{
	this.members = ents;

	for (let ent of this.members)
	{
		let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI.SetFormationController(this.entity);

		let cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		if (cmpAuras && cmpAuras.HasFormationAura())
		{
			this.formationMembersWithAura.push(ent);
			cmpAuras.ApplyFormationAura(ents);
		}
	}

	this.offsets = undefined;
	// Locate this formation controller in the middle of its members.
	this.MoveToMembersCenter();

	// Compute the speed etc. of the formation.
	this.ComputeMotionParameters();
};

/**
 * Remove the given list of entities.
 * The entities must already be members of this formation.
 * @param {boolean} rename - Whether the removal was part of an entity rename
	(prevents disbanding of the formation when under the member limit).
 */
Formation.prototype.RemoveMembers = function(ents, renamed = false)
{
	if (!ents.length)
		return;

	this.offsets = undefined;
	this.members = this.members.filter(ent => !ents.includes(ent));

	for (const ent of ents)
	{
		this.finishedEntities.delete(ent);
		const cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI.UpdateWorkOrders();
		cmpUnitAI.UnsetFormationController();
	}

	for (let ent of this.formationMembersWithAura)
	{
		const cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		cmpAuras.RemoveFormationAura(ents);

		// The unit with the aura is also removed from the formation.
		if (ents.includes(ent))
			cmpAuras.RemoveFormationAura(this.members);
	}

	this.formationMembersWithAura = this.formationMembersWithAura.filter(ent => !ents.includes(ent));

	// If there's nobody left, destroy the formation
	// unless this is a rename where we can have 0 members temporarily.
	if (!renamed && this.members.length < +this.template.RequiredMemberCount)
	{
		this.Disband();
		return;
	}

	this.ComputeMotionParameters();

	if (!this.rearrange)
		return;

	// Rearrange the remaining members.
	this.MoveMembersIntoFormation(true, true, this.lastOrderVariant);
};

Formation.prototype.AddMembers = function(ents)
{
	this.offsets = undefined;

	for (let ent of this.formationMembersWithAura)
	{
		let cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		cmpAuras.ApplyFormationAura(ents);
	}

	this.members = this.members.concat(ents);

	for (let ent of ents)
	{
		let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI.SetFormationController(this.entity);
		if (!cmpUnitAI.GetOrders().length)
			cmpUnitAI.SetNextState("FORMATIONMEMBER.IDLE");

		let cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		if (cmpAuras && cmpAuras.HasFormationAura())
		{
			this.formationMembersWithAura.push(ent);
			cmpAuras.ApplyFormationAura(this.members);
		}
	}

	this.ComputeMotionParameters();

	if (!this.rearrange)
		return;

	this.MoveMembersIntoFormation(true, true, this.lastOrderVariant);
};

/**
 * Remove all members and destroy the formation.
 */
Formation.prototype.Disband = function()
{
	this.RemoveMembers(this.members);

	// Hack: switch to a clean state to stop timers.
	const cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	cmpUnitAI.UnitFsm.SwitchToNextState(cmpUnitAI, "");
	Engine.QueryInterface(this.entity, IID_Position).MoveOutOfWorld();
	this.DeleteTwinFormations();
	Engine.DestroyEntity(this.entity);
};

/**
 * Set all members to form up into the formation shape.
 * @param {boolean} moveCenter - The formation center will be reinitialized
 * to the center of the units.
 * @param {boolean} force - All individual orders of the formation units are replaced,
 * otherwise the order to walk into formation is just pushed to the front.
 * @param {string | undefined} variant - Variant to be passed as order parameter.
 */
Formation.prototype.MoveMembersIntoFormation = function(moveCenter, force, variant)
{
	if (!this.members.length)
		return;

	let active = [];
	let positions = [];

	for (let ent of this.members)
	{
		let cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		active.push(ent);
		// Query the 2D position as the exact height calculation isn't needed,
		// but bring the position to the correct coordinates.
		positions.push(cmpPosition.GetPosition2D());
	}

	const cmpFormationUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	const cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	// Reposition the formation if we're told to or if we don't already have a position.
	if (cmpPosition && (moveCenter || !cmpPosition.IsInWorld()))
	{
		const avgpos = Vector2D.average(positions);
		const targetPosition = cmpFormationUnitAI.GetTargetPositions()[0];

		const oldRotation = cmpPosition.GetRotation().y;
		const newRotation = targetPosition !== undefined && avgpos.distanceToSquared(targetPosition) > g_RotateDistanceThreshold ? avgpos.angleTo(targetPosition) : oldRotation;

		// When we are out of world or the angle difference is big, trigger repositioning.
		// Do this before setting up the position, because then we will always be in world.
		if (!cmpPosition.IsInWorld() || !this.DoesAngleDifferenceAllowTurning(newRotation, oldRotation))
			this.offsets = undefined;

		this.SetupPositionAndHandleRotation(avgpos.x, avgpos.y, newRotation, true);
	}

	// Switch between column and box if necessary.
	const columnar = cmpFormationUnitAI.ComputeWalkingDistance() > g_ColumnDistanceThreshold;
	if (columnar != this.columnar)
	{
		this.columnar = columnar;
		this.offsets = undefined;
	}

	this.lastOrderVariant = variant;

	let offsetsChanged = false;
	if (!this.offsets)
	{
		this.offsets = this.ComputeFormationOffsets(active, positions);
		offsetsChanged = true;
	}

	let xMax = 0;
	let yMax = 0;
	let xMin = 0;
	let yMin = 0;

	if (force)
		// Reset finishedEntities as FormationWalk is called.
		this.ResetFinishedEntities();

	for (let i = 0; i < this.offsets.length; ++i)
	{
		let offset = this.offsets[i];

		let cmpUnitAI = Engine.QueryInterface(offset.ent, IID_UnitAI);
		if (!cmpUnitAI)
		{
			warn("Entities without UnitAI in formation are not supported.");
			continue;
		}

		let data =
		{
			"target": this.entity,
			"x": offset.x,
			"z": offset.y,
			"offsetsChanged": offsetsChanged,
			"variant": variant
		};
		cmpUnitAI.AddOrder("FormationWalk", data, !force);
		xMax = Math.max(xMax, offset.x);
		yMax = Math.max(yMax, offset.y);
		xMin = Math.min(xMin, offset.x);
		yMin = Math.min(yMin, offset.y);
	}
	this.width = xMax - xMin;
	this.depth = yMax - yMin;
};

Formation.prototype.MoveToMembersCenter = function()
{
	let positions = [];
	let rotations = 0;

	for (let ent of this.members)
	{
		let cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		positions.push(cmpPosition.GetPosition2D());
		rotations += cmpPosition.GetRotation().y;
	}

	let avgpos = Vector2D.average(positions);
	this.SetupPositionAndHandleRotation(avgpos.x, avgpos.y, rotations / positions.length, false);
};

/**
 * Set formation position.
 * If formation is not in world at time this is called, set new rotation and flag
 * for rangeManager. Also set the rotation if it is forced.
 */
Formation.prototype.SetupPositionAndHandleRotation = function(x, y, rot, forceRotation)
{
	const cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition)
		return;
	const wasInWorld = cmpPosition.IsInWorld();
	cmpPosition.JumpTo(x, y);

	if (!forceRotation && wasInWorld)
		return;

	cmpPosition.TurnTo(rot);
	if (!wasInWorld)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).SetEntityFlag(this.entity, "normal", false);
};

Formation.prototype.GetAvgFootprint = function(active)
{
	let footprints = [];
	for (let ent of active)
	{
		let cmpFootprint = Engine.QueryInterface(ent, IID_Footprint);
		if (cmpFootprint)
			footprints.push(cmpFootprint.GetShape());
	}
	if (!footprints.length)
		return { "width": 1, "depth": 1 };

	let r = { "width": 0, "depth": 0 };
	for (let shape of footprints)
	{
		if (shape.type == "circle")
		{
			r.width += shape.radius * 2;
			r.depth += shape.radius * 2;
		}
		else if (shape.type == "square")
		{
			r.width += shape.width;
			r.depth += shape.depth;
		}
	}
	r.width /= footprints.length;
	r.depth /= footprints.length;
	return r;
};

Formation.prototype.ComputeFormationOffsets = function(active, positions)
{
	let separation = this.GetAvgFootprint(active);
	separation.width *= this.separationMultiplier.width;
	separation.depth *= this.separationMultiplier.depth;

	let sortingClasses;
	if (this.columnar)
		sortingClasses = ["Cavalry", "Infantry"];
	else
		sortingClasses = this.sortingClasses.slice();
	sortingClasses.push("Unknown");

	// The entities will be assigned to positions in the formation in
	// the same order as the types list is ordered.
	let types = {};
	for (let i = 0; i < sortingClasses.length; ++i)
		types[sortingClasses[i]] = [];

	for (let i in active)
	{
		let cmpIdentity = Engine.QueryInterface(active[i], IID_Identity);
		let classes = cmpIdentity.GetClassesList();
		let done = false;
		for (let c = 0; c < sortingClasses.length; ++c)
		{
			if (classes.indexOf(sortingClasses[c]) > -1)
			{
				types[sortingClasses[c]].push({ "ent": active[i], "pos": positions[i] });
				done = true;
				break;
			}
		}
		if (!done)
			types.Unknown.push({ "ent": active[i], "pos": positions[i] });
	}

	let count = active.length;

	let shape = this.template.FormationShape;
	let shiftRows = this.shiftRows;
	let centerGap = this.centerGap;
	let sortingOrder = this.template.SortingOrder;
	let offsets = [];

	// Choose a sensible size/shape for the various formations, depending on number of units.
	let cols;

	if (this.columnar)
	{
		shape = "square";
		cols = Math.min(count, 3);
		shiftRows = false;
		centerGap = 0;
		sortingOrder = null;
	}
	else
	{
		let depth = Math.sqrt(count / this.widthDepthRatio);
		if (this.maxRows && depth > this.maxRows)
			depth = this.maxRows;
		cols = Math.ceil(count / Math.ceil(depth) + (this.shiftRows ? 0.5 : 0));
		if (cols < this.minColumns)
			cols = Math.min(count, this.minColumns);
		if (this.maxColumns && cols > this.maxColumns && this.maxRows != depth)
			cols = this.maxColumns;
	}

	// Define special formations here.
	if (this.template.FormationShape == "special" && Engine.QueryInterface(this.entity, IID_Identity).GetGenericName() == "Scatter")
	{
		let width = Math.sqrt(count) * (separation.width + separation.depth) * 2.5;

		for (let i = 0; i < count; ++i)
		{
			let obj = new Vector2D(randFloat(0, width), randFloat(0, width));
			obj.row = 1;
			obj.column = i + 1;
			offsets.push(obj);
		}
	}

	// For non-special formations, calculate the positions based on the number of entities.
	this.maxColumnsUsed = [];
	this.maxRowsUsed = 0;
	if (shape != "special")
	{
		offsets = [];
		let r = 0;
		let left = count;
		// While there are units left, start a new row in the formation.
		while (left > 0)
		{
			// Save the position of the row.
			let z = -r * separation.depth;
			// Alternate between the left and right side of the center to have a symmetrical distribution.
			let side = 1;
			let n;
			// Determine the number of entities in this row of the formation.
			if (shape == "square")
			{
				n = cols;
				if (shiftRows)
					n -= r % 2;
			}
			else if (shape == "triangle")
			{
				if (shiftRows)
					n = r + 1;
				else
					n = r * 2 + 1;
			}
			if (!shiftRows && n > left)
				n = left;
			for (let c = 0; c < n && left > 0; ++c)
			{
				// Switch sides for the next entity.
				side *= -1;
				let x;
				if (n % 2 == 0)
					x = side * (Math.floor(c / 2) + 0.5) * separation.width;
				else
					x = side * Math.ceil(c / 2) * separation.width;
				if (centerGap)
				{
					// Don't use the center position with a center gap.
					if (x == 0)
						continue;
					x += side * centerGap / 2;
				}
				let column = Math.ceil(n / 2) + Math.ceil(c / 2) * side;
				let r1 = randFloat(-1, 1) * this.sloppiness;
				let r2 = randFloat(-1, 1) * this.sloppiness;

				offsets.push(new Vector2D(x + r1, z + r2));
				offsets[offsets.length - 1].row = r + 1;
				offsets[offsets.length - 1].column = column;
				left--;
			}
			++r;
			this.maxColumnsUsed[r] = n;
		}
		this.maxRowsUsed = r;
	}

	// Make sure the average offset is zero, as the formation is centered around that
	// calculating offset distances without a zero average makes no sense, as the formation
	// will jump to a different position any time.
	let avgoffset = Vector2D.average(offsets);
	offsets.forEach(function(o) {o.sub(avgoffset);});

	// Sort the available places in certain ways.
	// The places first in the list will contain the heaviest units as defined by the order
	// of the types list.
	if (sortingOrder == "fillFromTheSides")
		offsets.sort(function(o1, o2) { return Math.abs(o1.x) < Math.abs(o2.x);});
	else if (sortingOrder == "fillToTheCenter")
		offsets.sort(function(o1, o2) {
			return Math.max(Math.abs(o1.x), Math.abs(o1.y)) < Math.max(Math.abs(o2.x), Math.abs(o2.y));
		});

	// Query the 2D position of the formation.
	const realPositions = this.GetRealOffsetPositions(offsets);

	// Use realistic place assignment,
	// every soldier searches the closest available place in the formation.
	let newOffsets = [];
	for (const i of sortingClasses.reverse())
	{
		const t = types[i];
		if (!t.length)
			continue;
		let usedOffsets = offsets.splice(-t.length);
		let usedRealPositions = realPositions.splice(-t.length);
		for (let entPos of t)
		{
			let closestOffsetId = this.TakeClosestOffset(entPos, usedRealPositions, usedOffsets);
			usedRealPositions.splice(closestOffsetId, 1);
			newOffsets.push(usedOffsets.splice(closestOffsetId, 1)[0]);
			newOffsets[newOffsets.length - 1].ent = entPos.ent;
		}
	}

	return newOffsets;
};

/**
 * Search the closest position in the realPositions list to the given entity.
 * @param entPos - Object with entity position and entity ID.
 * @param realPositions - The world coordinates of the available offsets.
 * @param offsets
 * @return The index of the closest offset position.
 */
Formation.prototype.TakeClosestOffset = function(entPos, realPositions, offsets)
{
	let pos = entPos.pos;
	let closestOffsetId = -1;
	let offsetDistanceSq = Infinity;
	for (let i = 0; i < realPositions.length; ++i)
	{
		let distSq = pos.distanceToSquared(realPositions[i]);
		if (distSq < offsetDistanceSq)
		{
			offsetDistanceSq = distSq;
			closestOffsetId = i;
		}
	}
	this.memberPositions[entPos.ent] = { "row": offsets[closestOffsetId].row, "column": offsets[closestOffsetId].column };
	return closestOffsetId;
};

/**
 * Get the world positions for a list of offsets in this formation.
 */
Formation.prototype.GetRealOffsetPositions = function(offsets)
{
	const cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	const pos = cmpPosition.GetPosition2D();
	const rot = cmpPosition.GetRotation().y;
	const sin = Math.sin(rot);
	const cos = Math.cos(rot);
	let offsetPositions = [];
	// Calculate the world positions.
	for (let o of offsets)
		offsetPositions.push(new Vector2D(pos.x + o.y * sin + o.x * cos, pos.y + o.y * cos - o.x * sin));

	return offsetPositions;
};

/**
 * Returns true if the difference between two given angles (in radians)
 * are smaller than the maximum turning angle of the formation and therfore allow
 * the formation turn without reassigning positions.
 */

Formation.prototype.DoesAngleDifferenceAllowTurning = function(a1, a2)
{
	const d = Math.abs(a1 - a2) % (2 * Math.PI);
	return d < this.maxTurningAngle || d > 2 * Math.PI - this.maxTurningAngle;
};

/**
 * Set formation controller's speed based on its current members.
 */
Formation.prototype.ComputeMotionParameters = function()
{
	if (!this.members.length)
		return;

	let minSpeed = Infinity;
	let minAcceleration = Infinity;
	let maxClearance = 0;
	let maxPassClass = "default";

	const cmpPathfinder = Engine.QueryInterface(SYSTEM_ENTITY, IID_Pathfinder);
	for (let ent of this.members)
	{
		const cmpUnitMotion = Engine.QueryInterface(ent, IID_UnitMotion);
		if (!cmpUnitMotion)
			continue;
		minSpeed = Math.min(minSpeed, cmpUnitMotion.GetWalkSpeed());
		minAcceleration = Math.min(minAcceleration, cmpUnitMotion.GetAcceleration());

		const passClass = cmpUnitMotion.GetPassabilityClassName();
		const clearance = cmpPathfinder.GetClearance(cmpPathfinder.GetPassabilityClass(passClass));
		if (clearance > maxClearance)
		{
			maxClearance = clearance;
			maxPassClass = passClass;
		}
	}
	minSpeed *= this.GetSpeedMultiplier();

	const cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpUnitMotion.SetSpeedMultiplier(minSpeed / cmpUnitMotion.GetWalkSpeed());
	cmpUnitMotion.SetAcceleration(minAcceleration);
	cmpUnitMotion.SetPassabilityClassName(maxPassClass);
};

Formation.prototype.ShapeUpdate = function()
{
	if (!this.rearrange)
		return;

	// Check the distance to twin formations, and merge if
	// the formations could collide.
	for (let i = this.twinFormations.length - 1; i >= 0; --i)
	{
		// Only do the check on one side.
		if (this.twinFormations[i] <= this.entity)
			continue;
		let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		let cmpOtherPosition = Engine.QueryInterface(this.twinFormations[i], IID_Position);
		let cmpOtherFormation = Engine.QueryInterface(this.twinFormations[i], IID_Formation);
		if (!cmpPosition || !cmpOtherPosition || !cmpOtherFormation ||
		     !cmpPosition.IsInWorld() || !cmpOtherPosition.IsInWorld())
			continue;

		let thisPosition = cmpPosition.GetPosition2D();
		let otherPosition = cmpOtherPosition.GetPosition2D();

		let dx = thisPosition.x - otherPosition.x;
		let dy = thisPosition.y - otherPosition.y;
		let dist = Math.sqrt(dx * dx + dy * dy);

		let thisSize = this.GetSize();
		let otherSize = cmpOtherFormation.GetSize();
		let minDist = Math.max(thisSize.width / 2, thisSize.depth / 2) +
			Math.max(otherSize.width / 2, otherSize.depth / 2) +
			this.formationSeparation;

		if (minDist < dist)
			continue;

		// Merge the members from the twin formation into this one
		// twin formations should always have exactly the same orders.
		const otherMembers = cmpOtherFormation.members;
		cmpOtherFormation.RemoveMembers(otherMembers);
		this.AddMembers(otherMembers);
	}
	// Switch between column and box if necessary.
	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	let walkingDistance = cmpUnitAI.ComputeWalkingDistance();
	let columnar = walkingDistance > g_ColumnDistanceThreshold;
	if (columnar != this.columnar)
	{
		this.offsets = undefined;
		this.columnar = columnar;
		// Disable moveCenter so we can't get stuck in a loop of switching
		// shape causing center to change causing shape to switch back.
		this.MoveMembersIntoFormation(false, true, this.lastOrderVariant);
	}
};

Formation.prototype.ResetOrderVariant = function()
{
	this.lastOrderVariant = undefined;
};

Formation.prototype.OnGlobalOwnershipChanged = function(msg)
{
	// When an entity is captured or destroyed, it should no longer be
	// controlled by this formation.
	if (this.members.indexOf(msg.entity) != -1)
		this.RemoveMembers([msg.entity]);
	if (msg.entity === this.entity && msg.to !== INVALID_PLAYER)
		Engine.QueryInterface(this.entity, IID_Visual)?.SetVariant("animationVariant", QueryPlayerIDInterface(msg.to, IID_Identity).GetCiv());
};

Formation.prototype.OnGlobalEntityRenamed = function(msg)
{
	if (this.members.indexOf(msg.entity) === -1)
		return;

	if (this.finishedEntities.delete(msg.entity))
		this.finishedEntities.add(msg.newentity);

	// Save rearranging to temporarily set it to false.
	let temp = this.rearrange;
	this.rearrange = false;

	// First remove the old member to be able to reuse its position.
	this.RemoveMembers([msg.entity], true);
	this.AddMembers([msg.newentity]);
	this.memberPositions[msg.newentity] = this.memberPositions[msg.entity];

	this.rearrange = temp;
};

Formation.prototype.RegisterTwinFormation = function(entity)
{
	let cmpFormation = Engine.QueryInterface(entity, IID_Formation);
	if (!cmpFormation)
		return;
	this.twinFormations.push(entity);
	cmpFormation.twinFormations.push(this.entity);
};

Formation.prototype.DeleteTwinFormations = function()
{
	for (let ent of this.twinFormations)
	{
		let cmpFormation = Engine.QueryInterface(ent, IID_Formation);
		if (cmpFormation)
			cmpFormation.twinFormations.splice(cmpFormation.twinFormations.indexOf(this.entity), 1);
	}
	this.twinFormations = [];
};

Formation.prototype.LoadFormation = function(newTemplate)
{
	const newFormation = ChangeEntityTemplate(this.entity, newTemplate);
	return Engine.QueryInterface(newFormation, IID_UnitAI);
};


Formation.prototype.OnEntityRenamed = function(msg)
{
	const members = clone(this.members);
	this.Disband();
	Engine.QueryInterface(msg.newentity, IID_Formation).SetMembers(members);
};

Engine.RegisterComponentType(IID_Formation, "Formation", Formation);
