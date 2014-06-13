function Formation() {}

Formation.prototype.Schema =
	"<element name='FormationName' a:help='Name of the formation'>" +
		"<text/>" +
	"</element>" +
	"<element name='Icon'>" +
		"<text/>" +
	"</element>" +
	"<element name='RequiredMemberCount' a:help='Minimum number of entities the formation should contain'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<element name='DisabledTooltip' a:help='Tooltip shown when the formation is disabled'>" +
		"<text/>" +
	"</element>" +
	"<element name='SpeedMultiplier' a:help='The speed of the formation is determined by the minimum speed of all members, multiplied with this number.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='FormationShape' a:help='Formation shape, currently supported are square, triangle and special, where special will be defined in the source code.'>" +
		"<text/>" +
	"</element>" +
	"<element name='ShiftRows' a:help='Set the value to true to shift subsequent rows'>" +
		"<text/>" +
	"</element>" +
	"<element name='SortingClasses' a:help='Classes will be added to the formation in this order. Where the classes will be added first depends on the formation'>" +
		"<text/>" +
	"</element>" +
	"<optional>" +
		"<element name='SortingOrder' a:help='The order of sorting. This defaults to an order where the formation is filled from the first row to the last, and the center of each row to the sides. Other possible sort orders are \"fillFromTheSides\", where the most important units are on the sides of each row, and \"fillToTheCenter\", where the most vulerable units are right in the center of the formation. '>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<element name='WidthDepthRatio' a:help='Average width/depth, counted in number of units.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Sloppyness' a:help='Sloppyness in meters (the max difference between the actual and the perfectly aligned formation position'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='MinColumns' a:help='When possible, this number of colums will be created. Overriding the wanted width depth ratio'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='MaxColumns' a:help='When possible within the number of units, and the maximum number of rows, this will be the maximum number of columns.'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='MaxRows' a:help='The maximum number of rows in the formation'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='CenterGap' a:help='The size of the central gap, expressed in number of units wide'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<element name='UnitSeparationWidthMultiplier' a:help='Place the units in the formation closer or further to each other. The standard separation is the footprint size.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='UnitSeparationDepthMultiplier' a:help='Place the units in the formation closer or further to each other. The standard separation is the footprint size.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Animations' a:help='Give a list of animations to use for the particular formation members, based on their positions'>" +
		"<zeroOrMore>" +
			"<element a:help='The name of the default animation (walk, idle, attack_ranged...) that will be transformed in the formation-specific ResetMoveAnimation'>" +
				"<anyName/>" +
				"<text a:help='example text: \"1..1,1..-1:animation1;2..2,1..-1;animation2\", this will set animation1 for the first row, and animation2 for the second row. The first part of the numbers (1..1 and 2..2) means the row range. Every row between (and including) those values will switch animations. The second part of the numbers (1..-1) denote the columns inside those rows that will be affected. Note that in both cases, you can use -1 for the last row/column, -2 for the second to last, etc.'/>" +
			"</element>" +
		"</zeroOrMore>" +
	"</element>";	

var g_ColumnDistanceThreshold = 128; // distance at which we'll switch between column/box formations

