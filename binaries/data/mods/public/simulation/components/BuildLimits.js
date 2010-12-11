function BuildLimits() {}

BuildLimits.prototype.Schema =
	"<a:help></a:help>" +
	"<element name='LimitMultiplier'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='Limits'>" +
		"<zeroOrMore>" +
			"<element>" +
				"<anyName />" +
				"<text />" +
			"</element>" +
		"</zeroOrMore>" +
	"</element>";

BuildLimits.prototype.Init = function()
{
	this.limits = [];
	this.unitCount = [];
	for (var category in this.template.Limits)
	{
		this.limits[category] = this.template.Limits[category];
		this.unitCount[category] = 0;
	}
};

BuildLimits.prototype.IncrementCount = function(category)
{
	if (this.unitCount[category] !== undefined)
		this.unitCount[category]++;
};

BuildLimits.prototype.DecrementCount = function(category)
{
	if (this.unitCount[category] !== undefined)
		this.unitCount[category]--;
};

BuildLimits.prototype.AllowedToBuild = function(category)
{
	if (this.unitCount[category])
	{
		if (this.unitCount[category] >= this.limits[category])
		{
			var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
			var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player); 
			var notification = {"player": cmpPlayer.GetPlayerID(), "message": "Build limit reached for this building"};
			var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
			cmpGUIInterface.PushNotification(notification);
			return false;
		}
		else
			return true;
	}
	else
	{
		//Check here for terrain
		return true;
	}
};

Engine.RegisterComponentType(IID_BuildLimits, "BuildLimits", BuildLimits);