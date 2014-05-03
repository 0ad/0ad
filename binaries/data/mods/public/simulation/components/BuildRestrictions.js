function BuildRestrictions() {}

BuildRestrictions.prototype.Schema =
	"<a:help>Specifies building placement restrictions as they relate to terrain, territories, and distance.</a:help>" +
	"<a:example>" +
		"<BuildRestrictions>" +
			"<PlacementType>land</PlacementType>" +
			"<Territory>own</Territory>" +
			"<Category>Special</Category>" +
			"<Distance>" +
				"<FromClass>CivilCentre</FromClass>" +
				"<MaxDistance>40</MaxDistance>" +
			"</Distance>" +
		"</BuildRestrictions>" +
	"</a:example>" +
	"<element name='PlacementType' a:help='Specifies the terrain type restriction for this building.'>" +
		"<choice>" +
			"<value>land</value>" +
			"<value>shore</value>" +
			"<value>land-shore</value>"+
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
	"<element name='Category' a:help='Specifies the category of this building, for satisfying special constraints. Choices include: CivilCentre, House, DefenseTower, Farmstead, Market, Barracks, Dock, Fortress, Field, Temple, Wall, Fence, Storehouse, Stoa, Resource, Special, Wonder, Apadana, Embassy, Monument'>" +
		"<text/>" +
	"</element>" +
	"<optional>" +
		"<element name='Distance' a:help='Specifies distance restrictions on this building, relative to buildings from the given category.'>" +
			"<interleave>" +
				"<element name='FromClass'>" +
					"<text/>" +
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
 *	3. Territory type is allowed (see note below)
 *	4. Dock is on shoreline and facing into water
 *	5. Distance constraints satisfied
 *
 * Returns result object:
 * 	{
 *		"success":             true iff the placement is valid, else false
 *		"message":             message to display in UI for invalid placement, else ""
 *		"parameters":          parameters to use in the GUI message
 *		"translateMessage":    always true
 *		"translateParameters": list of parameters to translate
 *  }
 *
 * Note: The entity which is used to check this should be a preview entity
 *  (template name should be "preview|"+templateName), as otherwise territory
 *  checks for buildings with territory influence will not work as expected.
 */
BuildRestrictions.prototype.CheckPlacement = function()
{
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	var name = cmpIdentity ? cmpIdentity.GetGenericName() : "Building";

	var result = {
		"success": false,
		"message": markForTranslation("%(name)s cannot be built due to unknown error"),
		"parameters": {
			"name": name,
		},
		"translateMessage": true,
		"translateParameters": ["name"],
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
			result.message = markForTranslation("%(name)s cannot be built in unexplored area");
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

	case "land-shore":
		// 'default' is everywhere a normal unit can go
		// So on passable land, and not too deep in the water
		passClassName = "default";
		break;
	
	case "land":
	default:
		passClassName = "building-land";
	}

	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return result; // Fail
	
	
	if (this.template.Category == "Wall")
	{
		// for walls, only test the center point
		var ret = cmpObstruction.CheckFoundation(passClassName, true);
	}
	else
	{
		var ret = cmpObstruction.CheckFoundation(passClassName, false);
	}

	if (ret != "success")
	{
		switch (ret)
		{
		case "fail_error":
		case "fail_no_obstruction":
			error("CheckPlacement: Error returned from CheckFoundation");
			break;
		case "fail_obstructs_foundation":
			result.message = markForTranslation("%(name)s cannot be built on another building or resource");
			break;
		case "fail_terrain_class":
			// TODO: be more specific and/or list valid terrain?
			result.message = markForTranslation("%(name)s cannot be built on invalid terrain");
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
		// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
		territoryType = markForTranslationWithContext("Territory type", "ally");
	else if (isOwn && !this.HasTerritory("own"))
		// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
		territoryType = markForTranslationWithContext("Territory type", "own");
	else if (isNeutral && !this.HasTerritory("neutral"))
		// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
		territoryType = markForTranslationWithContext("Territory type", "neutral");
	else if (isEnemy && !this.HasTerritory("enemy"))
		// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
		territoryType = markForTranslationWithContext("Territory type", "enemy");
	else
		territoryFail = false;

	if (territoryFail)
	{
		result.message = markForTranslation("%(name)s cannot be built in %(territoryType)s territory. Valid territories: %(validTerritories)s");
		result.translateParameters.push("territoryType");
		result.translateParameters.push("validTerritories");
		result.parameters.territoryType = {"context": "Territory type", "message": territoryType};
		// gui code will join this array to a string
		result.parameters.validTerritories = {"context": "Territory type list", "list": this.GetTerritories()};
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
			result.message = markForTranslation("%(name)s must be built on a valid shoreline");
			return result;	// Fail
		}
	}

	// Check distance restriction
	if (this.template.Distance)
	{
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
		var cat = this.template.Distance.FromClass;

		var filter = function(id)
		{
			var cmpIdentity = Engine.QueryInterface(id, IID_Identity);
			return cmpIdentity.GetClassesList().indexOf(cat) > -1;
		}
		
		if (this.template.Distance.MinDistance)
		{
			var dist = +this.template.Distance.MinDistance
			var nearEnts = cmpRangeManager.ExecuteQuery(this.entity, 0, dist, [cmpPlayer.GetPlayerID()], IID_BuildRestrictions).filter(filter);
			if (nearEnts.length)
			{
				result.message = markForTranslation("%(name)s too close to a %(category)s, must be at least %(distance)s meters away");
				result.parameters.category = cat;
				result.translateParameters.push("category");
				result.parameters.distance = this.template.Distance.MinDistance;
				return result;	// Fail
			}
		}
		if (this.template.Distance.MaxDistance)
		{
			var dist = +this.template.Distance.MaxDistance;
			var nearEnts = cmpRangeManager.ExecuteQuery(this.entity, 0, dist, [cmpPlayer.GetPlayerID()], IID_BuildRestrictions).filter(filter);
			if (!nearEnts.length)
			{
				result.message = markForTranslation("%(name)s too far from a %(category)s, must be within %(distance)s meters");
				result.parameters.category = cat;
				result.translateParameters.push("category");
				result.parameters.distance = this.template.Distance.MinDistance;
				return result;	// Fail
			}
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
	return ApplyValueModificationsToEntity("BuildRestrictions/Territory", this.territories, this.entity);
};

BuildRestrictions.prototype.HasTerritory = function(territory)
{
	return (this.GetTerritories().indexOf(territory) != -1);
};

// Translation: Territory types being displayed as part of a list like "Valid territories: own, ally".
markForTranslationWithContext("Territory type list", "own");
// Translation: Territory types being displayed as part of a list like "Valid territories: own, ally".
markForTranslationWithContext("Territory type list", "ally");
// Translation: Territory types being displayed as part of a list like "Valid territories: own, ally".
markForTranslationWithContext("Territory type list", "neutral");
// Translation: Territory types being displayed as part of a list like "Valid territories: own, ally".
markForTranslationWithContext("Territory type list", "enemy");

Engine.RegisterComponentType(IID_BuildRestrictions, "BuildRestrictions", BuildRestrictions);