Formation.prototype.Init = function()
{
	this.formationShape = this.template.FormationShape;
	this.sortingClasses = this.template.SortingClasses.split(/\s+/g);
	this.sortingOrder = this.template.SortingOrder;
	this.shiftRows = this.template.ShiftRows == "true";
	this.separationMultiplier = {
		"width": +this.template.UnitSeparationWidthMultiplier,
		"depth": +this.template.UnitSeparationDepthMultiplier
	};
	this.sloppyness = +this.template.Sloppyness;
	this.widthDepthRatio = +this.template.WidthDepthRatio;
	this.minColumns = +(this.template.MinColumns || 0);
	this.maxColumns = +(this.template.MaxColumns || 0);
	this.maxRows = +(this.template.MaxRows || 0);
	this.centerGap = +(this.template.CenterGap || 0);

	var animations = this.template.Animations;
	this.animations = {}
	for (var animationName in animations)
	{
		var differentAnimations = animations[animationName].split(/\s*;\s*/);
		this.animations[animationName] = [];
		// loop over the different rectangulars that will map to different animations
		for each (var rectAnimation in differentAnimations)
		{
			var rect, replacementAnimationName;
			[rect, replacementAnimationName] = rectAnimation.split(/\s*:\s*/);
			var rows, columns;
			[rows, columns] = rect.split(/\s*,\s*/);
			var minRow, maxRow, minColumn, maxColumn;
			[minRow, maxRow] = rows.split(/\s*\.\.\s*/);
			[minColumn, maxColumn] = columns.split(/\s*\.\.\s*/);
			this.animations[animationName].push({
				"minRow": +minRow,
				"maxRow": +maxRow,
				"minColumn": +minColumn,
				"maxColumn": +maxColumn,
				"animation": replacementAnimationName
			});
		}
	}

	this.members = []; // entity IDs currently belonging to this formation
	this.memberPositions = {};
	this.maxRowsUsed = 0;
	this.maxColumnsUsed = [];
	this.inPosition = []; // entities that have reached their final position
	this.columnar = false; // whether we're travelling in column (vs box) formation
	this.rearrange = true; // whether we should rearrange all formation members
	this.formationMembersWithAura = []; // Members with a formation aura
	this.width = 0;
	this.depth = 0;
	this.oldOrientation = {"sin": 0, "cos": 0};
	this.twinFormations = [];
	// distance from which two twin formations will merge into one.
	this.formationSeparation = 0;
	Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer)
		.SetInterval(this.entity, IID_Formation, "ShapeUpdate", 1000, 1000, null);
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
	return {"width": this.width, "depth": this.depth};
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
	var cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
	if (!cmpEntPosition)
		return INVALID_ENTITY;

	var entPosition = cmpEntPosition.GetPosition2D();
	var closestMember = INVALID_ENTITY;
	var closestDistance = Infinity;
	for each (var member in this.members)
	{
		if (filter && !filter(ent))
			continue;

		var cmpPosition = Engine.QueryInterface(member, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		var pos = cmpPosition.GetPosition2D();
		var dist = entPosition.distanceToSquared(pos);
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
 * TODO: actually implement something like that; currently this just returns
 * the arbitrary first one.
 */
Formation.prototype.GetPrimaryMember = function()
{
	return this.members[0];
};

/**
 * Get the formation animation for a certain member of this formation
 * @param entity The entity ID to get the animation for
 * @param defaultAnimation The name of the default wanted animation for the entity
 * E.g. "walk", "idle" ...
 * @return The name of the transformed animation as defined in the template
 * E.g. "walk_testudo_row1"
 */
Formation.prototype.GetFormationAnimation = function(entity, defaultAnimation)
{
	var animationGroup = this.animations[defaultAnimation];
	if (!animationGroup || this.columnar)
		return defaultAnimation;
	var row = this.memberPositions[entity].row;
	var column = this.memberPositions[entity].column;
	for (var i = 0; i < animationGroup.length; ++i)
	{
		var minRow = animationGroup[i].minRow;
		if (minRow < 0)
			minRow += this.maxRowsUsed + 1;
		if (row < minRow)
			continue;

		var maxRow = animationGroup[i].maxRow;
		if (maxRow < 0)
			maxRow += this.maxRowsUsed + 1;
		if (row > maxRow)
			continue;

		var minColumn = animationGroup[i].minColumn;
		if (minColumn < 0)
			minColumn += this.maxColumnsUsed[row] + 1;
		if (column < minColumn)
			continue;

		var maxColumn = animationGroup[i].maxColumn;
		if (maxColumn < 0)
			maxColumn += this.maxColumnsUsed[row] + 1;
		if (column > maxColumn)
			continue;

		return animationGroup[i].animation;
	}
	return defaultAnimation;
};

/**
 * Permits formation members to register that they've reached their destination.
 */
Formation.prototype.SetInPosition = function(ent)
{
	if (this.inPosition.indexOf(ent) != -1)
		return;

	// Rotate the entity to the right angle
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
	if (cmpEntPosition && cmpEntPosition.IsInWorld() && cmpPosition && cmpPosition.IsInWorld())
		cmpEntPosition.TurnTo(cmpPosition.GetRotation().y);

	this.inPosition.push(ent);
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

	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var templateName = cmpTemplateManager.GetCurrentTemplateName(this.entity);
	// keep the number of entities per pass class to find the most used
	// For land units, this will be "default", for ship units, it should be "ship"
	var passClasses = {};
	var bestPassClassNumber = 0;
	var bestPassClass = "default";

	for each (var ent in this.members)
	{
		var cmpUnitMotion = Engine.QueryInterface(ent,IID_UnitMotion);
		var passClass = cmpUnitMotion.GetPassabilityClassName();
		if (passClasses[passClass])
			var number = passClasses[passClass]++;
		else
			var number = passClasses[passClass] = 1;
		if (number > bestPassClassNumber)
		{
			bestPassClass = passClass;
			bestPassClassNumber = number;
		}

		var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		cmpUnitAI.SetFormationController(this.entity);
		cmpUnitAI.SetLastFormationTemplate(templateName);
		
		var cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		if (cmpAuras && cmpAuras.HasFormationAura())
		{
			this.formationMembersWithAura.push(ent);
			cmpAuras.ApplyFormationBonus(ents);
		}
	}
	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpUnitMotion.SetPassabilityClassName(bestPassClass);

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
	this.offsets = undefined;
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

	this.ComputeMotionParameters();

	// Rearrange the remaining members
	this.MoveMembersIntoFormation(true, true);
};

Formation.prototype.AddMembers = function(ents)
{
	this.offsets = undefined;
	this.inPosition = []; 

	for each (var ent in this.formationMembersWithAura)
	{
		var cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		cmpAuras.RemoveFormationBonus(ents);

		// the unit with the aura is also removed from the formation
		if (ents.indexOf(ent) !== -1)
			cmpAuras.RemoveFormationBonus(this.members);
	}

	this.members = this.members.concat(ents);

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
 * Set all members to form up into the formation shape.
 * If moveCenter is true, the formation center will be reinitialised
 * to the center of the units.
 * If force is true, all individual orders of the formation units are replaced,
 * otherwise the order to walk into formation is just pushed to the front.
 */
Formation.prototype.MoveMembersIntoFormation = function(moveCenter, force)
{
	if (!this.members.length)
		return;

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
		positions.push(pos);
	}

	var avgpos = Vector2D.avg(positions);

	// Reposition the formation if we're told to or if we don't already have a position
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var inWorld = cmpPosition.IsInWorld();
	if (moveCenter || !inWorld)
	{
		cmpPosition.JumpTo(avgpos.x, avgpos.y);
		// Don't make the formation controller entity show up in range queries
		if (!inWorld)
		{
			var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
			cmpRangeManager.SetEntityFlag(this.entity, "normal", false);
		}
	}

	// Switch between column and box if necessary
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var walkingDistance = cmpUnitAI.ComputeWalkingDistance();
	var columnar = walkingDistance > g_ColumnDistanceThreshold;
	if (columnar != this.columnar)
	{
		this.columnar = columnar;
		this.offsets = undefined;
	}

	var newOrientation = this.GetEstimatedOrientation(avgpos);
	var dSin = Math.abs(newOrientation.sin - this.oldOrientation.sin);
	var dCos = Math.abs(newOrientation.cos - this.oldOrientation.cos);
	// If the formation existed, only recalculate positions if the turning agle is somewhat biggish
	if (!this.offsets || dSin > 1 || dCos > 1)
		this.offsets = this.ComputeFormationOffsets(active, positions);

	this.oldOrientation = newOrientation;

	var xMax = 0;
	var yMax = 0;
	var xMin = 0;
	var yMin = 0;

	for (var i = 0; i < this.offsets.length; ++i)
	{
		var offset = this.offsets[i];

		var cmpUnitAI = Engine.QueryInterface(offset.ent, IID_UnitAI);
		if (!cmpUnitAI)
			continue;
		
		var data = 
		{
			"target": this.entity,
			"x": offset.x,
			"z": offset.y
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
	var positions = [];

	for each (var ent in this.members)
	{
		var cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			continue;

		positions.push(cmpPosition.GetPosition2D());
	}

	var avgpos = Vector2D.avg(positions);

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var inWorld = cmpPosition.IsInWorld();
	cmpPosition.JumpTo(avgpos.x, avgpos.y);

	// Don't make the formation controller show up in range queries
	if (!inWorld)
	{
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		cmpRangeManager.SetEntityFlag(this.entity, "normal", false);
	}
};

Formation.prototype.GetAvgFootprint = function(active)
{
	var footprints = [];
	for each (var ent in active)
	{
		var cmpFootprint = Engine.QueryInterface(ent, IID_Footprint);
		if (cmpFootprint)
			footprints.push(cmpFootprint.GetShape());
	}
	if (!footprints.length)
		return {"width":1, "depth": 1};

	var r = {"width": 0, "depth": 0};
	for each (var shape in footprints)
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
	var separation = this.GetAvgFootprint(active);
	separation.width *= this.separationMultiplier.width;
	separation.depth *= this.separationMultiplier.depth;

	if (this.columnar)
		var sortingClasses = ["Cavalry","Infantry"];
	else
		var sortingClasses = this.sortingClasses.slice();
	sortingClasses.push("Unknown");

	// the entities will be assigned to positions in the formation in 
	// the same order as the types list is ordered
	var types = {}; 
	for (var i = 0; i < sortingClasses.length; ++i)
		types[sortingClasses[i]] = [];

	for (var i in active)
	{
		var cmpIdentity = Engine.QueryInterface(active[i], IID_Identity);
		var classes = cmpIdentity.GetClassesList();
		var done = false;
		for (var c = 0; c < sortingClasses.length; ++c)
		{
			if (classes.indexOf(sortingClasses[c]) > -1)
			{
				types[sortingClasses[c]].push({"ent": active[i], "pos": positions[i]});
				done = true;
				break;
			}
		}
		if (!done)
			types["Unknown"].push({"ent": active[i], "pos": positions[i]});
	}

	var count = active.length;

	var shape = this.formationShape;
	var shiftRows = this.shiftRows;
	var centerGap = this.centerGap;
	var sortingOrder = this.sortingOrder;

	var offsets = [];

	// Choose a sensible size/shape for the various formations, depending on number of units
	var cols;

	if (this.columnar)
	{
		shape = "square";
		cols = Math.min(count,3);
		shiftRows = false;
		centerGap = 0;
		sortingOrder = null;
	}
	else
	{
		var depth = Math.sqrt(count / this.widthDepthRatio);
		if (this.maxRows && depth > this.maxRows)
			depth = this.maxRows;
		cols = Math.ceil(count / Math.ceil(depth) + (this.shiftRows ? 0.5 : 0));
		if (cols < this.minColumns)
			cols = Math.min(count, this.minColumns);
		if (this.maxColumns && cols > this.maxColumns && this.maxRows != depth)
			cols = this.maxColumns;
	}

	// define special formations here
	if (this.template.FormationName == "Scatter")
	{
		var width = Math.sqrt(count) * (separation.width + separation.depth) * 2.5;

		for (var i = 0; i < count; ++i)
		{
			var obj = new Vector2D(Math.random()*width, Math.random()*width);
			obj.row = 1;
			obj.column = i + 1;
			offsets.push(obj);
		}
	}

	// For non-special formations, calculate the positions based on the number of entities
	this.maxColumnsUsed = [];
	this.maxRowsUsed = 0;
	if (shape != "special")
	{
		offsets = [];
		var r = 0;
		var left = count;
		// while there are units left, start a new row in the formation
		while (left > 0)
		{
			// save the position of the row
			var z = -r * separation.depth;
			// switch between the left and right side of the center to have a symmetrical distribution
			var side = 1;
			// determine the number of entities in this row of the formation
			if (shape == "square")
			{
				var n = cols;
				if (shiftRows)
					n -= r%2;
			}
			else if (shape == "triangle")
			{
				if (shiftRows)
					var n = r + 1;
				else
					var n = r * 2 + 1;
			}
			if (!shiftRows && n > left)
				n = left;
			for (var c = 0; c < n && left > 0; ++c)
			{
				// switch sides for the next entity
				side *= -1;
				if (n%2 == 0)
					var x = side * (Math.floor(c/2) + 0.5) * separation.width;
				else
					var x = side * Math.ceil(c/2) * separation.width;
				if (centerGap)
				{
					if (x == 0) // don't use the center position with a center gap
						continue;
					x += side * centerGap / 2;
				}
				var column = Math.ceil(n/2) + Math.ceil(c/2) * side;
				var r1 = 0;
				var r2 = 0;
				if (this.sloppyness != 0)
				{
					r1 = (Math.random() * 2 - 1) * this.sloppyness;
					r2 = (Math.random() * 2 - 1) * this.sloppyness;
				}
				offsets.push(new Vector2D(x + r1, z + r2));
				offsets[offsets.length - 1].row = r+1;
				offsets[offsets.length - 1].column = column;
				left--
			}
			++r;
			this.maxColumnsUsed[r] = n;
		}
		this.maxRowsUsed = r;
	}

	// make sure the average offset is zero, as the formation is centered around that
	// calculating offset distances without a zero average makes no sense, as the formation
	// will jump to a different position any time
	var avgoffset = Vector2D.avg(offsets);
	offsets.forEach(function (o) {o.sub(avgoffset);});

	// sort the available places in certain ways
	// the places first in the list will contain the heaviest units as defined by the order
	// of the types list
	if (this.sortingOrder == "fillFromTheSides")
		offsets.sort(function(o1, o2) { return Math.abs(o1.x) < Math.abs(o2.x);});
	else if (this.sortingOrder == "fillToTheCenter")
		offsets.sort(function(o1, o2) { 
			return Math.max(Math.abs(o1.x), Math.abs(o1.y)) < Math.max(Math.abs(o2.x), Math.abs(o2.y));
		});

	// query the 2D position of the formation
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var formationPos = cmpPosition.GetPosition2D();

	// use realistic place assignment,
	// every soldier searches the closest available place in the formation
	var newOffsets = [];
	var realPositions = this.GetRealOffsetPositions(offsets, formationPos);
	for (var i = sortingClasses.length; i; --i)
	{
		var t = types[sortingClasses[i-1]];
		if (!t.length)
			continue;
		var usedOffsets = offsets.splice(-t.length);
		var usedRealPositions = realPositions.splice(-t.length);
		for each (var entPos in t)
		{
			var closestOffsetId = this.TakeClosestOffset(entPos, usedRealPositions, usedOffsets);
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
Formation.prototype.TakeClosestOffset = function(entPos, realPositions, offsets)
{
	var pos = entPos.pos;
	var closestOffsetId = -1;
	var offsetDistanceSq = Infinity;
	for (var i = 0; i < realPositions.length; i++)
	{
		var distSq = pos.distanceToSquared(realPositions[i]);
		if (distSq < offsetDistanceSq)
		{
			offsetDistanceSq = distSq;
			closestOffsetId = i;
		}
	}
	this.memberPositions[entPos.ent] = {"row": offsets[closestOffsetId].row, "column":offsets[closestOffsetId].column};
	return closestOffsetId;
};

/**
 * Get the world positions for a list of offsets in this formation
 */
Formation.prototype.GetRealOffsetPositions = function(offsets, pos)
{
	var offsetPositions = [];
	var {sin, cos} = this.GetEstimatedOrientation(pos);
	// calculate the world positions
	for each (var o in offsets)
		offsetPositions.push(new Vector2D(pos.x + o.y * sin + o.x * cos, pos.y + o.y * cos - o.x * sin));

	return offsetPositions;
};

/**
 * calculate the estimated rotation of the formation 
 * based on the first unitAI target position when ordered to walk,
 * based on the current rotation in other cases
 * Return the sine and cosine of the angle
 */
Formation.prototype.GetEstimatedOrientation = function(pos)
{
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var r = {"sin": 0, "cos": 1};
	var unitAIState = cmpUnitAI.GetCurrentState();
	if (unitAIState == "FORMATIONCONTROLLER.WALKING" || unitAIState == "FORMATIONCONTROLLER.COMBAT.APPROACHING")
	{
		var targetPos = cmpUnitAI.GetTargetPositions();
		if (!targetPos.length)
			return r;
		var d = targetPos[0].sub(pos).normalize();
		if (!d.x && !d.y)
			return r;
		r.cos = d.y;
		r.sin = d.x;
	}
	else
	{
		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		if (!cmpPosition)
			return r;
		var rot = cmpPosition.GetRotation().y;
		r.sin = Math.sin(rot);
		r.cos = Math.cos(rot);
	}
	return r;
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
	minSpeed *= this.GetSpeedMultiplier();

	var cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	cmpUnitMotion.SetUnitRadius(maxRadius);
	cmpUnitMotion.SetSpeed(minSpeed);

	// TODO: we also need to do something about PassabilityClass, CostClass
};

Formation.prototype.ShapeUpdate = function()
{
	// Check the distance to twin formations, and merge if when 
	// the formations could collide
	for (var i = this.twinFormations.length - 1; i >= 0; --i)
	{
		// only do the check on one side
		if (this.twinFormations[i] <= this.entity)
			continue;

		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		var cmpOtherPosition = Engine.QueryInterface(this.twinFormations[i], IID_Position);
		var cmpOtherFormation = Engine.QueryInterface(this.twinFormations[i], IID_Formation);
		if (!cmpPosition || !cmpOtherPosition || !cmpOtherFormation)
			continue;

		var thisPosition = cmpPosition.GetPosition2D();
		var otherPosition = cmpOtherPosition.GetPosition2D();
		var dx = thisPosition.x - otherPosition.x;
		var dy = thisPosition.y - otherPosition.y;
		var dist = Math.sqrt(dx * dx + dy * dy);

		var thisSize = this.GetSize();
		var otherSize = cmpOtherFormation.GetSize();
		var minDist = Math.max(thisSize.width / 2, thisSize.depth / 2) +
			Math.max(otherSize.width / 2, otherSize.depth / 2) +
			this.formationSeparation;

		if (minDist < dist)
			continue;

		// merge the members from the twin formation into this one
		// twin formations should always have exactly the same orders
		this.AddMembers(cmpOtherFormation.members);
		Engine.DestroyEntity(this.twinFormations[i]);
		this.twinFormations.splice(i,1);
	}
	// Switch between column and box if necessary
	var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var walkingDistance = cmpUnitAI.ComputeWalkingDistance();
	var columnar = walkingDistance > g_ColumnDistanceThreshold;
	if (columnar != this.columnar)
	{
		this.offsets = undefined;
		this.columnar = columnar;
		this.MoveMembersIntoFormation(false, true);
		// (disable moveCenter so we can't get stuck in a loop of switching
		// shape causing center to change causing shape to switch back)
	}
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
		this.offsets = undefined;
		var cmpNewUnitAI = Engine.QueryInterface(msg.newentity, IID_UnitAI);
		if (cmpNewUnitAI)
		{
			this.members[this.members.indexOf(msg.entity)] = msg.newentity;
			this.memberPositions[msg.newentity] = this.memberPositions[msg.entity];
		}

		var cmpOldUnitAI = Engine.QueryInterface(msg.entity, IID_UnitAI);
		cmpOldUnitAI.SetFormationController(INVALID_ENTITY);

		if (cmpNewUnitAI)
			cmpNewUnitAI.SetFormationController(this.entity);

		// Because the renamed entity might have different characteristics,
		// (e.g. packed vs. unpacked siege), we need to recompute motion parameters
		this.ComputeMotionParameters();
	}
}

Formation.prototype.RegisterTwinFormation = function(entity)
{
	var cmpFormation = Engine.QueryInterface(entity, IID_Formation);
	if (!cmpFormation)
		return;
	this.twinFormations.push(entity);
	cmpFormation.twinFormations.push(this.entity);
};

Formation.prototype.DeleteTwinFormations = function()
{
	for each (var ent in this.twinFormations)
	{
		var cmpFormation = Engine.QueryInterface(ent, IID_Formation);
		if (cmpFormation)
			cmpFormation.twinFormations.splice(cmpFormation.twinFormations.indexOf(this.entity), 1);
	}
	this.twinFormations = [];
};

Formation.prototype.LoadFormation = function(newTemplate)
{
	// get the old formation info
	var members = this.members.slice();
	var cmpThisUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var orders = cmpThisUnitAI.GetOrders().slice();

	this.Disband();

	var newFormation = Engine.AddEntity(newTemplate);
	// apply the info from the old formation to the new one

	var cmpNewPosition = Engine.QueryInterface(newFormation, IID_Position);
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition && cmpPosition.IsInWorld() && cmpNewPosition)
		cmpNewPosition.TurnTo(cmpPosition.GetRotation().y);

	var cmpFormation = Engine.QueryInterface(newFormation, IID_Formation);
	var cmpNewUnitAI = Engine.QueryInterface(newFormation, IID_UnitAI);
	cmpFormation.SetMembers(members);
	if (orders.length)
		cmpNewUnitAI.AddOrders(orders);
	else
		cmpNewUnitAI.MoveIntoFormation();

	Engine.BroadcastMessage(MT_EntityRenamed, {"entity": this.entity, "newentity": newFormation});
};

Engine.RegisterComponentType(IID_Formation, "Formation", Formation);
