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
		"<Damage>" +
			"<Hack>0.0</Hack>" +
			"<Pierce>10.0</Pierce>" +
			"<Crush>50.0</Crush>" +
		"</Damage>" +
	"</a:example>" +
	"<element name='Shape' a:help='Shape of the splash damage, can be circular'><text/></element>" +
	"<element name='Range' a:help='Size of the area affected by the splash'><ref name='nonNegativeDecimal'/></element>" +
	"<element name='FriendlyFire' a:help='Whether the splash damage can hurt non enemy units'><data type='boolean'/></element>" +
	Attacking.BuildAttackEffectsSchema();

DeathDamage.prototype.Init = function()
{
};

DeathDamage.prototype.Serialize = null; // we have no dynamic state to save

DeathDamage.prototype.GetDeathDamageEffects = function()
{
	return Attacking.GetAttackEffectsData("DeathDamage", this.template, this.entity);
};

DeathDamage.prototype.CauseDeathDamage = function()
{
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return;
	let pos = cmpPosition.GetPosition2D();

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let owner = cmpOwnership.GetOwner();
	if (owner == INVALID_PLAYER)
		warn("Unit causing death damage does not have any owner.");

	let radius = ApplyValueModificationsToEntity("DeathDamage/Range", +this.template.Range, this.entity);

	Attacking.CauseDamageOverArea({
		"type": "Death",
		"attackData": this.GetDeathDamageEffects(),
		"attacker": this.entity,
		"attackerOwner": owner,
		"origin": pos,
		"radius": radius,
		"shape": this.template.Shape,
		"friendlyFire": this.template.FriendlyFire == "true",
	});
};

Engine.RegisterComponentType(IID_DeathDamage, "DeathDamage", DeathDamage);
