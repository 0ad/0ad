function ResourceTrickle() {}

ResourceTrickle.prototype.Schema =
	"<a:help>Controls the resource trickle ability of the unit.</a:help>" +
	"<element name='Rates' a:help='Trickle Rates'>" +
		Resources.BuildSchema("nonNegativeDecimal") +
	"</element>" +
	"<element name='Interval' a:help='Number of miliseconds must pass for the player to gain the next trickle.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

ResourceTrickle.prototype.Init = function()
{
	this.ComputeRates();

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
 	cmpTimer.SetInterval(this.entity, IID_ResourceTrickle, "Trickle", this.GetTimer(), this.GetTimer(), undefined);
};

ResourceTrickle.prototype.GetTimer = function()
{
	return +this.template.Interval;
};

ResourceTrickle.prototype.GetRates = function()
{
	return this.rates;
};

ResourceTrickle.prototype.ComputeRates = function()
{
	this.rates = {};
	for (let resource in this.template.Rates)
		this.rates[resource] = ApplyValueModificationsToEntity("ResourceTrickle/Rates/"+resource, +this.template.Rates[resource], this.entity);
};

ResourceTrickle.prototype.Trickle = function(data, lateness)
{
	// The player entity may also have a ResourceTrickle component
	let cmpPlayer = QueryOwnerInterface(this.entity) || Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer)
		return;

	cmpPlayer.AddResources(this.rates);
};

ResourceTrickle.prototype.OnValueModification = function(msg)
{
	if (msg.component != "ResourceTrickle")
		return;

	this.ComputeRates();
};

Engine.RegisterComponentType(IID_ResourceTrickle, "ResourceTrickle", ResourceTrickle);
