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
	"<element name='Category' a:help='Specifies the category of this building, for satisfying special constraints. Choices include: Apadana, ArmyCamp, Barracks, CivilCentre, Colony, Council, DefenseTower, Dock, Embassy, Farmstead, Fence, Field, Fortress, Hall, House, Kennel, Library, Market, Monument, Outpost, Pillar, Resource, Special, Stoa, Storehouse, Temple, Theater, UniqueBuilding, Wall, Wonder'>" +
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
 *		"pluralMessage":       we might return a plural translation instead (optional)
 *		"pluralCount":         plural translation argument (optional)
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
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		if (!cmpRangeManager || !cmpOwnership)
			return result; // Fail

		var explored = (cmpRangeManager.GetLosVisibility(this.entity, cmpOwnership.GetOwner()) != "hidden");
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
		// 'default-terrain-only' is everywhere a normal unit can go, ignoring
		// obstructions (i.e. on passable land, and not too deep in the water)
		passClassName = "default-terrain-only";
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
	var isConnected = !cmpTerritoryManager.IsTerritoryBlinking(pos.x, pos.y);
	var isOwn = tileOwner == cmpPlayer.GetPlayerID();
	var isMutualAlly = cmpPlayer.IsExclusiveMutualAlly(tileOwner);
	var isNeutral = tileOwner == 0;

	var invalidTerritory = "";
	if (isOwn)
	{
		if (!this.HasTerritory("own"))
			// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
			invalidTerritory = markForTranslationWithContext("Territory type", "own");
		else if (!isConnected && !this.HasTerritory("neutral"))
			// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
			invalidTerritory = markForTranslationWithContext("Territory type", "unconnected own");
	}
	else if (isMutualAlly)
	{
		if (!this.HasTerritory("ally"))
			// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
			invalidTerritory = markForTranslationWithContext("Territory type", "allied");
		else if (!isConnected && !this.HasTerritory("neutral"))
			// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
			invalidTerritory = markForTranslationWithContext("Territory type", "unconnected allied");
	}
	else if (isNeutral)
	{
		if (!this.HasTerritory("neutral"))
			// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
			invalidTerritory = markForTranslationWithContext("Territory type", "neutral");
	}
	else
	{
		// consider everything else enemy territory
		if (!this.HasTerritory("enemy"))
			// Translation: territoryType being displayed in a translated sentence in the form: "House cannot be built in %(territoryType)s territory.".
			invalidTerritory = markForTranslationWithContext("Territory type", "enemy");
	}

	if (invalidTerritory)
	{
		result.message = markForTranslation("%(name)s cannot be built in %(territoryType)s territory. Valid territories: %(validTerritories)s");
		result.translateParameters.push("territoryType");
		result.translateParameters.push("validTerritories");
		result.parameters.territoryType = {"context": "Territory type", "message": invalidTerritory};
		// gui code will join this array to a string
		result.parameters.validTerritories = {"context": "Territory type list", "list": this.GetTerritories()};
		return result;	// Fail
	}

	// Check special requirements
	if (this.template.PlacementType == "shore")
	{
		if (!cmpObstruction.CheckShorePlacement())
		{
			result.message = markForTranslation("%(name)s must be built on a valid shoreline");
			return result;	// Fail
		}
	}

	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);

	let templateName = cmpTemplateManager.GetCurrentTemplateName(this.entity);
	let template = cmpTemplateManager.GetTemplate(removeFiltersFromTemplateName(templateName));

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
		};

		if (this.template.Distance.MinDistance !== undefined)
		{
			let minDistance = ApplyValueModificationsToTemplate("BuildRestrictions/Distance/MinDistance", +this.template.Distance.MinDistance, cmpPlayer.GetPlayerID(), template);
			if (cmpRangeManager.ExecuteQuery(this.entity, 0, minDistance, [cmpPlayer.GetPlayerID()], IID_BuildRestrictions).some(filter))
			{
				let result = markForPluralTranslation(
					"%(name)s too close to a %(category)s, must be at least %(distance)s meter away",
					"%(name)s too close to a %(category)s, must be at least %(distance)s meters away",
					minDistance);

				result.success = false;
				result.translateMessage = true;
				result.parameters = {
					"name": name,
					"category": cat,
					"distance": minDistance
				};
				result.translateParameters = ["name", "category"];
				return result;  // Fail
			}
		}
		if (this.template.Distance.MaxDistance !== undefined)
		{
			let maxDistance = ApplyValueModificationsToTemplate("BuildRestrictions/Distance/MaxDistance", +this.template.Distance.MaxDistance, cmpPlayer.GetPlayerID(), template);
			if (!cmpRangeManager.ExecuteQuery(this.entity, 0, maxDistance, [cmpPlayer.GetPlayerID()], IID_BuildRestrictions).some(filter))
			{
				let result = markForPluralTranslation(
					"%(name)s too far from a %(category)s, must be within %(distance)s meter",
					"%(name)s too far from a %(category)s, must be within %(distance)s meters",
					maxDistance);

				result.success = false;
				result.translateMessage = true;
				result.parameters = {
					"name": name,
					"category": cat,
					"distance": maxDistance
				};
				result.translateParameters = ["name", "category"];
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
