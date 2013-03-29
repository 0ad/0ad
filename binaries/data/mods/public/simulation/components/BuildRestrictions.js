function BuildRestrictions() {}

BuildRestrictions.prototype.Schema =
	"<a:help>Specifies building placement restrictions as they relate to terrain, territories, and distance.</a:help>" +
	"<a:example>" +
		"<BuildRestrictions>" +
			"<PlacementType>land</PlacementType>" +
			"<Territory>own</Territory>" +
			"<Category>Special</Category>" +
			"<Distance>" +
				"<FromCategory>CivilCentre</FromCategory>" +
				"<MaxDistance>40</MaxDistance>" +
			"</Distance>" +
		"</BuildRestrictions>" +
	"</a:example>" +
	"<element name='PlacementType' a:help='Specifies the terrain type restriction for this building.'>" +
		"<choice>" +
			"<value>land</value>" +
			"<value>shore</value>" +
		"</choice>" +
	"</element>" +
	"<element name='Territory' a:help='Specifies territory type restrictions for this building.'>" +
		"<list>" +
			"<oneOrMore>" +
				"<choice>" +
					"<value>own</value>" +
					"<value>ally</value>" +
					"<value>neutral</value>" +
					"<value>enemy</value>" +
				"</choice>" +
			"</oneOrMore>" +
		"</list>" +
	"</element>" +
	"<element name='Category' a:help='Specifies the category of this building, for satisfying special constraints.'>" +
		"<choice>" +
			"<value>CivilCentre</value>" +
			"<value>House</value>" +
			"<value>DefenseTower</value>" +
			"<value>Farmstead</value>" +
			"<value>Market</value>" +
			"<value>Barracks</value>" +
			"<value>Dock</value>" +
			"<value>Fortress</value>" +
			"<value>Field</value>" +
			"<value>Temple</value>" +
			"<value>Wall</value>" +
			"<value>Fence</value>" +
			"<value>Mill</value>" +
			"<value>Stoa</value>" +
			"<value>Resource</value>" +
			"<value>Special</value>" +
		"</choice>" +
	"</element>" +
	"<optional>" +
		"<element name='Distance' a:help='Specifies distance restrictions on this building, relative to buildings from the given category.'>" +
			"<interleave>" +
				"<element name='FromCategory'>" +
					"<choice>" +
						"<value>CivilCentre</value>" +
					"</choice>" +
				"</element>" +
				"<optional><element name='MinDistance'><data type='positiveInteger'/></element></optional>" +
				"<optional><element name='MaxDistance'><data type='positiveInteger'/></element></optional>" +
			"</interleave>" +
		"</element>" +
	"</optional>";

BuildRestrictions.prototype.Init = function()
{
	this.territories = this.template.Territory.split(/\s+/);
};

/**
 * Checks whether building placement is valid
 *	1. Visibility is not hidden (may be fogged or visible)
 *	2. Check foundation
 *		a. Doesn't obstruct foundation-blocking entities
 *		b. On valid terrain, based on passability class
 *	3. Territory type is allowed
 *	4. Dock is on shoreline and facing into water
 *	5. Distance constraints satisfied
 *
 * Returns result object:
 * 	{
 *		"success":	true iff the placement is valid, else false
 *		"message":	message to display in UI for invalid placement, else empty string
 *  }
 */
