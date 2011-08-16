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
			"<value>ScoutTower</value>" +
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
	// TODO: add phases, prerequisites, etc

BuildRestrictions.prototype.Init = function()
{
	this.territories = this.template.Territory.split(/\s+/);
};

BuildRestrictions.prototype.CheckPlacement = function(player)
{
	// TODO: Return error code for invalid placement, which can be handled by the UI

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
	if (!cmpObstruction || !cmpObstruction.CheckFoundation(passClassName))
	{
		return false;	// Fail
	}
	
	// Check territory restrictions
	var cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	var cmpPlayer = QueryPlayerIDInterface(player, IID_Player);
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!(cmpTerritoryManager && cmpPlayer && cmpPosition && cmpPosition.IsInWorld()))
	{
		return false;	// Fail
	}
	
	var pos = cmpPosition.GetPosition2D();
	var tileOwner = cmpTerritoryManager.GetOwner(pos.x, pos.y);
	var isOwn = (tileOwner == player);
	var isNeutral = (tileOwner == 0);
	var isAlly = !isOwn && cmpPlayer.IsAlly(tileOwner);
	var isEnemy = !isNeutral && cmpPlayer.IsEnemy(tileOwner);
	
	if ((isAlly && !this.HasTerritory("ally"))
		|| (isOwn && !this.HasTerritory("own"))
		|| (isNeutral && !this.HasTerritory("neutral"))
		|| (isEnemy && !this.HasTerritory("enemy")))
	{
		return false;	// Fail
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
		{
			return false;	// Fail
		}
		
		// Get building's footprint
		var shape = cmpFootprint.GetShape();
		var halfSize = 0;
		if (shape.type == "square")
		{
			halfSize = shape.depth/2;
		}
		else if (shape.type == "circle")
		{
			halfSize = shape.radius;
		}
		
		var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
		var cmpWaterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
		if (!cmpTerrain || !cmpWaterManager)
		{
			return false;	// Fail
		}
		
		var ang = cmpPosition.GetRotation().y;
		var sz = halfSize * Math.sin(ang);
		var cz = halfSize * Math.cos(ang);
		if ((cmpWaterManager.GetWaterLevel(pos.x + sz, pos.y + cz) - cmpTerrain.GetGroundLevel(pos.x + sz, pos.y + cz)) < 1.0 // front
			|| (cmpWaterManager.GetWaterLevel(pos.x - sz, pos.y - cz) - cmpTerrain.GetGroundLevel(pos.x - sz, pos.y - cz)) > 2.0)	// back
		{
			return false;	// Fail
		}
	}
	
	// Check distance restriction
	if (this.template.Distance)
	{
		var nearest = 65535;
		var ents = Engine.GetEntitiesWithInterface(IID_BuildRestrictions);
		for each (var ent in ents)
		{
			var cmpBuildRestrictions = Engine.QueryInterface(ent, IID_BuildRestrictions);
			if (cmpBuildRestrictions.GetCategory() == this.template.Distance.FromCategory && IsOwnedByPlayer(player, ent))
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
		
		if ((this.template.Distance.MinDistance && nearest < this.template.Distance.MinDistance)
			|| (this.template.Distance.MaxDistance && nearest > this.template.Distance.MaxDistance))
		{
			return false;	// Fail
		}
	}
	
	// Success
	return true;
};

BuildRestrictions.prototype.GetCategory = function()
{
	return this.template.Category;
};

BuildRestrictions.prototype.GetTerritories = function()
{
	return this.territories;
};

BuildRestrictions.prototype.HasTerritory = function(territory)
{
	return (this.territories.indexOf(territory) != -1);
};

Engine.RegisterComponentType(IID_BuildRestrictions, "BuildRestrictions", BuildRestrictions);
