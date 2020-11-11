function Resistance() {}

/**
 * Builds a RelaxRNG schema of possible attack effects.
 *
 * @return {string} - RelaxNG schema string.
 */
Resistance.prototype.BuildResistanceSchema = function()
{
	return "" +
		"<oneOrMore>" +
			"<choice>" +
				"<element name='Damage'>" +
					"<oneOrMore>" +
						"<element a:help='Resistance against any number of damage types affecting health.'>" +
							"<anyName/>" +
							"<ref name='nonNegativeDecimal'/>" +
						"</element>" +
					"</oneOrMore>" +
				"</element>" +
				"<element name='Capture' a:help='Resistance against Capture attacks.'>" +
					"<ref name='nonNegativeDecimal'/>" +
				"</element>" +
				"<element name='ApplyStatus' a:help='Resistance against StatusEffects.'>" +
					"<oneOrMore>" +
						"<element a:help='Resistance against any number of status effects.'>" +
							"<anyName/>" +
							"<interleave>" +
								"<optional>" +
									"<element name='Duration' a:help='The reduction in duration of the status. The normal duration time is multiplied by this factor.'>" +
										"<ref name='nonNegativeDecimal'/>" +
									"</element>" +
								"</optional>" +
								"<optional>" +
									"<element name='BlockChance' a:help='The chance of blocking the status. In the interval [0,1].'><ref name='nonNegativeDecimal'/></element>" +
								"</optional>" +
							"</interleave>" +
						"</element>" +
					"</oneOrMore>" +
				"</element>" +
			"</choice>" +
		"</oneOrMore>";
};

Resistance.prototype.Schema =
	"<a:help>Controls the damage resistance of the unit.</a:help>" +
	"<a:example>" +
		"<Foundation>" +
			"<Damage>" +
				"<Hack>10.0</Hack>" +
				"<Pierce>0.0</Pierce>" +
				"<Crush>5.0</Crush>" +
			"</Damage>" +
			"<Capture>10</Capture>" +
		"</Foundation>" +
		"<Entity>" +
			"<Damage>" +
				"<Poison>5</Poison>" +
			"</Damage>" +
		"</Entity>" +
	"</a:example>" +
	"<zeroOrMore>" +
		"<choice>" +
			"<element name='Foundation' a:help='Resistance of an unfinished structure (i.e. a foundation).'>" +
				Resistance.prototype.BuildResistanceSchema() +
			"</element>" +
			"<element name='Entity' a:help='Resistance of an entity.'>" +
				Resistance.prototype.BuildResistanceSchema() +
			"</element>" +
		"</choice>" +
	"</zeroOrMore>";

Resistance.prototype.Init = function()
{
	this.invulnerable = false;
};

Resistance.prototype.IsInvulnerable = function()
{
	return this.invulnerable;
};

Resistance.prototype.SetInvulnerability = function(invulnerability)
{
	this.invulnerable = invulnerability;
	Engine.PostMessage(this.entity, MT_InvulnerabilityChanged, { "entity": this.entity, "invulnerability": invulnerability });
};

/**
 * Calculate the effective resistance of an entity to a particular effect.
 * ToDo: Support resistance against status effects.
 * @param {string} effectType - The type of attack effect the resistance has to be calculated for (e.g. "Damage", "Capture").
 * @return {Object} - An object of the type { "Damage": { "Crush": number, "Hack": number }, "Capture": number }.
 */
Resistance.prototype.GetEffectiveResistanceAgainst = function(effectType)
{
	let ret = {};

	let template = this.GetResistanceOfForm(Engine.QueryInterface(this.entity, IID_Foundation) ? "Foundation" : "Entity");
	if (template[effectType])
		ret[effectType] = template[effectType];

	return ret;
};

/**
 * Get all separate resistances for showing in the GUI.
 * @return {Object} - All resistances ordered by type.
 */
Resistance.prototype.GetFullResistance = function()
{
	let ret = {};
	for (let entityForm in this.template)
		ret[entityForm] = this.GetResistanceOfForm(entityForm);

	return ret;
};

/**
 * Get the resistance of a particular type, i.e. Foundation or Entity.
 * @param {string} entityForm - The form of the entity to query.
 * @return {Object} - An object containing the resistances.
 */
Resistance.prototype.GetResistanceOfForm = function(entityForm)
{
	let ret = {};
	let template = this.template[entityForm];
	if (!template)
		return ret;

	if (template.Damage)
	{
		ret.Damage = {};
		for (let damageType in template.Damage)
			ret.Damage[damageType] = ApplyValueModificationsToEntity("Resistance/" + entityForm + "/Damage/" + damageType, +this.template[entityForm].Damage[damageType], this.entity);
	}

	if (template.Capture)
		ret.Capture = ApplyValueModificationsToEntity("Resistance/" + entityForm + "/Capture", +this.template[entityForm].Capture, this.entity);

	if (template.ApplyStatus)
	{
		ret.ApplyStatus = {};
		for (let effect in template.ApplyStatus)
			ret.ApplyStatus[effect] = {
				"duration": ApplyValueModificationsToEntity("Resistance/" + entityForm + "/ApplyStatus/" + effect + "/Duration", +(template.ApplyStatus[effect].Duration || 1), this.entity),
				"blockChance": ApplyValueModificationsToEntity("Resistance/" + entityForm + "/ApplyStatus/" + effect + "/BlockChance", +(template.ApplyStatus[effect].BlockChance || 0), this.entity)
			};
	}

	return ret;
};

Engine.RegisterComponentType(IID_Resistance, "Resistance", Resistance);