BuildRestrictions.prototype.CheckPlacement = function()
{
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	var name = cmpIdentity ? cmpIdentity.GetGenericName() : "Building";

	var result = {
		"success": false,
		"message": name+" cannot be built due to unknown error",
	};

	// TODO: AI has no visibility info
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	if (!cmpPlayer.IsAI())
	{
		// Check whether it's in a visible or fogged region
		// tell GetLosVisibility to force RetainInFog because preview entities set this to false,
		// which would show them as hidden instead of fogged
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		if (!cmpRangeManager || !cmpOwnership)
			return result; // Fail

		var explored = (cmpRangeManager.GetLosVisibility(this.entity, cmpOwnership.GetOwner(), true) != "hidden");
		if (!explored)
		{
			result.message = name+" cannot be built in unexplored area";
			return result; // Fail
		}
	}

	// Check obstructions and terrain passability
	var passClassName = "";
	switch (this.template.PlacementType)
	{
	case "shore":
		passClassName = "building-shore";
		break;
		
	case "land":
	default:
		passClassName = "building-land";
	}

	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return result; // Fail

	var ret = cmpObstruction.CheckFoundation(passClassName);
	if (ret != "success")
	{
		switch (ret)
		{
		case "fail_error":
		case "fail_no_obstruction":
			error("CheckPlacement: Error returned from CheckFoundation");
			break;
		case "fail_obstructs_foundation":
			result.message = name+" cannot be built on another building or resource";
			break;
		case "fail_terrain_class":
			// TODO: be more specific and/or list valid terrain?
			result.message = name+" cannot be built on invalid terrain";
		}
		return result; // Fail
	}

	// Check territory restrictions
	var cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!(cmpTerritoryManager && cmpPlayer && cmpPosition && cmpPosition.IsInWorld()))
		return result;	// Fail

	var pos = cmpPosition.GetPosition2D();
	var tileOwner = cmpTerritoryManager.GetOwner(pos.x, pos.y);
	var isOwn = (tileOwner == cmpPlayer.GetPlayerID());
	var isNeutral = (tileOwner == 0);
	var isAlly = !isOwn && cmpPlayer.IsAlly(tileOwner);
	// We count neutral players as enemies, so you can't build in their territory.
	var isEnemy = !isNeutral && (cmpPlayer.IsEnemy(tileOwner) || cmpPlayer.IsNeutral(tileOwner));

	var territoryFail = true;
	var territoryType = "";
	if (isAlly && !this.HasTerritory("ally"))
		territoryType = "ally";
	else if (isOwn && !this.HasTerritory("own"))
		territoryType = "own";
	else if (isNeutral && !this.HasTerritory("neutral"))
		territoryType = "neutral";
	else if (isEnemy && !this.HasTerritory("enemy"))
		territoryType = "enemy";
	else
		territoryFail = false

	if (territoryFail)
	{
		result.message = name+" cannot be built in "+territoryType+" territory. Valid territories: " + this.GetTerritories().join(", ");
		return result;	// Fail
	}

	// Check special requirements
	if (this.template.Category == "Dock")
	{
		// TODO: Probably should check unit passability classes here, to determine if:
		//		1. ships can be spawned "nearby"
		//		2. builders can pass the terrain where the dock is placed (don't worry about paths)
		//	so it's correct even if the criteria changes for these units
		var cmpFootprint = Engine.QueryInterface(this.entity, IID_Footprint);
		if (!cmpFootprint)
			return result;	// Fail

		// Get building's footprint
		var shape = cmpFootprint.GetShape();
		var halfSize = 0;
		if (shape.type == "square")
			halfSize = shape.depth/2;
		else if (shape.type == "circle")
			halfSize = shape.radius;

		var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
		var cmpWaterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
		if (!cmpTerrain || !cmpWaterManager)
			return result;	// Fail

		var ang = cmpPosition.GetRotation().y;
		var sz = halfSize * Math.sin(ang);
		var cz = halfSize * Math.cos(ang);
		if ((cmpWaterManager.GetWaterLevel(pos.x + sz, pos.y + cz) - cmpTerrain.GetGroundLevel(pos.x + sz, pos.y + cz)) < 1.0 // front
			|| (cmpWaterManager.GetWaterLevel(pos.x - sz, pos.y - cz) - cmpTerrain.GetGroundLevel(pos.x - sz, pos.y - cz)) > 2.0) // back
		{
			result.message = name+" must be built on a valid shoreline";
			return result;	// Fail
		}
	}

	// Check distance restriction
	if (this.template.Distance)
	{
		var nearest = 65535;
		var ents = Engine.GetEntitiesWithInterface(IID_BuildRestrictions);
		for each (var ent in ents)
		{
			// Ignore ourself
			if (ent == this.entity)
				continue;

			var cmpBuildRestrictions = Engine.QueryInterface(ent, IID_BuildRestrictions);
			if (cmpBuildRestrictions.GetCategory() == this.template.Distance.FromCategory && IsOwnedByPlayer(cmpPlayer.GetPlayerID(), ent))
			{	// Find nearest building matching this category
				var cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
				if (cmpEntPosition && cmpEntPosition.IsInWorld())
				{
					var entPos = cmpEntPosition.GetPosition2D();
					var dist = Math.sqrt((pos.x-entPos.x)*(pos.x-entPos.x) + (pos.y-entPos.y)*(pos.y-entPos.y));
					if (dist < nearest)
					{
						nearest = dist;
					}
				}
			}
		}

		if (this.template.Distance.MinDistance && nearest < +this.template.Distance.MinDistance)
		{
			result.message = name+" too close to a "+this.GetCategory()+", must be at least "+ +this.template.Distance.MinDistance+" units away";
			return result;	// Fail
		}
		else if (this.template.Distance.MaxDistance && nearest > +this.template.Distance.MaxDistance)
		{
			result.message = name+" too far away from a "+this.GetCategory()+", must be within "+ +this.template.Distance.MaxDistance+" units";
			return result;	// Fail
		}
	}

	// Success
	result.success = true;
	result.message = "";
	return result;
};

BuildRestrictions.prototype.GetCategory = function()
{
	return this.template.Category;
};

BuildRestrictions.prototype.GetTerritories = function()
{
	return ApplyTechModificationsToEntity("BuildRestrictions/Territory", this.territories, this.entity);
};

BuildRestrictions.prototype.HasTerritory = function(territory)
{
	return (this.GetTerritories().indexOf(territory) != -1);
};

Engine.RegisterComponentType(IID_BuildRestrictions, "BuildRestrictions", BuildRestrictions);
