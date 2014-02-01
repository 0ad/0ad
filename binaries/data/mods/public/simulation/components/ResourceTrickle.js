function ResourceTrickle() {}

ResourceTrickle.prototype.Schema = 
	"<a:help>Controls the resource trickle ability of the unit.</a:help>" +
	"<element name='FoodRate' a:help='Food given to the player every interval'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='WoodRate' a:help='Wood given to the player every interval'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='StoneRate' a:help='Stone given to the player every interval'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='MetalRate' a:help='Metal given to the player every interval'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Interval' a:help='Number of miliseconds must pass for the player to gain the next trickle.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

ResourceTrickle.prototype.Init = function()
{
	// Call the timer
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
 	cmpTimer.SetInterval(this.entity, IID_ResourceTrickle, "Trickle", this.GetTimer(), this.GetTimer(), undefined)
};

ResourceTrickle.prototype.GetTimer = function()
{
	var interval = +this.template.Interval;
	return interval;
};

ResourceTrickle.prototype.GetRates = function()
{
	var foodrate = +this.template.FoodRate;
	var woodrate = +this.template.WoodRate;
	var stonerate = +this.template.StoneRate;
	var metalrate = +this.template.MetalRate;

	foodrate = ApplyValueModificationsToEntity("ResourceTrickle/FoodRate", foodrate, this.entity);
	woodrate = ApplyValueModificationsToEntity("ResourceTrickle/WoodRate", woodrate, this.entity);
	stonerate = ApplyValueModificationsToEntity("ResourceTrickle/StoneRate", stonerate, this.entity);
	metalrate = ApplyValueModificationsToEntity("ResourceTrickle/MetalRate", metalrate, this.entity);
	
	return {"food": foodrate, "wood": woodrate, "stone": stonerate, "metal": metalrate };
};

// Do the actual work here
ResourceTrickle.prototype.Trickle = function(data, lateness)
{
	// Get the rates
	var rates = this.GetRates();
	
	// Get the player
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	if (cmpPlayer)
		for (var resource in rates)
			cmpPlayer.AddResource(resource, rates[resource]);
	
};

Engine.RegisterComponentType(IID_ResourceTrickle, "ResourceTrickle", ResourceTrickle);
