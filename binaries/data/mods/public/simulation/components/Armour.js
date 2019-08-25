function Armour() {}

Armour.prototype.DamageResistanceSchema = "" +
	"<oneOrMore>" +
		"<element a:help='Resistance against any number of damage types'>" +
			"<anyName>" +
				"<except><name>Foundation</name></except>" +
			"</anyName>" +
			"<ref name='nonNegativeDecimal' />" +
		"</element>" +
	"</oneOrMore>";

Armour.prototype.Schema =
	"<a:help>Controls the damage resistance of the unit.</a:help>" +
	"<a:example>" +
		"<Hack>10.0</Hack>" +
		"<Pierce>0.0</Pierce>" +
		"<Crush>5.0</Crush>" +
	"</a:example>" +
	Armour.prototype.DamageResistanceSchema +
	"<optional>" +
		"<element name='Foundation' a:help='Armour given to building foundations'>" +
			Armour.prototype.DamageResistanceSchema +
		"</element>" +
	"</optional>";

Armour.prototype.Init = function()
{
	this.invulnerable = false;
};

Armour.prototype.IsInvulnerable = function()
{
	return this.invulnerable;
};

Armour.prototype.SetInvulnerability = function(invulnerability)
{
	this.invulnerable = invulnerability;
	Engine.PostMessage(this.entity, MT_InvulnerabilityChanged, { "entity": this.entity, "invulnerability": invulnerability });
};

Armour.prototype.GetArmourStrengths = function(effectType)
{
	// Work out the armour values with technology effects.
	let applyMods = (type, foundation) => {
		let strength;
		if (foundation)
		{
			strength = +this.template.Foundation[type];
			type = "Foundation/" + type;
		}
		else
			strength = +this.template[type];

		return ApplyValueModificationsToEntity("Armour/" + type, strength, this.entity);
	};

	let foundation = Engine.QueryInterface(this.entity, IID_Foundation) && this.template.Foundation;

	let ret = {};

	if (effectType != "Damage")
		return ret;

	for (let damageType in this.template)
		if (damageType != "Foundation")
			ret[damageType] = applyMods(damageType, foundation);

	return ret;
};

Engine.RegisterComponentType(IID_Resistance, "Armour", Armour);
