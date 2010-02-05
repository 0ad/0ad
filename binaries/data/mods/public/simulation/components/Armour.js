function Armour() {}

Armour.prototype.Init = function()
{
};

Armour.prototype.TakeDamage = function(hack, pierce, crush)
{
	// Adjust damage values based on armour
	// (Default armour values to 0 if undefined)
	var adjHack = Math.max(0, hack - (this.template.Hack || 0));
	var adjPierce = Math.max(0, pierce - (this.template.Pierce || 0));
	var adjCrush = Math.max(0, crush - (this.template.Crush || 0));

	// Total is sum of individual damages, with minimum damage 1
	var total = Math.max(1, adjHack + adjPierce + adjCrush);

	// Reduce health
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	cmpHealth.Reduce(total);
};

Engine.RegisterComponentType(IID_DamageReceiver, "Armour", Armour);
