function ResourceTrickle() {}

ResourceTrickle.prototype.Schema = 
	"<a:help>Controls the resource trickle ability of the unit.</a:help>" +
	"<element name='Rates' a:help='Trickle Rates'>" +
		"<interleave>" +
			"<optional>" +
				"<element name='food' a:help='Food given to the player every interval'>" +
					"<ref name='nonNegativeDecimal'/>" +
				"</element>" +
			"</optional>" +
			"<optional>" +
				"<element name='wood' a:help='Wood given to the player every interval'>" +
					"<ref name='nonNegativeDecimal'/>" +
				"</element>" +
			"</optional>" +
			"<optional>" +
				"<element name='stone' a:help='Stone given to the player every interval'>" +
					"<ref name='nonNegativeDecimal'/>" +
				"</element>" +
			"</optional>" +
			"<optional>" +
				"<element name='metal' a:help='Metal given to the player every interval'>" +
					"<ref name='nonNegativeDecimal'/>" +
				"</element>" +
			"</optional>" +
		"</interleave>" +
	"</element>" +
	"<element name='Interval' a:help='Number of miliseconds must pass for the player to gain the next trickle.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

ResourceTrickle.prototype.Init = function()
{
	// Call the timer
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
 	cmpTimer.SetInterval(this.entity, IID_ResourceTrickle, "Trickle", this.GetTimer(), this.GetTimer(), undefined);
};

ResourceTrickle.prototype.GetTimer = function()
{
	var interval = +this.template.Interval;
	return interval;
};

ResourceTrickle.prototype.GetRates = function()
{
	var rates = {};
	for (var resource in this.template.Rates)
		rates[resource] = ApplyValueModificationsToEntity("ResourceTrickle/Rates/"+resource, +this.template.Rates[resource], this.entity);

	return rates;
};

// Do the actual work here
ResourceTrickle.prototype.Trickle = function(data, lateness)
{
	var cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return;

	var rates = this.GetRates();
	for (var resource in rates)
		cmpPlayer.AddResource(resource, rates[resource]);
};

Engine.RegisterComponentType(IID_ResourceTrickle, "ResourceTrickle", ResourceTrickle);
