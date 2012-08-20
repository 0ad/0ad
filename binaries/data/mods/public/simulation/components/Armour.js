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
	this.invulnerable = false;
};

Armour.prototype.Serialize = null; // we have no dynamic state to save

Armour.prototype.SetInvulnerability = function(invulnerability)
{
	this.invulnerable = invulnerability;
};

Armour.prototype.TakeDamage = function(hack, pierce, crush)
{
	if (this.invulnerable) 
		return { "killed": false };

	// Adjust damage values based on armour
	var armourStrengths = this.GetArmourStrengths();
	var adjHack = Math.max(0, hack - armourStrengths.hack);
	var adjPierce = Math.max(0, pierce - armourStrengths.pierce);
	var adjCrush = Math.max(0, crush - armourStrengths.crush);

	// Total is sum of individual damages, with minimum damage 1
	//	Round to nearest integer, since HP is integral
	var total = Math.max(1, Math.round(adjHack + adjPierce + adjCrush));

	// Reduce health
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	return cmpHealth.Reduce(total);
};

Armour.prototype.GetArmourStrengths = function()
{
	// Work out the armour values with technology effects
	var self = this;
	
	var foundationMultiplier = 1;
	if (Engine.QueryInterface(this.entity, IID_Foundation)) 
		foundationMultiplier = 0.2;
	
	var cmpTechMan = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	var applyTechs = function(type)
	{
		var strength = +self.template[type];
		if (cmpTechMan)
		{
			// All causes caching problems so disable it for now.
			// var allComponent = cmpTechMan.ApplyModifications("Armour/All", strength, self.entity) - self.template[type];
			strength = cmpTechMan.ApplyModifications("Armour/" + type, strength, self.entity);
		}
		return strength;
	};
	
	return {
		hack: foundationMultiplier*applyTechs("Hack"),
		pierce: foundationMultiplier*applyTechs("Pierce"),
		crush: foundationMultiplier*applyTechs("Crush")
	};
};

Engine.RegisterComponentType(IID_DamageReceiver, "Armour", Armour);
