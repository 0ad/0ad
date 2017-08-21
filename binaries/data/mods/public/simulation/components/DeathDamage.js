function DeathDamage() {}

DeathDamage.prototype.bonusesSchema =
	"<optional>" +
		"<element name='Bonuses'>" +
			"<zeroOrMore>" +
				"<element>" +
					"<anyName/>" +
					"<interleave>" +
						"<optional>" +
							"<element name='Civ' a:help='If an entity has this civ then the bonus is applied'><text/></element>" +
						"</optional>" +
						"<element name='Classes' a:help='If an entity has all these classes then the bonus is applied'><text/></element>" +
						"<element name='Multiplier' a:help='The attackers attack strength is multiplied by this'><ref name='nonNegativeDecimal'/></element>" +
					"</interleave>" +
				"</element>" +
			"</zeroOrMore>" +
		"</element>" +
	"</optional>";

DeathDamage.prototype.Schema =
	"<a:help>When a unit or building is destroyed, it inflicts damage to nearby units.</a:help>" +
	"<a:example>" +
		"<Shape>Circular</Shape>" +
		"<Range>20</Range>" +
		"<FriendlyFire>false</FriendlyFire>" +
		"<Hack>0.0</Hack>" +
		"<Pierce>10.0</Pierce>" +
		"<Crush>50.0</Crush>" +
	"</a:example>" +
	"<element name='Shape' a:help='Shape of the splash damage, can be circular'><text/></element>" +
	"<element name='Range' a:help='Size of the area affected by the splash'><ref name='nonNegativeDecimal'/></element>" +
	"<element name='FriendlyFire' a:help='Whether the splash damage can hurt non enemy units'><data type='boolean'/></element>" +
	"<element name='Hack' a:help='Hack damage strength'><ref name='nonNegativeDecimal'/></element>" +
	"<element name='Pierce' a:help='Pierce damage strength'><ref name='nonNegativeDecimal'/></element>" +
	"<element name='Crush' a:help='Crush damage strength'><ref name='nonNegativeDecimal'/></element>" +
	DeathDamage.prototype.bonusesSchema;

DeathDamage.prototype.Init = function()
{
};

DeathDamage.prototype.Serialize = null; // we have no dynamic state to save

DeathDamage.prototype.GetDeathDamageStrengths = function(type)
{
	// Work out the damage values with technology effects
	let applyMods = damageType =>
		ApplyValueModificationsToEntity("DeathDamage/" + damageType, +(this.template[damageType] || 0), this.entity);

	return {
		"hack": applyMods("Hack"),
		"pierce": applyMods("Pierce"),
		"crush": applyMods("Crush")
	};
};

DeathDamage.prototype.GetBonusTemplate = function()
{
	if (this.template.Bonuses)
		return clone(this.template.Bonuses);
	return null;
};

DeathDamage.prototype.CauseDeathDamage = function()
{
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return;
	let pos = cmpPosition.GetPosition2D();

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let owner = cmpOwnership.GetOwner();
	if (owner == -1)
		warn("Unit causing death damage does not have any owner.");

	let cmpDamage = Engine.QueryInterface(SYSTEM_ENTITY, IID_Damage);
	let playersToDamage = cmpDamage.GetPlayersToDamage(owner, this.template.FriendlyFire);

	let radius = ApplyValueModificationsToEntity("DeathDamage/Range", +this.template.Range, this.entity);

	cmpDamage.CauseSplashDamage({
		"attacker": this.entity,
		"origin": pos,
		"radius": radius,
		"shape": this.template.Shape,
		"strengths": this.GetDeathDamageStrengths("Death"),
		"splashBonus": this.GetBonusTemplate(),
		"playersToDamage": playersToDamage,
		"type": "Death",
		"attackerOwner": owner
	});
};

Engine.RegisterComponentType(IID_DeathDamage, "DeathDamage", DeathDamage);
