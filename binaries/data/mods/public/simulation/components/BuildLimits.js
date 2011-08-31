function BuildLimits() {}

BuildLimits.prototype.Schema =
	"<a:help>Specifies per category limits on number of buildings that can be constructed for each player.</a:help>" +
	"<a:example>" +
		"<Limits>" +
		  "<CivilCentre/>" +
		  "<ScoutTower>25</ScoutTower>" +
		  "<Fortress>10</Fortress>" +
		  "<Special>" +
			"<LimitPerCivCentre>1</LimitPerCivCentre>" +
		  "</Special>" +
		"</Limits>" +
	"</a:example>" +
	"<element name='LimitMultiplier'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='Limits'>" +
		"<zeroOrMore>" +
			"<element a:help='Specifies a category of building on which to apply this limit. See BuildRestrictions for list of categories.'>" +
				"<anyName />" +
				"<choice>" +
					"<text />" +
					"<element name='LimitPerCivCentre' a:help='Specifies that this limit is per number of civil centres.'>" +
						"<data type='nonNegativeInteger'/>" +
					"</element>" +
				"</choice>" +
			"</element>" +
		"</zeroOrMore>" +
	"</element>";

/*
 *	TODO: Use an inheriting player_{civ}.xml template for civ-specific limits
 */

BuildLimits.prototype.Init = function()
{
	this.limit = [];
	this.count = [];
	for (var category in this.template.Limits)
	{
		this.limit[category] = this.template.Limits[category];
		this.count[category] = 0;
	}
};

BuildLimits.prototype.IncrementCount = function(category)
{
	if (this.count[category] !== undefined)
	{
		this.count[category]++;
	}
};

BuildLimits.prototype.DecrementCount = function(category)
{
	if (this.count[category] !== undefined)
	{
		this.count[category]--;
	}
};

BuildLimits.prototype.AllowedToBuild = function(category)
{
	// TODO: The UI should reflect this before the user tries to place the building,
	//			since the limits are independent of placement location

	// Allow unspecified categories and those with no limit
	if (this.count[category] === undefined || this.limit[category] === undefined)
	{
		return true;
	}
	
	// Rather than complicating the schema unecessarily, just handle special cases here
	if (this.limit[category].LimitPerCivCentre !== undefined)
	{
		if (this.count[category] >= this.count["CivilCentre"] * this.limit[category].LimitPerCivCentre)
		{
			var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player); 
			var notification = {"player": cmpPlayer.GetPlayerID(), "message": category+" build limit of "+this.limit[category].LimitPerCivCentre+" per civil centre reached"};
			var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
			cmpGUIInterface.PushNotification(notification);
			
			return false;
		}
	}
	else if (this.count[category] >= this.limit[category])
	{
		var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
		var notification = {"player": cmpPlayer.GetPlayerID(), "message": category+" build limit of "+this.limit[category]+ " reached"};
		var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification(notification);
		
		return false;
	}
	
	return true;
};

BuildLimits.prototype.OnGlobalOwnershipChanged = function(msg)
{
	// This automatically updates build counts
	var cmpBuildRestrictions = Engine.QueryInterface(msg.entity, IID_BuildRestrictions);
	if (cmpBuildRestrictions)
	{
		var playerID = (Engine.QueryInterface(this.entity, IID_Player)).GetPlayerID();
		if (msg.from == playerID)
		{
			this.DecrementCount(cmpBuildRestrictions.GetCategory());
		}
		if (msg.to == playerID)
		{
			this.IncrementCount(cmpBuildRestrictions.GetCategory());
		}
	}
};

Engine.RegisterComponentType(IID_BuildLimits, "BuildLimits", BuildLimits);
