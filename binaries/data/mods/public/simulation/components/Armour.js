function Armour() {}

Armour.prototype.Schema =
	"<a:help>Controls the damage resistance of the unit.</a:help>" +
	"<a:example>" +
		"<Hack>10.0</Hack>" +
		"<Pierce>0.0</Pierce>" +
		"<Crush>5.0</Crush>" +
	"</a:example>" +
	"<element name='Hack' a:help='Hack damage protection'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Pierce' a:help='Pierce damage protection'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Crush' a:help='Crush damage protection'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

Armour.prototype.Init = function()
{
};

Armour.prototype.TakeDamage = function(hack, pierce, crush)
{
	// Adjust damage values based on armour
	var adjHack = Math.max(0, hack - this.template.Hack);
	var adjPierce = Math.max(0, pierce - this.template.Pierce);
	var adjCrush = Math.max(0, crush - this.template.Crush);

	// Total is sum of individual damages, with minimum damage 1
	var total = Math.max(1, adjHack + adjPierce + adjCrush);

	// Reduce health
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	cmpHealth.Reduce(total);
};

Armour.prototype.GetArmourStrengths = function()
{
	// Convert attack values to numbers
	return {
		hack: +this.template.Hack,
		pierce: +this.template.Pierce,
		crush: +this.template.Crush
	};
};

Engine.RegisterComponentType(IID_DamageReceiver, "Armour", Armour);
